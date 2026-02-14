#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
#
# lang_printf_to_positional.py - Convert printf specifiers to |!N codes
#
# Reads english.toml lines, replaces %s/%d/%c/%u/%lu/%ld/etc. with
# sequential |!1..|!F positional placeholders.  Skips keys that are
# format templates (date strings, archive formats, etc.).
#
# Usage: python3 scripts/lang_printf_to_positional.py [--dry-run]
#
# Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

import re
import sys
import os

# Keys whose values are format templates passed to sprintf/strftime/etc.
# and should NOT be converted to |!N positional codes.
SKIP_KEYS = {
    # Date/time format strings (passed to strftime or sprintf for dates)
    "asctime_format",
    "date_str",
    "datestr",
    "scan_str",
    "time_fmt",

    # Archive listing format (complex multi-field sprintf)
    "lzh_type",

    # External program command template (contains %%p, %%b etc.)
    "xtrn_caller",

    # Printf self-reference (meta error message)
    "printfstringtoolong",

    # Node address format (used as sprintf template for address formatting)
    "znnp",

    # Nodelist path templates (sprintf with single %s prefix)
    # These are path construction, not display strings
    "idxnode",
    "datnode",
    "sysnode",

    # ErrorLevel format (used with sprintf, not display)
    "erl_xx",

    # Bad menu option debug format
    "bad_menu_opt",

    # File offline format (binary control chars + sprintf)
    "file_offline",

    # Message attribute keys (not a format string)
    "msgattr_keys",

    # WFC banner (mixed strftime + %%s escape — needs manual handling)
    "wfc_maxbanner",

    # Column-aligned sprintf formats (field-width specifiers are meaningful)
    "trk_rep_fmt",
    "trk_owner_fmt",
    "trk_owner_lnk_fmt",
    "trk_list_type",
    "trk_rep_msgnum",

    # Complex multi-field address display format
    "addrfmt",

    # RIP-embedded format string (contains |c%02X|T%s sequences)
    "srchng",

    # Binary control char format
    "hfl_prmpt",
}

# Regex matching printf format specifiers: %s, %d, %c, %u, %lu, %ld,
# %02d, %-15.15s, %8lu, etc.  Does NOT match %% (escaped percent).
PRINTF_SPEC = re.compile(r'%(?!%)[-+0 #]*(?:\d+)?(?:\.\d+)?(?:[hlL])?[sdcuxXo]')


def convert_line(line, dry_run=False):
    """Convert printf specifiers in a TOML key=value line to |!N codes.

    Returns (converted_line, n_replacements, key_name).
    """
    # Skip comments and section headers
    stripped = line.strip()
    if not stripped or stripped.startswith('#') or stripped.startswith('['):
        return line, 0, None

    # Parse key = value
    eq = line.find('=')
    if eq < 0:
        return line, 0, None

    key_part = line[:eq].strip()
    val_part = line[eq+1:]

    # Check skip list
    if key_part in SKIP_KEYS:
        return line, 0, key_part

    # Find all printf specifiers in the value
    specs = list(PRINTF_SPEC.finditer(val_part))
    if not specs:
        return line, 0, key_part

    # Cap at 15 parameters (|!1..|!F)
    if len(specs) > 15:
        print(f"  WARNING: {key_part} has {len(specs)} specifiers, capping at 15",
              file=sys.stderr)
        specs = specs[:15]

    # Replace from right to left to preserve offsets
    new_val = val_part
    for i, m in enumerate(reversed(specs)):
        idx = len(specs) - i  # 1-based
        if idx <= 9:
            code = f"|!{idx}"
        else:
            code = f"|!{chr(ord('A') + idx - 10)}"
        new_val = new_val[:m.start()] + code + new_val[m.end():]

    new_line = line[:eq+1] + new_val
    return new_line, len(specs), key_part


def main():
    dry_run = '--dry-run' in sys.argv

    toml_path = os.path.join(os.path.dirname(__file__), '..',
                             'resources', 'lang', 'english.toml')
    toml_path = os.path.normpath(toml_path)

    if not os.path.isfile(toml_path):
        print(f"Error: {toml_path} not found", file=sys.stderr)
        sys.exit(1)

    # Read with latin-1 to handle CP437 bytes without loss
    with open(toml_path, 'r', encoding='latin-1') as f:
        lines = f.readlines()

    total_converted = 0
    total_specs = 0
    converted_keys = []
    skipped_keys = []

    new_lines = []
    for line in lines:
        new_line, n_specs, key = convert_line(line, dry_run)
        new_lines.append(new_line)

        if n_specs > 0:
            total_converted += 1
            total_specs += n_specs
            converted_keys.append((key, n_specs))
        elif key and key in SKIP_KEYS and PRINTF_SPEC.search(line):
            skipped_keys.append(key)

    print(f"Converted {total_converted} strings ({total_specs} specifiers total)")
    print(f"Skipped {len(skipped_keys)} format template keys")

    if converted_keys:
        print("\nConverted:")
        for key, n in converted_keys:
            print(f"  {key}: {n} specifier(s)")

    if skipped_keys:
        print("\nSkipped (format templates):")
        for key in skipped_keys:
            print(f"  {key}")

    if not dry_run:
        with open(toml_path, 'w', encoding='latin-1') as f:
            f.writelines(new_lines)
        print(f"\nWrote {toml_path}")
    else:
        print("\n(dry run — no files modified)")

        # Show diffs
        for orig, new in zip(lines, new_lines):
            if orig != new:
                print(f"  - {orig.rstrip()}")
                print(f"  + {new.rstrip()}")


if __name__ == '__main__':
    main()
