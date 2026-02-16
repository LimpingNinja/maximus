#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Description: Enrich @merge params metadata with desc and default values.
#              Reads call sites from C sources and applies knowledge base.
# Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

"""
Enrich params metadata entries in delta_english.toml with human-readable
descriptions and mock/default values derived from C call site analysis.

Reads the existing delta file, cross-references call sites in C sources,
and rewrites the @merge section with enriched {name, type, desc, default}
fields for each parameter.

Builds on add_toml_params_metadata.py — uses the same call site extraction
but adds a knowledge base for desc/default derivation.
"""

import os
import re
import sys
import glob

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOML_PATH = os.path.join(PROJECT_ROOT, "resources", "lang", "english.toml")
DELTA_PATH = os.path.join(PROJECT_ROOT, "resources", "lang", "delta_english.toml")
SRC_DIR = os.path.join(PROJECT_ROOT, "src", "max")

SUFFIX_TYPES = {'d': 'int', 'l': 'long', 'u': 'uint', 'c': 'char'}

# ========================================================================
# Knowledge base: C expression pattern → (desc, default)
#
# Each entry: (regex_pattern, description, default_value)
# Matched against the raw C argument expression from call sites.
# First match wins — order matters (specific before general).
# ========================================================================

ENRICHMENT_KB = [
    # --- User fields ---
    (r'^usr\.name$',              "Caller's display name",          "Kevin Morgan"),
    (r'^usrname$',                "Caller's display name",          "Kevin Morgan"),
    (r'^username$',               "Caller's display name",          "Kevin Morgan"),
    (r'^user\.name$',             "User's display name",            "Kevin Morgan"),
    (r'^huff->usr\.name$',        "User's display name",            "Kevin Morgan"),
    (r'^firstname$',              "Caller's first name",            "Kevin"),
    (r'^usr\.alias$',             "Caller's alias",                 "Ninja"),
    (r'^user\.alias$',            "User's alias",                   "Ninja"),
    (r'^usr\.city$',              "Caller's city",                  "Los Angeles"),
    (r'^user\.city$',             "User's city",                    "Los Angeles"),
    (r'^usr\.phone$',             "Caller's voice phone",           "555-1234"),
    (r'^user\.phone$',            "User's voice phone",             "555-1234"),
    (r'^user\.dataphone$',        "User's data phone",              "555-5678"),
    (r'^usr\.msg$',               "Current msg area tag",           "GENERAL"),
    (r'^usr\.files$',             "Current file area tag",          "FILES"),
    (r'^user\.msg$',              "User's msg area tag",            "GENERAL"),
    (r'^user\.files$',            "User's file area tag",           "FILES"),
    (r'^usr\.times$',             "Total call count",               "42"),
    (r'^usr\.credit',             "Credit balance (cents)",         "100"),
    (r'^usr\.debit',              "Debit balance (cents)",          "0"),
    (r'^user\.priv$',             "User's privilege level",         "Normal"),

    # --- Message fields ---
    (r'^msg->to$',                "Message recipient",              "All"),
    (r'^msg->from$',              "Message sender",                 "Sysop"),
    (r'^msg->subj$',              "Message subject",                "Hello World"),
    (r'^mmsg->to$',               "Message recipient",              "All"),
    (r'^mmsg->from$',             "Message sender",                 "Sysop"),
    (r'^mmsg->subj$',             "Message subject",                "Hello World"),
    (r'^f->tmsg\.to$',            "Forwarded-to recipient",         "All"),
    (r'^Strip_Ansi\(msg->to',     "Message recipient (plain)",      "All"),
    (r'^Strip_Ansi\(msg->from',   "Message sender (plain)",         "Sysop"),
    (r'^Strip_Ansi\(msg->subj',   "Message subject (plain)",        "Hello World"),
    (r'^Strip_Ansi\(msg\.to',     "Message recipient (plain)",      "All"),
    (r'^Strip_Ansi\(msg\.from',   "Message sender (plain)",         "Sysop"),
    (r'^Strip_Ansi\(msg\.subj',   "Message subject (plain)",        "Hello World"),
    (r'^Strip_Ansi\(b->msg\.to',  "Message recipient (plain)",      "All"),
    (r'^Strip_Ansi\(b->msg\.from',"Message sender (plain)",         "Sysop"),
    (r'^Strip_Ansi\(b->msg\.subj',"Message subject (plain)",        "Hello World"),

    # --- Area fields ---
    (r'^[PM]?MAS\(\w+,\s*name\)$',   "Message area name",          "General"),
    (r'^P?FAS\(\w+,\s*name\)$',      "File area name",             "Uploads"),
    (r'^PFAS\(\w+,\s*name\)$',       "File area name",             "Uploads"),
    (r'^PMAS\(\w+,\s*name\)$',       "Message area name",          "General"),
    (r'^[PM]?MAS\(\w+,\s*descript',  "Message area description",   "General Discussion"),
    (r'^P?FAS\(\w+,\s*descript',     "File area description",      "Public uploads"),
    (r'^FAS\(\w+,\s*uppath\)',       "Upload path",                "/files/uploads/"),
    (r'^an$',                        "Area name",                   "General"),
    (r'^aname$',                     "Area name",                   "General"),
    (r'^szArea$',                    "Area tag",                    "GENERAL"),
    (r'->szArea$',                   "Area tag",                    "GENERAL"),
    (r'^msgname$',                   "Message area name",           "General"),
    (r'^areano$',                    "Area number/tag",             "GENERAL"),
    (r'^orig_area$',                 "Original area name",          "General"),

    # --- Address/network ---
    (r'^Address\(',               "FidoNet address",                "1:234/567"),
    (r'^nf->found\.name$',        "Nodelist entry name",            "Sysop Station"),
    (r'^nf->found\.city$',        "Nodelist entry city",            "Los Angeles"),
    (r'^addr$',                   "Network address",                "1:234/567"),

    # --- File fields ---
    (r'^No_Path\(',               "Filename (no path)",             "FILE.ZIP"),
    (r'^upper_fn\(',              "Filename (uppercase)",            "FILE.ZIP"),
    (r'^fancy_fn\(',              "Filename (display form)",         "file.zip"),
    (r'^filename$',               "Filename",                       "FILE.ZIP"),
    (r'^fname$',                  "Filename",                       "FILE.ZIP"),
    (r'^filespec$',               "File specification",             "*.ZIP"),
    (r'^fent\.szName$',           "File entry name",                "FILE.ZIP"),
    (r'^name$',                   "Name",                           "FILE.ZIP"),
    (r'^poo_name$',               "Output filename",                "FILE.ZIP"),
    (r'^barfile$',                "Barricade filename",             "barricad.bbs"),
    (r'^mpath$',                  "Menu file path",                 "main.mnu"),

    # --- Transfer/protocol ---
    (r'^Protocol_Name\(',         "Transfer protocol name",         "Zmodem"),
    (r'^protocol_letter$',        "Protocol letter code",           "Z"),
    (r'^baud$',                   "Connection speed (bps)",         "9600"),
    (r'^min_baud$',               "Minimum baud rate",              "2400"),

    # --- Formatting buffers (generic integers) ---
    (r'^_ib\b',                   "Numeric value",                  "42"),
    (r'^_ib1\b',                  "First numeric value",            "42"),
    (r'^_ib2\b',                  "Second numeric value",           "10"),
    (r'^_i1\b',                   "First value",                    "42"),
    (r'^_i2\b',                   "Second value",                   "10"),
    (r'^_i3\b',                   "Third value",                    "5"),
    (r'^_i4\b',                   "Fourth value",                   "1"),
    (r'^_tb\b',                   "Formatted value",                "42"),
    (r'^_tb1\b',                  "First text buffer",              "abc"),
    (r'^_tb2\b',                  "Second text buffer",             "def"),
    (r'^_w1\b',                   "First width value",              "  "),
    (r'^_w2\b',                   "Second width value",             "  "),
    (r'^_cx\b',                   "Cursor X position",              "1"),
    (r'^_cb\b',                   "Character value",                "A"),
    (r'^_cc\b',                   "Channel/count",                  "1"),
    (r'^_tn\b',                   "Node number",                    "1"),
    (r'^_xb\b',                   "Target node number",             "2"),
    (r'^_nd\b',                   "Node/task number",               "1"),
    (r'^_xm\b',                   "Minutes value",                  "30"),
    (r'^_et\b',                   "Event time (minutes)",           "60"),
    (r'^_t1\b',                   "Time limit (minutes)",           "120"),
    (r'^_t2\b',                   "Time used (minutes)",            "15"),
    (r'^_nb\b',                   "Number of items",                "3"),
    (r'^_dl\b',                   "Download counter",               "1"),
    (r'^_col\b',                  "Color code",                     "|07"),
    (r'^_kb\b',                   "Kilobytes",                      "1024"),
    (r'^_nf\b',                   "File count",                     "5"),
    (r'^_cr\b',                   "Credits (cents)",                "100"),
    (r'^_db\b',                   "Debits (cents)",                 "50"),
    (r'^_co\b',                   "Cost (cents)",                   "10"),
    (r'^_bal\b',                  "Balance (cents)",                "50"),

    # --- Time/date subfields ---
    (r'^_h$',                     "Hours",                          "14"),
    (r'^_m$',                     "Minutes",                        "30"),
    (r'^_hh$',                    "Hours (zero-padded)",            "14"),
    (r'^_mm$',                    "Minutes (zero-padded)",          "30"),
    (r'^_ss$',                    "Seconds (zero-padded)",          "45"),
    (r'^_mo$',                    "Month",                          "2"),
    (r'^_da$',                    "Day",                            "15"),
    (r'^_yr$',                    "Year (2-digit)",                 "25"),
    (r'^_a$',                     "Date component A",               "2"),
    (r'^_b$',                     "Date component B",               "15"),
    (r'^_c$',                     "City or date component",          "Los Angeles"),

    # --- Archive/compression fields ---
    (r'^_uc$',                    "Uncompressed size",              "102400"),
    (r'^_cc$',                    "Compressed size",                "51200"),
    (r'^_tu$',                    "Total uncompressed",             "1048576"),
    (r'^_tc$',                    "Total compressed",               "524288"),
    (r'^_rp$',                    "Ratio whole part",               "50"),
    (r'^_rd$',                    "Ratio decimal part",             "0"),
    (r'^_pct$',                   "Compression percent",            "50"),
    (r'^_pd$',                    "Percent decimal",                "0"),
    (r'^_crc$',                   "CRC checksum",                   "A1B2"),
    (r'^_enc$',                   "Encryption flag char",           " "),
    (r'^method$',                 "Compression method",             "Deflate"),
    (r'^attr',                    "File attributes",                "----"),
    (r'^file_comment$',           "Archive file comment",           "No comment"),

    # --- Color codes ---
    (r'^LMAGENTA$',               "Color code (light magenta)",     "|13"),
    (r'^LRED$',                   "Color code (light red)",         "|12"),
    (r'^LGREEN$',                 "Color code (light green)",       "|10"),
    (r'^LCYAN$',                  "Color code (light cyan)",        "|11"),
    (r'^YELLOW$',                 "Color code (yellow)",            "|14"),
    (r'^WHITE$',                  "Color code (white)",             "|15"),
    (r'^CYAN$',                   "Color code (cyan)",              "|03"),
    (r'^GRAY$',                   "Color code (gray)",              "|07"),
    (r'^GREEN$',                  "Color code (green)",             "|02"),
    (r'^RED$',                    "Color code (red)",               "|04"),
    (r'^msg_\w*_col$',            "Message display color",          "|07"),
    (r'^file_\w*_col$',           "File display color",             "|07"),
    (r'^msg_quote_col$',          "Quote text color",               "|03"),

    # --- Privilege/status ---
    (r'^privstr\(',               "Privilege level name",            "Normal"),
    (r'^TrkGetStatus\(',          "Tracking status",                "Open"),
    (r'^TrkGetPriority\(',        "Tracking priority",              "Normal"),
    (r'^Keys\(',                  "Key flags string",               "12345678"),
    (r'^MsgDate\(',               "Message date string",            "15 Feb 25 14:30:00"),
    (r'^FileDateFormat\(',        "File date string",               "15 Feb 25"),
    (r'^commaize\(',              "Formatted number with commas",   "1,234,567"),
    (r'^Yes_or_No\(',             "Yes/No flag",                    "Yes"),
    (r'^Help_Level\(',            "Help level name",                "Novice"),
    (r'^Graphics_Mode\(',         "Video mode name",                "ANSI"),
    (r'^Sex\(',                   "Gender",                         "Unspecified"),
    (r'^Expire_Action\(',         "Expiry action",                  "Demote"),
    (r'^Expire_By\(',             "Expiry method",                  "Date"),
    (r'^Expire_At\(',             "Expiry date",                    "Never"),
    (r'^DOB\(',                   "Date of birth",                  "1990-01-15"),
    (r'^sc_time\(',               "Formatted timestamp",            "15 Feb 25 14:30"),
    (r'^Show_Attributes\(',       "Message attribute flags",        "Pvt"),

    # --- Tracker fields ---
    (r'^ptmn->szTrackID$',        "Tracking ID",                    "AB12"),
    (r'^tmn\.szTrackID$',         "Tracking ID",                    "AB12"),
    (r'^szOwner$',                "Tracking owner",                 "Sysop"),
    (r'^ton\.szOwner$',           "Owner name",                     "Sysop"),
    (r'^pton->szOwner$',          "Owner name",                     "Sysop"),
    (r'^ton\.to$',                "Owner alias",                    "SYS"),
    (r'^tan\.to$',                "Area owner alias",               "SYS"),
    (r'^tan\.szArea$',            "Tracked area",                   "GENERAL"),
    (r'^ptan->szArea$',           "Tracked area",                   "GENERAL"),
    (r'^szComment',               "Tracking comment",               "Working on it"),
    (r'^acbuf$',                  "Tracking ID buffer",             "AB12"),
    (r'^actrack$',                "Tracking ID",                    "AB12"),

    # --- Misc known patterns ---
    (r'^version$',                "Software version string",        "3.04a"),
    (r'^comp_date$',              "Compile date",                   "Feb 15 2025"),
    (r'^comp_time$',              "Compile time",                   "14:30:00"),
    (r'^errno$',                  "System error number",            "2"),
    (r'^task_num$',               "Task/node number",               "1"),
    (r'^initials$',               "Quoter's initials",              "KM"),
    (r'^quotebuf',                "Quoted text line",               "Hello world"),
    (r'^search$',                 "Search string",                  "hello"),
    (r'^searchfor$',              "Search pattern",                 "hello"),
    (r'^word$',                   "Input word",                     "hello"),
    (r'^path$',                   "File/directory path",            "/bbs/files/"),
    (r'^p$',                      "Path or filename",               "FILE.ZIP"),
    (r'^s$',                      "Text string",                    "text"),
    (r'^temp$',                   "Temporary buffer",               "text"),
    (r'^block$',                  "Block ID string",                "BBSID"),
    (r'^packet_name$',            "QWK packet name",                "MYBBS"),
    (r'^msg_name$',               "Message filename",               "001.MSG"),
    (r'^qwk_path$',               "QWK directory path",             "/tmp/qwk/"),
    (r'^qwk_busy$',               "QWK lock file path",             "/tmp/qwk.busy"),
    (r'^status$',                 "Status text",                    "Idle"),
    (r'^mt->msg$',                "Multitasker name",               "Unix"),
    (r'^szMcpPipe$',              "MCP pipe path",                  "/tmp/mcp"),
    (r'^original_path$',          "System path",                    "/bbs/"),
    (r'^oldto$',                  "Previous recipient",             "All"),
    (r'^do_what$',                "Action description",             "continue"),
    (r'^got$',                    "Entered password",               "****"),
    (r'^check$',                  "Expected password",              "****"),
    (r'^pwd$',                    "Password attempt",               "****"),
    (r'^parm$',                   "External program parameter",     "door.exe"),
    (r'^err$',                    "Error message",                  "syntax error"),
    (r'^cmd$',                    "Command string",                 "pkzip"),
    (r'^erl$',                    "Error level / exit code",        "0"),
    (r'^ret$',                    "Return code",                    "0"),
    (r'^rc$',                     "Return code",                    "0"),
    (r'^arg$',                    "Argument string",                "main.mnu"),
    (r'^szSize$',                 "File size string",               "1,024"),
    (r'^from$',                   "Source name/path",               "UPLOADS"),
    (r'^to$',                     "Destination/alias",              "GENERAL"),
    (r'^type$',                   "Type code",                      "1"),
    (r'^cc$',                     "CB channel number",              "1"),
    (r'^x$',                      "Node number",                    "2"),
    (r'^tid$',                    "Task/node ID",                   "1"),
    (r'^left$',                   "Minutes remaining",              "45"),
    (r'^cost$',                   "Message cost (cents)",           "10"),
    (r'^_n$',                     "Name field",                     "Kevin Morgan"),
    (r'^_c$',                     "City field",                     "Los Angeles"),
    (r'^ck_for_help$',            "Help hint text",                 "Type ? for help"),
    (r'^s->txt$',                 "Field label",                    "To/From/Subject"),
    (r'^brackets_encrypted$',     "Encrypted password display",     "[encrypted]"),
    (r'^ar\?',                    "Archiver name or none",          "ZIP"),
    (r'^proto_none$',             "No protocol text",               "None"),
    (r'lname$',                   "Language name",                  "English"),

    # --- Conditional/ternary expressions ---
    (r'blank_str\b.*notag\b|notag\b.*blank_str\b', "Tag status (blank if tagged)", ""),
    (r'blank_str\b',             "Conditional text (may be empty)", ""),
    (r'ued_sstat',               "User status indicator",           "Current"),
    (r'pl_match$',               "Plural suffix (es or blank)",     "es"),
    (r'remain_pvt$',             "Private area note",               ""),
    (r'orig_or_dest$',           "Origin/Destination label",        "From:"),
    (r'msg_abbr$',               "Area type abbreviation",          "Msg"),
    (r'deny_file$',              "Denied area type",                "File"),
    (r'entering$',               "Action (entering/exiting)",       "Entering"),
    (r'exiting$',                "Action (entering/exiting)",       "Exiting"),
    (r'the_chatmode$',           "Chat mode label",                 " chat mode"),
    (r'arq_info',                "ARQ correction info",             "V.42bis"),
    (r'szDefault\b',             "Default marker",                  " (default)"),

    # --- Message header color arguments ---
    (r'^msg_from_col$',          "From: label color",               "|03"),
    (r'^msg_from_txt_col$',      "From: text color",                "|14"),
    (r'^msg_to_col$',            "To: label color",                 "|03"),
    (r'^msg_to_txt_col$',        "To: text color",                  "|14"),
    (r'^msg_date_col$',          "Date color",                      "|03"),
    (r'^msg_attr_col$',          "Attribute color",                 "|12"),
    (r'^msg_locus_col$',         "Address color",                   "|07"),
    (r'^msg_addr_col$',          "Address label color",             "|03"),
    (r'^msgar_name$',            "Area type prefix",                "Msg "),
    (r'^file_offline_col$',      "Offline file color",              "|12"),

    # --- Message browse indicators ---
    (r'br_msg_new\b',            "New message indicator",           "*"),
    (r'br_msg_pvt\b',            "Private message indicator",       "P"),
    (r'br_msg_notnew\b',         "Read message indicator",          " "),

    # --- ngcfg fields ---
    (r'ngcfg_get_string_raw\("maximus\.sysop"\)', "Sysop name",    "Sysop"),
    (r'ngcfg_get_string_raw\("maximus\.net_info_path"\)', "Nodelist directory", "/bbs/nodelist/"),
    (r'ngcfg_get_bool',          "Config boolean flag",             "true"),
    (r'ngcfg_get_path',          "Config file path",                "/bbs/"),
    (r's_alias$',                "Alias prompt suffix",             " (or alias)"),

    # --- Transfer window fields ---
    (r'^_tl$',                   "Thread/reply link number",        "100"),
    (r'^_uid$',                  "Message UID number",              "1234"),
    (r'^_old$',                  "Old/changed flag char",           " "),
    (r'^tossto$',                "Area number for tossing",         "1"),
]


# ========================================================================
# Reuse call site extraction from add_toml_params_metadata.py
# ========================================================================

CALL_PATTERNS = [
    ("LangPrintf(", 0),
    ("logit(", 0),
    ("LangSprintf(", 2),
]


def find_toml_params():
    """Return dict: key_name → list of (slot_index, type_str) tuples."""
    result = {}
    with open(TOML_PATH, "r", encoding="latin-1") as f:
        for line in f:
            line = line.rstrip("\n")
            m = re.match(r'^([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*', line)
            if not m:
                continue
            key = m.group(1)
            params = re.findall(r'\|!([1-9A-F])([dlucDLUC])?', line)
            if not params:
                continue
            slots = {}
            for slot_ch, suffix in params:
                idx = int(slot_ch) if slot_ch.isdigit() else ord(slot_ch) - ord('A') + 10
                type_str = SUFFIX_TYPES.get(suffix.lower(), 'string') if suffix else 'string'
                slots[idx] = type_str
            max_slot = max(slots.keys())
            slot_list = [(i, slots.get(i, 'string')) for i in range(1, max_slot + 1)]
            result[key] = slot_list
    return result


def split_args(s):
    """Split a comma-separated argument string respecting nested parens and strings."""
    args, depth, current, in_string, escape = [], 0, [], False, False
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
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
        if ch == ',' and depth == 0:
            args.append(''.join(current))
            current = []
        else:
            current.append(ch)
    if current:
        args.append(''.join(current))
    return args


def extract_call_sites():
    """Return dict: lang_string_name → list of {file, line, func, args}."""
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
                line_start = content.rfind("\n", 0, pos) + 1
                prefix = content[line_start:pos].strip()
                if prefix.startswith("//") or prefix.startswith("*") or prefix.startswith("#"):
                    idx = pos + 1
                    continue
                paren_start = content.index("(", pos)
                depth, i = 1, paren_start + 1
                while i < len(content) and depth > 0:
                    if content[i] == "(":
                        depth += 1
                    elif content[i] == ")":
                        depth -= 1
                    i += 1
                if depth != 0:
                    idx = pos + 1
                    continue
                args_str = re.sub(r'\s+', ' ', content[paren_start + 1:i - 1]).strip()
                args = split_args(args_str)
                if len(args) <= fmt_idx:
                    idx = i
                    continue
                lang_name = args[fmt_idx].strip()
                param_args = [a.strip() for a in args[fmt_idx + 1:]]
                if re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*$', lang_name):
                    if lang_name not in calls:
                        calls[lang_name] = []
                    calls[lang_name].append(param_args)
                idx = i
    return calls


# ========================================================================
# Enrichment logic
# ========================================================================

# Fallback: derive name from C expression (reused from add_toml_params_metadata.py)
EXPR_NAME_PATTERNS = [
    (r'^usr\.name$', 'user_name'), (r'^usrname$', 'user_name'),
    (r'^usr\.city$', 'user_city'), (r'^usr\.phone$', 'user_phone'),
    (r'^usr\.alias$', 'user_alias'), (r'^usr\.msg$', 'msg_area'),
    (r'^msg\.to$', 'recipient'), (r'^msg\.from$', 'sender'),
    (r'^msg\.subj$', 'subject'), (r'^msg->to$', 'recipient'),
    (r'^msg->from$', 'sender'), (r'^msg->subj$', 'subject'),
    (r'^mmsg->to$', 'recipient'), (r'^mmsg->from$', 'sender'),
    (r'^mmsg->subj$', 'subject'),
    (r'^Strip_Ansi\(msg->to', 'recipient'), (r'^Strip_Ansi\(msg->from', 'sender'),
    (r'^Strip_Ansi\(msg->subj', 'subject'),
    (r'^Strip_Ansi\(b->msg\.from', 'sender'), (r'^Strip_Ansi\(b->msg\.to', 'recipient'),
    (r'^Strip_Ansi\(b->msg\.subj', 'subject'),
    (r'^[PM]?MAS\(\w+,\s*name\)$', 'area_name'),
    (r'^P?FAS\(\w+,\s*name\)$', 'area_name'),
    (r'^[PM]?MAS\(\w+,\s*descript', 'area_desc'),
    (r'^P?FAS\(\w+,\s*descript', 'area_desc'),
    (r'^Address\(', 'address'), (r'^No_Path\(', 'filename'),
    (r'^upper_fn\(', 'filename'), (r'^filename$', 'filename'),
    (r'^fname$', 'filename'), (r'^filespec$', 'filespec'),
    (r'^commaize\(', 'formatted_number'), (r'^Protocol_Name\(', 'protocol_name'),
    (r'^privstr\(', 'priv_level'), (r'^TrkGetStatus\(', 'status'),
    (r'^TrkGetPriority\(', 'priority'), (r'^Keys\(', 'key_flags'),
    (r'^MsgDate\(', 'date'), (r'^FileDateFormat\(', 'date'),
    (r'^Yes_or_No\(', 'yes_no'), (r'^Help_Level\(', 'help_level'),
    (r'^Graphics_Mode\(', 'video_mode'), (r'^Sex\(', 'gender'),
    (r'^version$', 'version'), (r'^baud$', 'baud_rate'),
    (r'^errno$', 'errno'), (r'^task_num$', 'task_num'),
]


def derive_name(expr, position):
    """Derive param name from C expression."""
    expr = expr.strip()
    for pattern, name in EXPR_NAME_PATTERNS:
        if re.search(pattern, expr):
            return name
    # Remove casts
    ec = re.sub(r'\((?:const\s+)?char\s*\*\)', '', expr).strip()
    m = re.match(r'^(\w+)->(\w+)$', ec)
    if m:
        return m.group(2)
    m = re.match(r'^(\w+)\.(\w+)$', ec)
    if m:
        return m.group(2)
    m = re.match(r'^[a-zA-Z_]\w*$', ec)
    if m:
        return ec
    m = re.match(r'^(\w+)\(', ec)
    if m:
        return m.group(1).lower()
    return f"param{position + 1}"


def enrich_param(expr, type_str, position):
    """Return (name, desc, default) for a parameter based on its C expression."""
    expr = expr.strip()

    # Try knowledge base for desc/default
    desc = ""
    default = ""
    for pattern, kb_desc, kb_default in ENRICHMENT_KB:
        if re.search(pattern, expr):
            desc = kb_desc
            default = kb_default
            break

    # If no KB match, generate generic desc from type
    if not desc:
        type_descs = {
            'int': 'Integer value',
            'long': 'Long integer value',
            'uint': 'Unsigned integer value',
            'char': 'Single character',
            'string': 'Text value',
        }
        desc = type_descs.get(type_str, 'Value')

    # Generate type-appropriate default if KB didn't provide one
    if not default:
        type_defaults = {
            'int': '0',
            'long': '0',
            'uint': '0',
            'char': ' ',
            'string': 'text',
        }
        default = type_defaults.get(type_str, '')

    name = derive_name(expr, position)
    return name, desc, default


def build_enriched_params_toml(slot_types, call_args):
    """Build enriched TOML params array from slot types and call site args."""
    expected = len(slot_types)

    # Find best matching call site
    best_args = None
    if call_args:
        for args in call_args:
            if len(args) == expected:
                best_args = args
                break
        if best_args is None and call_args:
            best_args = call_args[0]

    parts = []
    for i, (slot_idx, type_str) in enumerate(slot_types):
        if best_args and i < len(best_args):
            name, desc, default = enrich_param(best_args[i], type_str, i)
        else:
            name = f"param{i + 1}"
            type_descs = {'int': 'Integer value', 'long': 'Long integer value',
                          'uint': 'Unsigned integer value', 'char': 'Single character',
                          'string': 'Text value'}
            desc = type_descs.get(type_str, 'Value')
            type_defaults = {'int': '0', 'long': '0', 'uint': '0', 'char': ' ', 'string': 'text'}
            default = type_defaults.get(type_str, '')

        # Escape quotes in desc/default
        desc = desc.replace('"', '\\"')
        default = default.replace('"', '\\"')

        parts.append(f'{{name = "{name}", type = "{type_str}", desc = "{desc}", default = "{default}"}}')

    # Deduplicate names
    names = [p.split('"')[1] for p in parts]  # extract name from each part
    seen = {}
    for i, n in enumerate(names):
        if n in seen:
            first_idx = seen[n]
            old_name = names[first_idx]
            if not old_name.endswith(str(first_idx + 1)):
                parts[first_idx] = parts[first_idx].replace(f'name = "{old_name}"', f'name = "{old_name}{first_idx + 1}"', 1)
            parts[i] = parts[i].replace(f'name = "{n}"', f'name = "{n}{i + 1}"', 1)
        else:
            seen[n] = i

    return "[" + ", ".join(parts) + "]"


# ========================================================================
# Delta file output
# ========================================================================

def write_enriched_delta(toml_params, call_sites):
    """Rewrite the @merge section of delta_english.toml with enriched params."""
    SECTION_START = "# --- @merge: params metadata (auto-generated) ---\n"
    SECTION_END = "# --- end @merge params ---\n"

    # Read existing delta
    existing = []
    if os.path.isfile(DELTA_PATH):
        with open(DELTA_PATH, "r", encoding="latin-1") as f:
            existing = f.readlines()

    # Strip previous @merge section
    output = []
    in_section = False
    for line in existing:
        if line.strip() == SECTION_START.strip():
            in_section = True
            continue
        if line.strip() == SECTION_END.strip():
            in_section = False
            continue
        if not in_section:
            output.append(line)

    # Ensure blank line before section
    if output and not output[-1].endswith("\n"):
        output[-1] += "\n"
    if output and output[-1].strip():
        output.append("\n")

    # Generate enriched entries
    output.append(SECTION_START)
    count = 0
    for key in sorted(toml_params.keys()):
        slot_types = toml_params[key]
        call_args = call_sites.get(key, [])
        params_toml = build_enriched_params_toml(slot_types, call_args)
        output.append("# @merge\n")
        output.append(f"{key} = {{ params = {params_toml} }}\n")
        count += 1
    output.append(SECTION_END)

    with open(DELTA_PATH, "w", encoding="latin-1") as f:
        f.writelines(output)

    return count


def main():
    print("Step 1: Scanning english.toml for |!N parameters...")
    toml_params = find_toml_params()
    print(f"  Found {len(toml_params)} parameterized strings")

    print("Step 2: Extracting C call sites...")
    call_sites = extract_call_sites()
    matched = sum(1 for k in toml_params if k in call_sites)
    print(f"  {matched}/{len(toml_params)} strings matched to call sites")

    print("Step 3: Generating enriched @merge entries...")
    count = write_enriched_delta(toml_params, call_sites)
    print(f"  Wrote {count} enriched entries to {DELTA_PATH}")

    # Summary of coverage
    with_desc = 0
    for key, slots in toml_params.items():
        args_list = call_sites.get(key, [])
        if args_list:
            best = None
            for a in args_list:
                if len(a) == len(slots):
                    best = a
                    break
            if best is None and args_list:
                best = args_list[0]
            if best:
                for i, (_, ts) in enumerate(slots):
                    if i < len(best):
                        for pat, _, _ in ENRICHMENT_KB:
                            if re.search(pat, best[i].strip()):
                                with_desc += 1
                                break

    print(f"\n  KB-matched params: {with_desc} (have specific desc+default)")
    print("Done!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
