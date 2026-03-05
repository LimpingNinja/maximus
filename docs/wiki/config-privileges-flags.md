---
layout: default
title: "Privileges & Flags"
section: "configuration"
description: "Complete reference for general flags and mail flags in access_levels.toml"
---

This page is the complete reference for the `flags` and `mail_flags` arrays
in `config/security/access_levels.toml`. These string flags control
fine-grained behavior that privilege levels alone can't express — things like
"can this user see hidden files?" or "can they use the external editor?"

If you're looking for the big picture of how access control works, start with
[Security & Access]({{ site.baseurl }}{% link config-security-access.md %}). For the full
per-field reference of access level classes, see
[Access Levels]({{ site.baseurl }}{% link config-access-levels.md %}).

---

## General Flags (`flags`)

These flags control session behavior, file visibility, and enforcement
overrides. They go in the `flags` array of an `[[access_level]]` entry.

| Flag | What It Does |
|------|-------------|
| `ShowHidden` | User can see hidden users in the "Who's Online" display and user list |
| `ShowAllFiles` | User can see hidden file areas and files marked as hidden in file listings |
| `DloadHidden` | User can download files from hidden file areas (implies visibility) |
| `UploadAny` | User can upload to any area, even areas that don't normally accept uploads |
| `Hide` | User is hidden from "Who's Online" and the user list |
| `HangUp` | Auto-disconnect immediately after logon — used for the `Hidden` (banned) class |
| `NoFileLimit` | Bypass the `file_limit` download cap for this class |
| `NoTimeLimit` | Bypass the `time` and `cume` session time limits |
| `NoLimits` | Shorthand for both `NoFileLimit` and `NoTimeLimit` combined |

### Usage Notes

- **`ShowHidden`** and **`ShowAllFiles`** are separate on purpose. A co-sysop
  might need to see hidden users (to monitor the board) without necessarily
  needing access to restricted file areas, or vice versa.
- **`Hide`** is the opposite of `ShowHidden` — it makes *this* user invisible.
  Typically used for the sysop's test account or bot accounts.
- **`HangUp`** is a blunt instrument. It's used by the default `Hidden` class
  (level 65535) to instantly disconnect banned users. If you set a user's
  privilege to level 65535, they'll be booted as soon as Maximus looks up
  their class.
- **`NoLimits`** is just syntactic sugar — it sets both `NoFileLimit` and
  `NoTimeLimit` in one flag. Use it for sysop-level classes where limits
  would just get in the way.

### Example

A staff class that can see and download hidden files, with no download cap:

```toml
flags = ["ShowAllFiles", "DloadHidden", "NoFileLimit"]
```

---

## Mail Flags (`mail_flags`)

These flags control editor access, message attribute permissions, and mail
handling behavior. They go in the `mail_flags` array.

| Flag | What It Does |
|------|-------------|
| `ShowPvt` | User can read messages marked as Private in areas where private messages exist |
| `Editor` | User can use the full-screen BORED/MAGNET message editor |
| `LocalEditor` | User can use the external editor defined by `local_editor` in [Session & Login]({{ site.baseurl }}{% link config-session-login.md %}) |
| `NetFree` | NetMail doesn't count against the user's message quotas |
| `MsgAttrAny` | User can set any message attribute (Private, Crash, Hold, etc.) regardless of area restrictions |
| `WriteRdOnly` | User can post messages in read-only areas |
| `NoRealName` | User is not required to use their real name when posting — alias is always accepted |

### Usage Notes

- **`Editor`** vs **`LocalEditor`** — `Editor` enables the built-in
  full-screen editor (BORED/MAGNET). `LocalEditor` enables the external
  editor configured in `session.toml`. A class can have both, one, or
  neither. Without either flag, the user falls back to the line-by-line
  editor.
- **`MsgAttrAny`** is powerful — it lets the user set attributes like
  `Crash` (immediate delivery) and `Hold` (don't send until polled) on
  NetMail. Only give this to trusted users or it can cause routing chaos.
- **`WriteRdOnly`** is useful for moderation. You might mark an announcements
  area as read-only for most users but give staff classes this flag so they
  can still post.
- **`NoRealName`** is mainly relevant when `alias_system` is disabled in
  `session.toml`. In alias-first boards, everyone posts under their alias
  already. This flag matters in real-name boards where you want certain
  classes (like sysops) to be able to post under an alias anyway.

### Example

A co-sysop class with full editor access and NetMail privileges:

```toml
mail_flags = ["LocalEditor", "Editor", "NetFree", "WriteRdOnly", "MsgAttrAny"]
```

---

## Combining Flags with Levels

Flags and levels are independent. You can create two classes at the same
level with different flags — for example, level 80 "Clerk" with no special
flags and level 80 "FileClerk" with `ShowAllFiles` and `DloadHidden`. In
practice, most boards just use different level numbers, but the flexibility is
there if you need it.

The typical progression looks like this:

| Class | General Flags | Mail Flags |
|-------|--------------|------------|
| Normal (30) | *(none)* | *(none)* |
| Privil (50) | *(none)* | `Editor` |
| Clerk (80) | *(none)* | `Editor`, `LocalEditor` |
| AsstSysop (90) | `ShowHidden`, `ShowAllFiles`, `DloadHidden` | `LocalEditor`, `NetFree`, `WriteRdOnly` |
| Sysop (100) | `ShowHidden`, `ShowAllFiles`, `DloadHidden`, `UploadAny`, `NoFileLimit` | `LocalEditor`, `NetFree`, `WriteRdOnly`, `MsgAttrAny`, `NoRealName` |

---

## See Also

- [Security & Access]({{ site.baseurl }}{% link config-security-access.md %}) — how the access
  system works
- [Access Levels]({{ site.baseurl }}{% link config-access-levels.md %}) — per-field reference
  for `access_levels.toml`
- [Session & Login]({{ site.baseurl }}{% link config-session-login.md %}) — `local_editor`,
  `alias_system`, and other policy settings
