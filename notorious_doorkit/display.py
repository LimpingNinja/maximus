"""
CP437 file display and screen block management.
"""

import os
import struct
import sys
from typing import List, Optional

from .ansi import goto_xy, write, writeln, clear_screen
from .utils import encode_cp437, write_bytes


def _detect_remote_emulation(
    *,
    dropfile: Optional["DropfileInfo"] = None,
    terminal: Optional["TerminalInfo"] = None,
) -> str:
    if dropfile is not None:
        gm = (dropfile.graphics_mode or "").strip().lower()
        if "rip" in gm:
            return "rip"
        if "ansi" in gm or gm in ("gr", "7e"):
            return "ansi"
        if str(dropfile.extra.get("ansi", "")).strip() in ("1", "true", "yes"):
            return "ansi"
        return "ascii"

    if terminal is not None:
        caps = [str(c).strip().lower() for c in (terminal.capabilities or [])]
        if "rip" in caps:
            return "rip"
        if "ansi" in caps:
            return "ansi"
        return "ascii"

    return "ansi"


def _preferred_exts_for_mode(mode: str) -> List[str]:
    m = (mode or "").strip().lower()
    if m == "rip":
        return [".rip", ".ans", ".asc", ".txt"]
    if m == "ascii":
        return [".asc", ".txt", ".ans"]
    return [".ans", ".asc", ".txt"]


def _resolve_best_match_file(path: str, *, preferred_exts: Optional[List[str]] = None) -> str:
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
        exts = preferred_exts or [".ans", ".asc", ".txt"]
        for e in exts:
            candidates.append(p + e)
            candidates.append(p + e.upper())

    for c in candidates:
        if os.path.exists(c):
            return c
    return p


def display_file_best_match(
    path: str,
    *,
    pause: bool = False,
    clear: bool = False,
) -> None:
    from .runtime import get_runtime as _get_runtime

    rt = _get_runtime()
    mode = _detect_remote_emulation(dropfile=rt.dropfile, terminal=rt.terminal)
    preferred_exts = _preferred_exts_for_mode(mode)
    remote_file = _resolve_best_match_file(path, preferred_exts=preferred_exts)

    _, remote_ext = os.path.splitext(remote_file)
    remote_ext_l = remote_ext.lower()

    try:
        from .runtime import get_terminal_fd

        rt_fd = get_terminal_fd(default=None)
    except Exception:
        rt_fd = None
    output_fd = int(rt_fd) if rt_fd is not None else sys.stdout.fileno()
    if rt_fd is None and remote_ext_l == ".rip":
        local_fallback = _resolve_best_match_file(path, preferred_exts=[".ans", ".asc", ".txt"])
        if local_fallback and local_fallback != remote_file and os.path.exists(local_fallback):
            remote_file = local_fallback

    display_file(remote_file, pause=pause, clear=clear)


def load_file_cp437(path: str) -> bytes:
    """
    Load file as CP437 bytes.
    
    Args:
        path: File path
        
    Returns:
        File contents as bytes
    """
    with open(path, "rb") as f:
        return f.read()


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


def visible_length(text: str) -> int:
    """
    Get visible length (excluding ANSI codes).
    
    Args:
        text: Text to measure
        
    Returns:
        Visible character count
    """
    return len(strip_ansi(text))


def display_file(path: str, encoding: str = "cp437", pause: bool = False, clear: bool = False) -> None:
    """
    Display ANSI/ASCII file.
    
    Args:
        path: File path
        encoding: File encoding (default: cp437)
        pause: Show MORE prompt after display
        clear: Clear screen before display
    """
    if clear:
        clear_screen()
        
    try:
        with open(path, "rb") as f:
            content = f.read()
            write_bytes(content)
    except FileNotFoundError:
        writeln(f"File not found: {path}")
        return
    except Exception as e:
        writeln(f"Error displaying file: {e}")
        return
        
    if pause:
        more_prompt()


def display_block(path: str, x: int, y: int, width: int, height: int, 
                  offset_x: int = 0, offset_y: int = 0) -> None:
    """
    Display rectangular block from file at screen position.
    
    Args:
        path: File path
        x: Screen X position (1-indexed)
        y: Screen Y position (1-indexed)
        width: Block width
        height: Block height
        offset_x: Horizontal offset in source file
        offset_y: Vertical offset in source file
    """
    try:
        with open(path, "rb") as f:
            lines = f.read().decode("cp437", errors="replace").split("\n")
            
        for row in range(height):
            source_row = offset_y + row
            if source_row >= len(lines):
                break
                
            line = lines[source_row]
            # Strip ANSI for accurate positioning
            visible_chars = []
            ansi_codes = []
            i = 0
            while i < len(line):
                if line[i] == '\x1b' and i + 1 < len(line) and line[i + 1] == '[':
                    # ANSI sequence
                    j = i + 2
                    while j < len(line) and line[j] not in 'ABCDEFGHJKSTfmnsulh':
                        j += 1
                    if j < len(line):
                        j += 1
                    ansi_codes.append((len(visible_chars), line[i:j]))
                    i = j
                else:
                    visible_chars.append(line[i])
                    i += 1
                    
            # Extract the block
            block_chars = visible_chars[offset_x:offset_x + width]
            
            # Reconstruct with ANSI codes
            result = []
            char_pos = 0
            for ansi_pos, ansi_code in ansi_codes:
                if offset_x <= ansi_pos < offset_x + width:
                    result.append(ansi_code)
            result.extend(block_chars)
            
            goto_xy(x, y + row)
            write(''.join(result))
            
    except FileNotFoundError:
        goto_xy(x, y)
        write(f"File not found: {path}")
    except Exception as e:
        goto_xy(x, y)
        write(f"Error: {e}")


def save_screen_block(x: int, y: int, width: int, height: int) -> List[str]:
    """
    Save screen region (placeholder - requires terminal query support).
    
    Args:
        x: X position (1-indexed)
        y: Y position (1-indexed)
        width: Block width
        height: Block height
        
    Returns:
        List of saved lines
        
    Note:
        This is a placeholder. Actual implementation would require
        terminal query sequences which may not be supported.
    """
    # Placeholder - would need terminal query support
    return [" " * width for _ in range(height)]


def restore_screen_block(x: int, y: int, block: List[str]) -> None:
    """
    Restore saved screen region.
    
    Args:
        x: X position (1-indexed)
        y: Y position (1-indexed)
        block: Saved block lines
    """
    for row, line in enumerate(block):
        goto_xy(x, y + row)
        write(line)


def more_prompt() -> bool:
    """
    Display "MORE (Y/n)?" prompt.
    
    Returns:
        True to continue, False to stop
    """
    from .input import RawInput, KEY_ENTER
    
    write("\r\n[MORE (Y/n)?] ")
    
    with RawInput() as inp:
        key = inp.get_key()
        if key and key.lower() == 'n':
            writeln()
            return False
            
    writeln()
    return True


def _try_parse_sauce(data: bytes) -> Optional[dict]:
    if len(data) < 128:
        return None

    sauce = data[-128:]
    if sauce[0:5] != b"SAUCE":
        return None

    return {
        "version": sauce[5:7].decode("cp437", errors="replace"),
        "title": sauce[7:42].decode("cp437", errors="replace").rstrip(" "),
        "author": sauce[42:62].decode("cp437", errors="replace").rstrip(" "),
        "group": sauce[62:82].decode("cp437", errors="replace").rstrip(" "),
        "date": sauce[82:90].decode("cp437", errors="replace"),
        "filesize": struct.unpack_from("<I", sauce, 90)[0],
        "data_type": sauce[94],
        "file_type": sauce[95],
        "width": struct.unpack_from("<H", sauce, 96)[0],
        "height": struct.unpack_from("<H", sauce, 98)[0],
        "comments": sauce[104],
        "flags": sauce[105],
        "font": sauce[106:128].decode("cp437", errors="replace").rstrip(" \x00"),
    }


def _split_ansi_stream_fixed_width(data: bytes, width: int) -> List[bytes]:
    lines: List[bytes] = []
    buf = bytearray()
    col = 0
    i = 0

    def flush() -> None:
        nonlocal buf, col
        lines.append(bytes(buf))
        buf = bytearray()
        col = 0

    while i < len(data):
        b = data[i]

        if b == 0x1B:
            if i + 1 >= len(data):
                buf.append(b)
                i += 1
                continue

            n = data[i + 1]
            if n == 0x5B:
                j = i + 2
                while j < len(data):
                    fb = data[j]
                    j += 1
                    if 0x40 <= fb <= 0x7E:
                        break
                buf.extend(data[i:j])
                i = j
                continue

            if n == 0x5D:
                j = i + 2
                while j < len(data):
                    if data[j] == 0x07:
                        j += 1
                        break
                    if data[j] == 0x1B and j + 1 < len(data) and data[j + 1] == 0x5C:
                        j += 2
                        break
                    j += 1
                buf.extend(data[i:j])
                i = j
                continue

            buf.extend(data[i : i + 2])
            i += 2
            continue

        buf.append(b)
        col += 1
        i += 1

        if col >= width:
            flush()

    if buf:
        lines.append(bytes(buf))
    return lines


def display_file_paged(
    path: str,
    *,
    pause_prompt: Optional[str] = None,
    clear: bool = False,
    theme: Optional["Theme"] = None,
) -> None:
    """
    Display ANSI/ASCII file with non-destructive paging.
    
    Shows the file page-by-page, pausing at (rows-1) lines with a configurable
    prompt. After the user presses a key, clears the prompt line non-destructively
    and continues displaying.
    
    Args:
        path: File path
        pause_prompt: Message to show at page breaks (default: "Press any key to continue...")
        clear: Clear screen before display

    Screen sizing:
        Uses the initialized door runtime (`get_runtime()`) and prefers `rt.screen_rows`.
    """
    from .input import RawInput
    from .runtime import get_runtime as _get_runtime
    from .style import resolve_style
    from .theme import Theme, get_current_theme

    rt = _get_runtime()
    
    # Determine screen height
    screen_rows = int(rt.screen_rows)
    
    # Determine terminal width
    terminal_width = 80  # Default
    if rt.terminal is not None and rt.terminal.cols:
        terminal_width = int(rt.terminal.cols)
    
    # Reserve one row for the pause prompt
    lines_per_page = max(1, screen_rows - 1)

    th = get_current_theme() if theme is None else theme
    pause_prompt_s = resolve_style(pause_prompt, th.format.press_any_key)
    
    if clear:
        clear_screen()
    
    try:
        with open(path, "rb") as f:
            content = f.read()

        sauce = _try_parse_sauce(content)
        wrap_width = int(sauce.get("width") if sauce else 0) or 80

        comment_len = 0
        if sauce and int(sauce.get("comments") or 0) > 0:
            n = int(sauce["comments"])
            candidate_len = 5 + (n * 64)
            start = len(content) - 128 - candidate_len
            if start >= 0 and content[start : start + 5] == b"COMNT":
                comment_len = candidate_len

        sauce_start = len(content) - 128 if sauce else None
        art_end = len(content) - 128 - comment_len if sauce else len(content)

        ctrl_z = content.rfind(b"\x1a")
        if ctrl_z != -1 and (sauce_start is None or ctrl_z < sauce_start):
            art_end = min(art_end, ctrl_z)

        art = content[:art_end]
        art = art.replace(b"\r", b"").replace(b"\n", b"")

        lines = _split_ansi_stream_fixed_width(art, wrap_width)

        line_count = 0
        total_lines = len(lines)
        
        # Only add explicit CRLF if terminal is wider than file width
        add_crlf = terminal_width > wrap_width
        
        with RawInput() as inp:
            for i, line in enumerate(lines):
                write_bytes(line)

                if i < total_lines - 1:
                    if add_crlf:
                        write_bytes(b"\r\n")
                    line_count += 1
                
                # Check if we need to pause
                if line_count >= lines_per_page and i < total_lines - 1:
                    # Show pause prompt
                    write_bytes(encode_cp437(f"\r{pause_prompt_s}"))
                    
                    # Wait for any key
                    inp.get_key()
                    
                    # Clear the prompt line non-destructively
                    # Move to beginning of line, write spaces to cover prompt, return to beginning
                    prompt_len = visible_length(pause_prompt_s)
                    write_bytes(b"\r" + (b" " * prompt_len) + b"\r")
                    
                    # Reset line counter for next page
                    line_count = 0
        
    except FileNotFoundError:
        writeln(f"File not found: {path}")
        return
    except Exception as e:
        writeln(f"Error displaying file: {e}")
        return
