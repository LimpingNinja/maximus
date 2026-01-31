"""
Utility functions for CP437 encoding and string manipulation.
"""

import os
import sys
from typing import Optional


def write_all(fd: int, data: bytes) -> None:
    if not data:
        return
    mv = memoryview(data)
    total = 0
    while total < len(data):
        n = os.write(fd, mv[total:])
        if n <= 0:
            raise OSError("os.write returned 0")
        total += n


def encode_cp437(text: str) -> bytes:
    """
    Encode a string to CP437 bytes.
    
    Args:
        text: String to encode
        
    Returns:
        CP437-encoded bytes
    """
    return text.encode('cp437', errors='replace')


def decode_cp437(data: bytes) -> str:
    """
    Decode CP437 bytes to a string.
    
    Args:
        data: CP437-encoded bytes
        
    Returns:
        Decoded string
    """
    return data.decode('cp437', errors='replace')


def strip_color(text: str) -> str:
    """
    Remove ANSI color codes from text.
    
    Args:
        text: Text containing ANSI codes
        
    Returns:
        Text with ANSI codes removed
    """
    import re
    ansi_escape = re.compile(r'\x1b\[[0-9;]*m')
    return ansi_escape.sub('', text)


def visible_length(text: str) -> int:
    """
    Get the visible length of text (excluding ANSI codes).
    
    Args:
        text: Text to measure
        
    Returns:
        Visible character count
    """
    return len(strip_color(text))


def pad_string(text: str, width: int, justify: str = "left", fillchar: str = " ") -> str:
    """
    Pad a string to a specific width, preserving ANSI codes.
    
    Args:
        text: Text to pad
        width: Target width
        justify: Justification ("left", "center", "right")
        fillchar: Character to use for padding
        
    Returns:
        Padded string
    """
    visible_len = visible_length(text)
    if visible_len >= width:
        return text
    
    padding_needed = width - visible_len
    
    if justify == "left":
        return text + (fillchar * padding_needed)
    elif justify == "right":
        return (fillchar * padding_needed) + text
    elif justify == "center":
        left_pad = padding_needed // 2
        right_pad = padding_needed - left_pad
        return (fillchar * left_pad) + text + (fillchar * right_pad)
    else:
        return text


def truncate_string(text: str, width: int, ellipsis: str = "...") -> str:
    """
    Truncate a string to a specific width, preserving ANSI codes.
    
    Args:
        text: Text to truncate
        width: Maximum width
        ellipsis: String to append if truncated
        
    Returns:
        Truncated string
    """
    visible_len = visible_length(text)
    if visible_len <= width:
        return text
    
    # Extract ANSI codes and visible characters
    import re
    ansi_pattern = re.compile(r'(\x1b\[[0-9;]*m)')
    parts = ansi_pattern.split(text)
    
    result = []
    char_count = 0
    ellipsis_len = len(ellipsis)
    target_len = width - ellipsis_len
    
    for part in parts:
        if ansi_pattern.match(part):
            result.append(part)
        else:
            for ch in part:
                if char_count >= target_len:
                    result.append(ellipsis)
                    return ''.join(result)
                result.append(ch)
                char_count += 1
    
    return ''.join(result)


def write_bytes(data: bytes) -> None:
    """
    Write raw bytes to stdout.
    
    Args:
        data: Bytes to write
    """
    try:
        from .runtime import get_terminal_fd

        fd = get_terminal_fd(default=None)
        if fd is not None:
            write_all(int(fd), data)
            return
    except Exception:
        pass
    sys.stdout.buffer.write(data)
    sys.stdout.buffer.flush()


def write_text(text: str) -> None:
    """
    Write text to stdout using CP437 encoding.
    
    Args:
        text: Text to write
    """
    write_bytes(encode_cp437(text))
