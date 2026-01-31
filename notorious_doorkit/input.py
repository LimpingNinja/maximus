"""
Raw mode input handling and keystroke parsing.
"""

import os
import select
import sys
import time
from dataclasses import dataclass, field
from typing import Callable, Dict, List, Optional, Tuple

try:
    import termios  # type: ignore
    import tty  # type: ignore
except Exception:  # pragma: no cover
    termios = None  # type: ignore
    tty = None  # type: ignore

# Key constants
KEY_ENTER = "\r"
KEY_ESC = "\x1b"
KEY_BACKSPACE = "\x08"
# Note: many clients send DEL (0x7f) for backspace.
KEY_DELETE = "\x7f"
KEY_TAB = "\t"
KEY_UP = "KEY_UP"
KEY_DOWN = "KEY_DOWN"
KEY_LEFT = "KEY_LEFT"
KEY_RIGHT = "KEY_RIGHT"
KEY_HOME = "KEY_HOME"
KEY_END = "KEY_END"
KEY_PAGEUP = "KEY_PAGEUP"
KEY_PAGEDOWN = "KEY_PAGEDOWN"
KEY_INSERT = "KEY_INSERT"
KEY_F1 = "KEY_F1"
KEY_F2 = "KEY_F2"
KEY_F3 = "KEY_F3"
KEY_F4 = "KEY_F4"
KEY_F5 = "KEY_F5"
KEY_F6 = "KEY_F6"
KEY_F7 = "KEY_F7"
KEY_F8 = "KEY_F8"
KEY_F9 = "KEY_F9"
KEY_F10 = "KEY_F10"
KEY_F11 = "KEY_F11"
KEY_F12 = "KEY_F12"

GETIN_NORMAL = 0x0000
GETIN_RAW = 0x0001
GETIN_RAWCTRL = 0x0002

NO_TIMEOUT = 0xFFFFFFFF

KEYCODE_F1 = 0x3B
KEYCODE_F2 = 0x3C
KEYCODE_F3 = 0x3D
KEYCODE_F4 = 0x3E
KEYCODE_F5 = 0x3F
KEYCODE_F6 = 0x40
KEYCODE_F7 = 0x41
KEYCODE_F8 = 0x42
KEYCODE_F9 = 0x43
KEYCODE_F10 = 0x44
KEYCODE_UP = 0x48
KEYCODE_DOWN = 0x50
KEYCODE_LEFT = 0x4B
KEYCODE_RIGHT = 0x4D
KEYCODE_INSERT = 0x52
KEYCODE_DELETE = 0x53
KEYCODE_HOME = 0x47
KEYCODE_END = 0x4F
KEYCODE_PGUP = 0x49
KEYCODE_PGDN = 0x51
KEYCODE_F11 = 0x85
KEYCODE_F12 = 0x86
KEYCODE_SHIFTTAB = 0x0F

EVENT_CHARACTER = "EVENT_CHARACTER"
EVENT_EXTENDED_KEY = "EVENT_EXTENDED_KEY"

_GETIN_MAX_CHARACTER_LATENCY_S = 0.250
_GETIN_SEQUENCE_BUFFER_SIZE = 10


def _getin_iter_key_sequences(flags: int, extended_keys: bool = True):
    raw = (flags & GETIN_RAW) != 0
    rawctrl = (flags & GETIN_RAWCTRL) != 0
    if raw:
        return

    # (sequence_bytes, keycode, is_control_key)
    base_seqs = [
        (b"\x1bA", KEYCODE_UP, False),
        (b"\x1bB", KEYCODE_DOWN, False),
        (b"\x1bC", KEYCODE_RIGHT, False),
        (b"\x1bD", KEYCODE_LEFT, False),
        (b"\x1bH", KEYCODE_HOME, False),
        (b"\x1bK", KEYCODE_END, False),
        (b"\x1bP", KEYCODE_F1, False),
        (b"\x1bQ", KEYCODE_F2, False),
        (b"\x1b?w", KEYCODE_F3, False),
        (b"\x1b?x", KEYCODE_F4, False),
        (b"\x1b?t", KEYCODE_F5, False),
        (b"\x1b?u", KEYCODE_F6, False),
        (b"\x1b?q", KEYCODE_F7, False),
        (b"\x1b?r", KEYCODE_F8, False),
        (b"\x1b?p", KEYCODE_F9, False),

        (b"\x1b[A", KEYCODE_UP, False),
        (b"\x1b[B", KEYCODE_DOWN, False),
        (b"\x1b[C", KEYCODE_RIGHT, False),
        (b"\x1b[D", KEYCODE_LEFT, False),
        (b"\x1b[M", KEYCODE_PGUP, False),
        (b"\x1b[H\x1b[2J", KEYCODE_PGDN, False),
        (b"\x1b[H", KEYCODE_HOME, False),
        (b"\x1b[K", KEYCODE_END, False),
        (b"\x1bOP", KEYCODE_F1, False),
        (b"\x1bOQ", KEYCODE_F2, False),
        (b"\x1bOR", KEYCODE_F3, False),
        (b"\x1bOS", KEYCODE_F4, False),

        (b"\x1b[1~", KEYCODE_HOME, False),
        (b"\x1b[2~", KEYCODE_INSERT, False),
        (b"\x1b[3~", KEYCODE_DELETE, False),
        (b"\x1b[4~", KEYCODE_END, False),
        (b"\x1b[5~", KEYCODE_PGUP, False),
        (b"\x1b[6~", KEYCODE_PGDN, False),
        (b"\x1b[17~", KEYCODE_F6, False),
        (b"\x1b[18~", KEYCODE_F7, False),
        (b"\x1b[19~", KEYCODE_F8, False),
        (b"\x1b[20~", KEYCODE_F9, False),
        (b"\x1b[21~", KEYCODE_F10, False),
        (b"\x1b[23~", KEYCODE_F11, False),
        (b"\x1b[24~", KEYCODE_F12, False),

        (b"\x1b[15~", KEYCODE_F5, False),

        (b"\x1b[Z", KEYCODE_SHIFTTAB, False),

        (b"\x1b[11~", KEYCODE_F1, False),
        (b"\x1b[12~", KEYCODE_F2, False),
        (b"\x1b[13~", KEYCODE_F3, False),
        (b"\x1b[14~", KEYCODE_F4, False),

        (b"\x1b[L", KEYCODE_HOME, False),
        (b"\x1bOw", KEYCODE_F3, False),
        (b"\x1bOx", KEYCODE_F4, False),
        (b"\x1bOt", KEYCODE_F5, False),
        (b"\x1bOu", KEYCODE_F6, False),
        (b"\x1bOq", KEYCODE_F7, False),
        (b"\x1bOr", KEYCODE_F8, False),
        (b"\x1bOp", KEYCODE_F9, False),

        (b"\x1bOA", KEYCODE_UP, False),
        (b"\x1bOB", KEYCODE_DOWN, False),
        (b"\x1bOC", KEYCODE_RIGHT, False),
        (b"\x1bOD", KEYCODE_LEFT, False),
        (b"\x1bOH", KEYCODE_HOME, False),
        (b"\x1bOK", KEYCODE_END, False),

        (b"\x16\t", KEYCODE_INSERT, True),

        (b"\x7f", KEYCODE_DELETE, True),
        (b"\x05", KEYCODE_UP, True),
        (b"\x18", KEYCODE_DOWN, True),
        (b"\x13", KEYCODE_LEFT, True),
        (b"\x04", KEYCODE_RIGHT, True),
        (b"\x07", KEYCODE_DELETE, True),
        (b"\x16", KEYCODE_INSERT, True),
    ]

    if extended_keys:
        seqs = base_seqs
    else:
        seqs = [
            (b"\x1bA", KEYCODE_UP, False),
            (b"\x1bB", KEYCODE_DOWN, False),
            (b"\x1bC", KEYCODE_RIGHT, False),
            (b"\x1bD", KEYCODE_LEFT, False),
            (b"\x1bH", KEYCODE_HOME, False),
            (b"\x1bK", KEYCODE_END, False),
            (b"\x1b[A", KEYCODE_UP, False),
            (b"\x1b[B", KEYCODE_DOWN, False),
            (b"\x1b[C", KEYCODE_RIGHT, False),
            (b"\x1b[D", KEYCODE_LEFT, False),
            (b"\x1b[H", KEYCODE_HOME, False),
            (b"\x1b[K", KEYCODE_END, False),
        ]

    for seq, keycode, is_ctrl in seqs:
        if rawctrl and is_ctrl:
            continue
        yield (seq, keycode)


EDIT_FLAG_NORMAL = 0x0000
EDIT_FLAG_NO_REDRAW = 0x0001
EDIT_FLAG_FIELD_MODE = 0x0002
EDIT_FLAG_EDIT_STRING = 0x0004
EDIT_FLAG_STRICT_INPUT = 0x0008
EDIT_FLAG_PASSWORD_MODE = 0x0010
EDIT_FLAG_ALLOW_CANCEL = 0x0020
EDIT_FLAG_FILL_STRING = 0x0040
EDIT_FLAG_AUTO_ENTER = 0x0080
EDIT_FLAG_AUTO_DELETE = 0x0100
EDIT_FLAG_KEEP_BLANK = 0x0200
EDIT_FLAG_PERMALITERAL = 0x0400
EDIT_FLAG_LEAVE_BLANK = 0x0800
EDIT_FLAG_SHOW_SIZE = 0x1000

EDIT_RETURN_ERROR = 0
EDIT_RETURN_CANCEL = 1
EDIT_RETURN_ACCEPT = 2
EDIT_RETURN_PREVIOUS = 3
EDIT_RETURN_NEXT = 4

# Distinguish "forward delete" (Delete key / ESC [ 3 ~) from 0x7f backspace.
KEY_DEL = "KEY_DEL"

KEY_SHIFTTAB = "KEY_SHIFTTAB"


def parse_ansi_sequence(seq: str) -> Optional[str]:
    """
    Parse ANSI escape sequence to key constant.
    
    Args:
        seq: Escape sequence (including ESC)
        
    Returns:
        Key constant or None if not recognized
    """
    # SS3 variants (common in some terminals / keypad modes)
    if len(seq) == 3 and seq.startswith("\x1bO"):
        code = seq[2]
        if code == "A":
            return KEY_UP
        if code == "B":
            return KEY_DOWN
        if code == "C":
            return KEY_RIGHT
        if code == "D":
            return KEY_LEFT
        if code == "H":
            return KEY_HOME
        if code == "F":
            return KEY_END
        return None

    # Tilde-terminated CSI sequences
    if seq == "\x1b[H" or seq == "\x1b[1~":
        return KEY_HOME
    if seq == "\x1b[F" or seq == "\x1b[4~":
        return KEY_END
    if seq == "\x1b[5~":
        return KEY_PAGEUP
    if seq == "\x1b[6~":
        return KEY_PAGEDOWN
    if seq == "\x1b[2~":
        return KEY_INSERT
    if seq == "\x1b[3~":
        return KEY_DEL

    if seq == "\x1b[A":
        return KEY_UP
    elif seq == "\x1b[B":
        return KEY_DOWN
    elif seq == "\x1b[C":
        return KEY_RIGHT
    elif seq == "\x1b[D":
        return KEY_LEFT
    elif seq == "\x1b[Z":
        return KEY_SHIFTTAB
    elif seq.startswith("\x1b["):
        # CSI with modifiers, e.g. ESC [ 1 ; 2 B
        # Treat any recognized final letter as the key.
        import re
        m = re.match(r"^\x1b\[[0-9;]*([A-Za-z])$", seq)
        if m:
            code = m.group(1)
            if code == "A":
                return KEY_UP
            if code == "B":
                return KEY_DOWN
            if code == "C":
                return KEY_RIGHT
            if code == "D":
                return KEY_LEFT
            if code == "H":
                return KEY_HOME
            if code == "F":
                return KEY_END
    elif seq == "\x1bOP" or seq == "\x1b[11~":
        return KEY_F1
    elif seq == "\x1bOQ" or seq == "\x1b[12~":
        return KEY_F2
    elif seq == "\x1bOR" or seq == "\x1b[13~":
        return KEY_F3
    elif seq == "\x1bOS" or seq == "\x1b[14~":
        return KEY_F4
    elif seq == "\x1b[15~":
        return KEY_F5
    elif seq == "\x1b[17~":
        return KEY_F6
    elif seq == "\x1b[18~":
        return KEY_F7
    elif seq == "\x1b[19~":
        return KEY_F8
    elif seq == "\x1b[20~":
        return KEY_F9
    elif seq == "\x1b[21~":
        return KEY_F10
    elif seq == "\x1b[23~":
        return KEY_F11
    elif seq == "\x1b[24~":
        return KEY_F12
    return None


def is_printable(ch: str) -> bool:
    """
    Check if character is printable.
    
    Args:
        ch: Character to check
        
    Returns:
        True if printable
    """
    if len(ch) != 1:
        return False
    code = ord(ch)
    return 32 <= code <= 126 or code >= 160


@dataclass
class _InputStreamState:
    fd: int
    inbuf: bytearray = field(default_factory=bytearray)
    raw_mode_enabled: bool = False
    raw_mode_refcount: int = 0
    tty_saved_attrs: Optional[List[int]] = None
    tty_raw_applied: bool = False
    getin_sequence: bytearray = field(default_factory=bytearray)
    getin_doorway_sequence: bool = False
    getin_doorway_sequence_pending: bool = False
    extended_keys: bool = False


_STREAM_STATES: Dict[int, _InputStreamState] = {}


def _get_stream_state(fd: int) -> _InputStreamState:
    st = _STREAM_STATES.get(fd)
    if st is None:
        st = _InputStreamState(fd=fd)
        _STREAM_STATES[fd] = st
    return st


def _read_into_stream_state(fd: int, timeout: Optional[float]) -> bool:
    st = _get_stream_state(fd)
    try:
        ready, _, _ = select.select([fd], [], [], timeout)
    except Exception:
        return False
    if not ready:
        return False
    try:
        data = os.read(fd, 1024)
    except Exception:
        return False
    if not data:
        return False
    st.inbuf.extend(data)
    return True


def _enable_raw_mode_fd(fd: int) -> None:
    st = _get_stream_state(fd)
    was_enabled = st.raw_mode_refcount > 0
    st.raw_mode_refcount += 1
    st.raw_mode_enabled = True

    if was_enabled:
        return
    if termios is None or tty is None:
        return
    try:
        if not os.isatty(fd):
            return
    except Exception:
        return

    try:
        if st.tty_saved_attrs is None:
            st.tty_saved_attrs = termios.tcgetattr(fd)
        tty.setraw(fd, when=termios.TCSANOW)
        st.tty_raw_applied = True
    except Exception:
        st.tty_raw_applied = False


def _disable_raw_mode_fd(fd: int) -> None:
    st = _get_stream_state(fd)
    if st.raw_mode_refcount > 0:
        st.raw_mode_refcount -= 1
    if st.raw_mode_refcount > 0:
        return
    if not st.raw_mode_enabled:
        return
    st.raw_mode_enabled = False

    if termios is None:
        return
    if not st.tty_raw_applied:
        return
    saved = st.tty_saved_attrs
    if saved is None:
        return

    try:
        termios.tcsetattr(fd, termios.TCSANOW, saved)
    except Exception:
        return
    finally:
        st.tty_raw_applied = False


class RawInput:
    """
    Context manager for raw mode input.
    
    Example:
        >>> with RawInput() as inp:
            key = inp.get_key()
            if key == KEY_ENTER:
                print("Enter pressed")
        ...     key = inp.get_key()
        ...     if key == KEY_ENTER:
        ...         print("Enter pressed")
    """
    
    def __init__(self, extended_keys: bool = False):
        """Initialize raw input handler.
        
        Args:
            extended_keys: If True, parse longer escape sequences for PgUp/PgDn/Ins/Del/etc.
                          If False (default), only parse basic arrows/home/end.
        """
        self._escape_buffer = ""
        self._escape_timeout = 0.1  # 100ms timeout for escape sequences
        self._esc_grace_timeout = 0.03
        self._extended_keys = extended_keys
        try:
            from .runtime import get_terminal_fd

            fd = get_terminal_fd(default=None)
        except Exception:
            fd = None
        self._fd = int(fd) if fd is not None else sys.stdin.fileno()
        st = _STREAM_STATES.get(self._fd)
        if st is None:
            st = _InputStreamState(fd=self._fd)
            _STREAM_STATES[self._fd] = st
        self._st = st
        if extended_keys:
            self._st.extended_keys = True

    def _getin_shift_seq(self, chars: int) -> None:
        if chars <= 0:
            return
        if chars >= len(self._st.getin_sequence):
            self._st.getin_sequence = bytearray()
            return
        self._st.getin_sequence = self._st.getin_sequence[chars:]

    def _getin_have_start_of_sequence(self, flags: int, *, extended_keys: bool) -> bool:
        if (flags & GETIN_RAW) != 0:
            return False
        if not self._st.getin_sequence:
            return False
        prefix = bytes(self._st.getin_sequence)
        for seq, _keycode in _getin_iter_key_sequences(flags, extended_keys=extended_keys):
            if seq[: len(prefix)] == prefix:
                return True
        return False

    def _getin_get_code_if_longest(self, flags: int, *, extended_keys: bool):
        if (flags & GETIN_RAW) != 0:
            return None
        if not self._st.getin_sequence:
            return None

        buf = bytes(self._st.getin_sequence)
        buf_len = len(buf)
        curr_len = 0
        retval = None
        for seq, keycode in _getin_iter_key_sequences(flags, extended_keys=extended_keys):
            seqlen = len(seq)
            if seqlen <= curr_len:
                continue
            if seqlen <= buf_len:
                if seq[:seqlen] == buf[:seqlen]:
                    retval = (seq, keycode)
                    curr_len = seqlen
            else:
                if seq[:buf_len] == buf:
                    return None
        return retval

    def _getin_longest_full_code(self, flags: int, *, extended_keys: bool):
        if (flags & GETIN_RAW) != 0:
            return None
        if not self._st.getin_sequence:
            return None

        buf = bytes(self._st.getin_sequence)
        curr_len = 0
        retval = None
        for seq, keycode in _getin_iter_key_sequences(flags, extended_keys=extended_keys):
            seqlen = len(seq)
            if seqlen <= curr_len:
                continue
            if buf[:seqlen] == seq:
                retval = (seq, keycode)
                curr_len = seqlen
        return retval

    def _fill_inbuf(self, timeout: Optional[float]) -> bool:
        return _read_into_stream_state(self._fd, timeout)

    def _read_byte(self, timeout: Optional[float]) -> Optional[int]:
        if not self._st.inbuf:
            if not self._fill_inbuf(timeout):
                return None
        b = self._st.inbuf[0]
        del self._st.inbuf[0]
        return b

    def _pushback_bytes(self, bs: bytes) -> None:
        if not bs:
            return
        self._st.inbuf = bytearray(bs) + self._st.inbuf
        
    def __enter__(self):
        """Enable raw mode."""
        self.enable_raw_mode()
        return self
        
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Disable raw mode."""
        self.disable_raw_mode()
        return False
        
    def enable_raw_mode(self) -> None:
        """Enable raw mode.

        DoorKit input is treated as raw by default. This method is retained for
        API compatibility and for future transport-specific behavior.
        """
        was_enabled = self._st.raw_mode_refcount > 0
        self._st.raw_mode_refcount += 1
        self._st.raw_mode_enabled = True

        if was_enabled:
            return

        if termios is None or tty is None:
            return

        try:
            if not os.isatty(self._fd):
                return
        except Exception:
            return

        try:
            if self._st.tty_saved_attrs is None:
                self._st.tty_saved_attrs = termios.tcgetattr(self._fd)
            tty.setraw(self._fd, when=termios.TCSANOW)
            self._st.tty_raw_applied = True
        except Exception:
            self._st.tty_raw_applied = False

    def disable_raw_mode(self) -> None:
        """Disable raw mode."""
        if self._st.raw_mode_refcount > 0:
            self._st.raw_mode_refcount -= 1
        if self._st.raw_mode_refcount > 0:
            return
        if not self._st.raw_mode_enabled:
            return
        self._st.raw_mode_enabled = False

        if termios is None:
            return

        if not self._st.tty_raw_applied:
            return

        saved = self._st.tty_saved_attrs
        if saved is None:
            return

        try:
            termios.tcsetattr(self._fd, termios.TCSANOW, saved)
        except Exception:
            return
        finally:
            self._st.tty_raw_applied = False
        
    def key_pressed(self) -> bool:
        """
        Check if a key is available without blocking.
        
        Returns:
            True if key available
        """
        if self._st.inbuf:
            return True
        ready, _, _ = select.select([self._fd], [], [], 0)
        return bool(ready)

    def clear_keybuffer(self) -> None:
        self._st.inbuf.clear()
        self._st.getin_sequence = bytearray()
        self._st.getin_doorway_sequence = False
        self._st.getin_doorway_sequence_pending = False
        while True:
            ready, _, _ = select.select([self._fd], [], [], 0)
            if not ready:
                break
            data = os.read(self._fd, 4096)
            if not data:
                break

    def get_input_event(
        self,
        time_to_wait_ms: int = NO_TIMEOUT,
        flags: int = GETIN_NORMAL,
        *,
        extended_keys: Optional[bool] = None,
    ) -> Optional["InputEvent"]:
        ext = self._st.extended_keys if extended_keys is None else bool(extended_keys)
        timeout_first: Optional[float]
        if time_to_wait_ms == NO_TIMEOUT:
            timeout_first = None
        else:
            timeout_first = max(0.0, float(time_to_wait_ms) / 1000.0)

        if (flags & GETIN_RAW) != 0:
            b = self._read_byte(timeout_first)
            if b is None:
                return None
            return InputEvent(EVENT_CHARACTER, True, int(b))

        if (not self._st.getin_doorway_sequence) and self._st.getin_doorway_sequence_pending and (
            not self._st.getin_sequence
        ):
            self._st.getin_doorway_sequence_pending = False
            self._st.getin_doorway_sequence = True

        if (not self._st.getin_sequence) and (not self._st.getin_doorway_sequence):
            b0 = self._read_byte(timeout_first)
            if b0 is None:
                return None
            if int(b0) == 0:
                self._st.getin_doorway_sequence = True
            else:
                self._st.getin_sequence.append(int(b0) & 0xFF)

        if (not self._st.getin_doorway_sequence) and (
            not self._getin_have_start_of_sequence(flags, extended_keys=ext)
        ):
            ch = int(self._st.getin_sequence[0]) if self._st.getin_sequence else 0
            if self._st.getin_sequence:
                self._getin_shift_seq(1)
            return InputEvent(EVENT_CHARACTER, True, ch)

        match = None if self._st.getin_doorway_sequence else self._getin_get_code_if_longest(flags, extended_keys=ext)
        if match is not None:
            seq, keycode = match
            self._getin_shift_seq(len(seq))
            return InputEvent(EVENT_EXTENDED_KEY, True, int(keycode))

        while not self._st.getin_doorway_sequence_pending:
            b = self._read_byte(_GETIN_MAX_CHARACTER_LATENCY_S)
            if b is None:
                break

            bb = int(b) & 0xFF

            if self._st.getin_doorway_sequence:
                self._st.getin_doorway_sequence = False
                return InputEvent(EVENT_EXTENDED_KEY, True, bb)

            if bb == 0:
                self._st.getin_doorway_sequence_pending = True
                break

            if len(self._st.getin_sequence) < _GETIN_SEQUENCE_BUFFER_SIZE:
                self._st.getin_sequence.append(bb)

            match = self._getin_get_code_if_longest(flags, extended_keys=ext)
            if match is not None:
                seq, keycode = match
                self._getin_shift_seq(len(seq))
                return InputEvent(EVENT_EXTENDED_KEY, True, int(keycode))

        if self._st.getin_doorway_sequence:
            self._st.getin_doorway_sequence = False
            return InputEvent(EVENT_CHARACTER, True, 0)

        match = self._getin_longest_full_code(flags, extended_keys=ext)
        if match is not None:
            seq, keycode = match
            self._getin_shift_seq(len(seq))
            return InputEvent(EVENT_EXTENDED_KEY, True, int(keycode))

        ch = int(self._st.getin_sequence[0]) if self._st.getin_sequence else 0
        if self._st.getin_sequence:
            self._getin_shift_seq(1)
        if ch == 0:
            return None
        return InputEvent(EVENT_CHARACTER, True, ch)
        
    def get_key(self, timeout: Optional[float] = None) -> Optional[str]:
        """
        Read a single keystroke (blocking or with timeout).
        
        Args:
            timeout: Timeout in seconds (None = blocking)
            
        Returns:
            Key string or key constant, or None on timeout
        """
        if timeout is None:
            time_to_wait_ms = NO_TIMEOUT
        else:
            time_to_wait_ms = max(0, int(timeout * 1000))

        ev = self.get_input_event(
            time_to_wait_ms=time_to_wait_ms,
            flags=GETIN_RAWCTRL,
            extended_keys=self._st.extended_keys,
        )
        if ev is None:
            return None

        if ev.event_type == EVENT_CHARACTER:
            return chr(int(ev.key_press) & 0xFF)

        keycode = int(ev.key_press)
        keymap = {
            KEYCODE_UP: KEY_UP,
            KEYCODE_DOWN: KEY_DOWN,
            KEYCODE_LEFT: KEY_LEFT,
            KEYCODE_RIGHT: KEY_RIGHT,
            KEYCODE_HOME: KEY_HOME,
            KEYCODE_END: KEY_END,
            KEYCODE_PGUP: KEY_PAGEUP,
            KEYCODE_PGDN: KEY_PAGEDOWN,
            KEYCODE_INSERT: KEY_INSERT,
            KEYCODE_DELETE: KEY_DEL,
            KEYCODE_F1: KEY_F1,
            KEYCODE_F2: KEY_F2,
            KEYCODE_F3: KEY_F3,
            KEYCODE_F4: KEY_F4,
            KEYCODE_F5: KEY_F5,
            KEYCODE_F6: KEY_F6,
            KEYCODE_F7: KEY_F7,
            KEYCODE_F8: KEY_F8,
            KEYCODE_F9: KEY_F9,
            KEYCODE_F10: KEY_F10,
            KEYCODE_F11: KEY_F11,
            KEYCODE_F12: KEY_F12,
            KEYCODE_SHIFTTAB: KEY_SHIFTTAB,
        }
        return keymap.get(keycode, KEY_ESC)
            
    def read_line(self, prompt: str = "", max_length: int = 255, echo: bool = True) -> str:
        """
        Read a line of input with editing support.
        
        Args:
            prompt: Prompt to display
            max_length: Maximum input length
            echo: Echo input characters
            
        Returns:
            Input string
        """
        from .ansi import write, clear_to_eol
        
        if prompt:
            write(prompt)
            
        buffer = []
        cursor_pos = 0
        
        while True:
            key = self.get_key()
            
            if key == KEY_ENTER:
                if echo:
                    write("\r\n")
                return ''.join(buffer)
                
            elif key == KEY_BACKSPACE or key == KEY_DELETE:
                if cursor_pos > 0:
                    buffer.pop(cursor_pos - 1)
                    cursor_pos -= 1
                    if echo:
                        # Redraw line
                        write('\r' + prompt + ''.join(buffer) + ' ')
                        clear_to_eol()
                        write('\r' + prompt + ''.join(buffer))
                        # Position cursor
                        if cursor_pos < len(buffer):
                            write(f"\x1b[{len(prompt) + cursor_pos + 1}G")
                            
            elif key == KEY_LEFT:
                if cursor_pos > 0:
                    cursor_pos -= 1
                    if echo:
                        write("\x1b[D")
                        
            elif key == KEY_RIGHT:
                if cursor_pos < len(buffer):
                    cursor_pos += 1
                    if echo:
                        write("\x1b[C")
                        
            elif key == KEY_HOME:
                cursor_pos = 0
                if echo:
                    write('\r' + prompt)
                    
            elif key == KEY_END:
                cursor_pos = len(buffer)
                if echo:
                    write(f"\x1b[{len(prompt) + len(buffer) + 1}G")
                    
            elif key and len(key) == 1 and is_printable(key):
                if len(buffer) < max_length:
                    buffer.insert(cursor_pos, key)
                    cursor_pos += 1
                    if echo:
                        # Redraw from cursor position
                        write(''.join(buffer[cursor_pos-1:]))
                        if cursor_pos < len(buffer):
                            # Move cursor back
                            write(f"\x1b[{len(buffer) - cursor_pos}D")


class RegionEditor:
    """Raw-mode text editor constrained to a rectangular screen region.

    Coordinates are 1-indexed and inclusive:
    - (x1, y1) is the top-left
    - (x2, y2) is the bottom-right

    Controls:
    - arrows/home/end: navigation
    - PgUp/PgDn or Ctrl+U/Ctrl+D: page up/down
    - Insert: toggle insert/overwrite mode
    - enter: new line
    - backspace: delete before cursor
    - delete: delete at cursor
    - Ctrl+S: save
    - ESC: cancel
    """

    def __init__(
        self,
        x1: int,
        y1: int,
        x2: int,
        y2: int,
        max_lines: int = 500,
        show_scrollbar: bool = False,
    ) -> None:
        self.x1 = x1
        self.y1 = y1
        self.x2 = x2
        self.y2 = y2
        self.max_lines = max_lines
        self.show_scrollbar = show_scrollbar

    @property
    def width(self) -> int:
        return max(0, self.x2 - self.x1 + 1)

    @property
    def height(self) -> int:
        return max(0, self.y2 - self.y1 + 1)

    def edit(self, initial: str = "") -> Optional[str]:
        from .ansi import goto_xy, write, show_cursor, hide_cursor

        if self.width <= 0 or self.height <= 0:
            return initial

        text_width = self.width - 1 if self.show_scrollbar else self.width
        text_width = max(1, text_width)

        lines: List[str] = initial.split('\n') if initial else ['']
        lines = [ln[:text_width] for ln in lines]
        if not lines:
            lines = ['']

        cursor_line = 0
        cursor_col = 0
        view_top = 0
        insert_mode = True

        def clamp_cursor() -> None:
            nonlocal cursor_line, cursor_col
            cursor_line = max(0, min(cursor_line, len(lines) - 1))
            cursor_col = max(0, min(cursor_col, len(lines[cursor_line])))

        def ensure_visible() -> bool:
            nonlocal view_top
            old = view_top
            if cursor_line < view_top:
                view_top = cursor_line
            if cursor_line >= view_top + self.height:
                view_top = cursor_line - self.height + 1
            max_top = max(0, len(lines) - self.height)
            view_top = max(0, min(view_top, max_top))
            return view_top != old

        def redraw_scrollbar() -> None:
            if not self.show_scrollbar:
                return

            max_top = max(0, len(lines) - self.height)
            if max_top <= 0:
                thumb_top = 0
                thumb_len = self.height
            else:
                thumb_len = max(1, int(round((self.height * self.height) / max(1, len(lines)))))
                thumb_len = min(self.height, thumb_len)
                usable = max(1, self.height - thumb_len)
                thumb_top = int(round((view_top / max_top) * usable))

            sb_x = self.x2
            for row in range(self.height):
                ch = '█' if thumb_top <= row < (thumb_top + thumb_len) else '░'
                goto_xy(sb_x, self.y1 + row)
                write(ch)

        def redraw() -> None:
            for row in range(self.height):
                idx = view_top + row
                text = lines[idx] if 0 <= idx < len(lines) else ""
                text = text[:text_width]
                if len(text) < text_width:
                    text = text + (" " * (text_width - len(text)))
                goto_xy(self.x1, self.y1 + row)
                write(text)

            redraw_scrollbar()

        def place_cursor() -> None:
            if ensure_visible():
                redraw()
            screen_row = cursor_line - view_top
            screen_row = max(0, min(screen_row, self.height - 1))
            goto_xy(self.x1 + cursor_col, self.y1 + screen_row)

        redraw()
        show_cursor()
        place_cursor()

        with RawInput(extended_keys=True) as inp:
            while True:
                key = inp.get_key()

                if key == KEY_ESC:
                    hide_cursor()
                    redraw()
                    return None

                if key == '\x13':  # Ctrl+S
                    hide_cursor()
                    redraw()
                    return '\n'.join(lines)

                if key == KEY_ENTER:
                    if len(lines) < self.max_lines:
                        cur = lines[cursor_line]
                        left = cur[:cursor_col]
                        right = cur[cursor_col:]
                        lines[cursor_line] = left
                        lines.insert(cursor_line + 1, right)
                        cursor_line += 1
                        cursor_col = 0
                        clamp_cursor()
                        if ensure_visible():
                            redraw()
                        else:
                            redraw()
                        place_cursor()
                    continue

                if key in (KEY_BACKSPACE, KEY_DELETE):
                    if cursor_col > 0:
                        cur = lines[cursor_line]
                        lines[cursor_line] = cur[: cursor_col - 1] + cur[cursor_col:]
                        cursor_col -= 1
                        redraw()
                        place_cursor()
                        continue
                    if cursor_line > 0:
                        prev = lines[cursor_line - 1]
                        cur = lines[cursor_line]
                        if len(prev) + len(cur) <= text_width:
                            new_col = len(prev)
                            lines[cursor_line - 1] = prev + cur
                            lines.pop(cursor_line)
                            cursor_line -= 1
                            cursor_col = new_col
                            clamp_cursor()
                            if ensure_visible():
                                redraw()
                            else:
                                redraw()
                            place_cursor()
                    continue

                if key == KEY_DEL:
                    cur = lines[cursor_line]
                    if cursor_col < len(cur):
                        lines[cursor_line] = cur[:cursor_col] + cur[cursor_col + 1:]
                        redraw()
                        place_cursor()
                    elif cursor_line < len(lines) - 1:
                        nxt = lines[cursor_line + 1]
                        if len(cur) + len(nxt) <= text_width:
                            lines[cursor_line] = cur + nxt
                            lines.pop(cursor_line + 1)
                            clamp_cursor()
                            if ensure_visible():
                                redraw()
                            else:
                                redraw()
                            place_cursor()
                    continue

                if key == KEY_LEFT:
                    if cursor_col > 0:
                        cursor_col -= 1
                    elif cursor_line > 0:
                        cursor_line -= 1
                        cursor_col = len(lines[cursor_line])
                    clamp_cursor()
                    place_cursor()
                    continue

                if key == KEY_RIGHT:
                    if cursor_col < len(lines[cursor_line]):
                        cursor_col += 1
                    elif cursor_line < len(lines) - 1:
                        cursor_line += 1
                        cursor_col = 0
                    clamp_cursor()
                    place_cursor()
                    continue

                if key == KEY_UP:
                    if cursor_line > 0:
                        cursor_line -= 1
                    clamp_cursor()
                    place_cursor()
                    continue

                if key == KEY_DOWN:
                    if cursor_line < len(lines) - 1:
                        cursor_line += 1
                    clamp_cursor()
                    place_cursor()
                    continue

                if key == KEY_HOME:
                    cursor_col = 0
                    place_cursor()
                    continue

                if key == KEY_END:
                    cursor_col = len(lines[cursor_line])
                    place_cursor()
                    continue

                if key == KEY_PAGEUP or key == '\x15':  # PgUp or Ctrl+U
                    cursor_line = max(0, cursor_line - self.height)
                    clamp_cursor()
                    place_cursor()
                    continue

                if key == KEY_PAGEDOWN or key == '\x04':  # PgDn or Ctrl+D
                    cursor_line = min(len(lines) - 1, cursor_line + self.height)
                    clamp_cursor()
                    place_cursor()
                    continue

                if key == KEY_INSERT:
                    insert_mode = not insert_mode
                    place_cursor()
                    continue

                if key and len(key) == 1 and is_printable(key):
                    cur = lines[cursor_line]
                    if insert_mode:
                        if len(cur) < text_width:
                            lines[cursor_line] = cur[:cursor_col] + key + cur[cursor_col:]
                            cursor_col += 1
                            redraw()
                            place_cursor()
                    else:
                        if cursor_col < text_width:
                            if cursor_col < len(cur):
                                lines[cursor_line] = cur[:cursor_col] + key + cur[cursor_col + 1:]
                            else:
                                lines[cursor_line] = cur + key
                            cursor_col = min(cursor_col + 1, text_width)
                            redraw()
                            place_cursor()
                    continue


@dataclass
class InputEvent:
    event_type: str
    from_remote: bool
    key_press: int


def key_pending(inp: Optional[RawInput] = None) -> bool:
    if inp is None:
        with RawInput() as tmp:
            return tmp.key_pressed()
    return inp.key_pressed()


def clear_keybuffer(inp: Optional[RawInput] = None) -> None:
    if inp is None:
        with RawInput() as tmp:
            tmp.clear_keybuffer()
            return
    inp.clear_keybuffer()


def get_answer(options: str, inp: Optional[RawInput] = None) -> str:
    def match_option(key: str) -> Optional[str]:
        if not options:
            return None
        if key == KEY_ENTER:
            for c in options:
                if c in ("\r", "\n"):
                    return c
            return None
        if not key or len(key) != 1:
            return None
        k = key.lower()
        for c in options:
            if c.lower() == k:
                return c
        return None

    if inp is None:
        with RawInput(extended_keys=True) as tmp:
            while True:
                k = tmp.get_key()
                if not k:
                    continue
                m = match_option(k)
                if m is not None:
                    return m

    while True:
        k = inp.get_key()
        if not k:
            continue
        m = match_option(k)
        if m is not None:
            return m


def detect_capabilities_probe(
    *,
    timeout_ms: int = 250,
) -> "TerminalInfo":
    import re

    from .door import TerminalInfo
    from .utils import write_bytes

    try:
        from .runtime import get_terminal_fd

        rt_fd = get_terminal_fd(default=None)
    except Exception:
        rt_fd = None
    fd = int(rt_fd) if rt_fd is not None else sys.stdin.fileno()
    total_timeout = max(0.0, float(timeout_ms) / 1000.0)

    def read_for(deadline: float) -> None:
        while True:
            now = time.time()
            if now >= deadline:
                return
            remaining = max(0.0, deadline - now)
            if not _read_into_stream_state(fd, remaining):
                return

    def remove_matches(pattern: "re.Pattern[bytes]") -> None:
        st = _get_stream_state(fd)
        s = bytes(st.inbuf)
        matches = list(pattern.finditer(s))
        if not matches:
            return
        out = bytearray()
        last = 0
        for m in matches:
            out.extend(s[last : m.start()])
            last = m.end()
        out.extend(s[last:])
        st.inbuf = out

    caps: List[str] = ["ascii"]
    rip_ver: Optional[str] = None

    ansi_reply_re = re.compile(rb"\x1b\[[0-9]{1,3};[0-9]{1,3}R")
    rip_reply_re = re.compile(rb"RIPSCRIP([0-9]{6})")

    _enable_raw_mode_fd(fd)
    try:
        start = time.time()
        deadline = start + total_timeout

        st = _get_stream_state(fd)

        try:
            write_bytes(b"\x1b[6n")
            read_for(deadline)
            if ansi_reply_re.search(bytes(st.inbuf)):
                caps.append("ansi")
            remove_matches(ansi_reply_re)
        except Exception:
            pass

        try:
            write_bytes(b"\x1b[!")
            read_for(deadline)
            m = rip_reply_re.search(bytes(st.inbuf))
            if m:
                caps.append("rip")
                try:
                    rip_ver = m.group(1).decode("ascii", errors="replace")
                except Exception:
                    rip_ver = None
            remove_matches(rip_reply_re)
        except Exception:
            pass

        return TerminalInfo(
            capabilities=caps,
            rip_version=rip_ver,
            detected_at=time.time(),
            preferred_mode=None,
        )
    finally:
        _disable_raw_mode_fd(fd)


def detect_window_size_probe(
    *,
    timeout_ms: int = 250,
) -> Optional[Tuple[int, int]]:
    import re

    from .utils import write_bytes

    try:
        from .runtime import get_terminal_fd

        rt_fd = get_terminal_fd(default=None)
    except Exception:
        rt_fd = None
    fd = int(rt_fd) if rt_fd is not None else sys.stdin.fileno()
    total_timeout = max(0.0, float(timeout_ms) / 1000.0)

    def read_for(deadline: float) -> None:
        while True:
            now = time.time()
            if now >= deadline:
                return
            remaining = max(0.0, deadline - now)
            if not _read_into_stream_state(fd, remaining):
                return

    def remove_matches(pattern: "re.Pattern[bytes]") -> None:
        st = _get_stream_state(fd)
        s = bytes(st.inbuf)
        matches = list(pattern.finditer(s))
        if not matches:
            return
        out = bytearray()
        last = 0
        for m in matches:
            out.extend(s[last : m.start()])
            last = m.end()
        out.extend(s[last:])
        st.inbuf = out

    def new_deadline() -> float:
        return time.time() + total_timeout

    deadline = new_deadline()

    # Prefer XTerm-style report (CSI 18 t => CSI 8 ; rows ; cols t)
    xt_reply_re = re.compile(rb"\x1b\[8;([0-9]{1,4});([0-9]{1,4})t")
    dsr_reply_re = re.compile(rb"\x1b\[([0-9]{1,4});([0-9]{1,4})R")

    rows: Optional[int] = None
    cols: Optional[int] = None

    _enable_raw_mode_fd(fd)
    try:
        st = _get_stream_state(fd)

        try:
            write_bytes(b"\x1b[18t")
            read_for(deadline)
            m = xt_reply_re.search(bytes(st.inbuf))
            if m:
                rows = int(m.group(1))
                cols = int(m.group(2))
            remove_matches(xt_reply_re)
        except Exception:
            pass

        if rows is None or cols is None:
            try:
                deadline = new_deadline()
                write_bytes(b"\x1b[s\x1b[999;999H\x1b[6n\x1b[u")
                read_for(deadline)
                m = dsr_reply_re.search(bytes(st.inbuf))
                if m:
                    rows = int(m.group(1))
                    cols = int(m.group(2))
                remove_matches(dsr_reply_re)
            except Exception:
                pass

        if rows is None or cols is None:
            return None
        if rows <= 0 or cols <= 0:
            return None
        return (rows, cols)
    finally:
        _disable_raw_mode_fd(fd)


def prompt_graphics_mode(
    *,
    inp: Optional[RawInput] = None,
    default: Optional[str] = None,
) -> str:
    from .ansi import writeln
    from .runtime import get_runtime as _get_runtime
    from .runtime import get_terminal as _rt_terminal

    _get_runtime()

    terminal = _rt_terminal()

    caps = list(getattr(terminal, "capabilities", []) or [])
    if not caps:
        caps = ["ascii"]
    caps_l = {str(c).strip().lower() for c in caps if c}

    if "rip" in caps_l:
        choices = "rati"
        prompt = "Graphics mode [R]IP/[A]NSI/[T]TY/[I]gnore: "
    elif "ansi" in caps_l:
        choices = "ati"
        prompt = "Graphics mode [A]NSI/[T]TY/[I]gnore: "
    else:
        choices = "ti"
        prompt = "Graphics mode [T]TY/[I]gnore: "

    if default:
        d = str(default).strip().lower()
        if d and d[0] in choices:
            pass

    writeln(prompt)
    k = get_answer(choices, inp=inp)
    k = (k or "").strip().lower()

    if k == "r":
        return "rip"
    if k == "a":
        return "ansi"
    if k == "t":
        return "ascii"
    return "ansi" if "ansi" in caps_l else "ascii"


def hotkey_menu(
    ansi_file: str,
    hotkeys: Dict[str, Callable[[], Optional[str]]],
    *,
    inp: Optional[RawInput] = None,
    time_to_wait_ms: int = NO_TIMEOUT,
    clear: bool = True,
    case_sensitive: bool = False,
    redraw: bool = True,
    overlay_mode: int = 0,
    overlay_prompt: Optional[str] = None,
    overlay_pos: Tuple[int, int] = (1, 1),
    overlay_items: Optional[List[Tuple[str, str]]] = None,
    overlay_hotkey_format: Optional[str] = None,
    overlay_hotkey_color: Optional[str] = None,
    overlay_text_color: Optional[str] = None,
    theme: Optional["Theme"] = None,
    unknown_key: Optional[Callable[[str], None]] = None,
) -> Optional[str]:
    from .ansi import clear_screen, writeln
    from .utils import write_bytes

    from .style import resolve_style
    from .theme import Theme, get_current_theme

    from .dropfiles import DropfileInfo
    from .runtime import get_runtime as _get_runtime

    # DoorKit is a door runtime API. This menu helper assumes the door runtime
    # has been initialized (Door.start / Door.start_local).
    rt = _get_runtime()
    dropfile = rt.dropfile
    terminal = rt.terminal

    th = get_current_theme() if theme is None else theme
    if overlay_hotkey_format is None:
        overlay_hotkey_format = th.format.hotkey
    overlay_hotkey_color = resolve_style(overlay_hotkey_color, th.colors.key)
    overlay_text_color = resolve_style(overlay_text_color, th.colors.text)

    def detect_remote_emulation(df: Optional[DropfileInfo]) -> str:
        if df is None:
            if terminal is not None:
                caps = [str(c).strip().lower() for c in (terminal.capabilities or [])]
                if "rip" in caps:
                    return "rip"
                if "ansi" in caps:
                    return "ansi"
                return "ascii"
            return "ansi"

        gm = (df.graphics_mode or "").strip().lower()
        if "rip" in gm:
            return "rip"
        if "avt" in gm or "avatar" in gm:
            return "avatar"
        if "ansi" in gm or gm in ("gr", "7e"):
            return "ansi"

        # Best-effort hints from some dropfile formats.
        if str(df.extra.get("ansi", "")).strip() in ("1", "true", "yes"):
            return "ansi"

        return "ascii"

    remote_emulation = detect_remote_emulation(dropfile)

    def get_preferred_extensions_for_emulation(mode: str) -> Tuple[str, ...]:
        m = (mode or "").strip().lower()
        if m == "rip":
            return (".rip", ".avt", ".ans", ".asc", ".txt")
        if m == "avatar":
            return (".avt", ".ans", ".asc", ".txt")
        if m == "ascii":
            return (".asc", ".txt", ".ans")
        return (".ans", ".asc", ".txt")

    def resolve_menu_file(path: str, *, preferred_exts: Optional[Tuple[str, ...]] = None) -> str:
        p = "" if path is None else str(path)
        if not p:
            return p

        p = os.path.expanduser(p)
        if os.path.exists(p):
            return p

        base, ext = os.path.splitext(p)
        candidates: List[str] = []

        if ext:
            el = ext.lower()
            eu = ext.upper()
            if el != ext:
                candidates.append(base + el)
            if eu != ext and eu != el:
                candidates.append(base + eu)
        else:
            exts = preferred_exts or (".ans", ".asc", ".txt")
            for e in exts:
                candidates.append(p + e)
                candidates.append(p + e.upper())

        for c in candidates:
            if os.path.exists(c):
                return c
        return p

    preferred_exts = get_preferred_extensions_for_emulation(remote_emulation)
    remote_file = resolve_menu_file(ansi_file, preferred_exts=preferred_exts)

    _, remote_ext = os.path.splitext(remote_file)
    remote_ext_l = remote_ext.lower()

    # Stub local-display fallback:
    # - FD3 is the remote terminal stream in door32.
    # - When output is not FD3, prefer an ANSI/ASCII equivalent for AVT/RIP.
    try:
        from .runtime import get_terminal_fd

        rt_fd = get_terminal_fd(default=None)
    except Exception:
        rt_fd = None
    output_fd = int(rt_fd) if rt_fd is not None else sys.stdout.fileno()

    local_fallback_file: Optional[str] = None
    if remote_ext_l in (".rip", ".avt"):
        local_fallback_file = resolve_menu_file(
            ansi_file,
            preferred_exts=(".ans", ".asc", ".txt"),
        )
        if local_fallback_file == remote_file:
            local_fallback_file = None

    resolved_file = remote_file
    if rt_fd is None and local_fallback_file is not None:
        resolved_file = local_fallback_file

    def _load_converted_avatar_as_ansi_bytes(path: str) -> Optional[bytes]:
        if not path:
            return None
        try:
            raw = open(path, "rb").read()
        except Exception:
            return None
        try:
            s = raw.decode("cp437", errors="replace")
        except Exception:
            s = raw.decode("latin-1", errors="replace")
        try:
            from .text import _avatar_to_ansi
            ansi_s = _avatar_to_ansi(s)
        except Exception:
            return None
        return ansi_s.encode("cp437", errors="replace")

    def canon_keypress(k: str) -> str:
        s = "" if k is None else str(k)
        if not s:
            return ""
        if not case_sensitive and len(s) == 1:
            return s.lower()
        return s

    def _add_keymap_entry(keymap: Dict[str, Callable[[], Optional[str]]], spec: str, cb: Callable[[], Optional[str]]):
        if spec is None:
            return
        raw = str(spec).strip()
        if not raw:
            return

        up = raw.upper()
        aliases: List[str] = []

        # Single-character hotkeys
        if len(raw) == 1:
            aliases.append(raw)
        else:
            # Common multi-key specs
            if up in ("ESC", "ESCAPE"):
                aliases.append(KEY_ESC)
            elif up in ("ENTER", "RETURN"):
                aliases.append(KEY_ENTER)
            elif up in ("TAB",):
                aliases.append(KEY_TAB)
            elif up in ("UP",):
                aliases.append(KEY_UP)
            elif up in ("DOWN",):
                aliases.append(KEY_DOWN)
            elif up in ("LEFT",):
                aliases.append(KEY_LEFT)
            elif up in ("RIGHT",):
                aliases.append(KEY_RIGHT)
            elif up in ("HOME",):
                aliases.append(KEY_HOME)
            elif up in ("END",):
                aliases.append(KEY_END)
            elif up in ("PGUP", "PAGEUP"):
                aliases.append(KEY_PAGEUP)
            elif up in ("PGDN", "PAGEDOWN"):
                aliases.append(KEY_PAGEDOWN)
            elif up in ("INS", "INSERT"):
                aliases.append(KEY_INSERT)
            elif up in ("DELETE",):
                aliases.append(KEY_DEL)
            elif up in ("DEL",):
                aliases.append(KEY_DELETE)
            elif up in ("BACKSPACE", "BKSP", "BS"):
                aliases.append(KEY_BACKSPACE)
                aliases.append(KEY_DELETE)
            else:
                # Allow passing KEY_* tokens verbatim (e.g., KEY_UP)
                aliases.append(raw)

        for a in aliases:
            ck = canon_keypress(a)
            if ck:
                keymap[ck] = cb

    keymap: Dict[str, Callable[[], Optional[str]]] = {}
    for k, cb in (hotkeys or {}).items():
        if not k or cb is None:
            continue
        _add_keymap_entry(keymap, str(k), cb)

    def overlay_render() -> None:
        if int(overlay_mode) == 0:
            return

        if int(overlay_mode) == 1:
            from .ansi import writeln

            if overlay_prompt is not None:
                writeln(str(overlay_prompt))
                return

            keys = [str(k) for k in (hotkeys or {}).keys() if k]
            keys_str = "/".join(keys) if keys else ""
            if keys_str:
                writeln(f"Please select [{keys_str}] (ESC to return)")
            else:
                writeln("Please select an option (ESC to return)")
            return

        if int(overlay_mode) == 2:
            from .ansi import RESET, goto_xy, write

            row, col = overlay_pos

            def fmt_hotkey(k: str) -> str:
                fmt = str(overlay_hotkey_format or "")
                fmt_l = fmt.strip().lower()
                if not fmt_l or fmt_l == "none":
                    return k
                if fmt_l == "underline":
                    return f"\x1b[4m{k}\x1b[24m\x1b[22m"
                return fmt.replace("X", k)

            items = overlay_items
            if items is None:
                items = [(str(k), "") for k in (hotkeys or {}).keys() if k]

            for i, item in enumerate(items):
                if not item:
                    continue
                k = str(item[0]) if len(item) > 0 else ""
                label = str(item[1]) if len(item) > 1 else ""
                if not k:
                    continue
                goto_xy(int(col), int(row) + int(i))
                hk = fmt_hotkey(k)
                if overlay_hotkey_color:
                    hk = f"{overlay_hotkey_color}{hk}{RESET}"
                if label:
                    if overlay_text_color:
                        write(f"{hk} {overlay_text_color}{label}{RESET}")
                    else:
                        write(f"{hk} {label}")
                else:
                    write(hk)
            return

    timeout_s: Optional[float]
    if time_to_wait_ms == NO_TIMEOUT:
        timeout_s = None
    else:
        timeout_s = max(0.0, float(time_to_wait_ms) / 1000.0)

    def display_file_with_hotkeys(raw_inp: RawInput) -> Optional[str]:
        if clear:
            clear_screen()

        try:
            # If we're not writing to FD3 (local/dev), and the selected file is AVT,
            # attempt a best-effort AVATAR->ANSI conversion so the output is readable.
            if output_fd != 3 and resolved_file.lower().endswith(".avt"):
                converted = _load_converted_avatar_as_ansi_bytes(resolved_file)
                if converted is not None:
                    i = 0
                    while i < len(converted):
                        chunk = converted[i : i + 4096]
                        i += len(chunk)
                        write_bytes(chunk)

                        if raw_inp.key_pressed():
                            k = raw_inp.get_key(timeout=0)
                            if not k:
                                continue
                            ck = canon_keypress(k)
                            if ck in keymap:
                                return ck
                            if unknown_key is not None:
                                unknown_key(k)
                    return None

            with open(resolved_file, "rb") as f:
                while True:
                    chunk = f.read(4096)
                    if not chunk:
                        break
                    write_bytes(chunk)

                    # OpenDoors behavior: hotkey can interrupt file display.
                    if raw_inp.key_pressed():
                        k = raw_inp.get_key(timeout=0)
                        if not k:
                            continue
                        ck = canon_keypress(k)
                        if ck in keymap:
                            return ck
                        if unknown_key is not None:
                            unknown_key(k)
        except FileNotFoundError:
            writeln(f"File not found: {resolved_file}")
            return None
        except Exception as e:
            writeln(f"Error displaying file: {e}")
            return None

        return None

    def handle_key(k: str) -> Tuple[bool, Optional[str]]:
        """Return (should_exit, result)."""
        if not k:
            return (False, None)

        ck = canon_keypress(k)

        # ESC: allow explicit mapping, otherwise default to exit/back.
        if ck == canon_keypress(KEY_ESC) and (ck not in keymap):
            return (True, None)

        cb = keymap.get(ck)
        if cb is None:
            if unknown_key is not None:
                unknown_key(k)
            return (False, None)

        res = cb()
        if res is not None:
            return (True, res)

        # res is None => keep running
        if redraw:
            return (False, "__REDRAW__")
        return (False, None)

    def run_with(raw_inp: RawInput) -> Optional[str]:
        while True:
            hk = display_file_with_hotkeys(raw_inp)
            overlay_render()

            if hk is not None:
                should_exit, res = handle_key(hk)
                if should_exit:
                    return res
                if res == "__REDRAW__":
                    continue
            while True:
                k = raw_inp.get_key(timeout=timeout_s)
                if k is None:
                    return None

                should_exit, res = handle_key(k)
                if should_exit:
                    return res
                if res == "__REDRAW__":
                    break

    if inp is None:
        with RawInput(extended_keys=True) as tmp:
            return run_with(tmp)
    return run_with(inp)


def get_input(
    time_to_wait_ms: int = NO_TIMEOUT,
    flags: int = GETIN_NORMAL,
    inp: Optional[RawInput] = None,
) -> Optional[InputEvent]:
    if inp is None:
        with RawInput(extended_keys=True) as tmp:
            return tmp.get_input_event(time_to_wait_ms=time_to_wait_ms, flags=flags)

    return inp.get_input_event(time_to_wait_ms=time_to_wait_ms, flags=flags)


def input_str(
    max_length: int,
    min_char: int,
    max_char: int,
    *,
    initial: str = "",
    allow_cancel: bool = False,
    inp: Optional[RawInput] = None,
) -> Optional[str]:
    from .ansi import write, writeln

    min_v = ord(min_char) if isinstance(min_char, str) else int(min_char)
    max_v = ord(max_char) if isinstance(max_char, str) else int(max_char)

    buf = list(initial[: max(0, int(max_length))])

    def do_input(raw_inp: RawInput) -> Optional[str]:
        write("".join(buf))
        while True:
            k = raw_inp.get_key()
            if k == KEY_ENTER:
                writeln()
                return "".join(buf)
            if allow_cancel and k == KEY_ESC:
                writeln()
                return None
            if k in (KEY_BACKSPACE, KEY_DELETE):
                if buf:
                    buf.pop()
                    write("\x08 \x08")
                continue
            if k and len(k) == 1:
                o = ord(k)
                if min_v <= o <= max_v and len(buf) < max_length:
                    buf.append(k)
                    write(k)

    if inp is None:
        with RawInput(extended_keys=True) as tmp:
            return do_input(tmp)
    return do_input(inp)


def edit_str(
    value: str,
    fmt: str,
    row: int,
    col: int,
    normal_attr: int,
    highlight_attr: int,
    blank: str,
    flags: int = EDIT_FLAG_NORMAL,
    inp: Optional[RawInput] = None,
) -> Tuple[int, str]:
    from .ansi import goto_xy, write
    from .text import sgr_from_pc_attr

    max_len = len(fmt)
    if max_len <= 0:
        return (EDIT_RETURN_ERROR, value)

    blank_ch = blank[0] if blank else " "
    buf = list((value if (flags & EDIT_FLAG_EDIT_STRING) else "")[:max_len])
    cur = len(buf)

    normal = sgr_from_pc_attr(normal_attr)
    hi = sgr_from_pc_attr(highlight_attr)

    def paint(focused: bool) -> None:
        goto_xy(col, row)
        write(hi if focused else normal)
        s = "".join(buf)
        if len(s) < max_len:
            s = s + (blank_ch * (max_len - len(s)))
        else:
            s = s[:max_len]
        write(s)
        if focused:
            goto_xy(col + cur, row)

    def finish(code: int, out: str) -> Tuple[int, str]:
        paint(False)
        return (code, out)

    def do_edit(raw_inp: RawInput) -> Tuple[int, str]:
        nonlocal cur
        paint(True)

        while True:
            k = raw_inp.get_key()

            if k == KEY_ENTER:
                return finish(EDIT_RETURN_ACCEPT, "".join(buf))

            if (flags & EDIT_FLAG_ALLOW_CANCEL) and k == KEY_ESC:
                return finish(EDIT_RETURN_CANCEL, value)

            if (flags & EDIT_FLAG_FIELD_MODE):
                if k in (KEY_UP, KEY_SHIFTTAB):
                    return finish(EDIT_RETURN_PREVIOUS, "".join(buf))
                if k in (KEY_DOWN, "\t"):
                    return finish(EDIT_RETURN_NEXT, "".join(buf))

            if k in (KEY_BACKSPACE, KEY_DELETE):
                if cur > 0:
                    buf.pop(cur - 1)
                    cur -= 1
                    paint(True)
            elif k == KEY_LEFT:
                if cur > 0:
                    cur -= 1
                    goto_xy(col + cur, row)
            elif k == KEY_RIGHT:
                if cur < len(buf):
                    cur += 1
                    goto_xy(col + cur, row)
            elif k == KEY_HOME:
                cur = 0
                goto_xy(col + cur, row)
            elif k == KEY_END:
                cur = len(buf)
                goto_xy(col + cur, row)
            elif k and len(k) == 1 and 32 <= ord(k) <= 126:
                if len(buf) < max_len:
                    buf.insert(cur, k)
                    cur += 1
                    paint(True)

    if inp is None:
        with RawInput(extended_keys=True) as tmp:
            return do_edit(tmp)
    return do_edit(inp)
