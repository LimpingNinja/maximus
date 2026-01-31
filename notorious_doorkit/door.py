"""Main door runtime context.

This module holds the primary Door runtime class for session management.
LNWP protocol functionality is separate in `notorious_doorkit.lnwp`.
"""

from __future__ import annotations

import os
import re
import select
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple

from .config import get_config
from .runtime import clear_lnwp_fd, clear_terminal_fd, get_lnwp, init_runtime, set_lnwp_fd, set_terminal_fd
from .runtime import get_session, get_terminal_fd
from .dropfiles import DropfileInfo, DropfileType, DropfileUser, load_dropfile
from .lnwp import (
    DLE,
    ETX,
    STX,
    LnwpEvent,
    LnwpFramer,
    MessageEvent,
    NewMessageEvent,
    NodeInfoEvent,
    NodesInfoEvent,
    RawLnwpEvent,
    _escape_lnwp_payload,
    lnwp_payload,
    parse_lnwp_payload,
)
from .utils import write_all


@dataclass
class TerminalInfo:
    capabilities: list[str] = field(default_factory=lambda: ["ascii"])
    rip_version: Optional[str] = None
    detected_at: Optional[float] = None
    preferred_mode: Optional[str] = None
    rows: Optional[int] = None
    cols: Optional[int] = None


@dataclass
class DoorSession:
    node: Optional[int]
    dropfile: Optional[DropfileInfo]
    username: Optional[str]
    time_left_secs: Optional[int]
    session_deadline: Optional[float]
    terminal: TerminalInfo = field(default_factory=TerminalInfo)


class Door:
    """
    Main door runtime context for session management.
    
    Handles dropfile loading, session state, and terminal capability detection.
    Does not include LNWP protocol functionality - use LnwpDoor for that.
    """
    
    def __init__(self):
        self.session = DoorSession(
            node=None,
            dropfile=None,
            username=None,
            time_left_secs=None,
            session_deadline=None,
            terminal=TerminalInfo(),
        )

        self.door_key = os.environ.get("NOTORIOUS_DOOR_KEY", "UNKNOWN")

        self.framer = LnwpFramer()
        self._inbox: List[Dict[str, str]] = []
        self._nodes_info: Dict[int, Dict[str, str]] = {}
        self._node_info: Dict[str, str] = {}
        self._eof = False

        self._auto_drain_inbox = False
        self._auto_drain_box = "TEMP"
        self._draining = False

    @property
    def terminal_fd(self) -> int:
        fd = get_terminal_fd(default=None)
        if fd is not None and int(fd) > 0:
            return int(fd)
        return 1

    @property
    def control_fd(self) -> int:
        return int(get_lnwp())

    @property
    def input_fd(self) -> int:
        return int(get_lnwp())

    @property
    def door32_mode(self) -> bool:
        try:
            s = get_session()
        except Exception:
            return False
        df = getattr(s, "dropfile", None)
        p = getattr(df, "path", None)
        name = getattr(p, "name", "")
        return str(name).lower() == "door32.sys"

    def _chdir_to_script_dir(self) -> None:
        """Change CWD to the current door script directory (OpenDoors-style)."""
        try:
            argv0 = sys.argv[0] if sys.argv else ""
            if not argv0:
                return
            script_path = Path(argv0)
            if not script_path.is_absolute():
                script_path = (Path.cwd() / script_path)
            script_path = script_path.resolve()
            if not script_path.exists():
                return
            if script_path.is_file():
                os.chdir(str(script_path.parent))
        except Exception:
            return

    def _resolve_handle_arg(self) -> Optional[int]:
        """Parse --handle <fd> from command-line arguments."""
        i = 1
        while i < len(sys.argv):
            if sys.argv[i] == "--handle" and i + 1 < len(sys.argv):
                try:
                    return int(sys.argv[i + 1])
                except:
                    return None
            i += 1
        return None

    def _resolve_start_args(self) -> tuple[int, str, Optional[str]]:
        def _first_env(*keys: str) -> Optional[str]:
            for k in keys:
                v = os.environ.get(k)
                if v is not None and str(v).strip() != "":
                    return str(v)
            return None

        def _argv_positional() -> List[str]:
            out: List[str] = []
            i = 1
            while i < len(sys.argv):
                a = sys.argv[i]
                if a == "--lnwp":
                    i += 2
                    continue
                if a == "--handle":
                    i += 2
                    continue
                out.append(a)
                i += 1
            return out

        # ENV is authoritative if present.
        node_raw = _first_env(
            "NOTORIOUS_DOOR_NODE",
        )
        dropfile_path = _first_env(
            "NOTORIOUS_DOOR_DROPFILE_PATH",
        )
        drop_type = _first_env(
            "NOTORIOUS_DOOR_DROPFILE_TYPE",
            "NOTORIOUS_DROPFILE_TYPE",
            "DOOR_DROPFILE_TYPE",
        )

        # Fallback to argv[1]/argv[2] convention.
        argv_pos = _argv_positional()
        if node_raw is None and len(argv_pos) >= 1:
            node_raw = argv_pos[0]
        if dropfile_path is None and len(argv_pos) >= 2:
            dropfile_path = argv_pos[1]

        if node_raw is None or dropfile_path is None:
            raise RuntimeError(
                "Door.start() requires a node and dropfile path via env vars or argv[1]/argv[2]."
            )

        try:
            node = int(str(node_raw).strip())
        except Exception as e:
            raise RuntimeError(f"Invalid node number: {node_raw!r}") from e

        return (node, str(dropfile_path), drop_type)

    def _resolve_lnwp_fd(self) -> int:
        env_raw = os.environ.get("NOTORIOUS_DOOR_LNWP_FD")
        if env_raw is not None and str(env_raw).strip() != "":
            try:
                return int(str(env_raw).strip())
            except Exception:
                return 0

        for i, a in enumerate(sys.argv):
            if a == "--lnwp" and i + 1 < len(sys.argv):
                try:
                    return int(str(sys.argv[i + 1]).strip())
                except Exception:
                    return 0

        return 0

    def start(self) -> DoorSession:
        from .input import detect_capabilities_probe, detect_window_size_probe
        from .ansi import clear_screen

        self._chdir_to_script_dir()

        clear_lnwp_fd()
        set_lnwp_fd(self._resolve_lnwp_fd())

        node, dropfile_path, drop_type = self._resolve_start_args()

        cfg = get_config()
        if cfg.terminal.clear_on_entry:
            clear_screen()

        info = load_dropfile(dropfile_path, drop_type=drop_type)
        info.node = node

        if info.path.name.lower() == "door32.sys":
            comm_type_raw = str(info.extra.get("door32_comm_type", "")).strip()
            handle_raw = str(info.extra.get("door32_handle", "")).strip()

            try:
                comm_type = int(comm_type_raw)
            except Exception:
                comm_type = None

            try:
                handle = int(handle_raw)
            except Exception:
                handle = None

            # Check for --handle override
            handle_override = self._resolve_handle_arg()
            if handle_override is not None:
                # --handle 0 explicitly means: use stdio
                if handle_override <= 2:
                    clear_terminal_fd()
                else:
                    set_terminal_fd(handle_override)
            else:
                # Auto selection:
                # - If stdin is a TTY (Mystic-style stdio redirection), prefer stdio.
                # - Otherwise (NotoriousPTY-style), use the inherited handle.
                stdin_is_tty = False
                try:
                    stdin_is_tty = os.isatty(0)
                except Exception:
                    stdin_is_tty = False

                if stdin_is_tty:
                    clear_terminal_fd()
                elif comm_type == 2 and handle is not None and handle > 2:
                    set_terminal_fd(handle)
                else:
                    clear_terminal_fd()
        else:
            clear_terminal_fd()

        username: Optional[str] = None
        if info.user.alias and info.user.alias.strip():
            username = info.user.alias.strip()
        elif info.user.full_name and info.user.full_name.strip():
            username = info.user.full_name.strip()

        session_deadline: Optional[float] = None
        if info.time_left_secs is not None and info.time_left_secs > 0:
            session_deadline = time.time() + float(info.time_left_secs)

        self.session = DoorSession(
            node=node,
            dropfile=info,
            username=username,
            time_left_secs=info.time_left_secs,
            session_deadline=session_deadline,
        )

        # Initialize terminal from dropfile
        if info.graphics_mode:
            gm = info.graphics_mode.strip().upper()
            if gm in ("Y", "1", "ANSI", "COLOR", "COLOUR"):
                self.session.terminal.capabilities = ["ansi"]
                if self.session.terminal.cols is None:
                    self.session.terminal.cols = 80
                if self.session.terminal.rows is None:
                    self.session.terminal.rows = 24
        
        if info.screen_rows and self.session.terminal.rows is None:
            self.session.terminal.rows = info.screen_rows

        if cfg.terminal.detect_capabilities:
            term = detect_capabilities_probe(timeout_ms=cfg.terminal.probe_timeout_ms)
            self.session.terminal = term

        if cfg.terminal.detect_window_size:
            sz = detect_window_size_probe(timeout_ms=cfg.terminal.window_size_timeout_ms)
            if sz is not None:
                self.session.terminal.rows = int(sz[0])
                self.session.terminal.cols = int(sz[1])

        init_runtime(self.session)

        return self.session

    def lnwp_enabled(self) -> bool:
        return get_lnwp() > 0

    def is_eof(self) -> bool:
        return self._eof

    def poll(self, timeout: float = 0.0, max_bytes: int = 4096) -> Tuple[List[Tuple[str, Dict[str, str]]], bytes]:
        fd = get_lnwp()
        if fd <= 0:
            return ([], b"")

        if self._eof:
            return ([], b"")

        if timeout > 0:
            ready, _, _ = select.select([fd], [], [], timeout)
            if not ready:
                return ([], b"")
        else:
            ready, _, _ = select.select([fd], [], [], 0)
            if not ready:
                return ([], b"")

        try:
            data = os.read(fd, max_bytes)
        except BlockingIOError:
            return ([], b"")

        if not data:
            self._eof = True
            return ([], b"")

        frames, _passthrough = self.framer.feed(data)

        parsed_frames: List[Tuple[str, Dict[str, str]]] = []
        for frame in frames:
            payload_str = frame.payload.decode("utf-8", errors="replace")
            fields = parse_lnwp_payload(payload_str)
            parsed_frames.append((frame.kind, fields))

        return (parsed_frames, b"")

    def poll_events(self, timeout: float = 0.0, max_bytes: int = 4096) -> Tuple[List[LnwpEvent], bytes]:
        frames, passthrough = self.poll(timeout=timeout, max_bytes=max_bytes)
        events: List[LnwpEvent] = []

        for kind, fields in frames:
            prm = fields.get("PRM", "")

            if prm == "NEW_MSG":
                box = fields.get("BOX") or None
                count: Optional[int] = None
                raw_count = fields.get("COUNT")
                if raw_count is not None:
                    try:
                        count = int(raw_count)
                    except ValueError:
                        count = None
                events.append(NewMessageEvent(box=box, count=count))
                if self._auto_drain_inbox:
                    self._draining = True
                    self.request_next_message(box=self._auto_drain_box)
                continue

            if prm == "MSG":
                if fields.get("ERR") == "EMPTY":
                    self._draining = False
                else:
                    self._inbox.append(fields)
                    events.append(MessageEvent(fields=fields))
                    if self._auto_drain_inbox and self._draining:
                        self.request_next_message(box=self._auto_drain_box)
                continue

            if prm == "INFO":
                topic = fields.get("VAL", "")
                if topic == "NODES_INFO":
                    nodes = self._parse_nodes_info_fields(fields)
                    self._nodes_info = nodes
                    events.append(NodesInfoEvent(nodes_info=nodes, fields=fields))
                    continue
                if topic == "NODE_INFO":
                    self._node_info = dict(fields)
                    events.append(NodeInfoEvent(fields=fields))
                    continue

            events.append(RawLnwpEvent(kind=kind, fields=fields))

        return (events, passthrough)

    def poll_messages(self, timeout: float = 0.0) -> Tuple[List[Dict[str, str]], bytes]:
        _events, passthrough = self.poll_events(timeout=timeout)
        msgs = self._inbox.copy()
        self._inbox.clear()
        return (msgs, passthrough)

    def drain_messages(self) -> List[Dict[str, str]]:
        msgs = self._inbox.copy()
        self._inbox.clear()
        return msgs

    def send_lnwp(self, kind: str, payload: str) -> None:
        fd = get_lnwp()
        if fd <= 0:
            return

        kb = kind.encode("ascii", errors="strict")
        pb = payload.encode("ascii", errors="replace")
        pb = _escape_lnwp_payload(pb)

        frame = bytearray()
        frame.extend(bytes([DLE, STX]))
        frame.extend(kb)
        frame.extend(pb)
        frame.extend(bytes([DLE, ETX]))

        write_all(fd, bytes(frame))

    def recv_lnwp(self, timeout: float = 0.0) -> Optional[Tuple[str, Dict[str, str]]]:
        frames, _passthrough = self.poll(timeout=timeout)
        if not frames:
            return None
        return frames[0]

    def set_activity(self, activity: str) -> None:
        self.send_lnwp("N", lnwp_payload("ACTIVITY", activity))

    def set_username(self, username: str) -> None:
        u = (username or "").strip()
        if not u:
            return
        self.session.username = u
        self.send_lnwp("N", lnwp_payload("USERNAME", u))

    def enable_lnwp(self) -> None:
        self.send_lnwp("N", lnwp_payload("ARM_LNWP", "1"))

    def disable_lnwp(self) -> None:
        self.send_lnwp("N", lnwp_payload("DISARM_LNWP", "0"))

    def arm(self) -> None:
        self.enable_lnwp()

    def disarm(self) -> None:
        self.disable_lnwp()

    def set_push_messages(self, enabled: bool, box: str = "TEMP") -> None:
        self.send_lnwp("C", lnwp_payload("PUSH_MSG", "ON" if enabled else "OFF", BOX=box))

    def enable_push_messages(self, box: str = "TEMP") -> None:
        self.set_push_messages(True, box=box)

    def disable_push_messages(self, box: str = "TEMP") -> None:
        self.set_push_messages(False, box=box)

    def set_input_mode(self, mode: str) -> None:
        m = (mode or "").strip().lower()
        if m in ("menu",):
            m = "cooked"
        elif m in ("form", "editor", "chat"):
            m = "raw"

        if m in ("raw", "character", "char"):
            self.send_lnwp("C", lnwp_payload("RAW_MODE", "ON"))
            return
        if m in ("cooked", "line"):
            self.send_lnwp("C", lnwp_payload("RAW_MODE", "OFF"))
            return
        raise ValueError("mode must be 'raw' or 'cooked'")

    def send_message(self, node_id: int, text: str, box: str = "TEMP", tag: str = "CHAT") -> None:
        if node_id == 0:
            self.send_lnwp("C", lnwp_payload("SEND_MSG", text, BOX=box, TAG=tag))
        else:
            self.send_lnwp("C", lnwp_payload("SEND_MSG", text, NODE_ID=str(node_id), BOX=box, TAG=tag))

    def set_inbox_auto_drain(self, enabled: bool, box: str = "TEMP") -> None:
        self._auto_drain_inbox = bool(enabled)
        self._auto_drain_box = (box or "TEMP").strip() or "TEMP"
        if not self._auto_drain_inbox:
            self._draining = False

    def drain_inbox(self, box: str = "TEMP") -> None:
        self._draining = True
        self.request_next_message(box=box)

    def request_next_message(self, box: str = "TEMP") -> None:
        b = (box or "TEMP").strip() or "TEMP"
        self.send_lnwp("Q", lnwp_payload("GET_MSG", None, BOX=b))

    def request_nodes_info(self, keys: Optional[List[str]] = None) -> None:
        key_str = "*"
        if keys:
            key_str = ",".join([k.strip() for k in keys if k.strip()]) or "*"
        self.send_lnwp("Q", lnwp_payload("REQ_INFO", "NODES_INFO", KEY=key_str))

    def request_node_info(self, keys: Optional[List[str]] = None) -> None:
        key_str = "*"
        if keys:
            key_str = ",".join([k.strip() for k in keys if k.strip()]) or "*"
        self.send_lnwp("Q", lnwp_payload("REQ_INFO", "NODE_INFO", KEY=key_str))

    def get_nodes_info_cached(self) -> Dict[int, Dict[str, str]]:
        return self._nodes_info

    def get_node_info_cached(self) -> Dict[str, str]:
        return self._node_info

    def get_messages(self, timeout: float = 0.0) -> List[Dict[str, str]]:
        msgs, _passthrough = self.poll_messages(timeout=timeout)
        return msgs

    def get_nodes_info(self) -> Dict[int, Dict[str, str]]:
        self.request_nodes_info(keys=["*"])
        start_time = time.time()
        while time.time() - start_time < 1.0:
            self.poll_events(timeout=0.1)
            if self._nodes_info:
                break
        return self._nodes_info

    def _parse_nodes_info_fields(self, fields: Dict[str, str]) -> Dict[int, Dict[str, str]]:
        out: Dict[int, Dict[str, str]] = {}

        ids_raw = fields.get("IDS", "")
        ids: List[int] = []
        for part in ids_raw.split(","):
            part = part.strip()
            if not part:
                continue
            try:
                ids.append(int(part))
            except ValueError:
                continue
        for nid in ids:
            out.setdefault(nid, {})

        for k, v in fields.items():
            m = re.match(r"^N(\d+)_(.+)$", k)
            if not m:
                continue
            nid = int(m.group(1))
            key = m.group(2).upper()
            out.setdefault(nid, {})[key] = v

        return out

    def process_incoming(self, timeout: float = 0.0) -> bytes:
        _events, passthrough = self.poll_events(timeout=timeout)
        return passthrough

    def set_current_theme(self, theme: "Theme") -> None:
        from .theme import set_current_theme

        set_current_theme(theme)

    def start_local(
        self,
        *,
        username: str = "Sysop",
        node: int = 1,
        time_left_secs: Optional[int] = None,
        detect_capabilities: bool = True,
        detect_window_size: bool = True,
    ) -> DoorSession:
        """
        Start a local test session without requiring a dropfile.
        
        This is useful for testing doors during development. Creates a minimal
        session with sensible defaults and optional terminal capability detection.
        
        Args:
            username: Username for the local session (default: "Sysop")
            node: Node number (default: 1)
            time_left_secs: Time limit in seconds, or None for unlimited (default: None)
            detect_capabilities: Whether to probe for ANSI/RIP support (default: True)
            detect_window_size: Whether to detect terminal window size (default: True)
        
        Returns:
            DoorSession with minimal dropfile info
        """
        from .input import detect_capabilities_probe, detect_window_size_probe
        from .ansi import clear_screen

        self._chdir_to_script_dir()

        clear_terminal_fd()
        clear_lnwp_fd()
        set_lnwp_fd(self._resolve_lnwp_fd())

        cfg = get_config()
        if cfg.terminal.clear_on_entry:
            clear_screen()

        session_deadline: Optional[float] = None
        if time_left_secs is not None and time_left_secs > 0:
            session_deadline = time.time() + float(time_left_secs)

        dropfile_info = DropfileInfo(
            drop_type=DropfileType.UNKNOWN,
            path=Path(""),
            raw_lines=[],
            user=DropfileUser(full_name=username, alias=username, city="Local"),
            node=node,
            time_left_secs=time_left_secs,
            graphics_mode="ANSI",
            screen_rows=None,
            temp_path=None,
            bbs_root=None,
            extra={},
        )

        self.session = DoorSession(
            node=node,
            dropfile=dropfile_info,
            username=username,
            time_left_secs=time_left_secs,
            session_deadline=session_deadline,
        )

        if detect_capabilities:
            cfg = get_config()
            term = detect_capabilities_probe(timeout_ms=cfg.terminal.probe_timeout_ms)
            self.session.terminal = term

        if detect_window_size:
            cfg = get_config()
            sz = detect_window_size_probe(timeout_ms=cfg.terminal.window_size_timeout_ms)
            if sz is not None:
                self.session.terminal.rows = int(sz[0])
                self.session.terminal.cols = int(sz[1])

        init_runtime(self.session)

        return self.session
