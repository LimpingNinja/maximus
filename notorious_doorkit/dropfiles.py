"""Dropfile parsing utilities.

This module provides best-effort parsing for classic BBS dropfiles.

The goal is practical door usage:
- Load a dropfile into a normalized object (with raw lines retained)
- Provide typed accessors for commonly-used fields
- Optionally write back updates during teardown

Supported formats (best-effort):
- DOOR.SYS
- DORINFO1.DEF
- DORINFO2.DEF
- CHAIN.TXT

If a format is unknown or partially recognized, the returned object still
contains `raw_lines` so door authors can pull what they need.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import Dict, List, Optional


class DropfileType(str, Enum):
    DOOR_SYS = "door.sys"
    DOORFILE_SR = "doorfile.sr"
    DORINFO1 = "dorinfo1.def"
    DORINFO2 = "dorinfo2.def"
    CHAIN_TXT = "chain.txt"
    UNKNOWN = "unknown"


@dataclass
class DropfileUser:
    full_name: Optional[str] = None
    alias: Optional[str] = None
    city: Optional[str] = None
    phone: Optional[str] = None


@dataclass
class DropfileInfo:
    """Normalized dropfile information.

    `raw_lines` always preserves the original file (minus trailing CR/LF).
    """

    drop_type: DropfileType
    path: Path
    raw_lines: List[str]

    user: DropfileUser = field(default_factory=DropfileUser)

    node: Optional[int] = None
    time_left_secs: Optional[int] = None

    graphics_mode: Optional[str] = None
    screen_rows: Optional[int] = None

    temp_path: Optional[str] = None
    bbs_root: Optional[str] = None

    extra: Dict[str, str] = field(default_factory=dict)  # format-specific, best-effort


def _read_text_lines(path: Path, encoding: str = "cp437") -> List[str]:
    try:
        raw = path.read_text(encoding=encoding, errors="replace")
    except FileNotFoundError:
        return []
    except OSError:
        return []

    return [line.rstrip("\r\n") for line in raw.splitlines()]


def _safe_int(s: str) -> Optional[int]:
    try:
        return int(s.strip())
    except Exception:
        return None


def _line(lines: List[str], idx_1_based: int) -> str:
    if idx_1_based <= 0:
        return ""
    i = idx_1_based - 1
    return lines[i].strip() if 0 <= i < len(lines) else ""


def parse_door_sys(path: Path) -> DropfileInfo:
    lines = _read_text_lines(path, encoding="cp437")

    full_name = _line(lines, 10) or None
    city = _line(lines, 11) or None
    phone = _line(lines, 12) or None
    # DOOR.SYS formats vary, but in the common mapping (incl FSXNet docs),
    # line 14 is the user's password and line 36 is the alias.
    # We intentionally do NOT fall back to line 14 to avoid leaking passwords.
    alias = _line(lines, 36) or None

    # Time-left is commonly present as seconds (line 18) and minutes (line 19)
    secs = _safe_int(_line(lines, 18))
    mins = _safe_int(_line(lines, 19))

    time_left_secs: Optional[int] = None
    if secs is not None and secs > 0:
        time_left_secs = secs
    elif mins is not None and mins > 0:
        time_left_secs = mins * 60

    screen_rows = _safe_int(_line(lines, 21))

    # A lot of systems expose a graphics/mode indicator somewhere, but it is not
    # reliably standardized across all DOOR.SYS variants. We keep a best-effort
    # value if present.
    graphics_mode = _line(lines, 22) or None

    temp_path = _line(lines, 33) or None
    bbs_root = _line(lines, 34) or None

    user = DropfileUser(full_name=full_name, alias=alias or full_name, city=city, phone=phone)

    info = DropfileInfo(
        drop_type=DropfileType.DOOR_SYS,
        path=path,
        raw_lines=lines,
        user=user,
        time_left_secs=time_left_secs,
        graphics_mode=graphics_mode,
        screen_rows=screen_rows,
        temp_path=temp_path,
        bbs_root=bbs_root,
    )

    return info


def parse_chain_txt(path: Path) -> DropfileInfo:
    lines = _read_text_lines(path, encoding="cp437")

    user = DropfileUser(full_name=None, alias=None)

    info = DropfileInfo(
        drop_type=DropfileType.CHAIN_TXT,
        path=path,
        raw_lines=lines,
        user=user,
    )

    alias = _line(lines, 2) or None
    full_name = _line(lines, 3) or None
    if alias:
        info.user.alias = alias
    if full_name:
        info.user.full_name = full_name

    time_left = _safe_int(_line(lines, 16))
    if time_left is not None and time_left >= 0:
        info.time_left_secs = time_left

    cols = _safe_int(_line(lines, 9))
    if cols is not None and cols > 0:
        info.extra["screen_cols"] = str(cols)
    rows = _safe_int(_line(lines, 10))
    if rows is not None and rows > 0:
        info.screen_rows = rows

    ansi = _safe_int(_line(lines, 14))
    if ansi is not None:
        info.extra["ansi"] = "1" if ansi != 0 else "0"
        info.graphics_mode = "1" if ansi != 0 else "0"

    return info


def parse_door32_sys(path: Path) -> DropfileInfo:
    lines = _read_text_lines(path, encoding="cp437")

    comm_type = _safe_int(_line(lines, 1))
    handle = _safe_int(_line(lines, 2))

    real_name = _line(lines, 6) or None
    alias = _line(lines, 7) or None
    time_left_mins = _safe_int(_line(lines, 9))
    emulation = _safe_int(_line(lines, 10))
    node = _safe_int(_line(lines, 11))

    user = DropfileUser(full_name=real_name, alias=alias or real_name)

    time_left_secs: Optional[int] = None
    if time_left_mins is not None and time_left_mins > 0:
        time_left_secs = time_left_mins * 60

    graphics_mode = None
    if emulation is not None:
        graphics_mode = "1" if emulation != 0 else "0"

    info = DropfileInfo(
        drop_type=DropfileType.UNKNOWN,
        path=path,
        raw_lines=lines,
        user=user,
        node=node,
        time_left_secs=time_left_secs,
        graphics_mode=graphics_mode,
    )

    if comm_type is not None:
        info.extra["door32_comm_type"] = str(comm_type)
    if handle is not None:
        info.extra["door32_handle"] = str(handle)

    return info


def parse_doorfile_sr(path: Path) -> DropfileInfo:
    lines = _read_text_lines(path, encoding="cp437")

    handle = _line(lines, 1) or None
    time_limit_mins = _safe_int(_line(lines, 7))
    real_name = _line(lines, 8) or None

    user = DropfileUser(full_name=real_name or handle, alias=handle or real_name)

    time_left_secs: Optional[int] = None
    if time_limit_mins is not None and time_limit_mins >= 0:
        time_left_secs = time_limit_mins * 60

    info = DropfileInfo(
        drop_type=DropfileType.DOORFILE_SR,
        path=path,
        raw_lines=lines,
        user=user,
        time_left_secs=time_left_secs,
    )

    ansi = _safe_int(_line(lines, 2))
    if ansi is not None:
        info.extra["ansi"] = str(ansi)
    ibm = _safe_int(_line(lines, 3))
    if ibm is not None:
        info.extra["ibm_graphics"] = str(ibm)
    page_len = _safe_int(_line(lines, 4))
    if page_len is not None and page_len > 0:
        info.screen_rows = page_len
    baud = _safe_int(_line(lines, 5))
    if baud is not None:
        info.extra["baud"] = str(baud)
    com_port = _safe_int(_line(lines, 6))
    if com_port is not None:
        info.extra["com_port"] = str(com_port)

    return info


def parse_dorinfo_def(path: Path, which: DropfileType) -> DropfileInfo:
    lines = _read_text_lines(path, encoding="cp437")

    info = DropfileInfo(
        drop_type=which,
        path=path,
        raw_lines=lines,
        user=DropfileUser(),
    )

    # DORINFO1/2 formats vary. We use a conservative best-effort mapping.
    # If the file has an obvious username line, pick it up.
    for candidate in lines:
        c = candidate.strip()
        if c and not c.isdigit() and len(c) <= 40:
            info.user.full_name = c
            info.user.alias = c
            break

    return info


def detect_dropfile_type(path: Path) -> DropfileType:
    name = path.name.lower()
    if name == "door.sys":
        return DropfileType.DOOR_SYS
    if name == "door32.sys":
        return DropfileType.UNKNOWN  # Will be handled explicitly in load_dropfile
    if name == "doorfile.sr":
        return DropfileType.DOORFILE_SR
    if name == "dorinfo1.def":
        return DropfileType.DORINFO1
    if name == "dorinfo2.def":
        return DropfileType.DORINFO2
    if name == "chain.txt":
        return DropfileType.CHAIN_TXT
    return DropfileType.UNKNOWN


def load_dropfile(path: str | Path, drop_type: Optional[str] = None) -> DropfileInfo:
    p = Path(path)
    dt = DropfileType.UNKNOWN

    if drop_type:
        norm = drop_type.strip().lower()
        for t in DropfileType:
            if t.value == norm:
                dt = t
                break
    else:
        dt = detect_dropfile_type(p)

    # Check for door32.sys explicitly by filename
    if p.name.lower() == "door32.sys":
        return parse_door32_sys(p)

    if dt == DropfileType.DOOR_SYS:
        return parse_door_sys(p)
    if dt == DropfileType.DOORFILE_SR:
        return parse_doorfile_sr(p)
    if dt == DropfileType.CHAIN_TXT:
        return parse_chain_txt(p)
    if dt == DropfileType.DORINFO1:
        return parse_dorinfo_def(p, DropfileType.DORINFO1)
    if dt == DropfileType.DORINFO2:
        return parse_dorinfo_def(p, DropfileType.DORINFO2)

    lines = _read_text_lines(p, encoding="cp437")
    return DropfileInfo(drop_type=DropfileType.UNKNOWN, path=p, raw_lines=lines)


def write_door_sys(info: DropfileInfo, path: Optional[str | Path] = None) -> None:
    if info.drop_type != DropfileType.DOOR_SYS:
        raise ValueError("write_door_sys only supports DOOR.SYS")

    out_path = Path(path) if path is not None else info.path

    # Preserve the existing line count if possible.
    lines = list(info.raw_lines)

    def ensure_len(n: int) -> None:
        while len(lines) < n:
            lines.append("")

    # Some DOOR.SYS variants extend past 34 lines; we only extend as needed.
    ensure_len(34)

    if info.user.full_name is not None:
        lines[9] = info.user.full_name
    if info.user.city is not None:
        lines[10] = info.user.city
    if info.user.phone is not None:
        lines[11] = info.user.phone
    if info.user.alias is not None:
        # Avoid overwriting line 14 (index 13), which is commonly the password.
        ensure_len(36)
        lines[35] = info.user.alias

    if info.screen_rows is not None:
        lines[20] = str(int(info.screen_rows))

    if info.temp_path is not None:
        lines[32] = info.temp_path
    if info.bbs_root is not None:
        lines[33] = info.bbs_root

    out_path.write_text("\r\n".join(lines) + "\r\n", encoding="cp437", errors="replace")


class DropfileSession:
    """Context manager for "load, use, write-back" dropfile lifecycles."""

    def __init__(self, path: str | Path, drop_type: Optional[str] = None, write_back: bool = False):
        self.path = Path(path)
        self.drop_type = drop_type
        self.write_back = write_back
        self.info: Optional[DropfileInfo] = None

    def __enter__(self) -> DropfileInfo:
        self.info = load_dropfile(self.path, drop_type=self.drop_type)
        return self.info

    def __exit__(self, exc_type, exc, tb) -> bool:
        if self.write_back and self.info is not None:
            if self.info.drop_type == DropfileType.DOOR_SYS:
                write_door_sys(self.info)
        return False
