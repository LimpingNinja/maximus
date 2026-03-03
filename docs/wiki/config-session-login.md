---
layout: default
title: "Session & Login"
section: "configuration"
description: "session.toml — user experience, policy, and session behavior"
---

This page covers `config/general/session.toml` — the file that controls the
day-to-day "personality" of your BBS. How callers log on, what the system
considers a valid name, how graphics are negotiated, where new users start,
how long Maximus waits before timing out idle sessions — it's all here.

Think of `session.toml` as the policy layer. It doesn't create content (areas,
menus, file listings), but it decides how Maximus behaves while a caller is
online. If you're migrating from CTL and wondering "why does it keep asking
for a last name?" or "why are ANSI screens disabled?" — the answer is probably
in this file.

---

## Quick Reference

### Logon & Identity

These settings control how callers identify themselves during login and new
user registration.

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `alias_system` | bool | `true` | Prefer aliases (handles) for display and message authoring |
| `ask_alias` | bool | `true` | Prompt new users for an alias |
| `single_word_names` | bool | `true` | Allow single-word usernames (suppresses "What is your LAST name?") |
| `no_real_name` | bool | `false` | Skip real name requirement during registration |
| `ask_phone` | bool | `true` | Prompt new users for a phone number |

### Terminal & Graphics

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `check_ansi` | bool | `true` | Verify ANSI capability at logon |
| `check_rip` | bool | `true` | Verify RIP capability at logon |
| `min_graphics_baud` | int | `1200` | Minimum speed for ANSI/AVATAR graphics |
| `min_rip_baud` | int | `65535` | Minimum speed for RIP graphics (set to `65535` to disable RIP) |
| `charset` | string | `""` | Character set mode: `""` (default), `"swedish"`, or `"chinese"` |
| `global_high_bit` | bool | `false` | Allow high-bit (8-bit) characters in names and descriptions |

### Starting Points

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `first_menu` | string | `"MAIN"` | Menu displayed after logon |
| `first_message_area` | string | `"Local Areas.1"` | Default starting message area for new users |
| `first_file_area` | string | `"1"` | Default starting file area for new users |
| `logon_priv` | int | `20` | Default privilege level for new users |
| `logon_timelimit` | int | `15` | Maximum minutes allowed to complete login |
| `min_logon_baud` | int | `0` | Minimum connection speed to log on (0 = no minimum) |

### Timeouts & Session Limits

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `input_timeout` | int | `15` | Minutes of idle before timeout warning (then 1 more minute before disconnect) |
| `exit_after_call` | int | `5` | Process exit code returned after a caller disconnects |
| `compat_local_baud_9600` | bool | `false` | Treat local console as a 9600-baud connection for feature gating |

### Editors & Chat

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `edit_menu` | string | `""` | Menu name for editor functions (defaults to `"EDIT"` if blank) |
| `local_editor` | string | `"/usr/bin/pico \"%s\""` | External editor command — `%s` is replaced with the temp file path |
| `disable_magnet` | bool | `false` | Disable the full-screen BORED/MAGNET editor |
| `chat_program` | string | `""` | External chat program (prefix with `:` for a MEX script) |
| `chat_capture` | bool | `false` | Auto-log sysop/caller chat sessions |
| `yell_enabled` | bool | `true` | Allow the Y)ell command to make noise on the console |
| `disable_userlist` | bool | `false` | Disable the `?` user list at the "To:" prompt in private messages |

### File Transfers & Listings

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `strict_xfer` | bool | `false` | Terminate file transfers if the caller runs out of time |
| `min_free_kb` | uint | `200` | Minimum free disk space (KB) required to accept an upload |
| `upload_log` | string | `"log/uploads.log"` | Dedicated upload log file |
| `virus_check` | string | `""` | External virus checker command (invoked per uploaded file) |
| `upload_check_dupe` | bool | `true` | Reject duplicate file uploads |
| `upload_check_dupe_extension` | bool | `false` | Include file extension in duplicate checks |
| `autodate` | bool | `true` | Read file dates from the filesystem (vs. manual FILES.BBS entries) |
| `date_style` | int | `0` | Date format: 0 = mm-dd-yy, 1 = dd-mm-yy, 2 = yy-mm-dd, 3 = yymmdd |
| `filelist_margin` | int | `0` | Indentation for wrapped file descriptions in listings |
| `save_directories` | array | `[]` | Drive letters to save/restore when launching external programs (legacy DOS) |

### Area Navigation

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `area_change_keys` | string | `"[]?"` | Key bindings for area navigation: char 1 = prev, char 2 = next, char 3 = list |
| `highest_message_area` | string | `"ZZZZZZZZZZ"` | Upper bound for message area scanning/navigation |
| `highest_file_area` | string | `"ZZZZZZZZZZ"` | Upper bound for file area scanning/navigation |

### Mail & Attachments

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `comment_area` | string | `"1"` | Message area where log-off comments are stored |
| `kill_private` | string | `"Never"` | What to do after reading private mail: `Never`, `Ask`, or `Always` |
| `max_msgsize` | uint | `8192` | Maximum uploaded message size (bytes) |
| `use_umsgids` | bool | `false` | Use unique message IDs (Squish bases only — never reuses numbers) |
| `gate_netmail` | bool | `false` | Gate-route interzone NetMail through the FidoNet zone gate |
| `mailchecker_reply_priv` | int | `0` | Privilege required for mailchecker reply actions |
| `mailchecker_kill_priv` | int | `0` | Privilege required for mailchecker kill actions |
| `attach_base` | string | `"data/mail/attach"` | File attach database (root name) |
| `attach_path` | string | `"data/mail/attach"` | Holding directory for local file attaches |
| `attach_archiver` | string | `"ZIP"` | Archiver type for attach storage |
| `msg_localattach_priv` | int | `30` | Privilege required to create a local file attach |
| `kill_attach` | string | `"ask"` | How to handle received attaches: `never`, `ask`, or `always` |
| `kill_attach_priv` | int | `30` | Privilege threshold for the `ask` attach behavior |

### Message Tracking (MTS)

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `track_base` | string | `""` | Tracking database path (root name — leave empty to disable MTS) |
| `track_privview` | string | `""` | Privilege to view other users' tracking info |
| `track_privmod` | string | `""` | Privilege to modify/delete other users' tracking entries |
| `track_exclude` | string | `""` | Path to a text file listing users excluded from auto-tracking |

---

## Deep Dives

### Alias System

If you want a handle-based BBS where callers appear by their alias in "Who's
Online", message headers, and most displays:

```toml
alias_system = true
ask_alias = true
single_word_names = true
```

If you prefer real names but still want to collect aliases for specific areas
or doors, enable `ask_alias` alone and leave `alias_system` off. Callers can
always log in using either their real name or alias regardless of these
toggles.

### Terminal Detection

`check_ansi` and `check_rip` control whether Maximus probes for terminal
capabilities at logon. If auto-detection can't confirm support, the caller
is prompted. The `min_*_baud` settings gate features by connection speed —
set `min_rip_baud` to `65535` to effectively disable RIP for everyone.

The `charset` key activates special character set handling. Leave it empty
for standard operation. Setting it to `"swedish"` enables Swedish 7-bit
character mapping; `"chinese"` enables BIG5 support. Both interact with
`global_high_bit` in some configurations.

### Starting Points

`first_menu`, `first_message_area`, and `first_file_area` determine where new
users land after completing registration. Existing users remember their last
area. If you change `first_message_area`, make sure the area tag you specify
actually exists — a typo here means new users start in a nonexistent area.

`logon_priv` sets the default privilege level for new accounts. If you use
access levels to gate content, this is the "front door" level. The
[Access Levels]({% link config-access-levels.md %}) page covers the privilege
system in detail.

### Input Timeout

Maximus handles idle timeout in two stages:

1. After `input_timeout` minutes with no input, a warning is sent.
2. After one more minute of silence, the session is terminated.

Valid range is 1 to 127 minutes. Remote callers always have this timeout
active. Local console sessions only time out if `local_input_timeout` is
enabled in [Core Settings]({% link config-core-settings.md %}).

### External Editor

`local_editor` specifies a command to launch instead of the built-in MaxEd
editor. Use `%s` as a placeholder for the temporary message file path:

```toml
local_editor = "/usr/bin/nano \"%s\""
```

If the caller is replying to a message, Maximus pre-populates the temp file
with quoted text before invoking the editor. After the editor exits, the
contents of the temp file become the message body.

Prefix the command with `@` to allow qualifying remote users to use the
"local" editor (controlled via mail flags in the access level configuration).

### File Transfer Settings

- **`strict_xfer`** — when enabled, Maximus kills a file transfer mid-stream
  if the caller runs out of time. Mostly relevant for internal protocols.
- **`upload_check_dupe`** — checks for duplicate filenames when a user
  uploads. Depends on up-to-date file area metadata.
- **`autodate`** — when `true` (recommended), file sizes and dates come from
  the filesystem. When `false`, they're read from FILES.BBS entries — a
  legacy optimization for slow media like CD-ROMs.

### Message Tracking (MTS)

The Message Tracking System lets callers track the lifecycle of messages
(think lightweight ticket tracking). To enable it, set `track_base` to a
path root:

```toml
track_base = "data/tracking/mts"
```

`track_privview` and `track_privmod` control who can see and modify other
users' tracking entries. If you're not using MTS, leave `track_base` empty
and these settings are ignored.

---

## See Also

- [Core Settings]({% link config-core-settings.md %}) — system identity,
  paths, and logging (`maximus.toml`)
- [Equipment & Comm]({% link config-equipment-comm.md %}) — modem and
  connection settings (`equipment.toml`)
- [Access Levels]({% link config-access-levels.md %}) — privilege level
  configuration
- [MaxCFG Setup & Tools]({% link maxcfg-setup-tools.md %}) — editing these
  settings through the TUI
