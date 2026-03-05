---
layout: default
title: "Access Levels"
section: "configuration"
description: "access_levels.toml — defining privilege classes, time limits, ratios, and per-class behavior"
---

This page is the field-by-field reference for
`config/security/access_levels.toml` — the file that defines your privilege
classes. If you're looking for the big-picture overview of how levels, flags,
and access strings work together, start with
[Security & Access]({{ site.baseurl }}{% link config-security-access.md %}).

Each entry in this file is an `[[access_level]]` table. You can have as many
or as few classes as your board needs. Maximus loads them at startup and sorts
them by level number — the numeric level is what matters, not the order they
appear in the file.

---

## Quick Reference

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `name` | string | *(required)* | Symbolic name of the class (e.g., `"Normal"`, `"Sysop"`) |
| `level` | int | *(required)* | Numeric privilege level (0–65535, must be unique) |
| `description` | string | `""` | User-visible description shown in the user editor |
| `alias` | string | `""` | Alternate label for legacy compatibility |
| `key` | string | `""` | Single-character key used by legacy MECCA tokens |
| `time` | int | `60` | Per-session time limit in minutes |
| `cume` | int | `90` | Per-day cumulative time limit in minutes |
| `calls` | int | `-1` | Maximum logons per day (`-1` = unlimited) |
| `logon_baud` | int | `300` | Minimum connection speed to log on |
| `xfer_baud` | int | `300` | Minimum connection speed for file transfers |
| `file_limit` | int | `0` | Maximum kilobytes downloadable per day |
| `file_ratio` | int | `0` | Download-to-upload ratio (0 = no ratio enforcement) |
| `ratio_free` | int | `0` | KB the user can download before ratio kicks in |
| `upload_reward` | int | `0` | Percent time credit returned for uploads |
| `login_file` | string | `""` | Display file shown immediately after logon for this class |
| `flags` | array | `[]` | General behavior flags (see [Privileges & Flags]({{ site.baseurl }}{% link config-privileges-flags.md %})) |
| `mail_flags` | array | `[]` | Mail/editor flags (see [Privileges & Flags]({{ site.baseurl }}{% link config-privileges-flags.md %})) |
| `user_flags` | int | `0` | Sysop-defined bitfield for MEX scripts |
| `oldpriv` | int | `0` | Legacy Maximus 2.x compatibility field |

---

## Deep Dives

### Name and Level

`name` is the human-readable label for the class — it's what you see in the
user editor, in "Who's Online" displays, and in log entries. `level` is the
number that actually matters for access checks. Two classes cannot share the
same level number.

Convention: keep levels spaced in increments of 10 so you have room to insert
new classes later without renumbering everything.

### Time Limits

- **`time`** — the hard per-session ceiling. Once this many minutes have
  elapsed since logon, Maximus warns the caller and then disconnects.
- **`cume`** — the cumulative daily ceiling. If a caller logs on three times
  in one day, total time across all sessions is tracked against this value.

Set both to `1440` (24 hours) for classes that shouldn't have time limits.
Or use the `NoTimeLimit` flag in the `flags` array to bypass enforcement
entirely.

### Download Limits and Ratios

- **`file_limit`** — maximum kilobytes the user can download per day.
  Set to `0` to block downloads entirely, or use `NoFileLimit` in `flags`.
- **`file_ratio`** — enforced ratio of downloads to uploads. A value of `5`
  means the user must upload 1 KB for every 5 KB downloaded.
- **`ratio_free`** — a grace period in KB. The user can download this much
  before ratio enforcement begins. Useful for giving new users a head start.
- **`upload_reward`** — percent of upload time credited back to the session.
  A value of `100` means upload time doesn't count against the session clock.

### Login File

`login_file` specifies a display file (relative to the display path) shown
immediately after the caller completes logon. This lets you show different
welcome screens to different classes — a "welcome, new user" screen for
Limited, a sysop bulletin for AsstSysop, etc.

Leave it empty to skip the per-class display.

### Flags

The `flags` and `mail_flags` arrays are where the real fine-grained control
lives. These are documented in detail on the
[Privileges & Flags]({{ site.baseurl }}{% link config-privileges-flags.md %}) page. Here's a
quick example of a co-sysop class:

```toml
[[access_level]]
name = "AsstSysop"
level = 90
description = "AsstSysop"
key = "A"
time = 120
cume = 180
calls = -1
file_limit = 5000
flags = ["ShowHidden", "ShowAllFiles", "DloadHidden"]
mail_flags = ["LocalEditor", "NetFree", "WriteRdOnly"]
```

### User Flags

`user_flags` is a 32-bit integer bitfield that Maximus itself doesn't
interpret — it's reserved for your MEX scripts. If you write a MEX door or
automation that needs custom per-class behavior, you can test individual bits
of this field in your script logic.

### Legacy Fields

- **`alias`** — an alternate name for the class, carried over from legacy
  Maximus. Rarely used in modern configs.
- **`key`** — a single character used by MECCA `[iflevel]` tokens. Only
  relevant if you have MECCA display files that check access levels.
- **`oldpriv`** — maps to the numeric privilege values from Maximus 2.x.
  Used internally during CTL-to-TOML migration. Don't change this unless
  you're debugging a legacy conversion.

---

## Adding a New Class

To add a new access level, just add another `[[access_level]]` block with a
unique `level` number:

```toml
[[access_level]]
name = "Trusted"
level = 35
description = "Trusted — validated and active"
key = ""
time = 90
cume = 120
calls = -1
logon_baud = 300
xfer_baud = 300
file_limit = 8000
file_ratio = 0
ratio_free = 2000
upload_reward = 100
login_file = ""
flags = []
mail_flags = []
user_flags = 0
oldpriv = 0
```

Restart Maximus (or the affected node) for the new class to take effect. Then
use the [User Editor]({{ site.baseurl }}{% link maxcfg-user-editor.md %}) to promote users to
the new level.

---

## See Also

- [Security & Access]({{ site.baseurl }}{% link config-security-access.md %}) — big-picture
  overview of the access control system
- [Privileges & Flags]({{ site.baseurl }}{% link config-privileges-flags.md %}) — complete
  flag reference
- [Session & Login]({{ site.baseurl }}{% link config-session-login.md %}) — `logon_priv`
  (default level for new users)
- [MaxCFG User Editor]({{ site.baseurl }}{% link maxcfg-user-editor.md %}) — changing a
  user's privilege level
