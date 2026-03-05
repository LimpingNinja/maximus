---
layout: default
title: "Joining a Network"
section: "administration"
description: "A practical, step-by-step tutorial for joining fsxNet — from application to your first posted message"
---

So you read the [overview]({{ site.baseurl }}{% link fidonet.md %}), you know what FTN is,
and now you want to actually *do* it. Good. This page walks you through
the entire process of joining an FTN network and getting messages
flowing to and from your Maximus BBS.

We're using **fsxNet** as our example — it's a real, active network
designed to be fun, simple, and welcoming. Everything here applies to
other FTN networks too (FidoNet, AgoraNet, etc.), just with different
addresses and hub details.

By the end of this page, you'll have:

- A node number on fsxNet
- BinkD connecting to your hub
- Squish tossing and scanning messages
- Maximus displaying echomail areas to your callers
- Your first test message posted to the network

One page. One cup of coffee. Let's go.

---

## On This Page

- [Step 1: Apply for Your Node Number](#apply)
- [Step 2: Set Up Your Directories](#directories)
- [Step 3: Install and Configure BinkD](#binkd)
- [Step 4: Configure Squish](#squish)
- [Step 5: Configure the Maximus Side](#maximus)
- [Step 6: Your First Toss](#first-toss)
- [Step 7: Your First Post](#first-post)
- [Step 8: Adding More Echoes](#more-echoes)
- [Step 9: Automate It](#automate)
- [Where to Go From Here](#next-steps)

---

## Step 1: Apply for Your Node Number {#apply}

Before you configure anything, you need an address on the network. For
fsxNet, this is straightforward:

1. Download the latest infopack from
   [fsxnet.nz](https://fsxnet.nz/fsxnet/start) or grab it from
   `fsxnet.nz/fsxnet.zip`.

2. Open `fsxnet.txt` from the infopack. Near the bottom you'll find
   the application form.

3. Fill it out and email it to **avon@bbs.nz**. The key fields:

   | Field | What to Put |
   |-------|-------------|
   | BBS / System Name | Your board's name |
   | BBS Software | Maximus |
   | Binkp Server Address | Your BBS's hostname or IP |
   | Binkp Port | `24554` (the standard) |
   | CRASH or HOLD | See below |
   | Areafix/Filefix/BinkP Password | Up to 8 chars, uppercase A–Z and 0–9 |

4. Wait for a reply. You'll get back:
   - Your **node number** (e.g., `21:1/200`)
   - Your **hub assignment** (e.g., `21:1/100` at `net1.fsxnet.nz:24554`)
   - A **session password** for BinkD

### CRASH vs. HOLD

This tells your hub how to deliver mail to you:

- **CRASH** — The hub pushes mail to your BBS as soon as it arrives.
  Your BinkD needs to be listening for incoming connections. This is
  faster but requires an open port.
- **HOLD** — The hub holds mail until your BBS polls to collect it.
  Your BinkD connects outbound on a schedule. This works behind
  firewalls and NAT without port forwarding.

If you're not sure, choose **HOLD**. It's simpler and works everywhere.
You can always switch to CRASH later.

> **While you wait** for your node number, you can still set up
> everything else. fsxNet offers a test address (`21:1/999`) with
> access to a few echoes (`FSX_TST`, `FSX_GEN`, `FSX_MYS`), so you can
> even start testing before your real address arrives.

---

## Step 2: Set Up Your Directories {#directories}

FTN mail processing needs a few directories that may not exist yet.
Relative to your Maximus `sys_path`:

```bash
mkdir -p data/mail/inbound
mkdir -p data/mail/outbound
mkdir -p data/mail/netmail
mkdir -p data/mail/bad
mkdir -p data/mail/dupes
mkdir -p data/nodelist
```

| Directory | Purpose |
|-----------|---------|
| `data/mail/inbound` | Where BinkD drops incoming packets and archives |
| `data/mail/outbound` | Where Squish places outbound archives for BinkD to send |
| `data/mail/netmail` | Your NetMail message area (private FTN messages) |
| `data/mail/bad` | Where Squish puts messages it can't toss (bad area, security rejection) |
| `data/mail/dupes` | Where Squish puts duplicate messages |
| `data/nodelist` | Nodelist files for address lookups |

These paths match what's already in the default `maximus.toml`:

```toml
net_info_path = "data/nodelist"
outbound_path = "data/mail/outbound"
inbound_path = "data/mail/inbound"
```

If your `maximus.toml` uses different paths, adjust accordingly.

---

## Step 3: Install and Configure BinkD {#binkd}

BinkD is the mailer — the piece that actually transfers files between
your system and the network. It's a separate program from Maximus.

### Installing BinkD

On most Linux distributions:

```bash
# Debian / Ubuntu
sudo apt install binkd

# Fedora
sudo dnf install binkd

# From source (if not in your package manager)
# See https://github.com/pgul/binkd
```

On macOS, you'll likely need to build from source or use a port.

### Minimal binkd.cfg

Create a BinkD configuration file. Here's a minimal working config for
fsxNet — replace the placeholders with your actual details:

```
# binkd.cfg — fsxNet configuration for Maximus

# Your system info
sysname   "Your BBS Name"
location  "Your City, Country"
sysop     "Your Name"

# Logging
log       /var/max/log/binkd.log
loglevel  4

# Your FTN address
address   21:1/200@fsxnet

# Where BinkD puts/finds files
inbound         /var/max/data/mail/inbound
temp-inbound    /var/max/data/mail/inbound/temp
outbound        /var/max/data/mail/outbound

# Create temp-inbound if it doesn't exist
try-create-directories yes

# Your hub
node 21:1/100@fsxnet net1.fsxnet.nz:24554 YOUR_PASSWORD
```

That's it for a HOLD setup. BinkD will connect to your hub when you run
it, transfer any waiting files, and exit.

For a **CRASH** setup (hub pushes to you), add a listening port:

```
# Listen for incoming connections
iport     24554
```

### Testing the Connection

Run BinkD manually to see if it connects:

```bash
binkd -p /path/to/binkd.cfg
```

The `-p` flag tells BinkD to poll and exit (one-shot mode). You should
see it connect to your hub, possibly transfer some files, and
disconnect. If you see `session with 21:1/100@fsxnet` in the output,
you're in business.

Don't worry if there are no files to transfer yet — that's normal before
you've configured Squish.

---

## Step 4: Configure Squish {#squish}

Squish is the tosser — it takes the raw packets from BinkD and sorts
messages into the right areas (and vice versa). Squish ships with
Maximus, so it's already installed.

### squish.cfg

Edit `config/squish.cfg` (or create it). Here's a minimal config for
fsxNet:

```
; squish.cfg — fsxNet configuration

; Your fsxNet address
Address 21:1/200

; Where BinkD puts incoming mail
NetFile         /var/max/data/mail/inbound

; Where to put outbound archives
Outbound        /var/max/data/mail/outbound

; Log file
LogFile         /var/max/log/squish.log
LogLevel        6

; Compression config
Compress        /var/max/config/compress.cfg

; Routing control
Routing         /var/max/config/route.cfg

; Security — reject mail from nodes not listed for an area
Secure

; Check zone numbers on incoming packets
CheckZones

; Dupe checking
Duplicates      1000
DupeCheck       Header MSGID
DupeLongHeader

; Suppress archiver screen output
QuietArc

; Keep SEEN-BY and PATH info
SaveControlInfo

; Use large buffers (plenty of RAM these days)
Buffers         Large

; Required areas — every system needs these
NetArea         NETMAIL     /var/max/data/mail/netmail
BadArea         BAD_MSGS    /var/max/data/mail/bad
DupeArea        DUPES       /var/max/data/mail/dupes

; fsxNet echo areas — start with just a couple
EchoArea  FSX_TST   /var/max/data/msgbase/fsx_tst   -$   21:1/100
EchoArea  FSX_GEN   /var/max/data/msgbase/fsx_gen   -$   21:1/100
```

Let's break down the key parts:

- **`Address`** — Your fsxNet node number.
- **`NetFile`** — Where Squish looks for incoming packets (same as
  BinkD's `inbound`).
- **`Outbound`** — Where Squish places outbound mail (same as BinkD's
  `outbound`).
- **`NetArea`** — Your netmail area. Required.
- **`BadArea`** — Where rejected messages go. Required.
- **`EchoArea`** lines — Each echo you carry. The format is:
  `EchoArea <TAG> <path> <flags> <nodes>`
  - `-$` means use Squish message format (not ancient *.MSG)
  - `21:1/100` is your hub — the node you exchange this echo with

### route.cfg

Create a simple `config/route.cfg`:

```
; route.cfg — Route all mail through our fsxNet hub
Send Crash 21:1/100
HostRoute Crash World
```

This tells Squish: send everything through your hub. That's all you need
for a single-network setup.

### compress.cfg

The default `compress.cfg` that ships with Maximus already has
definitions for ZIP, ARC, LHarc, and ARJ with UNIX paths. You shouldn't
need to change it. Just make sure the path in your `squish.cfg`
`Compress` line points to the right file.

### Create the message base directories

Squish needs the directories for your echo areas to exist:

```bash
mkdir -p /var/max/data/msgbase
```

Squish will create the actual `.sqd`, `.sqi`, and `.sql` files
automatically the first time it tosses messages into an area.

---

## Step 5: Configure the Maximus Side {#maximus}

Now you need to tell Maximus about your FTN setup and the new message
areas.

### matrix.toml

Edit `config/matrix.toml`. This file controls Maximus's FidoNet-aware
behavior:

```toml
# Echotoss log — Maximus writes area names here when callers post echomail
echotoss_name = "log/echotoss.log"

# Log echomail activity
log_echomail = true

# Errorlevel exits (legacy — used if running Maximus from a batch loop)
after_edit_exit = 11
after_echomail_exit = 12
after_local_exit = 0

# Nodelist settings
nodelist_version = "7"
fidouser = "data/nodelist/fidouser.lst"
```

The most important setting is **`echotoss_name`** — this is how Maximus
tells Squish which areas have new outbound messages. When a caller posts
in an echomail area, Maximus writes the area tag to this file. When you
run `squish out`, Squish reads it to know what to scan.

### Message area definitions

Add your fsxNet echoes to `config/areas/msg/areas.toml`. The exact
format depends on your existing area configuration, but the key fields
for each echo area are:

```toml
[[area]]
name = "fsxNet General"
tag = "FSX_GEN"
type = "echo"
path = "data/msgbase/fsx_gen"
origin = "Your BBS Name - yourbbs.example.com"

[[area]]
name = "fsxNet Test"
tag = "FSX_TST"
type = "echo"
path = "data/msgbase/fsx_tst"
origin = "Your BBS Name - yourbbs.example.com"

[[area]]
name = "fsxNet NetMail"
tag = "NETMAIL"
type = "netmail"
path = "data/mail/netmail"
```

The **`origin`** line appears at the bottom of every echomail message
your callers post. It's your board's signature on the network — usually
your BBS name and address. Your FTN address gets appended automatically.

You can also add these areas through **MaxCFG** if you prefer a menu
interface — go to **Areas → Message Areas** and add each echo there.

For full details on area definitions, see
[MaxCFG Areas]({{ site.baseurl }}{% link maxcfg-areas.md %}).

---

## Step 6: Your First Toss {#first-toss}

Everything's configured. Time for the magic moment.

### Poll your hub

```bash
binkd -p /path/to/binkd.cfg
```

If your hub has mail waiting for you (and it probably does — fsxNet
echoes are active), you'll see files being transferred into your inbound
directory.

### Toss, scan, and pack

```bash
squish in out squash
```

Watch the output. You should see Squish:

1. Find and decompress archives from your inbound directory
2. Toss messages into your echo areas (`FSX_GEN`, `FSX_TST`, etc.)
3. Report how many messages were placed in each area

```
Tossing packet: 00010064.pkt
  FSX_GEN: 14 msgs
  FSX_TST: 3 msgs
Done.
```

### Check the message bases

Log into your BBS (locally or via telnet) and navigate to your fsxNet
message areas. You should see messages from other sysops around the
world. Read a few. Enjoy the moment — your board is no longer an island.

If you see messages, **congratulations** — the hard part is done.

If you don't see messages, check:
- Did BinkD actually transfer files? Check `binkd.log`.
- Did Squish find the packets? Check `squish.log`.
- Are the area paths in `squish.cfg` and `areas.toml` pointing to the
  same locations?
- See [Logging & Troubleshooting]({{ site.baseurl }}{% link admin-logging-troubleshooting.md %})
  for general debugging guidance.

---

## Step 7: Your First Post {#first-post}

Now let's send a message *out* to the network.

1. Log into your BBS.
2. Go to the **FSX_TST** area (the test echo — that's what it's for).
3. Write a message. Something like "Testing from [your BBS name] on
   Maximus!" works great.
4. Post it.

Maximus writes the area name to the echotoss log. Now export it:

```bash
squish out squash
```

Squish finds the new message, builds a packet, compresses it, and places
it in the outbound directory. Now send it:

```bash
binkd -p /path/to/binkd.cfg
```

BinkD connects to your hub and delivers the archive. Within minutes to
hours (depending on polling schedules across the network), your message
will appear on other BBSes carrying `FSX_TST`.

**You're on the network.**

> **Tip:** Some sysops on fsxNet run automated test-response bots. If
> you post a message with the subject "Test" in `FSX_TST`, you may get
> an automated reply confirming your message made it through. It's a
> nice way to verify the round trip works.

---

## Step 8: Adding More Echoes {#more-echoes}

fsxNet has about 13 echo areas. You started with two — now you probably
want more. Here are the available echoes:

| Tag | Topic |
|-----|-------|
| `FSX_GEN` | General discussion — the main hangout |
| `FSX_BBS` | BBS software, development, and support |
| `FSX_MYS` | Mystic BBS discussions |
| `FSX_RETRO` | Retro computing and old tech |
| `FSX_GAMING` | Games and gaming |
| `FSX_VIDEO` | Movies, TV, music |
| `FSX_HAM` | Amateur radio |
| `FSX_TST` | Test messages and experimentation |
| `FSX_NET` | fsxNet admin and announcements |
| `FSX_CRY` | Cryptography discussion |
| `FSX_ADS` | BBS ads and ANSI art |
| `FSX_BOT` | Automated bot posts |
| `FSX_DAT` | InterBBS data (hide from users) |

To add a new echo:

1. **Add it to `squish.cfg`:**
   ```
   EchoArea  FSX_BBS   /var/max/data/msgbase/fsx_bbs   -$   21:1/100
   ```

2. **Add it to your Maximus area definitions** (`areas.toml` or
   MaxCFG).

3. **Tell your hub** you want to receive it. You can do this by sending
   a netmail to **SqaFix** at your hub's address (`21:1/100`) with the
   subject set to your session password and the message body:
   ```
   +FSX_BBS
   ```
   Or simply email your hub admin and ask them to link the area.

4. **Run Squish** to pick up the new messages:
   ```bash
   binkd -p /path/to/binkd.cfg
   squish in out squash
   ```

For a full list of SqaFix commands, see
[SqaFix]({{ site.baseurl }}{% link sqafix.md %}) in the Tools section.

---

## Step 9: Automate It {#automate}

You don't want to run BinkD and Squish by hand every time. Set up a
cron job to poll automatically.

### Simple cron approach

Create a script at `/usr/local/bin/ftn-poll.sh`:

```bash
#!/bin/bash
# ftn-poll.sh — Poll hub and process mail

BINKD_CFG="/var/max/config/binkd.cfg"
SQUISH_CFG="/var/max/config/squish.cfg"
LOCK="/var/max/run/ftn-poll.lock"

# Prevent overlapping runs
if [ -f "$LOCK" ]; then
    exit 0
fi
touch "$LOCK"
trap "rm -f $LOCK" EXIT

# Poll the hub
binkd -p "$BINKD_CFG" 2>>/var/max/log/binkd.log

# Toss incoming, scan outgoing, pack
squish in out squash -c"$SQUISH_CFG" >>/var/max/log/squish.log 2>&1
```

Add it to crontab:

```bash
# Poll every 30 minutes
*/30 * * * * /usr/local/bin/ftn-poll.sh
```

### BinkD as a daemon

If you chose CRASH mode (hub pushes to you), you'll want BinkD running
continuously:

```bash
binkd /path/to/binkd.cfg
```

Without the `-p` flag, BinkD stays running and accepts incoming
connections. You can run it under systemd, in a screen session, or
however you prefer to manage daemons. When BinkD receives files, you
still need Squish to toss them — either trigger it from BinkD's
`exec` directive or run Squish on a short cron interval.

### How often to poll?

fsxNet asks that you poll **at least once daily**, and recommends
**hourly** or more. Every 15–30 minutes is a sweet spot — frequent
enough that conversations feel lively, not so frequent that you're
hammering the hub. The hub won't mind if you poll more often, but
there's diminishing returns.

---

## Where to Go From Here {#next-steps}

You're connected. Messages are flowing. Your callers are reading
echomail from sysops around the world and posting replies. The basics
are working.

Here's where to go to learn more:

- **[Network Configuration]({{ site.baseurl }}{% link fidonet-configuration.md %})** —
  deep dive into `matrix.toml`, privilege levels, nodelist setup, and
  Maximus-side FTN settings.
- **[Squish Tosser]({{ site.baseurl }}{% link fidonet-squish.md %})** — the full
  `squish.cfg` reference. Every keyword, every flag, every option.
- **[Echomail]({{ site.baseurl }}{% link fidonet-echomail.md %})** — dupe checking,
  security, SEEN-BY and PATH lines, passthru areas, and automated area
  management with SqaFix.
- **[Netmail & Routing]({{ site.baseurl }}{% link fidonet-netmail-routing.md %})** —
  private messaging, `route.cfg`, mail flavors, schedules, and
  forwarding.
- **[Squish Utilities]({{ site.baseurl }}{% link squish.md %})** — SQPACK, SQFIX, and
  other tools for maintaining your message bases.
- **[SqaFix]({{ site.baseurl }}{% link sqafix.md %})** — automated area management
  via netmail commands.

### Joining additional networks

The process is the same for any FTN network — apply, get an address,
configure BinkD and Squish. To add a second network:

1. Add the new address as an AKA in `squish.cfg`:
   ```
   Address 21:1/200
   Address 1:249/128
   ```
2. Add a new `node` line in `binkd.cfg` for the new hub.
3. Add `EchoArea` lines for the new echoes.
4. Add area definitions in Maximus.

Squish and BinkD handle multiple networks seamlessly. Your callers won't
even know (or care) which network a particular echo comes from — it all
just looks like message areas on your board.

### Getting help

If you run into trouble:

- Post in **FSX_BBS** on fsxNet — it's full of sysops who've been
  through this and are happy to help.
- Check the [Logging & Troubleshooting]({{ site.baseurl }}{% link admin-logging-troubleshooting.md %})
  page for general debugging techniques.
- Email the fsxNet coordinator at **avon@bbs.nz** for
  network-specific issues.

Welcome to the network.
