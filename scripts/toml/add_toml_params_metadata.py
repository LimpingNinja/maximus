#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Description: Generate @merge params metadata for delta_english.toml.
#              Scans C call sites to derive parameter names and types.
# Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

"""
Generate params metadata entries for the language delta overlay file.

Scans english.toml for strings with |!N positional parameters, extracts type
suffixes (d/l/u/c), greps C sources for call sites to derive parameter names,
and appends # @merge entries to delta_english.toml.

The delta overlay's @merge directive triggers shallow field merge: the params
array is appended to the base TOML line without replacing existing fields
(text, flags, rip, etc.).

Output format:
  # @merge
  key = { params = [{name = "version", type = "string"}, {name = "task", type = "int"}] }

Call site patterns scanned:
  - LangPrintf(format_key, ...)     — format = arg[0], params = arg[1:]
  - logit(format_key, ...)          — format = arg[0], params = arg[1:]
  - LangSprintf(buf, sz, format_key, ...) — format = arg[2], params = arg[3:]
"""

import os
import re
import sys
import glob

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOML_PATH = os.path.join(PROJECT_ROOT, "resources", "lang", "english.toml")
DELTA_PATH = os.path.join(PROJECT_ROOT, "resources", "lang", "delta_english.toml")
SRC_DIR = os.path.join(PROJECT_ROOT, "src", "max")

# Type suffix → human-readable type name
SUFFIX_TYPES = {
    'd': 'int',
    'l': 'long',
    'u': 'uint',
    'c': 'char',
}

# --- Step 1: Extract |!N info from TOML (with type suffixes) ---

def find_toml_params():
    """Return dict: key_name → list of (slot_index, type_str) tuples.

    Parses |!N[suffix] where N is 1-9/A-F and suffix is d/l/u/c or absent (string).
    """
    result = {}
    with open(TOML_PATH, "r", encoding="latin-1") as f:
        for line in f:
            line = line.rstrip("\n")
            m = re.match(r'^([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*', line)
            if not m:
                continue
            key = m.group(1)
            # Find all |!N[suffix] references
            params = re.findall(r'\|!([1-9A-F])([dlucDLUC])?', line)
            if not params:
                continue
            slots = {}
            for slot_ch, suffix in params:
                idx = int(slot_ch) if slot_ch.isdigit() else ord(slot_ch) - ord('A') + 10
                type_str = SUFFIX_TYPES.get(suffix.lower(), 'string') if suffix else 'string'
                slots[idx] = type_str
            # Build ordered list from slot 1..max
            max_slot = max(slots.keys())
            slot_list = [(i, slots.get(i, 'string')) for i in range(1, max_slot + 1)]
            result[key] = slot_list
    return result


# --- Step 2: Extract call sites from C sources ---

# Call site patterns: (function_prefix, format_arg_index)
CALL_PATTERNS = [
    ("LangPrintf(", 0),      # LangPrintf(format, ...)
    ("logit(", 0),           # logit(format, ...)
    ("LangSprintf(", 2),     # LangSprintf(buf, bufsz, format, ...)
]


def extract_call_sites():
    """Return dict: lang_string_name → list of argument expression lists.

    Each call gives one list of arg expressions (the varargs AFTER the format).
    Scans LangPrintf, logit, and LangSprintf call sites.
    """
    calls = {}
    c_files = glob.glob(os.path.join(SRC_DIR, "**", "*.c"), recursive=True)

    for fpath in c_files:
        try:
            with open(fpath, "r", encoding="latin-1") as f:
                content = f.read()
        except Exception:
            continue

        for func_prefix, fmt_idx in CALL_PATTERNS:
            idx = 0
            while True:
                pos = content.find(func_prefix, idx)
                if pos == -1:
                    break
                # Skip if inside a comment or preprocessor directive
                line_start = content.rfind("\n", 0, pos) + 1
                prefix = content[line_start:pos].strip()
                if prefix.startswith("//") or prefix.startswith("*") or prefix.startswith("#"):
                    idx = pos + 1
                    continue

                # Extract the full argument list by matching parens
                paren_start = content.index("(", pos)
                depth = 1
                i = paren_start + 1
                while i < len(content) and depth > 0:
                    if content[i] == "(":
                        depth += 1
                    elif content[i] == ")":
                        depth -= 1
                    i += 1
                if depth != 0:
                    idx = pos + 1
                    continue

                args_str = content[paren_start + 1:i - 1]
                args_str = re.sub(r'\s+', ' ', args_str).strip()
                args = split_args(args_str)

                if len(args) <= fmt_idx:
                    idx = i
                    continue

                lang_name = args[fmt_idx].strip()
                param_args = [a.strip() for a in args[fmt_idx + 1:]]

                # Only track calls where format arg is a simple identifier
                if re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*$', lang_name):
                    if lang_name not in calls:
                        calls[lang_name] = []
                    calls[lang_name].append(param_args)

                idx = i

    return calls


def split_args(s):
    """Split a comma-separated argument string respecting nested parens and strings."""
    args = []
    depth = 0
    current = []
    in_string = False
    escape = False
    for ch in s:
        if escape:
            current.append(ch)
            escape = False
            continue
        if ch == '\\':
            current.append(ch)
            escape = True
            continue
        if ch == '"' and not in_string:
            in_string = True
            current.append(ch)
            continue
        if ch == '"' and in_string:
            in_string = False
            current.append(ch)
            continue
        if in_string:
            current.append(ch)
            continue
        if ch in '(':
            depth += 1
        elif ch in ')':
            depth -= 1
        if ch == ',' and depth == 0:
            args.append(''.join(current))
            current = []
        else:
            current.append(ch)
    if current:
        args.append(''.join(current))
    return args


# --- Step 3: Derive param names from C expressions ---

# Heuristic mapping: pattern → human-readable name
EXPR_PATTERNS = [
    # Color strings
    (r'^(LMAGENTA|LRED|LGREEN|LCYAN|YELLOW|WHITE|CYAN|GRAY|GREEN|RED)$', 'color'),
    (r'^msg_\w*_col$', 'color'),
    (r'^file_\w*_col$', 'color'),
    (r'^msg_quote_col$', 'quote_color'),
    # User fields
    (r'^usr\.name$', 'user_name'),
    (r'^usr\.msg$', 'msg_area_num'),
    (r'^usr\.city$', 'user_city'),
    (r'^usr\.phone$', 'user_phone'),
    (r'^usr\.alias$', 'user_alias'),
    (r'^usrname$', 'user_name'),
    # Message fields
    (r'^msg\.to$', 'recipient'),
    (r'^msg\.from$', 'sender'),
    (r'^msg\.subj$', 'subject'),
    (r'^mmsg->to$', 'recipient'),
    (r'^mmsg->from$', 'sender'),
    (r'^mmsg->subj$', 'subject'),
    # Strip_Ansi patterns
    (r'^Strip_Ansi\(msg->to.*\)$', 'recipient'),
    (r'^Strip_Ansi\(msg->from.*\)$', 'sender'),
    (r'^Strip_Ansi\(msg->subj.*\)$', 'subject'),
    (r'^Strip_Ansi\(msg\.to.*\)$', 'recipient'),
    (r'^Strip_Ansi\(msg\.from.*\)$', 'sender'),
    (r'^Strip_Ansi\(msg\.subj.*\)$', 'subject'),
    # Area fields
    (r'^[PM]?MAS\(\w+,\s*name\)$', 'area_name'),
    (r'^P?FAS\(\w+,\s*name\)$', 'area_name'),
    (r'^[PM]?MAS\(\w+,\s*descript\)$', 'area_desc'),
    (r'^P?FAS\(\w+,\s*descript\)$', 'area_desc'),
    (r'^PMAS\(\w+,\s*name\)$', 'area_name'),
    (r'^PMAS\(\w+,\s*descript\)$', 'area_desc'),
    # Addresses
    (r'^Address\(', 'address'),
    # File names
    (r'^No_Path\(', 'filename'),
    (r'^upper_fn\(', 'filename'),
    (r'^filename$', 'filename'),
    (r'^fname$', 'filename'),
    (r'^filespec$', 'filespec'),
    # Pre-formatted integer buffers
    (r'^_ib\b', 'number'),
    (r'^_ib1\b', 'number1'),
    (r'^_ib2\b', 'number2'),
    (r'^_i1\b', 'number1'),
    (r'^_i2\b', 'number2'),
    (r'^_i3\b', 'number3'),
    (r'^_i4\b', 'number4'),
    (r'^_tb\b', 'value'),
    (r'^_tb1\b', 'value1'),
    (r'^_tb2\b', 'value2'),
    (r'^_w1\b', 'width1'),
    (r'^_w2\b', 'width2'),
    (r'^_h\b', 'hours'),
    (r'^_m\b', 'minutes'),
    # Character buffers
    (r'^_cb\b', 'char'),
    # Misc known patterns
    (r'^initials$', 'initials'),
    (r'^quotebuf', 'quote_text'),
    (r'^temp$', 'text'),
    (r'^word$', 'word'),
    (r'^search$', 'search_term'),
    (r'^searchfor$', 'search_term'),
    (r'^expr$', 'expression'),
    (r'^path$', 'path'),
    (r'^poo_name$', 'filename'),
    (r'^name$', 'name'),
    (r'^packet_name$', 'packet_name'),
    (r'^block$', 'block_id'),
    (r'^an$', 'area_name'),
    (r'^aname$', 'area_name'),
    # String variables
    (r'^s->txt$', 'field_name'),
    (r'^version$', 'version'),
    (r'^test$', 'build_type'),
    (r'^oldto$', 'original_to'),
    (r'^szOwner$', 'owner'),
    (r'^acbuf$', 'track_id'),
    (r'^szArea$', 'area'),
    # Function return values
    (r'^TrkGetStatus\(', 'status'),
    (r'^TrkGetPriority\(', 'priority'),
    (r'^Keys\(', 'key_flags'),
    (r'^MsgDate\(', 'date'),
    (r'^FileDateFormat\(', 'date'),
    (r'^commaize\(', 'formatted_number'),
    # Ternary/conditional expressions containing status strings
    (r'ued_sstat', 'status'),
    (r'blank_str', 'tag_status'),
    (r'notag$', 'tag_status'),
    (r'yep|nope', 'value'),
    # Tracker fields
    (r'^tmn\.szTrackID$', 'track_id'),
    (r'^szComment', 'comment'),
    (r'^ton\.to$', 'alias'),
    (r'^ton\.szOwner$', 'owner'),
    (r'^tan\.to$', 'owner'),
    (r'^tan\.szArea$', 'area'),
    (r'^to$', 'alias'),
    (r'^from$', 'source'),
    (r'^avail\b', 'availability'),
    (r'^huff->usr\.name$', 'user_name'),
    # Generic string patterns
    (r'^p$', 'path'),
    (r'^s$', 'text'),
    (r'^_n$', 'name'),
    (r'^_c$', 'city'),
]


def derive_param_name(expr, position):
    """Derive a human-readable param name from a C expression."""
    expr = expr.strip()

    for pattern, name in EXPR_PATTERNS:
        if re.search(pattern, expr):
            return name

    # Fallback: try to extract a meaningful name from the expression
    # Remove casts
    expr_clean = re.sub(r'\(char\s*\*\)', '', expr).strip()
    expr_clean = re.sub(r'\(const\s+char\s*\*\)', '', expr_clean).strip()

    # Simple variable or field access
    m = re.match(r'^(\w+)->(\w+)$', expr_clean)
    if m:
        return m.group(2)
    m = re.match(r'^(\w+)\.(\w+)$', expr_clean)
    if m:
        return m.group(2)
    m = re.match(r'^[a-zA-Z_]\w*$', expr_clean)
    if m:
        return expr_clean

    # Function call - use function name as hint
    m = re.match(r'^(\w+)\(', expr_clean)
    if m:
        return m.group(1).lower()

    return f"param{position + 1}"


def get_param_names(lang_name, call_args_list, expected_count):
    """Get the best param names for a lang string from its call sites."""
    if not call_args_list:
        return [f"param{i+1}" for i in range(expected_count)]

    # Use the first call site with the right number of args
    best_args = None
    for args in call_args_list:
        if len(args) == expected_count:
            best_args = args
            break
    if best_args is None:
        best_args = call_args_list[0]

    names = []
    for i, arg in enumerate(best_args[:expected_count]):
        names.append(derive_param_name(arg, i))

    # Pad if needed
    while len(names) < expected_count:
        names.append(f"param{len(names)+1}")

    # Deduplicate: if two params have the same name, suffix with position
    seen = {}
    for i, n in enumerate(names):
        if n in seen:
            first_idx = seen[n]
            if not names[first_idx].endswith(str(first_idx + 1)):
                names[first_idx] = f"{n}{first_idx + 1}"
            names[i] = f"{n}{i + 1}"
        else:
            seen[n] = i

    return names


def build_params_toml(slot_types, param_names):
    """Build a TOML params array string from slot types and param names.

    Returns e.g.: [{name = "version", type = "string"}, {name = "task", type = "int"}]
    """
    parts = []
    for i, (slot_idx, type_str) in enumerate(slot_types):
        name = param_names[i] if i < len(param_names) else f"param{i+1}"
        parts.append(f'{{name = "{name}", type = "{type_str}"}}')
    return "[" + ", ".join(parts) + "]"


# --- Step 4: Generate @merge entries for delta file ---

def generate_delta_entries(toml_params, call_sites):
    """Generate # @merge delta entries for params metadata.

    Returns list of (key, params_toml_str) tuples.
    """
    entries = []
    for key, slot_types in sorted(toml_params.items()):
        expected_count = len(slot_types)
        call_args = call_sites.get(key, [])
        param_names = get_param_names(key, call_args, expected_count)
        params_toml = build_params_toml(slot_types, param_names)
        entries.append((key, params_toml))
    return entries


def write_delta_entries(entries):
    """Append @merge params entries to delta_english.toml.

    Preserves existing non-@merge content. Replaces the @merge section
    if it already exists (delimited by the @merge-section markers).
    """
    SECTION_START = "# --- @merge: params metadata (auto-generated) ---\n"
    SECTION_END = "# --- end @merge params ---\n"

    # Read existing delta file
    existing_lines = []
    if os.path.isfile(DELTA_PATH):
        with open(DELTA_PATH, "r", encoding="latin-1") as f:
            existing_lines = f.readlines()

    # Strip any previous @merge section
    output = []
    in_section = False
    for line in existing_lines:
        if line.strip() == SECTION_START.strip():
            in_section = True
            continue
        if line.strip() == SECTION_END.strip():
            in_section = False
            continue
        if not in_section:
            output.append(line)

    # Ensure trailing newline before our section
    if output and not output[-1].endswith("\n"):
        output[-1] += "\n"
    if output and output[-1].strip():
        output.append("\n")

    # Append the new @merge section
    output.append(SECTION_START)
    for key, params_toml in entries:
        output.append("# @merge\n")
        output.append(f"{key} = {{ params = {params_toml} }}\n")
    output.append(SECTION_END)

    with open(DELTA_PATH, "w", encoding="latin-1") as f:
        f.writelines(output)

    return len(entries)


def main():
    print("Step 1: Scanning english.toml for |!N parameters (with type suffixes)...")
    toml_params = find_toml_params()
    print(f"  Found {len(toml_params)} strings with positional parameters")

    print("Step 2: Extracting call sites (LangPrintf, logit, LangSprintf)...")
    call_sites = extract_call_sites()
    print(f"  Found {len(call_sites)} unique lang string names in call sites")

    matched = sum(1 for k in toml_params if k in call_sites)
    print(f"  {matched}/{len(toml_params)} TOML strings have matching call sites")

    print("Step 3: Generating @merge params entries...")
    entries = generate_delta_entries(toml_params, call_sites)

    print(f"Step 4: Writing {len(entries)} entries to {DELTA_PATH}...")
    written = write_delta_entries(entries)
    print(f"  Wrote {written} @merge entries")

    # Show unmatched strings
    unmatched = [k for k in toml_params if k not in call_sites]
    if unmatched:
        print(f"\n  {len(unmatched)} strings without call sites (may use Puts/other paths):")
        for k in sorted(unmatched)[:20]:
            slots = toml_params[k]
            print(f"    - {k} ({len(slots)} params)")
        if len(unmatched) > 20:
            print(f"    ... and {len(unmatched) - 20} more")

    print("\nDone!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
