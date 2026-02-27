# QWK Message Networking — Implementation Specification

**MaximusNG — Soup-to-Nuts QWK Networking**
**Author:** Kevin Morgan (Limping Ninja)
**Date:** 2025-02-23
**Status:** Draft Specification

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Background & Existing Infrastructure](#2-background--existing-infrastructure)
3. [QWK Format Primer](#3-qwk-format-primer)
4. [Architecture Overview](#4-architecture-overview)
5. [TOML Configuration Schema](#5-toml-configuration-schema)
6. [Component 1: Enhanced QWK Packet Writer (Download)](#6-component-1-enhanced-qwk-packet-writer-download)
7. [Component 2: Enhanced QWK Packet Reader (Upload/Toss)](#7-component-2-enhanced-qwk-packet-reader-uploadtoss)
8. [Component 3: QWK Network Node Account System](#8-component-3-qwk-network-node-account-system)
9. [Component 4: QWK Network Hub/Spoke Engine](#9-component-4-qwk-network-hubspoke-engine)
10. [Component 5: NetMail Routing](#10-component-5-netmail-routing)
11. [Component 6: @VIA Path Tracking & Dupe Detection](#11-component-6-via-path-tracking--dupe-detection)
12. [Component 7: QWKE / HEADERS.DAT Extensions](#12-component-7-qwke--headersdat-extensions)
13. [Component 8: Automated Poll/Call-Out Scheduler](#13-component-8-automated-pollcall-out-scheduler)
14. [Component 9: maxcfg Integration](#14-component-9-maxcfg-integration)
15. [Implementation Phases](#15-implementation-phases)
16. [File Inventory](#16-file-inventory)
17. [Testing Strategy](#17-testing-strategy)
18. [Compatibility Matrix](#18-compatibility-matrix)
19. [References](#19-references)

---

## 1. Executive Summary

This document specifies the complete implementation of QWK message networking
for MaximusNG. The system builds on the **existing** QWK offline reader
infrastructure (`mb_qwk.c` / `mb_qwkup.c`) and extends it to support
full inter-BBS message networking using the QWK/REP packet exchange model,
compatible with Synchronet QWKnet extensions and the QWKE standard.

### Goals

- **User QWK offline mail** — already functional, enhanced with QWKE and
  Synchronet HEADERS.DAT support for richer metadata
- **QWK networking** — hub/spoke topology where networked nodes exchange
  echomail and routed netmail via QWK/REP packets
- **Compatibility** — interoperable with Synchronet, Mystic, Enigma½,
  and any system speaking the Synchronet QWKnet extensions
- **TOML-native configuration** — all QWK network config lives in the
  next-gen TOML config tree, no legacy CTL/PRM paths
- **Automation** — scheduled poll/export cycle for unattended networking

### Non-Goals

- Replacing FidoNet/Squish echomail tosser — QWKnet is a parallel network
  technology, not a replacement for `squish` toss/scan
- Real-time streaming — QWK is batch-oriented by design
- QWK-over-IP as a transport — we use HTTP(S) or direct dial/telnet to
  exchange packets; the *packet format* is QWK

---

## 2. Background & Existing Infrastructure

### 2.1 What We Already Have

| Component | Files | Status |
|---|---|---|
| QWK packet writer (download) | `src/max/msg/mb_qwk.c` | Working — packs MESSAGES.DAT, CONTROL.DAT, NDX files, compresses, sends via protocol |
| QWK packet reader (upload/toss) | `src/max/msg/mb_qwkup.c` | Working — receives REP, decompresses, tosses messages into Squish areas |
| QWK header structures | `src/max/msg/qwk.h` | Complete — `_qmhdr` (128-byte msg header), `_qmndx` (NDX record), `_akh`/`_akd` (kludge file for area↔conf mapping) |
| MSGAPI (Squish + SDM) | `src/libs/msgapi/` | Full read/write/kill, linked-list frames, index, locking |
| XMSG structure | `src/libs/msgapi/msgapi.h` | 4D NETADDR (zone/net/node/point), from/to/subj (36/36/72 bytes), attr flags, reply threading |
| Squish tosser/scanner | `src/utils/squish/` | FidoNet toss/scan with SEEN-BY, PATH, dupe detection, routing — reference implementation |
| Browse/scan engine | `src/max/msg/mb_list.c` etc. | Callback-driven message iteration: `QWK_Begin`, `QWK_Display`, `QWK_Status`, `QWK_After`, `QWK_End` |
| Area↔conference mapping | `_akh`/`_akd` kludge file | Per-user binary file mapping area names to 1-based conference numbers (max 254) |
| Archiver subsystem | `src/max/file/arcmatch.c` | Load/invoke external archivers (ZIP, ARJ, etc.) via archiver.ctl |
| Reader TOML config | `general/reader.toml` | `work_directory`, `packet_name`, `archivers_ctl`, `phone`, `max_pack` |
| Matrix TOML config | `matrix.toml` | FidoNet addresses, echotoss, edit attrs, nodelist |
| libmaxcfg | `src/libs/libmaxcfg/` | TOML parser, `ngcfg_get_*()` API used by all runtime code |
| mbedTLS | `src/libs/mbedtls/` | TLS support for HTTPS — available for HTTP-based packet exchange |

### 2.2 Key Constraints from Existing Code

- **Conference numbers are 1-based** — the existing `InsertAkh()` uses a
  1-based conference numbering scheme. Conference 0 is reserved (we will
  use it for E-mail/NetMail, matching Synchronet convention).
- **MAX_QWK_AREAS = 254** — the current kludge file format limits areas
  to 254. This needs to be raised to **65535** for networking (16-bit
  conference numbers in the QWK header's `confLSB`/`confMSB` fields).
- **128-byte record size** — immutable QWK constant (`QWK_RECSIZE`).
- **QWK EOL = 0xE3** — PCBoard convention, must be preserved for
  compatibility (except when encoding is UTF-8 per HEADERS.DAT).
- **Kludge file is per-user** — stored in `<work_dir>/dats/<lastread_ptr>.dat`.
  Network node accounts get their own kludge files.

---

## 3. QWK Format Primer

### 3.1 Packet Structure (BBSID.QWK)

```
BBSID.QWK (compressed archive)
├── CONTROL.DAT      — BBS info + conference number↔name mapping (CRLF text)
├── DOOR.ID          — door capabilities (CONTROLTYPE lines)
├── MESSAGES.DAT     — all messages, 128-byte blocks
│   ├── Block 0: 128-byte space-padded creator ID string
│   ├── Block 1..N: struct _qmhdr (128 bytes) + message body blocks
│   └── ...
├── ###.NDX          — per-conference index (MBF32 offsets + conf byte)
├── PERSONAL.NDX     — index of messages addressed to the user
├── NETFLAGS.DAT     — 1 byte per conference: 0=local, 1=networked (optional)
├── NEWFILES.DAT     — new files listing (optional)
├── TOREADER.EXT     — QWKE extension: area flags, aliases, etc. (optional)
├── HEADERS.DAT      — Synchronet extension: per-message INI metadata (optional)
├── BLT-*.           — bulletins (optional)
├── HELLO            — welcome file (optional)
├── BBSNEWS          — news file (optional)
└── GOODBYE          — logoff file (optional)
```

### 3.2 Message Header (`_qmhdr`, 128 bytes)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 1 | status | ' '=public, '-'=read public, '*'=private, '+'=read private |
| 1 | 7 | msgn | Message number (ASCII, space-padded) |
| 8 | 8 | date | MM-DD-YY |
| 16 | 5 | time | HH:MM |
| 21 | 25 | to | Recipient name (space-padded) |
| 46 | 25 | from | Sender name (space-padded) |
| 71 | 25 | subj | Subject (space-padded) |
| 96 | 12 | pwd | Password (unused) |
| 108 | 8 | replyto | Reply-to message number (ASCII) |
| 116 | 6 | len | Total 128-byte blocks including header (ASCII) |
| 122 | 1 | msgstat | 225=active, 226=killed |
| 123 | 1 | confLSB | Conference number low byte |
| 124 | 1 | confMSB | Conference number high byte |
| 125 | 3 | rsvd | Reserved |

### 3.3 Reply Packet (BBSID.REP)

```
BBSID.REP (compressed archive)
├── BBSID.MSG        — same format as MESSAGES.DAT
│   ├── Block 0: BBSID string (space-padded to 128)
│   └── Messages: _qmhdr + body blocks
├── TODOOR.EXT       — QWKE reply extension (optional)
└── <attached files>  — file attaches (optional)
```

In a REP, the `msgn` field (offset 1–7) contains the **conference number**
(not the message number) for the destination area.

### 3.4 NDX File Format

Each record is 5 bytes:
- **Bytes 0–3**: MBF32 (Microsoft Binary Format single-precision float)
  encoding the 1-based block offset in MESSAGES.DAT
- **Byte 4**: Conference number (low byte only — limited, prefer header fields)

### 3.5 QWKnet Extensions (Synchronet-Compatible)

#### Conference 0 = E-mail / NetMail

All private mail and network netmail uses conference 0. This is the
universal convention across Synchronet, Mystic, and Enigma½.

#### @VIA: Path Tracking

First line(s) of message body:
```
@VIA: HUBID/RELAYID/.../ORIGINID
```

Used for:
- Circular path detection (if our BBSID appears → dupe loop, discard)
- Dynamic route map building
- Adding our hop when re-exporting

#### Routed NetMail

Messages in conf 0 with `To: NETMAIL`:
```
To: NETMAIL
1st line: username@DESTID              (direct)
1st line: username@NEXTID/.../DESTID   (routed)
```

#### @TZ: Time Zone

```
@TZ: xxxx
```
Synchronet SMB-style hex timezone (e.g., `41E0` = US Pacific).

#### @MSGID: / @REPLY:

Standard FTN-style message ID and reply linkage, carried through QWK.

#### HEADERS.DAT (Synchronet Extension)

INI-file format, sections keyed by hex byte offset of message in MESSAGES.DAT:
```ini
[80]
Utf8: true
Message-ID: <12345.abcd@mybbs>
WhenWritten: 20250223120000-0800 41E0
Sender: Kevin Morgan
Subject: Test message with long subject that exceeds 25 chars
To: Some User With A Really Long Name
Conference: 2001
X-FTN-MSGID: 1:103/705 aabbccdd
X-FTN-PATH: 103/705
```

#### NETFLAGS.DAT

One byte per conference number (0-indexed). `1` = conference is networked
(messages posted here get exported to hubs). `0` = local only.

#### Control Messages (REP Packet, conf 0)

| Subject | Action |
|---------|--------|
| `ADD [ptr]` | Subscribe to current conference |
| `DROP [conf#]` | Unsubscribe from conference |
| `RESET [ptr\|-msgs\|date]` | Reset message pointer |
| `RESETALL [ptr]` | Reset all conference pointers |

---

## 4. Architecture Overview

```
┌──────────────────────────────────────────────────┐
│                MaximusNG BBS                      │
│                                                   │
│  ┌─────────────┐  ┌──────────────────────────┐   │
│  │ User QWK    │  │  QWK Network Engine       │   │
│  │ (existing)  │  │  (new: qwknet.c/h)        │   │
│  │ mb_qwk.c    │  │                           │   │
│  │ mb_qwkup.c  │  │  ┌─────────────────────┐ │   │
│  │             │  │  │ Pack for node       │ │   │
│  │ Download ──►│  │  │ (export echomail +  │ │   │
│  │ Upload   ◄──│  │  │  netmail to QWK)    │ │   │
│  └─────────────┘  │  └─────────────────────┘ │   │
│         │         │  ┌─────────────────────┐ │   │
│         │         │  │ Toss from node      │ │   │
│         │         │  │ (import REP into    │ │   │
│         │         │  │  Squish areas)      │ │   │
│         │         │  └─────────────────────┘ │   │
│         │         │  ┌─────────────────────┐ │   │
│         │         │  │ Route map / @VIA    │ │   │
│         │         │  │ Dupe detection      │ │   │
│         │         │  │ NetMail forwarding  │ │   │
│         │         │  └─────────────────────┘ │   │
│         │         └──────────────────────────┘   │
│         │                    │                    │
│  ┌──────▼────────────────────▼──────────────┐    │
│  │          MSGAPI  (Squish / SDM)           │    │
│  │     Read / Write / Kill / Lock / Browse   │    │
│  └───────────────────────────────────────────┘    │
│                                                   │
│  ┌───────────────────────────────────────────┐    │
│  │        TOML Configuration (libmaxcfg)      │    │
│  │  qwknet.toml — nodes, routing, areas       │    │
│  │  matrix.toml — FidoNet addresses (ref)     │    │
│  │  reader.toml — work dirs, archivers        │    │
│  └───────────────────────────────────────────┘    │
│                                                   │
│  ┌───────────────────────────────────────────┐    │
│  │      Scheduler / Poll Engine (new)         │    │
│  │  Cron-style or event-driven poll cycle     │    │
│  │  HTTP(S) GET/PUT or local file exchange    │    │
│  └───────────────────────────────────────────┘    │
└──────────────────────────────────────────────────┘
```

### 4.1 Operational Modes

1. **User mode** (existing) — a human caller downloads QWK, reads offline
   in a reader, uploads REP. Enhanced with QWKE + HEADERS.DAT.

2. **Network node mode** (new) — a special user account with the `Q`
   restriction flag. At "logon," Maximus immediately packs a QWK for the
   node (only networked conferences), the node uploads its REP, and the
   session ends. This is the Synchronet model.

3. **Network hub mode** (new) — Maximus acts as a hub. It collects messages
   from multiple nodes, re-exports them to other nodes, and handles
   netmail routing.

4. **Batch/CLI mode** (new) — a standalone utility or mode of `max` that
   packs/tosses QWK packets without an interactive session, suitable for
   cron-driven polling via scripts.

---

## 5. TOML Configuration Schema

### 5.1 `config/qwknet.toml` — Master QWK Network Configuration

```toml
[qwknet]
# Our QWK ID — 2-8 alphanumeric chars, first char alpha, no '@'
bbsid = "MAXNG"

# Enable QWK networking globally
enabled = true

# Work directory for QWK network packet staging
work_directory = "/opt/maximus/qwknet"

# Route map file — dynamically maintained from @VIA lines
route_dat = "/opt/maximus/qwknet/route.dat"

# Days to keep stale route entries before purging
route_max_age_days = 90

# Enable @VIA line generation on export
enable_via = true

# Enable @TZ line generation on export
enable_tz = true

# Enable @MSGID/@REPLY generation on export
enable_msgid = true

# Enable Synchronet HEADERS.DAT generation
enable_headers_dat = true

# Enable QWKE TOREADER.EXT generation
enable_qwke = true

# Enable NETFLAGS.DAT generation
enable_netflags = true

# Default archiver for network packets (must match archiver.ctl entry)
archiver = "zip"

# Maximum message age (days) to include in network packets (0=unlimited)
max_message_age_days = 30

# ── Nodes ──────────────────────────────────────────

# Each [[qwknet.node]] defines a directly connected QWK network node.

[[qwknet.node]]
qwkid = "DOVE1"
description = "DOVE-Net Node 1"
# sysop name for this node
sysop = "Some Sysop"
# password for packet authentication (stored in packet header)
password = ""
# Whether this node is a hub we call (vs a leaf that calls us)
hub = false
# Archive format preference for this node
archiver = "zip"
# Poll schedule — cron-style or "on-demand"
poll_schedule = "*/30 * * * *"
# Transport: "local" (file drop), "telnet", "http", "https"
transport = "local"
# Transport-specific settings
[qwknet.node.transport_config]
# For local: directory paths
inbound = "/opt/maximus/qwknet/dove1.in"
outbound = "/opt/maximus/qwknet/dove1.out"
# For http/https: URL endpoints
# url = "https://dove1.example.com/qwknet"
# For telnet: host/port
# host = "dove1.example.com"
# port = 23

[[qwknet.node]]
qwkid = "SYNCHRO"
description = "Synchronet Hub"
sysop = "Digital Man"
password = "secret"
hub = true
archiver = "zip"
poll_schedule = "0 */4 * * *"
transport = "https"
[qwknet.node.transport_config]
url = "https://vert.synchro.net/qwknet"

# ── Area Mappings ──────────────────────────────────

# Conference number ↔ message area tag mapping for networking.
# Only areas listed here participate in QWK networking.
# Conference 0 is always reserved for E-mail/NetMail.

[[qwknet.area]]
conference = 1001
area_tag = "GENERAL"
description = "General Discussion"
# Which nodes receive this area (empty = all nodes)
nodes = []

[[qwknet.area]]
conference = 1002
area_tag = "SYSOP"
description = "SysOp Discussion"
nodes = ["SYNCHRO"]

[[qwknet.area]]
conference = 2001
area_tag = "DOVE_NET"
description = "DOVE-Net Echo"
nodes = ["SYNCHRO", "DOVE1"]
```

### 5.2 Extension to `general/reader.toml`

The existing reader config is sufficient for user-mode QWK. No changes
needed unless we want to add QWKE/HEADERS.DAT toggles per-user.

### 5.3 User Record Extension

The `usr` structure needs a flag to indicate a QWK network node account:

```c
/* In the user flags or a new field */
#define USRFLAG_QWKNET  0x0001  /* This account is a QWK network node */
```

Alternatively, use the Synchronet approach: a `Q` restriction character
stored in the user's restriction flags. When a user with this restriction
logs on, skip the normal menu system and go directly to the QWK
pack/exchange cycle.

---

## 6. Component 1: Enhanced QWK Packet Writer (Download)

### 6.1 Current Flow (mb_qwk.c)

```
QWK_Begin()     → create work dir, CONTROL.DAT, MESSAGES.DAT, load kludge
  ↓ (per area)
QWK_Status()    → InsertAkh() for area→conf mapping, open NDX, add to CONTROL.DAT
  ↓ (per message)
QWK_Display()   → BuildQWKHeader(), write message body blocks, BuildIndex()
  ↓
QWK_After()     → flush NDX for area
  ↓
QWK_End()       → flush MESSAGES.DAT, finish CONTROL.DAT, compress, download
```

### 6.2 Required Enhancements

#### 6.2.1 Raise MAX_QWK_AREAS to 65535

In `qwk.h`:
```c
#define MAX_QWK_AREAS   65535   /* Full 16-bit conference range */
```

The kludge file (`_akh`/`_akd`) format already uses `word num_areas` (16-bit),
so the binary format is compatible. The heap allocation in `Read_Kludge_File()`
currently does `malloc(sizeof(struct _akd) * MAX_QWK_AREAS)` — at 65535
entries × ~66 bytes each ≈ 4MB, acceptable for modern systems. Consider
switching to dynamic growth if memory is a concern.

#### 6.2.2 Conference 0 for NetMail

Currently, conference numbering starts at 1. Add special handling:
- If the area is the user's e-mail box (or netmail area), use conference 0
- Write `000.NDX` for conference 0 messages

#### 6.2.3 QWKE TOREADER.EXT Generation

After `QWK_End()` creates the archive, also generate `TOREADER.EXT`:
```
AREA <conf#> <flags>
```

Where flags encode area properties from `mah.ma.attribs`:
- `a`/`p`/`g` — all/personal/general scan mode
- `P`/`O`/`X` — private-only/public-only/either
- `R` — read-only
- `L`/`N`/`E` — local/netmail/echomail

#### 6.2.4 HEADERS.DAT Generation

For each message written to MESSAGES.DAT, also write an INI section to
HEADERS.DAT keyed by the byte offset:

```ini
[<hex_offset>]
WhenWritten: <ISO8601> <SMBTZ>
Sender: <from_name>
To: <to_name>
Subject: <full_subject>
Conference: <conf_number>
```

If the message originated from FidoNet (has MSGID kludge in ctrl info),
also emit:
```ini
X-FTN-MSGID: <value>
X-FTN-REPLY: <value>
X-FTN-PATH: <value>
X-FTN-SEEN-BY: <value>
```

#### 6.2.5 @VIA Line Injection (Network Mode)

When packing for a network node (not a user), prepend `@VIA:` to the
message body if the message has traversed at least one hop:

```c
if (is_network_export && msg_has_qwknet_origin) {
    /* Build @VIA from stored route + our BBSID */
    snprintf(via_line, sizeof(via_line), "@VIA: %s\n", build_via_path(msg));
}
```

Messages originating locally on this system do NOT get @VIA when sent to
directly connected nodes. They only get @VIA when those nodes re-export.

#### 6.2.6 @TZ Line Injection

```c
snprintf(tz_line, sizeof(tz_line), "@TZ: %04X\n", get_local_smb_tz());
```

#### 6.2.7 NETFLAGS.DAT Generation

For network node accounts, generate a file with one byte per conference:
```c
for (int i = 0; i <= max_conf; i++) {
    byte flag = is_networked_conference(i) ? 1 : 0;
    write(fd, &flag, 1);
}
```

#### 6.2.8 Network-Specific DOOR.ID

```
DOOR = MaximusNG
VERSION = <version>
SYSTEM = MaximusNG <version>
CONTROLNAME = MAXNG
CONTROLTYPE = ADD
CONTROLTYPE = DROP
CONTROLTYPE = RESET
CONTROLTYPE = RESETALL
CONTROLTYPE = TZ
CONTROLTYPE = VIA
CONTROLTYPE = CONTROL
CONTROLTYPE = NDX
MIXEDCASE = YES
```

### 6.3 New Function Signatures

```c
/**
 * @brief Pack a QWK packet for a network node.
 *
 * Unlike user-mode QWK_Begin/QWK_End which work through the browse
 * callback system, this function iterates all configured networked
 * areas for the given node, exporting messages since the node's
 * last highwater mark.
 *
 * @param node_qwkid  QWK ID of the destination node
 * @param out_path    Output path for the .QWK archive
 * @return 0 on success, -1 on error
 */
int qwknet_pack_for_node(const char *node_qwkid, char *out_path);

/**
 * @brief Pack QWK packets for all configured nodes.
 *
 * Iterates qwknet.node[] entries and calls qwknet_pack_for_node()
 * for each.
 *
 * @return Number of packets generated, -1 on fatal error
 */
int qwknet_pack_all(void);
```

---

## 7. Component 2: Enhanced QWK Packet Reader (Upload/Toss)

### 7.1 Current Flow (mb_qwkup.c)

```
QWK_Upload()
  ↓
Receive_REP()       → get REP file (upload or local path)
  ↓
Decompress_REP()    → extract using archiver
  ↓
Toss_QWK_Packet()   → iterate BBSID.MSG:
  ↓ (per message)
  QWK_Get_Rep_Header()  → verify BBSID matches
  QWK_To_Xmsg()         → convert _qmhdr → XMSG
  QWKRetrieveAreaFromPkt() → conference# → area name via kludge
  QWKGetValidArea()      → validate area access
  QWKTossMsgBody()       → read body, write to Squish
  WroteMessage()         → post-write hooks (echotoss log, etc.)
```

### 7.2 Required Enhancements

#### 7.2.1 @VIA Parsing and Storage

When tossing a message, scan the first lines of the body for kludges:
```c
if (strncmp(line, "@VIA: ", 6) == 0) {
    store_via_path(msg, line + 6);
    /* Also perform circular path detection */
    if (via_contains_our_id(line + 6, our_bbsid)) {
        log("!Circular path detected, discarding message");
        return TOSS_SKIP;
    }
}
```

Strip @VIA from the message body before storing in the local message base.
Store the complete route address in a message kludge/ctrl field for later
re-export.

#### 7.2.2 Route Map Updates

When we see `@VIA: HUBID/.../ORIGINID`, update the route map:
```
<date> ORIGINID:HUBID/HUBID2/.../ORIGINID
```

This lets us expand QWK IDs into full routing addresses for netmail.

#### 7.2.3 NetMail Handling (Conference 0)

When tossing a message from conference 0:

1. **To our BBSID or "SYSOP"** → deliver to sysop's local e-mail
2. **To "NETMAIL"** → routed netmail, parse first line for `user@route`
3. **To a known local username** → deliver to that user's e-mail
4. **Otherwise** → attempt to identify; if unknown, deliver to sysop

For routed netmail (To: NETMAIL), see [Component 5](#10-component-5-netmail-routing).

#### 7.2.4 Network Toss Mode

When tossing from a network node (as opposed to a user), additional behavior:

- **Accept any "From:" name** — network nodes relay messages from many users
- **Process control messages** (ADD, DROP, RESET) in conference 0
  addressed to our CONTROLNAME
- **Re-export to other nodes** — after tossing, mark areas as needing
  re-export to other connected nodes (echotoss log equivalent)
- **Handle TODOOR.EXT** if present
- **Handle HEADERS.DAT** if present — override header fields with
  extended data (longer names, subjects, UTF-8, FTN kludges)

#### 7.2.5 HEADERS.DAT Import

When a HEADERS.DAT file is present in the incoming packet:

```c
/* Parse INI sections keyed by hex byte offset */
for each message at offset X:
    section = headers_ini[hex(X)]
    if section.Utf8 == "true":
        decode body as UTF-8 (skip 0xE3 → LF conversion)
    if section.To:
        override msg->to with section.To  /* longer than 25 chars */
    if section.Subject:
        override msg->subj with section.Subject
    if section.Sender:
        override msg->from with section.Sender
    if section["X-FTN-MSGID"]:
        add to ctrl kludges
    /* etc. */
```

#### 7.2.6 Dupe Detection

Two levels:
1. **@MSGID-based** — if the message has an @MSGID kludge, check against
   a persistent dupe database (hash ring or similar)
2. **@VIA circular path** — if our BBSID appears in the @VIA chain,
   silently discard

### 7.3 New Function Signatures

```c
/**
 * @brief Toss a REP packet from a network node.
 *
 * Handles all QWKnet extensions: @VIA, netmail routing, route map
 * updates, dupe detection, and re-export flagging.
 *
 * @param node_qwkid   QWK ID of the node that sent this packet
 * @param rep_path     Path to the .REP archive file
 * @return 0 on success, -1 on error
 */
int qwknet_toss_from_node(const char *node_qwkid, const char *rep_path);

/**
 * @brief Process all pending inbound packets.
 *
 * Scans inbound directories for all configured nodes and tosses
 * any .REP or .QWK files found.
 *
 * @return Number of packets processed, -1 on fatal error
 */
int qwknet_toss_all_inbound(void);
```

---

## 8. Component 3: QWK Network Node Account System

### 8.1 Node Account Creation

Each directly connected QWK network node is represented by a user account
on the BBS. The account:

- **Username** = the node's QWK ID (e.g., `DOVE1`)
- **Restriction flag** = `Q` (QWK network node)
- **Privilege level** = sufficient to access all networked areas
- **No time limit** — network sessions are not time-bounded

### 8.2 Network Logon Flow

When a user with the `Q` restriction logs in:

```
1. Skip normal menu system
2. Load qwknet.toml, find matching node entry by username
3. Check for pending inbound REP:
   a. If the node uploads a REP → qwknet_toss_from_node()
4. Pack outbound QWK:
   a. qwknet_pack_for_node() → iterate networked areas
   b. Only include conferences listed in the node's area mappings
   c. Use highwater marks per-area-per-node for incremental export
5. Send QWK packet to node (download)
6. On successful send, update highwater marks
7. Disconnect
```

### 8.3 Highwater Mark Storage

Per-node, per-area highwater marks track the last message number exported.
Stored in a binary or TOML file:

```
<qwknet_work_dir>/hwm/<NODEID>.dat
```

Format (simple binary, one `dword` per conference slot):
```c
struct qwknet_hwm {
    char node_qwkid[9];        /* NUL-terminated QWK ID */
    word num_areas;
    struct {
        word conference;       /* conference number */
        dword highwater;       /* last exported UMSGID */
    } areas[];
};
```

Or in TOML for human readability:
```toml
# hwm/DOVE1.toml
[hwm]
GENERAL = 1542
SYSOP = 891
DOVE_NET = 2203
```

**Recommendation:** Use TOML for maintainability. Read/write via libmaxcfg.

---

## 9. Component 4: QWK Network Hub/Spoke Engine

### 9.1 Hub Responsibilities

When Maximus is a hub:

1. **Collect** — toss incoming REP packets from leaf nodes
2. **Redistribute** — for each tossed message, check which other nodes
   subscribe to that area and flag for re-export
3. **Route netmail** — forward conf-0 netmail toward the destination
4. **Maintain route map** — parse @VIA, update ROUTE.DAT

### 9.2 Export/Re-export Logic

After tossing messages from node A:
```
for each message M tossed into area X:
    for each node N subscribed to area X:
        if N != A:  /* don't echo back to sender */
            if M.@VIA does not contain N.qwkid:  /* dupe prevention */
                mark M for export to N
```

This is the QWK equivalent of the Squish echomail scan. The "echotoss log"
equivalent is a per-node pending-export list.

### 9.3 Spoke (Leaf Node) Behavior

When Maximus is a leaf calling a hub:

1. **Connect** to hub (via configured transport)
2. **Upload** our REP (messages posted locally since last exchange)
3. **Download** hub's QWK (new messages from the network)
4. **Toss** the received QWK locally
5. **Update** highwater marks

### 9.4 Message Flow Example

```
Node A posts in GENERAL (conf 1001)
    ↓
Node A packs REP → sends to Hub
    ↓
Hub tosses REP:
    - Stores message in Hub's GENERAL area
    - Flags GENERAL for re-export to Node B, Node C
    - Does NOT re-export to Node A (sender)
    ↓
Hub packs QWK for Node B:
    - Includes the message from Node A
    - Adds @VIA: NODEA (since it passed through Hub)
    ↓
Node B tosses QWK:
    - Stores message in local GENERAL area
    - Strips @VIA from stored body, saves in ctrl/kludge
    - Updates route map: NODEA reachable via HUB
```

---

## 10. Component 5: NetMail Routing

### 10.1 Sending NetMail

A user composes a message in conference 0 addressed to `user@DESTID`:

1. Look up DESTID in the route map
2. If found, expand to full route: `user@NEXTID/.../DESTID`
3. If DESTID is a directly connected node, send directly
4. If not found, attempt to route through default hub
5. Store in outbound netmail queue

### 10.2 Forwarding Routed NetMail

When we toss a message with `To: NETMAIL` and body starts with
`username@ROUTE`:

```c
parse_netmail_route(body_first_line, &dest_user, &route);

if (route_next_hop(route) == our_direct_node) {
    if (route_is_final_hop(route)) {
        /* Destination is our direct node — change To: to dest_user,
         * remove the routing line */
        strcpy(msg->to, dest_user);
        strip_first_body_line(body);
    } else {
        /* Strip our ID and next hop from route, forward */
        advance_route(route);
        rewrite_first_body_line(body, dest_user, route);
    }
    queue_for_node(next_hop_id, msg);
} else {
    /* Not our direct node — use route map to find path */
    expanded = expand_route(route);
    queue_for_node(first_hop(expanded), msg);
}
```

### 10.3 Receiving NetMail

When tossing conf-0 messages:

| To: field | Action |
|-----------|--------|
| Our BBSID | Deliver to sysop |
| "SYSOP" | Deliver to sysop |
| "NETMAIL" | Forward (routed netmail) |
| Known local user | Deliver to that user's mailbox |
| Unknown | Deliver to sysop with warning |

### 10.4 Route Map File Format

```
# route.dat — one entry per line
# DATE DESTID:HOP1/HOP2/.../DESTID
02/23/25 DOVE1:SYNCHRO/DOVE1
02/23/25 CIRCLE7:SYNCHRO/PHOUSE/CIRCLE7
```

Parsed at startup, updated on each toss, pruned of entries older than
`route_max_age_days`.

---

## 11. Component 6: @VIA Path Tracking & Dupe Detection

### 11.1 @VIA Generation Rules

When exporting message M to node N:

- If M originated **locally** and N is **directly connected** → no @VIA
- If M originated **locally** and N is reached through relay → @VIA: OURBBS
- If M has an existing @VIA → prepend our BBSID:
  `@VIA: OURBBS/PREVIOUS_VIA_CONTENT`

### 11.2 @VIA Consumption on Import

1. Parse @VIA line from message body
2. Check for circular path (our BBSID in the chain)
3. Update route map for the originating system
4. Strip @VIA from stored message body
5. Store the complete path in the message's ctrl/kludge area for
   re-export

### 11.3 Dupe Detection Database

A lightweight hash-based dupe ring:

```c
#define DUPE_RING_SIZE  32768  /* configurable */

struct qwknet_dupe_ring {
    dword hashes[DUPE_RING_SIZE];
    int head;  /* circular write position */
};
```

Hash inputs: `from + to + subject + date + first_64_bytes_of_body`

On import, compute hash. If present in ring → dupe, discard.
On successful import, add hash to ring.

Persist to disk between runs:
```
<qwknet_work_dir>/dupes.dat
```

---

## 12. Component 7: QWKE / HEADERS.DAT Extensions

### 12.1 QWKE Support

#### TOREADER.EXT (Generated on Export)

```
AREA 0 aPL
AREA 1001 aEX
AREA 1002 aPEX
AREA 2001 aE
```

#### TODOOR.EXT (Processed on Import)

```
AREA 1005 a        → subscribe node to conf 1005, all messages
AREA 1002 D        → unsubscribe node from conf 1002
RESET 1001 100     → reset pointer 100 msgs back from end
```

### 12.2 HEADERS.DAT Support

#### Writing (Export)

For each message block written to MESSAGES.DAT, emit a corresponding
section in HEADERS.DAT:

```c
void qwknet_write_headers_dat_entry(FILE *hdr_fp, dword byte_offset,
                                      const XMSG *msg, const char *ctrl,
                                      const char *area_tag, bool is_utf8)
{
    fprintf(hdr_fp, "[%lx]\r\n", (unsigned long)byte_offset);
    fprintf(hdr_fp, "Utf8: %s\r\n", is_utf8 ? "true" : "false");

    /* Full-length fields that may exceed 25 chars */
    fprintf(hdr_fp, "Sender: %s\r\n", msg->from);
    fprintf(hdr_fp, "To: %s\r\n", msg->to);
    fprintf(hdr_fp, "Subject: %s\r\n", msg->subj);

    /* Timestamp with timezone */
    fprintf(hdr_fp, "WhenWritten: %s\r\n", format_sync_timestamp(msg));

    fprintf(hdr_fp, "Conference: %s\r\n", area_tag);

    /* FTN kludges from ctrl */
    emit_ftn_headers(hdr_fp, ctrl);

    fprintf(hdr_fp, "\r\n");
}
```

#### Reading (Import)

Parse HEADERS.DAT as INI. For each message at byte offset X, look up
section `[X_in_hex]`. Override header fields and add FTN kludges.

### 12.3 UTF-8 Support

If HEADERS.DAT indicates `Utf8: true` for a message:
- Do NOT convert 0xE3 → LF in the message body (it's real UTF-8 data)
- Decode the body as UTF-8 instead of CP437
- The `Sender`, `To`, `Subject` fields in HEADERS.DAT are UTF-8

For messages originating on MaximusNG, we can export as UTF-8 when the
message contains characters outside CP437. Set `Utf8: true` accordingly.

---

## 13. Component 8: Automated Poll/Call-Out Scheduler

### 13.1 Design Options

#### Option A: Built-in Scheduler (Preferred)

A lightweight timer-based scheduler within the MaximusNG daemon process
(or a dedicated `maxqwknet` daemon):

```c
/**
 * @brief Main QWK network poll loop.
 *
 * Runs as a background thread or standalone process. Checks each
 * node's poll_schedule and executes pack/toss cycles as needed.
 */
void qwknet_scheduler_run(void);
```

Cron expression parsing for `poll_schedule` field.

#### Option B: External Script (Simpler)

A shell script invoked by system cron that calls a CLI mode:

```bash
#!/bin/bash
# /etc/cron.d/maxqwknet
*/30 * * * * bbs /opt/maximus/bin/max --qwknet-poll
```

The `--qwknet-poll` flag would:
1. Load config
2. For each node whose schedule matches: pack, transport, toss
3. Exit

**Recommendation:** Start with Option B (external cron + CLI flag) for
simplicity. Evolve to Option A later if daemon-mode QWK networking is
desirable.

### 13.2 Transport Layer

#### Local File Exchange

Simplest model. Each node has `<QWKID>.IN` and `<QWKID>.OUT` directories:
- **Export**: write .QWK to `<DESTID>.OUT/`
- **Import**: read .REP from `<DESTID>.IN/`, toss, delete

Works for nodes on the same machine or via shared filesystem/rsync.

#### HTTP(S) Exchange

For remote nodes, use mbedTLS-backed HTTPS:
- **Export**: HTTP POST the .QWK to the node's endpoint
- **Import**: HTTP GET the .REP from the node's endpoint

Protocol TBD — could use a simple REST API or the Synchronet QWKnet
HTTP protocol if documented.

#### Telnet/Dial-Up

Emulate a user login to the remote BBS:
1. Connect via telnet
2. Log in as our QWK node account
3. Upload REP
4. Download QWK
5. Disconnect

This is the traditional model and works with any BBS software.

### 13.3 CLI Flags

```
max --qwknet-poll              Poll all nodes per schedule
max --qwknet-pack <QWKID>     Pack QWK for specific node
max --qwknet-toss <path>       Toss a specific packet
max --qwknet-toss-all          Toss all pending inbound packets
max --qwknet-status            Show network status / route map
```

---

## 14. Component 9: maxcfg Integration

### 14.1 QWK Network Configuration Screen

Add a new top-level section to the maxcfg editor:

```
┌─ QWK Network Configuration ─────────────────────┐
│                                                   │
│  BBS ID:  [MAXNG   ]   Status: [Enabled]          │
│  Work Dir: /opt/maximus/qwknet                    │
│                                                   │
│  ── Nodes ──────────────────────────────────────  │
│  DOVE1     leaf  local    */30 * * * *            │
│  SYNCHRO   hub   https    0 */4 * * *             │
│                                                   │
│  ── Networked Areas ────────────────────────────  │
│  1001  GENERAL     [DOVE1, SYNCHRO]               │
│  1002  SYSOP       [SYNCHRO]                      │
│  2001  DOVE_NET    [DOVE1, SYNCHRO]               │
│                                                   │
│  [A]dd Node  [E]dit Node  [D]elete Node           │
│  [N]ew Area  [M]odify Area  [R]emove Area         │
└───────────────────────────────────────────────────┘
```

### 14.2 TOML Export

The maxcfg nextgen export pipeline generates `config/qwknet.toml` from
the editor's in-memory structures. This follows the same pattern as
`matrix.toml` and `reader.toml` export.

---

## 15. Implementation Phases

### Phase 1: Foundation (Enhance Existing QWK)

**Goal:** Get the existing user-mode QWK up to modern standards.

1. Raise `MAX_QWK_AREAS` to 65535
2. Add conference 0 support for E-mail/NetMail
3. Implement QWKE TOREADER.EXT generation
4. Implement Synchronet HEADERS.DAT generation (export)
5. Implement HEADERS.DAT parsing (import)
6. Generate enhanced DOOR.ID
7. Add @TZ line support (export and import)
8. Add @MSGID/@REPLY passthrough

**Deliverables:** Enhanced `mb_qwk.c`, `mb_qwkup.c`, `qwk.h`
**Testing:** Verify compatibility with MultiMail, SyncTERM QWK mode, Synchronet

### Phase 2: QWKnet Core Engine

**Goal:** Implement the networking engine as a library.

1. Create `src/max/netmail/qwknet.c` / `qwknet.h`
2. Implement `qwknet.toml` config parser (via libmaxcfg)
3. Implement `qwknet_pack_for_node()` — network-mode packet writer
4. Implement `qwknet_toss_from_node()` — network-mode packet tosser
5. Implement @VIA generation and parsing
6. Implement circular path detection
7. Implement route map (ROUTE.DAT) read/write/update/prune
8. Implement dupe detection ring
9. Implement highwater mark storage (per-node, per-area)
10. Implement NETFLAGS.DAT generation

**Deliverables:** `qwknet.c`, `qwknet.h`, config schema
**Testing:** Unit tests with synthetic packets; exchange with local Synchronet instance

### Phase 3: NetMail Routing

**Goal:** Full routed netmail support.

1. Implement `user@ROUTE` parsing for conf-0 messages
2. Implement route expansion via ROUTE.DAT
3. Implement netmail forwarding (hop-by-hop)
4. Implement netmail delivery (final hop → local user)
5. Implement netmail composition UI (address lookup)

**Deliverables:** Additions to `qwknet.c`, UI changes in message editor
**Testing:** Multi-hop routing test with 3+ nodes

### Phase 4: Node Account System

**Goal:** Automated QWK exchange via node accounts.

1. Implement `Q` restriction flag handling in logon
2. Implement network logon flow (skip menus → pack/exchange → disconnect)
3. Implement TODOOR.EXT processing (ADD/DROP/RESET commands)
4. Implement control message processing
5. Integrate with existing `QWK_Upload()` / `QWK_Begin()`...`QWK_End()`

**Deliverables:** Changes to `max_locl.c`/`max_init.c` logon flow
**Testing:** Simulate node login via telnet

### Phase 5: Scheduler & Transport

**Goal:** Unattended QWK networking.

1. Implement `--qwknet-poll` CLI mode
2. Implement local file transport (IN/OUT directories)
3. Implement HTTP(S) transport (using mbedTLS)
4. Implement cron expression parser (or use simple interval)
5. Write sample cron scripts

**Deliverables:** CLI mode in `max` or standalone `maxqwknet` binary
**Testing:** End-to-end networking with Synchronet hub over HTTPS

### Phase 6: maxcfg & Polish

**Goal:** Configuration UI and documentation.

1. Add QWK Network screen to maxcfg editor
2. Implement `qwknet.toml` export in nextgen pipeline
3. Write sysop documentation
4. Write inter-op testing guide
5. Performance tuning (large packet handling, memory)

---

## 16. File Inventory

### New Files

| File | Description |
|------|-------------|
| `src/max/netmail/qwknet.h` | QWK networking public API, structures, constants |
| `src/max/netmail/qwknet.c` | Core QWK networking engine |
| `src/max/netmail/qwknet_via.c` | @VIA path handling, route map |
| `src/max/netmail/qwknet_route.c` | NetMail routing logic |
| `src/max/netmail/qwknet_dupe.c` | Dupe detection ring |
| `src/max/netmail/qwknet_headers.c` | HEADERS.DAT read/write |
| `src/max/netmail/qwknet_transport.c` | Transport layer (local, HTTP) |
| `config/qwknet.toml` | Default QWK network configuration |
| `docs/qwk-networking-spec.md` | This document |
| `docs/qwk-networking-sysop.md` | Sysop guide (future) |

### Modified Files

| File | Changes |
|------|---------|
| `src/max/msg/qwk.h` | Raise MAX_QWK_AREAS, add network-mode constants |
| `src/max/msg/mb_qwk.c` | QWKE, HEADERS.DAT, @VIA/@TZ export, conf 0, DOOR.ID |
| `src/max/msg/mb_qwkup.c` | HEADERS.DAT import, @VIA parsing, network toss mode |
| `src/max/core/max_init.c` | Load qwknet.toml config |
| `src/max/core/max_locl.c` | Q-restriction logon flow |
| `src/max/core/protod.h` | New function prototypes |
| `src/max/Makefile` | Add netmail/qwknet objects |
| `src/apps/maxcfg/` | QWK Network config screen, TOML export |
| `src/libs/libmaxcfg/` | qwknet.toml schema support |

---

## 17. Testing Strategy

### 17.1 Unit Tests

- **Packet writer**: generate QWK from known messages, verify CONTROL.DAT
  format, MESSAGES.DAT block alignment, NDX offsets, HEADERS.DAT sections
- **Packet reader**: parse known-good QWK/REP packets (from Synchronet,
  Enigma½), verify message extraction
- **@VIA**: test path building, circular detection, route map updates
- **NetMail routing**: test hop-by-hop route rewriting
- **Dupe ring**: test hash collision, ring wrap, persistence

### 17.2 Integration Tests

- **Round-trip**: pack QWK on system A, toss on system B, verify messages
- **Synchronet interop**: exchange packets with a live Synchronet BBS
- **Multi-hop**: A → Hub → B, verify @VIA chains and netmail routing
- **User-mode**: download QWK in MultiMail/SyncTERM, reply, upload REP

### 17.3 Regression Tests

- Existing user QWK download/upload must continue to work unchanged
- FidoNet echomail (Squish toss/scan) must be unaffected
- Message area access controls must be enforced for network nodes

---

## 18. Compatibility Matrix

| System | QWK | REP | QWKE | HEADERS.DAT | @VIA | NetMail | Status |
|--------|-----|-----|------|-------------|------|---------|--------|
| Synchronet | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | Primary target |
| Mystic BBS | ✓ | ✓ | Partial | ✗ | ✓ | ✓ | Good compat |
| Enigma½ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | Good compat |
| MultiMail | ✓ | ✓ | ✓ | ✗ | N/A | N/A | Reader only |
| WWIV | ✓ | ✓ | ✗ | ✗ | ✗ | ✗ | Basic only |
| Talisman | ✓ | ✓ | ? | ? | ? | ? | TBD |

---

## 19. References

1. **QWK Format** — http://fileformats.archiveteam.org/wiki/QWK
2. **QWKE Specification 1.02** — Peter Rocca, 1994-1997
   (https://github.com/wwivbbs/wwiv/blob/main/specs/qwk/qwke.txt)
3. **Synchronet QWK Network Extensions** — Digital Dynamics, 1995
   (https://github.com/wwivbbs/wwiv/blob/main/specs/qwk/synchronet_qwk_networking.txt)
4. **Enigma½ QWK Implementation** — Bryan Ashby
   (https://github.com/NuSkooler/enigma-bbs/blob/master/core/qwk_mail_packet.js)
5. **Synchronet QWK Reference** — http://wiki.synchro.net/ref:qwk
6. **Synchronet SMB TZ Definitions** — smbdefs.h
7. **FTSC-0001** — FidoNet packet format (reference for XMSG/kludge handling)
