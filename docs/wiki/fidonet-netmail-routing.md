---
layout: default
title: "Netmail & Routing"
section: "administration"
description: "Private FTN messaging, route.cfg verbs, mail flavors, schedules, forwarding, and inter-zone routing"
---

NetMail is the private, point-to-point side of FTN — messages sent from
one specific address to another. Unlike echomail (which floods out to
every system carrying an echo), netmail is routed: it travels from your
system to your hub, through intermediate hops if needed, and arrives at
the destination system.

This page covers how netmail works, how routing is configured in
`route.cfg`, and how to manage mail flavors, schedules, and forwarding.
For the echomail side, see [Echomail]({{ site.baseurl }}{% link fidonet-echomail.md %}).
For the full `squish.cfg` reference, see
[Squish Tosser]({{ site.baseurl }}{% link fidonet-squish.md %}).

---

## On This Page

- [How NetMail Works](#how-it-works)
- [NetMail vs. EchoMail — The Quick Refresher](#vs-echomail)
- [The NetArea](#netarea)
- [route.cfg — Routing Control](#route-cfg)
- [Routing Verbs](#verbs)
- [Mail Flavors](#flavors)
- [Address Wildcards](#wildcards)
- [Schedules](#schedules)
- [HostRoute — The Simple Default](#hostroute)
- [Forwarding NetMail](#forwarding)
- [Inter-Zone Routing](#inter-zone)
- [NetMail Tracking](#tracking)
- [Common NetMail Scenarios](#scenarios)
- [Troubleshooting](#troubleshooting)

---

## How NetMail Works {#how-it-works}

When a caller (or the sysop) writes a netmail message on your BBS,
here's what happens:

1. **Composition.** Maximus prompts for the destination address
   (e.g., `21:1/101`) and the recipient's name. The message is saved
   to your NetArea.

2. **Scanning.** When you run `squish out`, Squish finds the new
   netmail in your NetArea and builds an outbound packet. Squish
   consults `route.cfg` to determine *where* to send the packet and
   *what flavor* (priority) to use.

3. **Packing.** `squish squash` compresses the packet and places it
   in the outbound directory, addressed to the next hop (usually your
   hub).

4. **Delivery.** BinkD transfers the packet to the next hop. From
   there, it may be forwarded through additional hops until it reaches
   the destination system.

5. **Receipt.** The destination system's tosser places the message in
   their NetArea. The recipient reads it.

The key difference from echomail: netmail is **routed**, not flooded.
Your system decides which system to hand the message to next, and that
system decides the next hop, and so on. The routing decisions are
controlled by `route.cfg`.

---

## NetMail vs. EchoMail — The Quick Refresher {#vs-echomail}

| | NetMail | EchoMail |
|---|---------|----------|
| **Audience** | One specific recipient | Everyone carrying the echo |
| **Addressing** | To a specific `zone:net/node` | To an area tag (e.g., `FSX_GEN`) |
| **Delivery** | Routed hop-by-hop | Flood-fill to all linked nodes |
| **Privacy** | "Mostly private" — not broadcast, but readable in transit | Public by design |
| **Control info** | INTL kludge, no SEEN-BY | SEEN-BY, PATH, origin line |
| **Uses** | Private messages, SqaFix commands, admin coordination | Public discussions, announcements |

---

## The NetArea {#netarea}

```
NetArea  NETMAIL  /var/max/data/mail/netmail
```

Every system must have at least one NetArea defined in `squish.cfg`.
This is where:

- **Inbound** netmail addressed to your system is placed.
- **Outbound** netmail composed by your callers is saved (Maximus
  writes here; Squish scans from here).
- **In-transit** netmail (messages passing through your system to reach
  another destination) is temporarily stored.

You can define multiple NetAreas if needed — Squish scans all of them
when packing outbound mail. But one is sufficient for most systems.

In Maximus, your netmail message area must be defined with `type =
"netmail"` so the BBS knows to prompt for a destination FTN address
when composing:

```toml
[[area]]
name = "NetMail"
tag = "NETMAIL"
type = "netmail"
path = "data/mail/netmail"
```

---

## route.cfg — Routing Control {#route-cfg}

**File:** `config/route.cfg`

This file tells Squish how to route outbound netmail during the
`squash` phase. It determines:

- **Where** each message goes (which system to hand it to)
- **What flavor** (priority) to use for the delivery

Here's a minimal `route.cfg` for a leaf node on fsxNet:

```
; route.cfg — route everything through our hub
Send Crash 21:1/100
HostRoute Crash World
```

That's it. Every outbound message — regardless of destination — gets
sent to your hub (`21:1/100`) with Crash flavor (immediate delivery).
Your hub handles the rest.

The general syntax for routing commands is:

```
<verb> <flavor> <addresses>
```

---

## Routing Verbs {#verbs}

Squish supports several routing verbs, each controlling a different
aspect of mail handling:

### Send

```
Send Crash 21:1/100
Send Hold  21:2/All
Send Normal World
```

**Send** is the most common verb. It tells Squish to place outbound
mail for the specified addresses into the outbound directory with the
given flavor. The mail is addressed directly to the destination node.

Use `Send` when you connect directly to the destination system (e.g.,
your hub, or a system you poll directly).

### Route

```
Route Crash 21:1/100 21:1/All
```

**Route** tells Squish to bundle mail for the listed addresses and send
it *through* a specific host. The first address is the routing host;
everything after it specifies which destinations to route through that
host.

This is how hub routing works: you route all your zone's mail through
your hub.

```
; Route all of net 1 through hub 21:1/100
Route Crash 21:1/100 21:1/All
```

### HostRoute

```
HostRoute Crash World
```

**HostRoute** is a convenience verb. It automatically routes mail to
the appropriate net host (hub) for each destination. Squish looks up
the destination's net and routes through that net's host address
(`net/0`).

For simple setups, `HostRoute Crash World` combined with a `Send` line
for your hub handles everything. For leaf nodes on a single network,
this is usually all you need.

### Leave

```
Leave 1:All
```

**Leave** tells Squish to skip the listed addresses during this
routing pass. Mail for these addresses stays in the outbound area,
unprocessed. This is useful in schedules (see below) when you want to
hold certain mail until a specific time.

### Unleave

```
Unleave 1:All
```

**Unleave** reverses a previous `Leave` command. Used in schedules to
re-enable processing for addresses that were previously held.

### Change

```
Change Hold Crash 2:All
```

**Change** converts mail from one flavor to another. The first flavor
is the source; the second is the target. This is used in schedules to
upgrade or downgrade mail priority.

```
; Change all zone 2 hold mail to crash (time to send!)
Change Hold Crash 2:All
```

### Poll

```
Poll 21:1/100
```

**Poll** creates an empty outbound packet for the listed addresses.
This causes BinkD to connect to those systems even if there's no mail
to send — useful for picking up held mail from your hub.

### Dos (External Command)

```
Dos "echo Mail packed for zone 2"
```

**Dos** executes an external command during routing. Rarely used, but
available for custom scripting.

---

## Mail Flavors {#flavors}

Flavors control how mail is prioritized in the outbound directory.
They originate from the modem era but still have meaning:

| Flavor | Outbound File | Meaning |
|--------|--------------|---------|
| **Normal** | `.OUT` / `.OLO` | Standard priority. Sent during regular sessions. |
| **Crash** | `.CUT` / `.CLO` | High priority. Send immediately — trigger an outbound connection. |
| **Hold** | `.HUT` / `.HLO` | Low priority. Hold until the destination system polls you. |
| **Direct** | `.DUT` / `.DLO` | Send directly to the destination, bypassing routing. |

### Which flavor to use?

For most modern setups over TCP/IP:

- **Crash** for everything you want sent promptly. BinkD treats Crash
  as "connect and send now." This is the standard choice for hub
  connections.
- **Hold** if you want mail to wait until the destination connects to
  you. Useful if you can't reach a system directly.
- **Normal** is the middle ground — sent during scheduled sessions.

The practical difference between Crash and Normal is mostly relevant
for BinkD's polling behavior. If BinkD is running as a daemon and
polling on a schedule, Normal and Crash mail both go out on the next
connection. The distinction matters more if you're triggering BinkD
per-event.

---

## Address Wildcards {#wildcards}

Routing commands support wildcards for matching groups of addresses:

| Pattern | Matches |
|---------|---------|
| `21:1/100` | Exactly node 21:1/100 |
| `21:1/All` | All nodes in net 21:1 |
| `21:All` | All nodes in zone 21 |
| `World` | Every address in every zone |
| `All` | Same as World |

You can also use `Except` to exclude specific addresses:

```
Send Crash 21:1/All Except 21:1/150
```

This sends crash mail to everything in net 1 **except** node 150.

### Processing order

Routing commands are processed top-to-bottom. The **first match wins**.
Put specific rules before general ones:

```
; Specific: hold mail for node 150 (they poll us)
Send Hold 21:1/150

; General: crash everything else through our hub
Send Crash 21:1/100
HostRoute Crash World
```

If you reversed the order, the `HostRoute Crash World` would match
node 150 first, and the Hold rule would never trigger.

---

## Schedules {#schedules}

Schedules let you define named routing configurations that you activate
at specific times. They're invoked with the `-s` command-line switch:

```bash
squish squash -sSendZ2
```

### Defining schedules

```
Sched SendZ2

    ; Hold all zone 1 mail (don't send during this window)
    Leave 1:All

    ; Upgrade zone 2 mail from hold to crash
    Change Hold Crash 2:All

Sched NoSendZ2

    ; Undo the SendZ2 changes
    Unleave 1:All
    Change Crash Hold 2:All
```

### When to use schedules

Schedules are a holdover from the modem era when long-distance calls
were expensive at certain times. You'd hold inter-zone mail during
peak hours and send it during off-peak.

On modern TCP/IP networks, schedules are rarely needed. The main
use case is if you have a specific time window when a remote system
is available, or if you want to batch inter-zone mail for efficiency.

For most sysops on fsxNet or similar modern networks: **you don't need
schedules.** A simple `Send Crash` + `HostRoute Crash World` handles
everything.

---

## HostRoute — The Simple Default {#hostroute}

For a leaf node on a single network, here's the complete `route.cfg`:

```
Send Crash 21:1/100
HostRoute Crash World
```

Line 1: Send mail directly to your hub.
Line 2: Route everything else through net hosts.

Since your hub *is* your net host (21:1/100 = net 1, node 0-equivalent
for routing purposes), line 2 effectively routes everything through
your hub too. But the explicit `Send` on line 1 ensures your hub
connection uses the right flavor.

### Multi-network routing

If you're on multiple networks:

```
; fsxNet hub
Send Crash 21:1/100

; FidoNet hub
Send Crash 1:249/0

; Everything else routes through net hosts
HostRoute Crash World
```

Each network's mail goes through the appropriate hub.

---

## Forwarding NetMail {#forwarding}

By default, Squish doesn't forward netmail that passes through your
system. You need to explicitly enable it in `squish.cfg`:

```
ForwardTo World
```

Or for specific nodes:

```
ForwardTo 21:1/0 21:1/100
ForwardTo File 21:1/100
```

### Why you might forward

- You're a hub feeding leaf nodes — their netmail passes through you.
- You're a gateway between networks.
- You run points that route through your system.

### Why you might not

- You're a leaf node. Your hub does the forwarding; you don't need to.
- Security: forwarding means you're responsible for mail transit.

### Controlling forwarding

| squish.cfg Keyword | Effect |
|-------------------|--------|
| `ForwardTo <nodes>` | Allow forwarding *to* these destinations |
| `ForwardTo File <nodes>` | Also forward file attaches |
| `ForwardFrom <nodes>` | Allow forwarding *from* these sources to anywhere |
| `ForwardFrom File <nodes>` | Also forward file attaches from these sources |
| `KillIntransit` | Delete in-transit netmail after packing (don't keep copies) |
| `StripAttributes` | Strip Crash/Hold from forwarded mail (prevent abuse) |

For a leaf node, none of these are needed. For a hub, `ForwardTo World`
and `StripAttributes` are the minimum.

---

## Inter-Zone Routing {#inter-zone}

Sending netmail between zones (e.g., from fsxNet zone 21 to FidoNet
zone 1) requires special handling. There are two approaches:

### Route through your hub

The simplest method. If your hub supports inter-zone routing (fsxNet
hubs do), just route everything through your hub and let them handle
the zone crossing:

```
Send Crash 21:1/100
HostRoute Crash World
```

Your hub knows how to reach other zones. This is the recommended
approach for leaf nodes.

### GateRoute (direct zone gating)

If you need to gate netmail yourself:

```
GateRoute Normal 1:1/2 2:All
GateRoute Normal 1:1/3 3:All
```

This routes all zone-2 mail through the zone 2 gate at `1:1/2`, and
zone-3 mail through `1:1/3`. GateRoute performs the necessary address
conversions for cross-zone delivery.

**You almost certainly don't need this.** It's for systems that
actively participate in zone gating. Let your hub handle it.

---

## NetMail Tracking {#tracking}

```
Track /var/max/log/msgtrack.log
```

The `Track` keyword in `squish.cfg` creates a separate log of all
netmail that passes through your system — who sent it, who it's
addressed to, and the subject line. This is useful for:

- **Debugging routing** — confirm messages are flowing correctly.
- **Auditing** — know what's transiting through your system.
- **Diagnosing delivery issues** — "did that netmail actually leave?"

The track log must point to a **different file** than Squish's main
`LogFile`. Check it when netmail seems to vanish — it'll tell you
whether Squish processed and forwarded the message.

---

## Common NetMail Scenarios {#scenarios}

### Sending a private message to another sysop

1. Log into your BBS, go to the NetMail area.
2. Compose a message. Maximus asks for the destination address —
   enter `21:1/101` (or whatever their address is).
3. Enter the recipient's name and write your message.
4. Post it.
5. Run `squish out squash` then `binkd -p` (or wait for cron).
6. Your message routes through your hub to the destination.

### Sending SqaFix commands to your hub

1. Go to the NetMail area.
2. Address the message to `SqaFix` at your hub (e.g., `21:1/100`).
3. Set the subject to your session password.
4. In the body, write the command: `+FSX_BBS` to subscribe,
   `-FSX_BBS` to unsubscribe, `%LIST` for available areas.
5. Post and run Squish.

### Receiving a reply

When someone sends netmail to your address, it arrives in your
NetArea the next time you toss incoming mail (`squish in`). Check
your NetMail area for new messages.

---

## Troubleshooting {#troubleshooting}

### NetMail I sent never arrived

1. **Check your NetArea.** Is the message still there? If yes, Squish
   hasn't scanned it yet. Run `squish out squash`.
2. **Check squish.log.** Did Squish process the message? Look for the
   destination address.
3. **Check msgtrack.log.** Did Squish route it? The track log shows
   the next hop.
4. **Check BinkD.** Did it transfer the packet? Check `binkd.log`.
5. **Contact the recipient.** Their system may be down or not polling.

### NetMail I'm expecting hasn't arrived

1. **Poll your hub.** Run `binkd -p` to check for waiting mail.
2. **Check squish.log.** Did Squish toss any netmail on the last run?
3. **Check BadArea.** If the message had a problem (wrong address,
   security rejection), it might be there.
4. **Ask the sender** to verify they addressed it correctly and that
   their system exported it.

### Messages stuck in NetArea

If messages accumulate in your NetArea without being sent:

- **Check route.cfg.** Does a routing rule match the destination? If
  no rule matches, the message isn't processed.
- **Check that `squish squash` is running.** The squash phase is what
  actually creates the outbound files.
- **Check flavors.** If mail is marked Hold but your hub expects Crash,
  BinkD may not send it.

### In-transit mail accumulating

If you're forwarding mail and messages are piling up:

- **Check that the destination is reachable.** BinkD logs will show
  connection failures.
- **Enable `KillIntransit`** if you don't want to keep copies of
  forwarded mail.
- **Check `ForwardTo` settings.** If the destination isn't listed,
  Squish won't forward it.

---

For the full `squish.cfg` reference, see
[Squish Tosser]({{ site.baseurl }}{% link fidonet-squish.md %}). For echomail specifics,
see [Echomail]({{ site.baseurl }}{% link fidonet-echomail.md %}). For general debugging
techniques, see
[Logging & Troubleshooting]({{ site.baseurl }}{% link admin-logging-troubleshooting.md %}).
