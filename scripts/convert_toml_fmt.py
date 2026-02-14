#!/usr/bin/env python3
"""
SPDX-License-Identifier: GPL-2.0-or-later
Convert printf format specifiers in english.toml to MCI positional parameters.

Replaces %s, %d, %ld, %lu, %c, %-N.Ns, etc. with |!1..|!F and MCI format ops.

Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
"""

import re
import sys

# Match printf format specifiers: %[flags][width][.precision][length]type
# Examples: %s, %d, %ld, %lu, %-20.20s, %02d, %7lu, %c, %-5u, %08lx
FMT_RE = re.compile(
    r'%'
    r'(?!%)'               # not %%
    r'(-?)'                # group 1: left-align flag
    r'(0?)'                # group 2: zero-pad flag
    r'(\d*)'               # group 3: width
    r'(?:\.(\d+))?'        # group 4: precision (optional)
    r'(l?)'                # group 5: length modifier
    r'([sdcuxXo])'         # group 6: conversion type
)


def convert_specifier(m, slot):
    """Convert a single printf format specifier to MCI positional param.

    Returns the MCI replacement string and the next slot number.
    """
    left_align = m.group(1) == '-'
    zero_pad = m.group(2) == '0'
    width_str = m.group(3)
    prec_str = m.group(4)
    # length_mod = m.group(5)  # not needed for MCI
    # conv_type = m.group(6)   # not needed for MCI

    width = int(width_str) if width_str else 0
    prec = int(prec_str) if prec_str else 0

    param = f"|!{slot}" if slot <= 9 else f"|!{chr(ord('A') + slot - 10)}"
    parts = []

    # Truncation (precision for strings: %.Ns or %-N.Ns)
    if prec > 0:
        parts.append(f"$T{prec:02d}")

    # Padding
    if width > 0:
        if zero_pad and not left_align:
            # %02d → $l020  (left-pad with '0')
            parts.append(f"$l{width:02d}0")
        elif left_align:
            # %-20s → $R20  (right-pad / left-align)
            parts.append(f"$R{width:02d}")
        else:
            # %20s → $L20  (left-pad / right-align)
            parts.append(f"$L{width:02d}")

    parts.append(param)
    return ''.join(parts)


# Sentinel used to protect literal %% from the format regex
_PCT_SENTINEL = '\x00PCT\x00'


def convert_string(s):
    """Convert all printf format specifiers in a string to MCI positional params.

    Returns (converted_string, number_of_params).
    """
    # Protect literal %% from being partially matched
    protected = s.replace('%%', _PCT_SENTINEL)

    slot = 1
    result = []
    last_end = 0

    for m in FMT_RE.finditer(protected):
        if slot > 15:
            # |!F is the max (15 params)
            break
        result.append(protected[last_end:m.start()])
        result.append(convert_specifier(m, slot))
        slot += 1
        last_end = m.end()

    result.append(protected[last_end:])
    converted = ''.join(result)

    # Restore literal %%
    converted = converted.replace(_PCT_SENTINEL, '%%')
    return converted, slot - 1


def process_toml(inpath, outpath):
    """Process a TOML file, converting format specifiers in all string values."""
    with open(inpath, 'rb') as f:
        data = f.read()

    lines = data.decode('latin-1').split('\n')
    out_lines = []
    converted = 0
    skipped = 0

    for line in lines:
        # Find all quoted strings in the line and convert each independently
        new_line = []
        pos = 0
        while pos < len(line):
            # Find next quoted string
            qstart = line.find('"', pos)
            if qstart == -1:
                new_line.append(line[pos:])
                break

            # Find matching close quote (handle escaped quotes)
            new_line.append(line[pos:qstart + 1])  # up to and including opening "
            qend = qstart + 1
            while qend < len(line):
                if line[qend] == '\\' and qend + 1 < len(line):
                    qend += 2  # skip escaped char
                    continue
                if line[qend] == '"':
                    break
                qend += 1

            if qend >= len(line):
                # No closing quote found, append rest
                new_line.append(line[qstart + 1:])
                break

            # Extract string content (between quotes)
            content = line[qstart + 1:qend]

            # Check for format specifiers
            if FMT_RE.search(content):
                new_content, n_params = convert_string(content)
                if new_content != content:
                    converted += 1
                new_line.append(new_content)
            else:
                new_line.append(content)

            new_line.append('"')  # closing quote
            pos = qend + 1

        out_lines.append(''.join(new_line))

    result = '\n'.join(out_lines)
    with open(outpath, 'wb') as f:
        f.write(result.encode('latin-1'))

    print(f"Converted {converted} strings with format specifiers")
    return converted


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <input.toml> <output.toml>")
        sys.exit(1)

    inpath = sys.argv[1]
    outpath = sys.argv[2]
    process_toml(inpath, outpath)
