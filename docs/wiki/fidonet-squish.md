---
layout: default
title: "Squish Tosser"
section: "administration"
description: "Complete squish.cfg reference — addresses, areas, dupe checking, security, compression, points, and advanced options"
---

Squish is the mail processor at the heart of your FTN setup. It takes
incoming packets from your mailer, sorts messages into the right areas,
extracts outbound messages from your local areas, and compresses them
into archives for your mailer to send. This page is the full reference
for `squish.cfg` — every keyword, organized by function rather than
file order.

If you're setting up Squish for the first time, the
[Joining a Network]({% link fidonet-joining-network.md %}) tutorial
gives you a minimal working config. Come back here when you need to
understand what all the knobs do.

---

## On This Page

- [Command-Line Operation](#command-line)
- [Identity — Addresses & Origin](#identity)
- [Paths — Inbound, Outbound, Logs](#paths)
- [Area Definitions](#areas)
- [AREAS.BBS Format](#areas-bbs)
- [Area Flags](#area-flags)
- [Dupe Checking](#dupe-checking)
- [Security](#security)
- [Compression & Packing](#compression)
- [Points — Fakenet & 4D](#points)
- [Forwarding](#forwarding)
- [SEEN-BY & Control Info](#seenby)
- [Zone Gating](#zone-gating)
- [Performance & Limits](#performance)
- [Advanced Options](#advanced)

---

## Command-Line Operation {#command-line}

Squish operates in three phases, invoked via command-line arguments:

```bash
squish in out squash
```

| Phase | Command | What It Does |
|-------|---------|-------------|
| **Toss** | `in` | Decompresses archives from your inbound directory, reads packets, places messages into local areas |
| **Scan** | `out` | Reads the echotoss log, extracts new outbound messages from local areas, builds packets |
| **Pack** | `squash` | Compresses outbound packets into archives, places them in the outbound directory |

You can run them individually or combine them. The most common
invocation is all three at once:

```bash
squish in out squash
```

### Useful command-line switches

| Switch | Purpose |
|--------|---------|
| `-c<path>` | Specify config file path (default: `squish.cfg` in current dir) |
| `-f<file>` | Specify echotoss log path (overrides AreasBBS default) |
| `-s<sched>` | Invoke a named schedule from `route.cfg` |
| `-l<file>` | Override log file path |
| `-a<area>` | Process only the specified area |
| `-n` | No-op mode — show what would be done without doing it |

### Multipass vs. single-pass

Running `squish in out squash` in one command is **single-pass mode** —
Squish does everything in one run. This is simpler and is what most
systems use.

**Multipass mode** runs each phase separately:

```bash
squish in
squish out
squish squash
```

Multipass is useful when disk space is tight (pack between scans) or
when you need to run specific phases at different times. In multipass
mode, SEEN-BY and PATH lines are always preserved between passes. In
single-pass mode, they're only preserved if `SaveControlInfo` is
enabled.

---

## Identity — Addresses & Origin {#identity}

### Address

```
Address 21:1/200
```

Your primary FTN address. This is the address used in outgoing packets,
SEEN-BY lines, PATH lines, and origin lines.

You can specify multiple addresses for AKAs (Also Known As). The first
address is always your primary:

```
Address 21:1/200
Address 1:249/128
```

Squish uses the appropriate address automatically based on which zone a
message is destined for.

### Origin

```
Origin  Your BBS Name - yourbbs.example.com
```

The default origin line appended to echomail messages. Your address is
added automatically after this text, so the final origin looks like:

```
 * Origin: Your BBS Name - yourbbs.example.com (21:1/200)
```

This keyword is only required if you're **not** using an AREAS.BBS file
(which can carry its own origin line). If both are present, the
AREAS.BBS origin takes precedence.

---

## Paths — Inbound, Outbound, Logs {#paths}

### NetFile — Inbound mail

```
NetFile  /var/max/data/mail/inbound
```

Where your mailer (BinkD) places received packets and archives. Squish
searches these directories during `squish in`.

You can specify multiple `NetFile` paths with modifiers:

```
NetFile                /var/max/data/mail/inbound
NetFile NoEcho         /var/max/data/mail/inbound/unknown
NetFile NoArc          /var/max/data/mail/inbound/known
NetFile NoArc NoEcho   /var/max/data/mail/inbound/secure
```

| Modifier | Effect |
|----------|--------|
| `NoPkt` | Don't toss `.PKT` files from this path |
| `NoArc` | Don't toss compressed archives from this path |
| `NoEcho` | Don't toss EchoMail from this path |

These modifiers support multi-level security schemes where your mailer
sorts incoming files into different directories based on trust level.

### Outbound

```
Outbound  /var/max/data/mail/outbound
```

Where Squish places outbound archives for your mailer to send. For
BinkD-style outbound directories, specify the base directory without an
extension. Squish creates zone-specific subdirectories automatically
(e.g., `.015` for zone 21).

### AreasBBS

```
AreasBBS  /var/max/config/areas.bbs
```

Optional path to a standard-format AREAS.BBS file. If you define all
your areas using `EchoArea` lines in `squish.cfg`, you can omit this.
See [AREAS.BBS Format](#areas-bbs) below.

### LogFile and LogLevel

```
LogFile   /var/max/log/squish.log
LogLevel  6
```

Squish's own log file, separate from the Maximus system log. Log levels
range from 0 (minimal) to 6 (maximum detail). Level 6 is recommended
during setup; you can drop to 3–4 once everything is stable.

### Track

```
Track  /var/max/log/msgtrack.log
```

A separate log tracking routed netmail — records who sent what to whom
through your system. Must point to a **different** file than `LogFile`.
Useful for debugging routing issues or monitoring in-transit mail.

### Routing and Compress

```
Routing   /var/max/config/route.cfg
Compress  /var/max/config/compress.cfg
```

Paths to the routing control file and compression configuration. Both
are always required. See
[Netmail & Routing]({% link fidonet-netmail-routing.md %}) for full
`route.cfg` details and [Compression & Packing](#compression) below
for `compress.cfg`.

---

## Area Definitions {#areas}

Every system must define at least a **NetArea** and a **BadArea**. The
general format is:

```
<type>  <tag>  <path>  [flags]  [nodes]
```

### Required areas

```
NetArea   NETMAIL    /var/max/data/mail/netmail
BadArea   BAD_MSGS   /var/max/data/mail/bad
```

- **NetArea** — Where inbound netmail is placed. You can declare
  multiple NetAreas; Squish scans all of them when packing mail. All
  inbound netmail goes into the first one.
- **BadArea** — Where Squish puts messages it can't toss: wrong area
  name, security rejection, garbled packets. Check this area
  periodically.

### Optional areas

```
DupeArea  DUPES  /var/max/data/mail/dupes
```

- **DupeArea** — Where duplicate messages go (instead of BadArea). If
  not defined, dupes go to BadArea. Having a separate DupeArea makes it
  easier to spot dupe problems vs. other tossing errors.

### EchoArea

```
EchoArea  FSX_GEN  /var/max/data/msgbase/fsx_gen  -$  21:1/100
EchoArea  FSX_TST  /var/max/data/msgbase/fsx_tst  -$  21:1/100
```

Each EchoArea line defines one echomail conference:

- **Tag** (`FSX_GEN`) — The network-wide name for this echo. Must match
  exactly what your hub uses.
- **Path** — Where the message base files live. For Squish format, this
  is the base filename (Squish creates `.sqd`, `.sqi`, `.sql` files).
- **Flags** — Area modifiers. See [Area Flags](#area-flags).
- **Nodes** — The nodes you exchange this echo with (usually your hub).

---

## AREAS.BBS Format {#areas-bbs}

As an alternative to `EchoArea` lines in `squish.cfg`, you can define
echoes in a standard AREAS.BBS file:

```
; Standard AREAS.BBS format
; <path>  <tag>  <nodes>

/var/max/data/msgbase/fsx_gen    FSX_GEN    21:1/100
/var/max/data/msgbase/fsx_tst    FSX_TST    21:1/100

; Prefix with $ for Squish format
$/var/max/data/msgbase/fsx_bbs   FSX_BBS    21:1/100

; Prefix with # for passthru
#OTHERNET.ECHO                   OTHERNET.ECHO  21:1/100
```

| Prefix | Meaning |
|--------|---------|
| *(none)* | FTSC-0001 `*.MSG` format |
| `$` | Squish message format |
| `#` | Passthru — messages forwarded but not stored locally |

Most sysops find `EchoArea` lines in `squish.cfg` cleaner and more
flexible (flags, per-area options). AREAS.BBS exists for compatibility
with other software that expects it. You can use both — areas defined
in either place are merged.

---

## Area Flags {#area-flags}

Flags modify the behavior of individual areas. Place them between the
path and the node list:

```
EchoArea  TAG  /path/to/area  -$ -$m500 -$d30  21:1/100
```

### Storage format flags

| Flag | Meaning |
|------|---------|
| `-$` | Use Squish message format (recommended for all areas) |
| `-f` | Use FTSC-0001 `*.MSG` format (the default if neither is specified) |

### Squish format management

| Flag | Meaning |
|------|---------|
| `-$m<n>` | Keep a maximum of `n` messages. Older messages are deleted automatically during tossing. |
| `-$s<n>` | Skip the first `n` messages when applying `-$m` limit (protect pinned/important messages). |
| `-$d<n>` | Keep messages up to `n` days old. Requires running SQPACK regularly to purge. |

### Behavior flags

| Flag | Meaning |
|------|---------|
| `-0` | **Passthru** — messages pass through but are not stored locally. Use for areas you relay but don't read. (That's a zero, not the letter O.) |
| `-s` | Strip the private flag from incoming messages in this area. |
| `-h` | Hide messages — add the private flag to every message tossed into this area. |
| `-p<addr>` | Use an alternate primary address for this area (overrides your default Address for SEEN-BY, PATH, and packet headers). |
| `-x<addr>` | Exclude `<addr>` from sending to this area (block a node). |
| `-u<addr>` | Accept broadcast update/delete messages from `<addr>`. |
| `-+<addr>` | Add `<addr>` to SEEN-BYs for this area only. |

### Practical examples

```
; Standard Squish area, max 500 messages, linked with hub
EchoArea  FSX_GEN  /var/max/data/msgbase/fsx_gen  -$ -$m500  21:1/100

; Passthru area — relay to downlinks without storing locally
EchoArea  OTHERNET.CHAT  #  -0  21:1/100 21:1/150

; Area with age limit — keep 30 days of messages
EchoArea  FSX_BBS  /var/max/data/msgbase/fsx_bbs  -$ -$d30  21:1/100

; Area with alternate AKA
EchoArea  FIDO_SYSOP  /var/max/data/msgbase/fido_sysop  -$ -p1:249/128  1:249/0
```

---

## Dupe Checking {#dupe-checking}

Duplicate messages are a fact of life in FTN — network topology means
the same message can arrive via multiple paths. Squish has robust dupe
detection:

### Duplicates

```
Duplicates 1000
```

Number of message IDs to remember per area. Squish uses a 64-bit hash,
so false positives are extremely rare. The default of 1000 is fine for
most systems. Increase for very high-traffic areas.

### DupeCheck

```
DupeCheck Header MSGID
```

Which dupe-checking algorithms to use:

| Method | How It Works |
|--------|-------------|
| `Header` | Compares message headers (from, to, subject, date) |
| `MSGID` | Compares the `^aMSGID` kludge line (unique per message) |

Use both for maximum reliability. MSGID is the stronger check; Header
catches messages from software that doesn't generate MSGIDs.

### DupeLongHeader

```
DupeLongHeader
```

Check the **entire** subject line when doing header-based dupe detection.
Without this, only the first 24 characters are compared. Enable this —
there's no good reason not to.

### KillDupes

```
;KillDupes
```

When enabled, duplicate messages are deleted immediately instead of
being placed in the DupeArea. Saves disk space but makes it harder to
diagnose dupe problems. Leave it off during setup; consider enabling
it on a stable, high-traffic system.

---

## Security {#security}

### Secure

```
Secure
```

The most important security keyword. When enabled, Squish checks the
source address of every incoming echomail message against the node list
for that area. Messages from nodes not listed for an area are rejected
and placed in BadArea.

**Always enable this.** Without it, any system that can send packets to
you can inject messages into any of your echo areas.

### CheckZones

```
CheckZones
```

Check zone numbers on incoming packets. Treats different zones as
distinct entities. Some ancient software doesn't include zone
information in packets; if you're dealing with such systems, you may
need to disable this. For modern networks, keep it on.

### Password

```
Password  21:1/100  MYSECRET
```

Set a password for packets exchanged with a specific node. When
`Secure` is enabled, Squish also checks incoming packet passwords and
rejects packets with wrong passwords. Passwords are case-insensitive,
max 8 characters.

Use this for your hub connection. Your hub admin will give you a session
password when you join the network.

### StripAttributes

```
StripAttributes
```

Strip CRASH and HOLD attributes from incoming netmail. This prevents
someone from routing a crash-priority message through your system and
having it delivered as crash on your dime. Recommended for any system
that forwards netmail.

You can also target specific nodes:

```
StripAttributes World EXCEPT 1:123/456
```

---

## Compression & Packing {#compression}

### compress.cfg

**File:** `config/compress.cfg`

This file defines the external compression programs Squish uses to
pack and unpack mail archives. The default ships with definitions for
ARC, PAK, ZIP, LHarc, ZOO, and ARJ. Each archiver entry specifies:

- **Extension** and **magic bytes** for identification
- **Add**, **Extract**, and **View** commands for DOS, OS/2, and UNIX

For UNIX/Linux, the relevant commands use standard tools:

```
; ZIP archiver (UNIX section)
Archiver ZIP
  Extension     ZIP
   Stripping     4 00
  Add           zip -j -9 %a %f
  Extract       unzip -j -o %a -d %p
  View          unzip -v %a
End Archiver
```

You shouldn't need to edit this unless you want to add a new archiver
or change compression tool paths.

### Pack and DefaultPacker

```
DefaultPacker LHarc
```

The default compression method for nodes not listed in any `Pack`
statement. The official FidoNet standard is ARC, but most modern
networks use ZIP or LHarc.

To specify compression per node:

```
Pack Zip    21:1/100 21:2/100
Pack LHarc  1:249/All
```

### QuietArc

```
QuietArc
```

Suppress screen output from external compression programs. Always
enable this unless you're debugging archiver issues.

### BatchUnarc

```
BatchUnarc
```

Decompress all incoming archives before starting to toss, rather than
tossing one archive at a time. This prevents messages from getting out
of order (packets are tossed by date). Recommended for any system that
forwards mail.

---

## Points — Fakenet & 4D {#points}

If you're running points (sub-systems hanging off your node), Squish
supports two methods:

### Fakenet points

The traditional method. Points get a "fake" net number:

```
Address 1:23914/1
Address 1:123/456.1
Pointnet 23914
```

The `Pointnet` keyword tells Squish to translate between the fakenet
address and the real 4D address. Both the boss node and the point must
use this keyword.

### 4D points

The modern method. Just use the full 4D address:

```
Address 1:123/456.1
```

Both you and your boss must be running 4D-aware software (Squish is;
most modern mailers are too). No `Pointnet` keyword needed.

### Remap

```
Remap  .2  Mark Kaye
Remap  .3  Kevin Kell
Remap  .1  Scott*
```

Automatically forward netmail addressed to your node to a point, based
on the recipient's name. The `*` wildcard matches partial names.
Squish uses Soundex matching by default; disable with `NoSoundex` if
it causes false matches.

---

## Forwarding {#forwarding}

### ForwardTo

```
ForwardTo        21:1/0 21:1/100
ForwardTo File   21:1/100
```

Allow netmail destined for the listed nodes to be forwarded through
your system. The `File` modifier also forwards file attaches.
`ForwardTo World` forwards all mail.

### ForwardFrom

```
ForwardFrom      21:1/100
ForwardFrom File 21:1/100
```

Allow mail **from** the listed nodes to be forwarded anywhere. The
`File` modifier is powerful and potentially dangerous — it lets those
nodes send files through your system to any destination. Only use for
trusted nodes (your own points, your hub).

### KillIntransit

```
;KillIntransit
```

Delete in-transit netmail after packing. Without this, forwarded
netmail stays in your NetArea after being sent. Enable if you don't
want to keep copies of other people's mail.

### KillIntransitFile

```
;KillIntransitFile
```

Mark forwarded file attaches for deletion after sending (adds `^`
prefix to the filename in `.?LO` files). Only applies to
non-ArcmailAttach systems.

### KillBlank

```
KillBlank
```

Delete blank netmail messages (no body text). Some systems generate
these for file requests and file attaches. Safe to enable.

---

## SEEN-BY & Control Info {#seenby}

### SaveControlInfo

```
SaveControlInfo
```

Preserve SEEN-BY and PATH control lines inside Squish-format message
bases. Without this (in single-pass mode), control info is discarded
after tossing — fine for leaf nodes, but you lose the ability to see
message propagation history. Enable it.

### TinySeenbys

```
TinySeenbys  21:1/150 21:2/100
```

Strip excess SEEN-BY entries when exporting to the listed nodes. Only
the nodes defined for each echo area are kept. This significantly
reduces packet sizes for widely-distributed echoes. Useful for
downlinks with slow connections.

### AddToSeen

```
AddToSeen  21:1/200
```

Add a specific node to SEEN-BYs for all areas. Normally not needed,
but useful when changing your primary address (add the old address so
messages don't loop back).

---

## Zone Gating {#zone-gating}

### ZoneGate

```
ZoneGate 2:253/68  249/106 253/68
```

Perform FTSC-0004 compliant zone gating. When echomail is sent to the
host node, SEEN-BYs are stripped and replaced with the specified nodes.
This is required when gating conferences between zones (net/node
numbers can collide across zones).

**Use with care.** Incorrect zone gating can cause message loops or
lost SEEN-BY history. Only needed if you're actually gating echoes
between zones.

### GateRoute

```
GateRoute Normal 1:1/2 2:All
```

Route inter-zone netmail through a zone gate. The first address is the
gate node; following addresses are the destinations to route through it.
The `Except` keyword excludes specific nodes:

```
GateRoute Normal 1:1/2 2:All Except 2:123/456
```

Only needed for systems that actively route inter-zone netmail.

---

## Performance & Limits {#performance}

### Buffers

```
Buffers Large
```

Controls Squish's memory usage. Options: `Large`, `Medium`, `Small`.
Modern systems have plenty of RAM — always use `Large`. `Small` is
unsuitable for systems that forward mail to other nodes.

### MaxMsgs

```
;MaxMsgs 150
```

Stop scanning after `n` messages, pack the results, then continue.
Useful when disk space is tight or when you want to limit packet sizes.
Not needed on modern systems.

### MaxPkt

```
;MaxPkt 256
```

Maximum number of packets in the outbound area at once. Default 256.
Rarely needs changing.

### MaxArchive

```
;MaxArchive 250
```

Maximum size (KB) of outbound archives. Squish won't add packets to
archives exceeding this size. Useful for networks with size limits.

### BusyFlags

```
BusyFlags
```

Use BinkleyTerm-style `.BSY` lock files in the outbound area. Prevents
Squish and BinkD from accessing the same bundle simultaneously. **Always
enable this** on any system where Squish and BinkD might run
concurrently.

---

## Advanced Options {#advanced}

### ArcmailAttach

```
;ArcmailAttach
```

Use FroDo/InterMail/D'bridge-style attach messages instead of
BinkleyTerm-style `.?LO` files. Not needed for BinkD — this is for
legacy mailers that use attach messages. Leave commented out.

### BinkPoint

```
;BinkPoint
```

Enable BinkleyTerm 2.50+ point directory support. Only needed if you're
running 4D points with BinkleyTerm as your mailer. Not relevant for
BinkD.

### AddMode

```
;AddMode
```

When adding to `.?LO` files, check for existing files with different
flavors (Crash, Hold, Direct) and add to the existing flavor instead
of the one specified in routing. Useful for temporarily holding mail
for a down node without changing your routing config.

### LinkMsgid

```
;LinkMsgid
```

Link message replies using MSGID/REPLY kludges instead of subject-line
matching. More accurate for threading, but only works if all systems
in the echo generate proper MSGIDs.

### TossBadMsgs

```
TossBadMsgs
```

Re-toss messages from BadArea on every `squish in`. If you add a new
area definition and messages for that area were previously rejected to
BadArea, they'll be automatically tossed on the next run. Very useful
— enable it.

### Statistics

```
;Statistics squish.stt
```

Keep a statistics file tracking messages sent/received per node. The
file can be processed by external tools to generate billing reports.
Uses significant memory — only enable if you need it.

### NoStomp

```
;NoStomp
```

Don't update packet headers during routing. Normally Squish rewrites
the destination and password in packet headers to match the routing
target. Disable this only for debugging.

### Nuke (ArcmailAttach only)

```
;Nuke
```

Delete orphaned archives that have no corresponding attach message.
Only for ArcmailAttach systems with mailers that can't delete files
after sending. Potentially dangerous — don't enable unless required.

### OldArcmailExts

```
;OldArcmailExts
```

Use only `.MO?` extensions for archives instead of day-of-week
extensions (`.TU?`, `.WE?`, etc.). Only needed for compatibility with
very old software (Opus 1.03 era). Not relevant for modern networks.

---

For day-to-day maintenance of your Squish message bases, see the
[Squish]({% link squish.md %}) utilities page (SQPACK, SQFIX, etc.).
For routing configuration, see
[Netmail & Routing]({% link fidonet-netmail-routing.md %}).
