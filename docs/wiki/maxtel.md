---
layout: default
section: "administration"
title: "MaxTel"
description: "MaxTel — the telnet supervisor for your Maximus BBS"
permalink: /maxtel/
---

MaxTel is the telnet front door for your Maximus BBS. It listens for incoming
connections, assigns callers to available nodes, and gives you a live dashboard
to watch everything happen. If you've run a BBS before, think of it as
the modern equivalent of FrontDoor or BinkleyTerm — except it speaks TCP/IP
instead of dialing phone numbers.

---

## What MaxTel Does

When a caller telnets to your BBS, MaxTel handles the connection from start to
finish:

1. Accepts the TCP connection and negotiates telnet options (terminal type,
   ANSI support)
2. Finds an available node and spawns a Maximus instance on a PTY
3. Bridges the caller's socket to that PTY so they get a full BBS session
4. Tracks the session — who's online, how long, what they're doing
5. Cleans up when the caller disconnects or the session ends

You can run up to 32 nodes simultaneously. MaxTel manages all of them.

---

## Features at a Glance

- **Multi-node management** — 1 to 32 simultaneous BBS sessions
- **Live dashboard** — ncurses interface showing nodes, callers, stats, and
  user details in real time
- **Automatic node assignment** — callers get routed to the next free node;
  if all are busy, they get a polite "try again later"
- **Snoop mode** — watch (and interact with) any active session live
- **Inline configuration** — launch the MaxCFG editor without leaving MaxTel
- **Headless & daemon modes** — run without a UI for servers, systemd, or
  startup scripts
- **Responsive layout** — the dashboard adapts from 80×25 all the way up to
  132×60+ terminals
- **Telnet negotiation** — handles terminal type detection and ANSI capability
  checks automatically

---

## System Requirements

- **OS** — Linux, FreeBSD, or macOS. Any modern Unix-like system should work.
- **Maximus** — a working Maximus installation. Make sure `max` runs correctly
  in local mode before pointing MaxTel at it.
- **ncurses** — required for building (and for interactive mode at runtime).
  Most systems include it by default.
- **Network** — the ability to bind a TCP port. The default is 2323; ports
  below 1024 need root.

---

## Getting Started

Before your first launch, make sure a few things are in order:

1. **Maximus runs locally** — try `./bin/max -l` to confirm it works on its
   own. If it doesn't, fix that first — MaxTel spawns the same binary.
2. **Config is in place** — your TOML config files should be under `config/`.
3. **Port is available** — pick a port that isn't already in use. Check with
   `netstat -an | grep LISTEN` or `ss -tlnp`.
4. **Firewall allows it** — if you're on a VPS or behind a firewall, open the
   port.

Then start MaxTel:

```bash
cd /path/to/maximus
./bin/maxtel -p 2323 -n 4
```

That gives you 4 nodes on port 2323. You'll see the dashboard come up
immediately. If you'd rather run headless, add `-H` — see
[Running MaxTel]({{ site.baseurl }}{% link maxtel-running.md %}) for all the options.

### Directory Layout

MaxTel expects these paths relative to the base Maximus directory (override
the base with `-d`):

| Path | Purpose |
|------|---------|
| `bin/max` | The Maximus binary (override with `-m`) |
| `config/` | TOML configuration files |
| `etc/callers.dat` | Caller history log |
| `etc/stats.dat` | System statistics |
| `m/1`, `m/2`, … | Per-node working directories |

---

## Where to Go Next

- **[Running MaxTel]({{ site.baseurl }}{% link maxtel-running.md %})** — command-line options,
  interactive mode, headless mode, and daemon mode
- **[The Dashboard]({{ site.baseurl }}{% link maxtel-dashboard.md %})** — understanding the UI
  panels and responsive layouts
- **[Sysop Features]({{ site.baseurl }}{% link maxtel-sysop-features.md %})** — snoop mode,
  inline config editor, kicking and restarting nodes
- **[Troubleshooting]({{ site.baseurl }}{% link maxtel-troubleshooting.md %})** — common issues
  and log files
- **[Building]({{ site.baseurl }}{% link building.md %})** — compiling Maximus and MaxTel from
  source
