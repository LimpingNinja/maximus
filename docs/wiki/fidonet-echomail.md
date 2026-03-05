---
layout: default
title: "Echomail"
section: "administration"
description: "How echomail works under the hood — SEEN-BY, PATH, dupe prevention, passthru, security, and area management with SqaFix"
---

Echomail is the public messaging backbone of FTN — distributed
conversations that span every BBS carrying a given echo. The
[overview]({{ site.baseurl }}{% link fidonet.md %}) introduced the concept; this page
goes deeper into how echomail actually works, what the control
information means, and how to manage your echo areas effectively.

This is the reference page for sysops who already have echomail flowing
and want to understand the machinery — dupe checking tuning, security
hardening, passthru relaying, and automated area management. If you're
still setting up, the
[Joining a Network]({{ site.baseurl }}{% link fidonet-joining-network.md %}) tutorial
is a better starting point.

---

## On This Page

- [How Echomail Propagates](#propagation)
- [Anatomy of an Echomail Message](#anatomy)
- [SEEN-BY Lines](#seenby)
- [PATH Lines](#path)
- [Origin and Tear Lines](#origin)
- [Dupe Prevention in Depth](#dupes)
- [Security — Keeping Your Areas Clean](#security)
- [Passthru Areas](#passthru)
- [Area Management with SqaFix](#sqafix)
- [Message Limits and Housekeeping](#housekeeping)
- [Common Echomail Problems](#problems)

---

## How Echomail Propagates {#propagation}

Echomail is a **flood-fill** system. When a message is posted in an
echo area, it propagates outward from the origin system through the
network topology until every system carrying that echo has a copy. The
mechanism that prevents infinite loops is the SEEN-BY list — each
system adds its address before forwarding, and no system forwards a
message to a node already in the SEEN-BY.

Here's the flow for a typical message:

```
Caller posts in FSX_GEN on your BBS (21:1/200)
  ↓
Maximus writes "FSX_GEN" to echotoss.log
  ↓
squish out → scans FSX_GEN, builds packet
  Adds SEEN-BY: 21:1/100 200  (your hub + you)
  Adds PATH: 21:1/200
  ↓
squish squash → compresses, places in outbound for 21:1/100
  ↓
BinkD sends to hub (21:1/100)
  ↓
Hub's tosser unpacks, re-exports to all other nodes in FSX_GEN
  Adds each node to SEEN-BY, adds hub to PATH
  ↓
Each receiving node's tosser places message in local FSX_GEN area
```

The key insight: **your system only sends to nodes listed in your area
definition** (typically just your hub). The hub handles redistribution.
This star topology keeps things simple for leaf nodes.

---

## Anatomy of an Echomail Message {#anatomy}

An echomail message contains several pieces beyond the visible text.
Here's what a complete message looks like internally:

```
From: Kevin Morgan
To: All
Subj: Testing from Maximus!
Date: 02 Mar 2026 17:30:00
----
^aINTL 21:1/100 21:1/200
^aMSGID: 21:1/200 abcd1234
^aPID: Maximus 4.0
Hello from my Maximus BBS! Just testing echomail.
---
 * Origin: My Awesome BBS - mybbs.example.com (21:1/200)
SEEN-BY: 1/100 200
PATH: 1/200
```

The `^a` prefix denotes **kludge lines** — hidden control information
that most callers never see (controlled by `ctla_priv` in
`matrix.toml`). Everything below the `---` tear line is control
information added by the tosser.

---

## SEEN-BY Lines {#seenby}

```
SEEN-BY: 1/100 105 128 150 200 2/100 101
```

SEEN-BY lines track every node that has processed this message. They
serve two purposes:

1. **Loop prevention** — Squish won't send a message to any node
   already in the SEEN-BY list.
2. **Audit trail** — You can see which systems have handled the message.

### Format

SEEN-BY uses a compressed format. The first entry is a full `net/node`;
subsequent entries in the same net omit the net number:

```
SEEN-BY: 1/100 105 128 200     ← all in net 1
SEEN-BY: 2/100 101 103         ← all in net 2
```

Multiple SEEN-BY lines may be present. Squish merges them when
processing.

### SEEN-BY and zones

Within a single zone, SEEN-BY lines don't include the zone number
(it's implied). When gating echoes between zones, SEEN-BYs must be
stripped and rebuilt — that's what the `ZoneGate` keyword in
`squish.cfg` handles. See
[Zone Gating]({{ site.baseurl }}{% link fidonet-squish.md %}#zone-gating).

### Controlling SEEN-BY size

On widely-distributed echoes, SEEN-BY lists can get very long. Use
`TinySeenbys` in `squish.cfg` to strip excess entries when exporting
to specific nodes:

```
TinySeenbys 21:1/150
```

This sends only the nodes defined for each area, keeping packets small.

The per-area `-+<node>` flag adds a specific node to SEEN-BYs for one
area. The global `AddToSeen` keyword adds a node to all areas. See
[SEEN-BY & Control Info]({{ site.baseurl }}{% link fidonet-squish.md %}#seenby) in the
Squish reference.

---

## PATH Lines {#path}

```
PATH: 1/200 1/100
```

PATH lines record the **route** a message took — which systems actually
forwarded it, in order. Unlike SEEN-BY (which lists every system that
*has* the message), PATH only lists systems that actively relayed it.

PATH is primarily a diagnostic tool. If messages are arriving via
unexpected routes, PATH tells you how. It's also used by some dupe
detection systems.

PATH lines are preserved in Squish-format areas when `SaveControlInfo`
is enabled in `squish.cfg`.

---

## Origin and Tear Lines {#origin}

Every echomail message gets two marker lines added by the tosser:

### Tear line

```
---
```

A simple three-dash separator. Marks the boundary between message text
and control information. Some tossers add their name after the dashes
(e.g., `--- Squish/Linux v1.11`).

### Origin line

```
 * Origin: My Awesome BBS - mybbs.example.com (21:1/200)
```

The origin line identifies the system where the message was created.
It includes your custom text (set via the `Origin` keyword in
`squish.cfg` or per-area in `areas.toml`) and your FTN address in
parentheses.

The origin line is your BBS's signature on the network. Make it count —
include your board name and address so people can find you. It's a
tradition and a point of pride.

---

## Dupe Prevention in Depth {#dupes}

Duplicate messages happen. Network topology, retransmissions, and
software quirks all contribute. Squish has two dupe-detection methods
that work together:

### MSGID-based detection

Every echomail message should have a `^aMSGID` kludge:

```
^aMSGID: 21:1/200 abcd1234
```

The address + serial combination is unique per message. Squish hashes
this into a 64-bit ID and stores it. If the same MSGID appears again,
it's a dupe.

This is the strongest detection method. It only fails when the
originating software doesn't generate MSGIDs (rare these days).

### Header-based detection

Squish compares the message header fields: from, to, subject, and date.
If all match, it's flagged as a dupe. This catches messages from
software that doesn't generate MSGIDs.

Enable `DupeLongHeader` to compare the full subject line instead of
just the first 24 characters.

### Tuning

```
Duplicates 1000
DupeCheck Header MSGID
DupeLongHeader
```

- **`Duplicates 1000`** — IDs remembered per area. Increase for
  high-traffic areas (5000+ messages/week). Decrease if memory is tight
  (unlikely on modern systems).
- **Both methods together** catch the widest range of dupes.
- **`KillDupes`** deletes dupes immediately; without it, they go to
  `DupeArea` for inspection. Leave `KillDupes` off during setup.

### When you see a lot of dupes

Common causes:

- **Your hub re-sent a batch** — Sometimes hubs resend mail after
  errors. The dupe checker handles this automatically.
- **You're receiving the same echo via two paths** — Check your area
  definitions for duplicate node entries.
- **Dupe database was lost** — If `squish.cfg` points to a new area
  path, the dupe history resets. Old messages may be re-tossed.
- **TossBadMsgs re-tossed old messages** — If you enabled
  `TossBadMsgs` and your BadArea had old messages, they may re-enter
  the system.

---

## Security — Keeping Your Areas Clean {#security}

### The Secure keyword

```
Secure
```

This is your first line of defense. With `Secure` enabled, Squish
rejects echomail from any node not listed in the area definition.
Unauthorized messages go to BadArea.

**Always enable Secure.** Without it, anyone who can send you a packet
can inject messages into any of your echo areas.

### Packet passwords

```
Password 21:1/100 MYSECRET
```

Adds a password check to packets from your hub. If an incoming packet
doesn't have the right password, it's rejected. This prevents
spoofing — someone pretending to be your hub.

### What ends up in BadArea

Check your BadArea periodically. Messages land there for several
reasons:

| Reason | What It Means |
|--------|--------------|
| Unknown area tag | You received mail for an echo you don't carry. Request the area from your hub, or ignore it. |
| Security rejection | A node not listed for the area tried to send to it. Could be misconfiguration or something suspicious. |
| Bad packet | The packet was malformed or corrupted in transit. |
| Wrong password | Packet password didn't match your `Password` setting. |

If you see a lot of "unknown area" messages for echoes you actually
want, add them to your `squish.cfg` and let `TossBadMsgs` pick them
up automatically on the next run.

---

## Passthru Areas {#passthru}

A **passthru** area is one you relay to other nodes but don't store
locally. Messages pass through your system and are forwarded to your
downlinks, but they don't appear in any local message area.

```
EchoArea  OTHERNET.CHAT  #  -0  21:1/100 21:1/150
```

The `-0` flag (zero, not O) marks the area as passthru. The `#` path
prefix is a shorthand for the same thing in AREAS.BBS format.

### When to use passthru

- You're a hub or relay node feeding echoes to downlinks.
- You carry an echo for downstream nodes but your callers don't need
  to read it.
- You want to reduce disk usage on high-traffic echoes you don't
  personally follow.

### Passthru and dupe checking

Dupe checking still works on passthru areas — Squish maintains the dupe
database even though messages aren't stored locally. This prevents dupes
from propagating to your downlinks.

---

## Area Management with SqaFix {#sqafix}

**SqaFix** (Squish Area Fix) is an automated area management utility.
It processes netmail commands from linked nodes, allowing them to
subscribe and unsubscribe from echo areas without manual intervention
by the sysop. It's particularly useful if you feed downlinks.

### Basic commands

Send a netmail to SqaFix at your hub's address (e.g., `21:1/100`) with
the subject set to your session password. In the message body, use
these commands:

| Command | Action |
|---------|--------|
| `%HELP` | Get a list of available commands |
| `%LIST` | List all available echo areas |
| `+FSX_BBS` | Subscribe to FSX_BBS |
| `-FSX_BBS` | Unsubscribe from FSX_BBS |
| `%QUERY` | List your current subscriptions |
| `%UNLINKED` | List areas available but not subscribed |
| `%RESCAN FSX_BBS` | Request a rescan of recent messages in FSX_BBS |

### For hub operators

If you're running SqaFix on your own system (feeding downlinks), see
the [SqaFix]({{ site.baseurl }}{% link sqafix.md %}) utility page for full configuration
details — autocreate, area forwarding, access control, and more.

---

## Message Limits and Housekeeping {#housekeeping}

Echomail areas grow over time. Without management, they'll eat your
disk. There are two approaches:

### Automatic limits in squish.cfg

Set per-area limits using area flags:

```
; Keep max 500 messages
EchoArea FSX_GEN /var/max/data/msgbase/fsx_gen -$ -$m500 21:1/100

; Keep messages up to 30 days old
EchoArea FSX_BBS /var/max/data/msgbase/fsx_bbs -$ -$d30 21:1/100

; Max 500 messages, but protect the first 10
EchoArea FSX_TST /var/max/data/msgbase/fsx_tst -$ -$m500 -$s10 21:1/100
```

- **`-$m<n>`** — Message count limit. Applied automatically during
  tossing. When the count exceeds `n`, the oldest messages are purged.
- **`-$s<n>`** — Skip the first `n` messages when purging (protects
  pinned or important messages at the start of the area).
- **`-$d<n>`** — Age limit in days. **Not** applied during tossing —
  requires running SQPACK separately.

### SQPACK

For age-based cleanup and general maintenance, run SQPACK periodically:

```bash
sqpack /var/max/data/msgbase/fsx_gen
```

SQPACK purges expired messages, reclaims space in the Squish database
files, and reindexes. Run it via cron — daily or weekly depending on
message volume.

See [Squish Utilities]({{ site.baseurl }}{% link squish.md %}) for full SQPACK
documentation.

### Recommended approach

Use `-$m` limits on all echo areas (500–2000 messages depending on
traffic) for automatic day-to-day management. Run SQPACK weekly as a
deeper cleanup pass. This combination keeps areas tidy without manual
intervention.

---

## Common Echomail Problems {#problems}

### Messages appear locally but don't reach the network

- **Check the echotoss log.** Did Maximus write the area tag? Look in
  `log/echotoss.log` after posting.
- **Run `squish out`.** Did Squish scan the area? Check `squish.log`.
- **Run `squish squash`.** Did Squish create an outbound archive?
- **Run BinkD.** Did it transfer the archive to your hub?

The chain is: Maximus → echotoss → Squish scan → Squish pack → BinkD.
If any link breaks, messages don't flow.

### Messages from the network don't appear locally

- **Check BinkD.** Did it receive files? Check `binkd.log`.
- **Run `squish in`.** Did Squish find and toss the packets? Check
  `squish.log`.
- **Check paths.** Does BinkD's `inbound` match Squish's `NetFile`?
- **Check area definitions.** Does `squish.cfg` have an `EchoArea`
  line for the missing echo? If not, messages go to BadArea.

### Messages appear in BadArea

See the [Security section](#security) above for common causes. The
most frequent: you're receiving mail for an echo you haven't defined.
Add the area and enable `TossBadMsgs` to recover the messages.

### Dupe storms

If you suddenly get a flood of duplicate messages:

1. Check if your hub re-sent a batch (ask in the network support echo).
2. Verify you're not subscribed to the same echo via two paths.
3. Check that your dupe database files (`.sqd`/`.sqi`) haven't been
   deleted or corrupted.
4. Consider temporarily increasing `Duplicates` in `squish.cfg`.

### Origin line is wrong or missing

- Set the `Origin` keyword in `squish.cfg` for a global default.
- Set the `origin` field per area in `areas.toml`.
- Make sure area type is `echo` (not `local`) in Maximus.

---

For Squish configuration details, see the
[Squish Tosser]({{ site.baseurl }}{% link fidonet-squish.md %}) reference. For netmail
and routing, see
[Netmail & Routing]({{ site.baseurl }}{% link fidonet-netmail-routing.md %}).
