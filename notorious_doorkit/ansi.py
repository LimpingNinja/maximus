"""
ANSI escape codes and terminal control functions.
"""

import sys
from typing import Optional, Tuple
from .utils import encode_cp437, write_bytes

# Soft cursor tracking state (row, col) - 1-indexed
_cursor_row = 1
_cursor_col = 1
_cursor_tracking_enabled = True

# Color codes
RESET = "\x1b[0m"
BLACK = "\x1b[30m"
RED = "\x1b[31m"
GREEN = "\x1b[32m"
YELLOW = "\x1b[33m"
BLUE = "\x1b[34m"
MAGENTA = "\x1b[35m"
CYAN = "\x1b[36m"
WHITE = "\x1b[37m"

BRIGHT_BLACK = "\x1b[90m"
BRIGHT_RED = "\x1b[91m"
BRIGHT_GREEN = "\x1b[92m"
BRIGHT_YELLOW = "\x1b[93m"
BRIGHT_BLUE = "\x1b[94m"
BRIGHT_MAGENTA = "\x1b[95m"
BRIGHT_CYAN = "\x1b[96m"
BRIGHT_WHITE = "\x1b[97m"

BG_BLACK = "\x1b[40m"
BG_RED = "\x1b[41m"
BG_GREEN = "\x1b[42m"
BG_YELLOW = "\x1b[43m"
BG_BLUE = "\x1b[44m"
BG_MAGENTA = "\x1b[45m"
BG_CYAN = "\x1b[46m"
BG_WHITE = "\x1b[47m"

BG_BRIGHT_BLACK = "\x1b[100m"
BG_BRIGHT_RED = "\x1b[101m"
BG_BRIGHT_GREEN = "\x1b[102m"
BG_BRIGHT_YELLOW = "\x1b[103m"
BG_BRIGHT_BLUE = "\x1b[104m"
BG_BRIGHT_MAGENTA = "\x1b[105m"
BG_BRIGHT_CYAN = "\x1b[106m"
BG_BRIGHT_WHITE = "\x1b[107m"

# Text styles
BOLD = "\x1b[1m"
DIM = "\x1b[2m"
ITALIC = "\x1b[3m"
UNDERLINE = "\x1b[4m"
BLINK = "\x1b[5m"
REVERSE = "\x1b[7m"
HIDDEN = "\x1b[8m"


def goto_xy(x: int, y: int) -> None:
    """
    Move cursor to position (1-indexed).
    
    Args:
        x: Column (1-indexed)
        y: Row (1-indexed)
    """
    global _cursor_row, _cursor_col
    write_bytes(encode_cp437(f"\x1b[{y};{x}H"))
    if _cursor_tracking_enabled:
        _cursor_row = int(y)
        _cursor_col = int(x)


def set_cursor(row: int, col: int) -> None:
    """
    Move cursor to position (OpenDoors-compatible signature).
    
    Args:
        row: Row (1-indexed)
        col: Column (1-indexed)
    """
    goto_xy(int(col), int(row))


def get_cursor() -> Tuple[int, int]:
    """
    Get estimated cursor position (soft tracking).
    
    Returns a best-effort estimate of the current cursor position based on
    output operations. This is NOT a query to the terminal; it's maintained
    by tracking goto_xy/write/writeln calls.
    
    Returns:
        Tuple of (row, col) - both 1-indexed
    """
    return (_cursor_row, _cursor_col)


def move_up(n: int = 1) -> None:
    """Move cursor up n lines."""
    write_bytes(encode_cp437(f"\x1b[{n}A"))


def move_down(n: int = 1) -> None:
    """Move cursor down n lines."""
    write_bytes(encode_cp437(f"\x1b[{n}B"))


def move_right(n: int = 1) -> None:
    """Move cursor right n columns."""
    write_bytes(encode_cp437(f"\x1b[{n}C"))


def move_left(n: int = 1) -> None:
    """Move cursor left n columns."""
    write_bytes(encode_cp437(f"\x1b[{n}D"))


def save_cursor() -> None:
    """Save current cursor position."""
    write_bytes(b"\x1b[s")


def restore_cursor() -> None:
    """Restore saved cursor position."""
    write_bytes(b"\x1b[u")


def clear_screen() -> None:
    """Clear entire screen and move cursor to home."""
    global _cursor_row, _cursor_col
    write_bytes(b"\x1b[2J\x1b[H")
    if _cursor_tracking_enabled:
        _cursor_row = 1
        _cursor_col = 1


def clear_line() -> None:
    """Clear current line."""
    global _cursor_col
    write_bytes(b"\x1b[2K\r")
    if _cursor_tracking_enabled:
        _cursor_col = 1


def clear_to_eol() -> None:
    """Clear from cursor to end of line."""
    write_bytes(b"\x1b[K")


def clear_to_bol() -> None:
    """Clear from cursor to beginning of line."""
    write_bytes(b"\x1b[1K")


def hide_cursor() -> None:
    """Hide cursor."""
    write_bytes(b"\x1b[?25l")


def show_cursor() -> None:
    """Show cursor."""
    write_bytes(b"\x1b[?25h")


def write(text: str) -> None:
    """
    Write text with CP437 encoding.
    
    Args:
        text: Text to write
    """
    global _cursor_col
    write_bytes(encode_cp437(text))
    if _cursor_tracking_enabled and text:
        # Simple tracking: advance column by printable chars (ignoring ANSI codes)
        visible_text = strip_ansi(text)
        _cursor_col += len(visible_text)


def writeln(text: str = "") -> None:
    """
    Write text with CRLF line ending.
    
    Args:
        text: Text to write (empty for blank line)
    """
    global _cursor_row, _cursor_col
    write_bytes(encode_cp437(text + "\r\n"))
    if _cursor_tracking_enabled:
        _cursor_row += 1
        _cursor_col = 1


def write_at(x: int, y: int, text: str) -> None:
    """
    Write text at specific position.
    
    Args:
        x: Column (1-indexed)
        y: Row (1-indexed)
        text: Text to write
    """
    goto_xy(x, y)
    write(text)


def strip_ansi(text: str) -> str:
    """
    Remove ANSI codes from text.
    
    Args:
        text: Text containing ANSI codes
        
    Returns:
        Text with ANSI codes removed
    """
    import re
    ansi_escape = re.compile(r'\x1b\[[0-9;]*[a-zA-Z]')
    return ansi_escape.sub('', text)


def play_ansi_music(sequence: str, *, reset: bool = False) -> None:
    """Emit an ANSI music sequence to the caller.

    This sends the classic "ANSI music" prefix (CSI / ESC[) followed by the
    provided sequence bytes.

    Args:
        sequence: The raw ANSI music sequence payload (everything after ESC[).
        reset: If True, also send a reset sequence after the payload.
               (Equivalent to sending "00m" as a separate music sequence.)
    """

    s = "" if sequence is None else str(sequence)
    write_bytes(b"\x1b[" + s.encode("ascii", errors="replace"))

    if reset:
        write_bytes(b"\x1b[00m")


def play_music(sequence: str, *, reset: bool = False) -> None:
    """Alias for `play_ansi_music()`."""

    play_ansi_music(sequence, reset=reset)
