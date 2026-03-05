---
layout: default
title: "FidoNet & FTN Networking"
section: "administration"
description: "What FidoNet-Technology Networks are, how mail flows, addressing, and the pieces you need to get connected"
---

Your BBS is up. Callers are logging in. The message areas are working,
the file section looks great, and you've got a nice ANSI welcome screen.
But right now your board is an island — the only messages on it are the
ones your own callers write.

FidoNet-Technology Networking (FTN) changes that. It connects your board
to other boards around the world, sharing messages and files
automatically. Your callers post in a discussion area, and that message
shows up on dozens — maybe hundreds — of other BBSes. Messages from
those boards flow back to yours. It's distributed, asynchronous,
store-and-forward messaging that's been running since 1984 and is still
going strong.

This page explains what FTN is, how it works, and what the pieces are.
No config files yet — just concepts. When you're ready to actually set
it up, the [Joining a Network]({{ site.baseurl }}{% link fidonet-joining-network.md %})
tutorial walks you through the whole process step by step.

---

## On This Page

- [What Is FTN?](#what-is-ftn)
- [It's Not Just FidoNet](#not-just-fidonet)
- [How Addressing Works](#addressing)
- [Two Kinds of Mail](#two-kinds-of-mail)
- [The Software Stack](#software-stack)
- [How Mail Flows](#how-mail-flows)
- [What You Need Before You Start](#before-you-start)

---

## What Is FTN? {#what-is-ftn}

FTN stands for **FidoNet-Technology Network**. It's a set of protocols
and conventions for exchanging messages and files between BBSes. The
original network — FidoNet itself — was created by Tom Jennings in 1984.
The technology proved so useful that dozens of other networks adopted the
same protocols, each with their own communities, topics, and culture.

The core idea is simple: BBSes connect to each other (usually over the
internet these days, originally over phone lines), exchange bundles of
messages, and then disconnect. Each BBS unpacks those messages into its
local message areas so callers can read and reply. Replies get bundled
up and sent back out on the next connection. It's email, but for BBSes,
and it's been working for over 40 years.

---

## It's Not Just FidoNet {#not-just-fidonet}

When people say "FidoNet" they sometimes mean the specific network
(Zone 1–6, governed by the FidoNet organization) and sometimes mean the
technology in general. This documentation uses **FTN** when talking
about the technology and **FidoNet** when talking about the specific
network.

There are many active FTN networks today:

| Network | Zone | Vibe |
|---------|------|------|
| **FidoNet** | 1–6 | The original. Large, established, somewhat formal. |
| **fsxNet** | 21 | Fun, Simple, eXperimental. Friendly community, easy to join, great for your first network. |
| **AgoraNet** | 46 | Small, focused on retro computing and BBS culture. |
| **DoveNet** | — | Synchronet-affiliated, active community. |
| **WhisperNet** | — | Smaller, community-focused. |

You can join multiple networks simultaneously — Maximus and Squish
handle multiple addresses (called AKAs) just fine. But for your first
network, we recommend **fsxNet**. It's designed to be approachable, the
community is welcoming, and setup is straightforward. The
[tutorial]({{ site.baseurl }}{% link fidonet-joining-network.md %}) uses fsxNet as its
example for exactly this reason.

---

## How Addressing Works {#addressing}

Every system on an FTN network has an address in the format:

```
zone:net/node.point
```

Think of it like a postal address:

| Part | What It Means | Example |
|------|--------------|---------|
| **Zone** | The broadest division — like a country. FidoNet uses zones 1–6 (North America, Europe, etc.). fsxNet uses zone 21. | `21` |
| **Net** | A regional grouping within a zone — like a city. Each net has a hub that coordinates mail flow. | `1` |
| **Node** | Your specific system within the net — like a street address. This is what uniquely identifies your BBS. | `128` |
| **Point** | Optional. A sub-system hanging off a node — like an apartment number. Points are rare these days. | `.0` (usually omitted) |

So `21:1/128` means "zone 21 (fsxNet), net 1, node 128." That's a
complete address. When someone wants to send you netmail, that's where
it goes.

Your network coordinator assigns your address when you join. You don't
pick it yourself — you apply, they give you a number in the appropriate
net, and you configure your software to use it.

### AKAs (Also Known As)

If you join multiple networks, your system has multiple addresses. These
are called AKAs. For example, a system might be `21:1/128` on fsxNet
and `1:249/128` on FidoNet. Squish knows about all your addresses and
uses the right one depending on which network a message is for.

---

## Two Kinds of Mail {#two-kinds-of-mail}

FTN carries two fundamentally different kinds of messages:

### EchoMail — Public Conversations

EchoMail is the main attraction for most sysops. It's like a
distributed forum: when someone posts a message in an echo area, that
message gets copied to every BBS that carries that echo. Replies
propagate the same way. Your callers read and post in what feels like
a local message area, but the conversation spans the entire network.

Each echo has a **tag** — a short name like `FSX_GEN` (fsxNet general
discussion) or `FIDO_SYSOP` (FidoNet sysop chat). You subscribe to the
echoes you want, and only those messages flow to your system.

Some networks have just a handful of echoes (fsxNet has about 13).
FidoNet has hundreds. You don't have to carry them all — pick the ones
your callers will enjoy.

### NetMail — Private Messages

NetMail is private, point-to-point messaging between two specific
addresses. Think of it like email: you write a message to a person at a
specific FTN address, and it gets delivered to that system. The
recipient reads it in their netmail area.

NetMail is usually routed through hubs — your message goes to your hub,
which passes it along until it reaches the destination. It's not
encrypted, so "private" means "not broadcast to everyone" rather than
"impossible to read in transit." For sensitive content, treat it like a
postcard: assume the mail carrier *could* read it even though they
probably won't.

NetMail is also how some administrative functions work — area requests,
network coordination messages, and SqaFix commands all travel as
netmail.

---

## The Software Stack {#software-stack}

Getting FTN working involves three pieces of software that each handle
one part of the job. Here's how they fit together:

```
YOUR SYSTEM                                                YOUR HUB
═══════════════════════════════════════════════════         ═══════════
                                                            
  ┌───────────┐    ┌──────────────┐    ┌──────────┐        ┌─────────┐
  │  Maximus  │    │    Squish    │    │  BinkD   │  BinkP │  Hub's  │
  │  (BBS)    │    │   (tosser)   │    │ (mailer) │◀══════▶│  BinkD  │
  └─────┬─────┘    └──┬───────┬──┘    └────┬─────┘  TCP/  └────┬────┘
        │             │       │             │        IP         │
        │             │       │             │               ┌───┴────┐
        ▼             ▼       ▼             ▼               │ Hub's  │
  ┌──────────┐  ┌─────────┐ ┌──────────┐ ┌──────────┐     │ Squish │
  │ Squish   │  │echotoss │ │ outbound/│ │ inbound/ │     └───┬────┘
  │ message  │  │  .log   │ │  (*.pkt  │ │  (*.pkt  │         │
  │ bases    │  │         │ │   *.zip) │ │   *.zip) │         ▼
  │ (*.sqd)  │  └─────────┘ └──────────┘ └──────────┘     other nodes
  └──────────┘                                             in the net

─── OUTBOUND (your caller posts a message) ──────────────────────────

  Caller posts     Maximus         Squish          Squish        BinkD
  in echo area ──▶ writes to ──▶  scans log, ──▶  packs into ──▶ sends
                   msg base &     extracts msg,   archive for    to hub
                   echotoss.log   builds .pkt     hub address

─── INBOUND (a message arrives from the network) ───────────────────

  BinkD           Squish          Squish           Maximus
  receives ──▶   unpacks ──▶    tosses msgs ──▶  caller sees
  archive        archive from    into local       new messages
  from hub       inbound dir     msg bases        in echo area
```

### Maximus — Your BBS

This is where your callers interact with messages. Maximus stores
messages in Squish-format message bases. When a caller posts in an
EchoMail area, Maximus notes it in the **echotoss log** — a signal to
Squish that there's outbound mail to process.

### Squish — The Tosser

Squish is the mail processor. It does three things:

- **Toss** (`squish in`) — Takes incoming packets from your mailer's
  inbound directory, unpacks them, and places messages into the correct
  local message areas.
- **Scan** (`squish out`) — Looks at your message areas for new
  outbound messages (flagged in the echotoss log), extracts them, and
  builds outbound packets.
- **Pack** (`squish squash`) — Compresses outbound packets into
  archives and places them in the outbound directory for your mailer to
  send.

You usually run all three at once: `squish in out squash`.

### BinkD — The Mailer

BinkD handles the actual network connections. It connects to your hub
(or your hub connects to you), transfers packet files in both
directions, and disconnects. BinkD speaks the **BinkP** protocol, which
is the modern standard for FTN transfers over TCP/IP.

BinkD can run as a daemon, polling your hub on a schedule or accepting
incoming connections. It doesn't know or care what's inside the packets
— it just moves files.

> **Historical note:** In the modem era, the mailer was the most complex
> piece — it had to handle phone dialing, modem negotiation, file
> transfer protocols, and scheduling around phone rates. BinkD is
> comparatively simple because TCP/IP handles all of that. You'll still
> see references to "crash mail," "hold mail," and other modem-era
> concepts in the configuration — they still work, they just map to
> priorities instead of phone behavior.

---

## How Mail Flows {#how-mail-flows}

Here's what happens when your caller posts a message in an EchoMail
area, end to end:

1. **Caller posts.** Maximus saves the message to the Squish message
   base and writes the area name to the echotoss log.

2. **Squish scans.** You run `squish out` (or `squish in out squash`).
   Squish reads the echotoss log, finds the new message, extracts it
   into an outbound packet, and adds network control information
   (SEEN-BY lines, PATH lines, origin line).

3. **Squish packs.** `squish squash` compresses the packet into an
   archive and places it in the outbound directory, addressed to your
   hub.

4. **BinkD sends.** BinkD connects to your hub (or waits for your hub
   to connect) and transfers the archive.

5. **The hub distributes.** Your hub's tosser unpacks your archive,
   extracts the message, and re-packs it into archives for every other
   node that carries that echo. Their mailers pick up those archives.

6. **Other BBSes toss.** Each receiving BBS runs their tosser, which
   unpacks the archive and places the message into their local copy of
   the echo area. Their callers can now read it.

The whole cycle typically takes minutes to hours, depending on polling
schedules. It's not instant messaging — it's store-and-forward, and
that's part of the charm.

Incoming mail works the same way in reverse: your hub sends you
archives, BinkD receives them, Squish tosses the messages into your
local areas, and your callers find new messages waiting for them.

---

## What You Need Before You Start {#before-you-start}

Here's the checklist for getting your Maximus BBS onto an FTN network:

- **A working Maximus installation.** Your BBS should be up and
  accepting callers before you add networking. Get the basics solid
  first.

- **A network to join.** We recommend **[fsxNet](https://fsxnet.nz)**
  for your first network — it's friendly, well-run, and designed to be
  easy. FidoNet is also an option if you want the classic experience.

- **A node number.** You get this by applying to the network. For
  fsxNet, it's an email to the coordinator. For FidoNet, you contact
  your local net coordinator.

- **BinkD** installed and configured. This is your mailer — the piece
  that actually talks to other systems over the internet.

- **Squish** configured. This is already part of your Maximus
  installation — it's the tosser/scanner/packer that moves messages
  between your BBS and the network.

- **A cup of coffee and about an hour.** Seriously, that's about how
  long the first setup takes if you follow the tutorial.

### Ready?

The **[Joining a Network]({{ site.baseurl }}{% link fidonet-joining-network.md %})**
tutorial walks you through the entire process using fsxNet as a
real-world example — from applying for your node number to posting your
first test message. It's friendly, it's practical, and it assumes you've
never done this before.

Once you're up and running, the reference pages go deeper:

- [Network Configuration]({{ site.baseurl }}{% link fidonet-configuration.md %}) —
  `matrix.toml` and Maximus-side FTN settings
- [Squish Tosser]({{ site.baseurl }}{% link fidonet-squish.md %}) — full `squish.cfg`
  reference
- [Echomail]({{ site.baseurl }}{% link fidonet-echomail.md %}) — dupe checking, security,
  SEEN-BY, passthru areas
- [Netmail & Routing]({{ site.baseurl }}{% link fidonet-netmail-routing.md %}) — private
  messaging and mail routing
