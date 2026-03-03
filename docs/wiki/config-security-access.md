---
layout: default
title: "Security & Access"
section: "configuration"
description: "How Maximus controls who can do what — privilege levels, flags, and access strings"
---

Every BBS has to answer the same question: *who gets to do what?* Maximus
handles this with a layered system that's simple once you see how the pieces
fit together, but flexible enough to handle everything from a casual single-node
board to a multi-zone FidoNet hub.

This page gives you the big picture. The child pages go deeper into each piece.

---

## The Three Layers

Maximus access control has three layers that work together:

### 1. Privilege Levels

Every user has a numeric **privilege level** (0–65535). Every access level
class (Transient, Normal, Sysop, etc.) maps to a specific number. When the
user logs on, Maximus looks up their level and loads the matching class from
`config/security/access_levels.toml`.

The level determines:

- **Time limits** — per-session and per-day cumulative
- **Download limits** — kilobytes per day, upload/download ratios
- **Baud gates** — minimum connection speed to log on or transfer files
- **Login file** — an optional display file shown at logon for that class

See [Access Levels]({% link config-access-levels.md %}) for the full
reference.

### 2. Flags

Each access level class carries two sets of string flags:

- **General flags** — control visibility, file access, and session behavior
  (e.g., `ShowHidden`, `NoFileLimit`, `HangUp`)
- **Mail flags** — control editor access, message attributes, and mail
  privileges (e.g., `LocalEditor`, `NetFree`, `MsgAttrAny`)

Flags are the fine-grained knobs. Two classes can have the same privilege level
but different flags — one might see hidden files while the other can't.

See [Privileges & Flags]({% link config-privileges-flags.md %}) for a
complete flag reference.

### 3. Access Control Strings (ACS)

Throughout Maximus — in menu options, area definitions, and many config
settings — you'll see fields like `priv_level`, `acs`, or `lock`. These are
**access control strings** that gate a feature to users at or above a given
privilege level.

The simplest ACS is just a number:

```toml
# Only users at privilege level 50+ can see this menu option
priv_level = 50
```

In area configurations, you can combine level requirements with key locks for
even more control. The access system is intentionally straightforward — most
boards only need a handful of levels.

---

## How It All Fits Together

Here's the flow when a caller logs on:

1. Maximus looks up the user's stored privilege level.
2. It finds the matching (or nearest lower) class in `access_levels.toml`.
3. The class determines time/download limits and loads the flag sets.
4. As the caller navigates, each menu option and area checks the caller's
   level against its ACS requirement.
5. Flags provide additional gates — e.g., a file area might be visible to
   level 50+ users, but only users with `ShowHidden` can see hidden files
   within it.

New users start at the level defined by `logon_priv` in
[Session & Login]({% link config-session-login.md %}). You promote users by
changing their privilege level in the
[User Editor]({% link maxcfg-user-editor.md %}).

---

## The Default Hierarchy

Maximus ships with 12 access level classes. You can modify these freely — add
new ones, remove unused ones, or change the limits. The only hard rule is that
level numbers must be unique and the `Sysop` class should be the highest
non-hidden level.

| Level | Name | Time/Day | DL Limit | Notes |
|-------|------|----------|----------|-------|
| 0 | Transient | 15 min | 0 KB | Minimal access |
| 10 | Demoted | 45 min | 500 KB | Restricted |
| 20 | Limited | 60 min | 1000 KB | New user default |
| 30 | Normal | 60 min | 5000 KB | Validated user |
| 40 | Worthy | 60 min | 5000 KB | Trusted |
| 50 | Privil | 60 min | 5000 KB | Privileged |
| 60 | Favored | 60 min | 5000 KB | Favored |
| 70 | Extra | 60 min | 5000 KB | Extra |
| 80 | Clerk | 90 min | 5000 KB | Staff |
| 90 | AsstSysop | 120 min | 5000 KB | Co-sysop (has `ShowHidden`, `LocalEditor`) |
| 100 | Sysop | 1440 min | 30000 KB | Full access, all flags |
| 65535 | Hidden | 0 min | 0 KB | Auto-hangup; used for banned users |

---

## Configuration File

**File:** `config/security/access_levels.toml`

**Legacy source:** `etc/access.ctl`

This single TOML file defines all your access level classes. Each class is an
`[[access_level]]` table entry. Maximus reads it at startup and builds the
class table in memory. Changes take effect on the next restart (or next user
logon if you're running MaxTel with hot-reload).

---

## See Also

- [Access Levels]({% link config-access-levels.md %}) — per-field reference
  for `access_levels.toml`
- [Privileges & Flags]({% link config-privileges-flags.md %}) — complete
  flag reference with behavior descriptions
- [Session & Login]({% link config-session-login.md %}) — `logon_priv` and
  other policy settings
- [MaxCFG User Editor]({% link maxcfg-user-editor.md %}) — promoting and
  managing users
