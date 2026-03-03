---
layout: default
title: "Info Codes"
section: "display"
description: "Information codes for dynamic data substitution"
---

Information codes are two **uppercase letters** (or special characters) that
are replaced with live data about the BBS, the current user, or the system.

---

## User Information

| Code   | Description                    |
|--------|--------------------------------|
| `\|UN`  | User name                      |
| `\|UH`  | User handle (alias)            |
| `\|UR`  | User real name                 |
| `\|UC`  | User city / state              |
| `\|UP`  | User home phone                |
| `\|UD`  | User data phone                |
| `\|U#`  | User number (account ID)       |
| `\|US`  | User screen length (lines)     |
| `\|TE`  | Terminal emulation (TTY, ANSI, AVATAR) |

---

## Call & Activity Statistics

| Code   | Description                    |
|--------|--------------------------------|
| `\|CS`  | Total calls (lifetime)         |
| `\|CT`  | Calls today                    |
| `\|MP`  | Total messages posted          |
| `\|DK`  | Total download KB              |
| `\|FK`  | Total upload KB                |
| `\|DL`  | Total downloaded files         |
| `\|FU`  | Total uploaded files           |
| `\|DT`  | Downloaded KB today            |
| `\|TL`  | Time left (minutes)            |

---

## System Information

| Code   | Description                    |
|--------|--------------------------------|
| `\|BN`  | BBS / system name              |
| `\|SN`  | Sysop name                     |

---

## Area Information

| Code   | Description                    |
|--------|--------------------------------|
| `\|MB`  | Current message area name      |
| `\|MD`  | Current message area description |
| `\|FB`  | Current file area name         |
| `\|FD`  | Current file area description  |

---

## Date & Time

| Code   | Description                    |
|--------|--------------------------------|
| `\|DA`  | Current date (DD Mon YY)       |
| `\|TM`  | Current time (HH:MM)          |
| `\|TS`  | Current time (HH:MM:SS)       |

---

## Example

```
|14Welcome to |15|BN|14, |15|UN|14!
You have |15|TL|14 minutes remaining.
Current message area: |11|MB
```

Produces something like:

```
Welcome to Maximus BBS, Kevin!
You have 60 minutes remaining.
Current message area: General
```

Info codes can be combined with
[format operators]({% link mci-format-operators.md %}) for aligned output:

```
$R30|UN          translates to: "Kevin Morgan                  "
```

---

## See Also

- [Display Codes]({% link display-codes.md %}) — overview and quick reference
- [Color Codes]({% link mci-color-codes.md %}) — numeric and theme color codes
- [Format Operators]({% link mci-format-operators.md %}) — padding, alignment, repetition
- [Terminal Control]({% link mci-terminal-control.md %}) — screen and cursor codes
