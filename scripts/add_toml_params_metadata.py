#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Description: Add params metadata to english.toml strings that use |!N positional parameters.
#              Extracts parameter names from LangPrintf() C call sites.
# Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

"""
Phase 2.6 tool: annotate english.toml strings that contain |!N positional
parameters with a 'params' array describing each parameter.

Workflow:
  1. Parse english.toml (latin-1 for CP437 compat) to find strings with |!N
  2. Grep C sources for LangPrintf() calls to extract argument expressions
  3. Apply naming heuristics to derive human-readable param names
  4. Rewrite english.toml with params metadata added

Output format examples:
  Simple string:
    key = "Hello |!1"  →  key = { text = "Hello |!1", params = ["name"] }
  Existing inline table:
    key = { text = "...|!1...", flags = [...] }  →  adds params = [...]
"""

import os
import re
import sys
import glob

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOML_PATH = os.path.join(PROJECT_ROOT, "resources", "lang", "english.toml")
SRC_DIR = os.path.join(PROJECT_ROOT, "src", "max")

# --- Step 1: Extract |!N info from TOML ---

def find_toml_params():
    """Return dict: key_name → max_param_index for strings containing |!N."""
    result = {}
    with open(TOML_PATH, "r", encoding="latin-1") as f:
        for line in f:
            line = line.rstrip("\n")
            m = re.match(r'^([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*', line)
            if not m:
                continue
            key = m.group(1)
            # Find all |!N references in the line
            params = re.findall(r'\|!([1-9A-F])', line)
            if not params:
                continue
            max_idx = 0
            for p in params:
                if p.isdigit():
                    idx = int(p)
                else:
                    idx = ord(p) - ord('A') + 10
                if idx > max_idx:
                    max_idx = idx
            result[key] = max_idx
    return result


# --- Step 2: Extract LangPrintf call sites ---

def extract_langprintf_calls():
    """
    Return dict: lang_string_name → list of argument expression lists.
    Each call gives one list of arg expressions (excluding the format string).
    """
    calls = {}
    c_files = glob.glob(os.path.join(SRC_DIR, "**", "*.c"), recursive=True)

    for fpath in c_files:
        try:
            with open(fpath, "r", encoding="latin-1") as f:
                content = f.read()
        except Exception:
            continue

        # Find LangPrintf calls - may span multiple lines
        # Pattern: LangPrintf( identifier , args... )
        # We need to handle multi-line calls and nested parens
        idx = 0
        while True:
            pos = content.find("LangPrintf(", idx)
            if pos == -1:
                break
            # Check it's not in a comment or #define
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
            # Normalize whitespace
            args_str = re.sub(r'\s+', ' ', args_str).strip()

            # Split by top-level commas (not inside parens)
            args = split_args(args_str)
            if len(args) < 1:
                idx = pos + 1
                continue

            lang_name = args[0].strip()
            param_args = [a.strip() for a in args[1:]]

            # Only track calls where first arg is a simple identifier (lang string name)
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
            # Rename the first occurrence too if not already
            first_idx = seen[n]
            if not names[first_idx].endswith(str(first_idx + 1)):
                names[first_idx] = f"{n}{first_idx + 1}"
            names[i] = f"{n}{i + 1}"
        else:
            seen[n] = i

    return names


# --- Step 4: Rewrite TOML with params metadata ---

def rewrite_toml(toml_params, call_sites):
    """Rewrite english.toml adding params metadata to strings with |!N."""
    with open(TOML_PATH, "r", encoding="latin-1") as f:
        lines = f.readlines()

    output = []
    modified = 0

    for line in lines:
        raw = line.rstrip("\n")
        m = re.match(r'^([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*', raw)
        if not m or m.group(1) not in toml_params:
            output.append(line)
            continue

        key = m.group(1)
        expected_count = toml_params[key]
        call_args = call_sites.get(key, [])
        param_names = get_param_names(key, call_args, expected_count)
        params_str = ", ".join(f'"{n}"' for n in param_names)

        value_part = raw[m.end():].strip()

        # Check if already has params
        if "params" in value_part:
            output.append(line)
            continue

        # Case 1: inline table  { text = "...", ... }
        if value_part.startswith("{"):
            # Insert params before the closing }
            closing = value_part.rfind("}")
            if closing >= 0:
                inner = value_part[:closing].rstrip().rstrip(",")
                new_line = f"{key} = {inner}, params = [{params_str}] }}\n"
                output.append(new_line)
                modified += 1
                continue

        # Case 2: simple string  "..."
        if value_part.startswith('"'):
            new_line = f'{key} = {{ text = {value_part}, params = [{params_str}] }}\n'
            output.append(new_line)
            modified += 1
            continue

        # Unknown format, leave as-is
        output.append(line)

    with open(TOML_PATH, "w", encoding="latin-1") as f:
        f.writelines(output)

    return modified


def main():
    print("Step 1: Scanning english.toml for |!N parameters...")
    toml_params = find_toml_params()
    print(f"  Found {len(toml_params)} strings with positional parameters")

    print("Step 2: Extracting LangPrintf() call sites from C sources...")
    call_sites = extract_langprintf_calls()
    print(f"  Found {len(call_sites)} unique lang string names in call sites")

    # Show coverage
    matched = sum(1 for k in toml_params if k in call_sites)
    print(f"  {matched}/{len(toml_params)} TOML strings have matching call sites")

    print("Step 3: Deriving parameter names and rewriting TOML...")
    modified = rewrite_toml(toml_params, call_sites)
    print(f"  Modified {modified} TOML entries with params metadata")

    # Show unmatched strings (TOML has |!N but no call site found)
    unmatched = [k for k in toml_params if k not in call_sites]
    if unmatched:
        print(f"\n  {len(unmatched)} strings without call sites (may use Puts/other paths):")
        for k in sorted(unmatched)[:20]:
            print(f"    - {k} ({toml_params[k]} params)")
        if len(unmatched) > 20:
            print(f"    ... and {len(unmatched) - 20} more")

    print("\nDone!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
