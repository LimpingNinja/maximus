---
layout: default
title: "Dropfile Formats"
section: "configuration"
description: "The four dropfile formats Maximus generates automatically for door programs"
---

When Maximus launches a door program, it automatically writes **dropfiles** —
small text files that tell the door who the caller is, how much time they have,
what node they're on, and how to talk to the terminal. This happens every time
a door runs; you don't need to configure anything.

Maximus generates all four formats simultaneously into the node's temp
directory. The door reads whichever format it expects.

---

## Dropfile Location

All dropfiles are written to the per-node temp directory:

```
<sys_path>/<temp_path>/node<XX>/
```

Where `<XX>` is the two-digit hex node number (e.g., `node01`, `node02`).
The directory is created automatically if it doesn't exist, and cleaned up
after the door exits.

For example, on a default installation running node 1:

```
/opt/maximus/run/tmp/node01/Dorinfo1.Def
/opt/maximus/run/tmp/node01/Door.Sys
/opt/maximus/run/tmp/node01/Chain.Txt
/opt/maximus/run/tmp/node01/door32.sys
```

---

## Supported Formats

### DORINFO1.DEF

The oldest and simplest format. Many classic doors from the late 1980s and
early 1990s expect this file.

| Line | Content | Example |
|------|---------|---------|
| 1 | BBS system name | `MaximusNG BBS` |
| 2 | Sysop first name | `Kevin` |
| 3 | Sysop last name | `Morgan` |
| 4 | COM port (`COM0` = local) | `COM0` |
| 5 | Baud rate and params | `38400 BAUD,N,8,1` |
| 6 | Network flag (always `0`) | ` 0` |
| 7 | Caller first name (uppercase) | `JOHN` |
| 8 | Caller last name (uppercase) | `DOE` |
| 9 | Caller city | `Portland, OR` |
| 10 | Graphics mode (0=TTY, 1=ANSI) | `1` |
| 11 | Privilege level | `30` |
| 12 | Minutes remaining | `45` |
| 13 | FOSSIL flag (always `-1`) | `-1` |

**Caveats:**
- User names are split on spaces. Single-word aliases get `NLN` as the last
  name — this is standard DORINFO behavior.
- Names are uppercased per the format spec.

### DOOR.SYS

The most widely supported format. If a door says "supports DOOR.SYS," this is
what it expects.

| Line | Content | Example |
|------|---------|---------|
| 1 | COM port (with colon) | `COM0:` |
| 2 | Baud rate | `38400` |
| 3 | Data bits | `8` |
| 4 | Node number | `1` |
| 5 | DTE locked (`N`) | `N` |
| 6 | Screen display | `Y` |
| 7 | Printer toggle | `Y` |
| 8 | Page bell | `Y` |
| 9 | Caller alarm | `Y` |
| 10 | User name | `John Doe` |
| 11 | City | `Portland, OR` |
| 12 | Home phone | `503-555-1234` |
| 13 | Work phone | `503-555-1234` |
| 14 | Password | *(user's password)* |
| 15 | Privilege level | `30` |
| 16 | Times called | `42` |
| 17 | Last date called | `01/01/90` |
| 18 | Seconds remaining | `2700` |
| 19 | Minutes remaining | `45` |
| 20 | Graphics (`GR`=ANSI, `NG`=none) | `GR` |
| 21 | Screen length | `24` |
| 22 | Expert mode | `N` |
| 23 | Conferences registered | `1,2,3,4,5,6,7` |
| 24 | Conference last in | `1` |
| 25 | Expiration date | `01/01/99` |
| 26 | User record number | `1` |
| 27 | Default protocol | `X` |
| 28–31 | *(padding fields)* | `0` / `9999` |

**Caveats:**
- Line 14 contains the user's password. If `no_password_encryption` is `false`
  (the default), this is the hashed password — which may confuse doors that
  expect plaintext. Most modern doors ignore this field.
- Date fields use placeholder values (`01/01/90`, `01/01/99`) since Maximus
  tracks session data differently than PCBoard-style systems.

### CHAIN.TXT

The WWIV-style dropfile. Used by doors written for WWIV BBS and some
multi-platform doors that support CHAIN.TXT as an alternative.

| Line | Content | Example |
|------|---------|---------|
| 1 | User record number | `1` |
| 2 | First name | `John` |
| 3 | Last name (or `NLN`) | `Doe` |
| 4 | Callsign/handle | *(empty)* |
| 5 | Privilege level | `30` |
| 6 | Minutes remaining | `45` |
| 7 | Graphics (0=TTY, 1=ANSI) | `1` |
| 8 | Node number | `1` |
| 9 | COM port (0=local) | `0` |
| 10 | Baud rate | `38400` |
| 11 | System name | `MaximusNG BBS` |
| 12 | Sysop first name | `Kevin` |
| 13 | Sysop last name | `Morgan` |
| 14 | Sysop callsign | *(empty)* |
| 15 | Event time | `00:00` |
| 16 | Error correcting | `N` |
| 17 | ANSI in NG mode | `N` |
| 18 | Record locking | `Y` |
| 19 | Default color | `7` |
| 20 | Times called | `42` |
| 21 | Last date | `01/01/90` |
| 22 | Seconds remaining | `2700` |
| 23 | Daily file limit | `9999` |
| 24 | Files downloaded today | `0` |
| 25 | KB uploaded | `1024` |
| 26 | KB downloaded | `512` |
| 27 | User comment | `8` |
| 28 | Doors opened | `0` |
| 29 | Messages left | `0` |

### DOOR32.SYS

The modern telnet-aware format. This is what you should use for any door that
supports it — see [Door32 Support]({% link door32-support.md %}) for the full
story.

| Line | Content | Example |
|------|---------|---------|
| 1 | Comm type (0=local, 2=telnet) | `2` |
| 2 | Socket/comm handle (fd number) | `5` |
| 3 | Baud rate | `38400` |
| 4 | System name | `MaximusNG BBS` |
| 5 | User record number | `1` |
| 6 | User real name | `John Doe` |
| 7 | User alias/handle | `John Doe` |
| 8 | Privilege level | `30` |
| 9 | Minutes remaining | `45` |
| 10 | Emulation (`ANSI` or `ASCII`) | `ANSI` |
| 11 | Node number | `1` |

**Key detail:** Line 2 contains the actual file descriptor number of the
caller's TCP socket. Door32 doors use this fd directly for I/O instead of
reading/writing through a PTY. This is what makes Door32 the cleanest
approach for telnet-era doors. See
[Door32 Support]({% link door32-support.md %}) for how this works under
the hood.

---

## Which Format Should I Use?

- **New doors or doors you control:** Use `door32.sys` — it's the only format
  designed for socket-based connections.
- **Classic doors with DOOR.SYS support:** Use `Door.Sys` — it's the most
  widely supported legacy format.
- **WWIV doors:** Use `Chain.Txt`.
- **Very old doors:** Use `Dorinfo1.Def` as a fallback.

You don't need to choose — Maximus writes all four every time. The door reads
whichever one it's configured to look for.

---

## See Also

- [Door Games]({% link config-door-games.md %}) — overview of running doors
  on Maximus
- [Door32 Support]({% link door32-support.md %}) — native Door32 protocol
  and socket-passing
- [Core Settings]({% link config-core-settings.md %}) — `temp_path` and
  `node_path` configuration
