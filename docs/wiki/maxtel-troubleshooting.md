---
layout: default
section: "administration"
title: "Troubleshooting"
description: "Troubleshooting MaxTel — common issues and log files"
permalink: /maxtel-troubleshooting/
---

Most MaxTel problems come down to a handful of common causes: port conflicts,
path issues, or Maximus itself not starting cleanly. This page covers the
usual suspects and how to track down anything unusual.

---

## Common Issues

### "Failed to bind to port"

MaxTel can't listen on the port you specified. The usual reasons:

- **Another process is already using it.** Check with
  `netstat -an | grep LISTEN` or `ss -tlnp`.
- **Ports below 1024 need root.** Either run as root, use `sudo`, or pick a
  higher port like 2323.
- **Firewall is blocking it.** Make sure your firewall rules allow inbound
  TCP on the port.

---

### Nodes Stuck in "Starting"

The Maximus binary is failing to launch. Things to check:

- **Is the path correct?** Verify the `-m` flag (or the default `./bin/max`)
  points to an actual executable.
- **Does it run on its own?** Try `./bin/max -l` from the Maximus base
  directory. If Maximus itself crashes, MaxTel can't help it.
- **Check `maxtel.log`** — it records the exact command line used to spawn
  each node, plus any errors that come back.

---

### Users Connect but See Nothing

The telnet connection succeeds but the caller gets a blank screen:

- **PTY allocation failed.** This can happen on systems with restrictive
  `/dev/pts` settings or in containers without a proper devpts mount.
- **Maximus isn't configured for telnet.** Make sure your Maximus config
  is set up for socket/telnet operation, not just local mode.
- **Permissions.** The `max` binary needs execute permission, and the
  per-node directories (`m/1`, `m/2`, etc.) need to be writable.

---

### Interface Displays Incorrectly

The dashboard looks garbled or panels are misaligned:

- **Terminal size detection.** Some terminals don't report their size
  correctly. Use `-s COLSxROWS` to force a specific size
  (e.g., `maxtel -s 132x60`).
- **Terminal compatibility.** MaxTel needs a terminal that supports standard
  ANSI escape sequences and ncurses. Most modern terminals work fine, but
  very minimal ones (like a raw serial console) may not.

---

### Nodes Restart Continuously

A node spawns, crashes almost immediately, respawns, crashes again:

- **Maximus is crashing on startup.** Run `max` manually with the same
  options to see the error output directly.
- **Configuration problem.** A missing or corrupt TOML config file can cause
  Maximus to exit immediately. Check `maxtel.log` for the specific error.
- **Missing per-node directory.** Each node needs its own directory under
  `m/` (e.g., `m/1`, `m/2`). If these don't exist, create them.

---

## Log Files

MaxTel writes to `maxtel.log` in the working directory (or the directory you
specified with `-d`). The log captures:

- Startup and shutdown messages
- Node spawn and termination events (including the exact command line)
- Connection attempts and node assignments
- Errors and warnings

### Watching the Log Live

```bash
tail -f maxtel.log
```

This is especially useful when running in headless mode where you don't have
the dashboard.

### Log Rotation

MaxTel overwrites `maxtel.log` each time it starts. If you want to preserve
logs across restarts, redirect output through `tee`:

```bash
maxtel -H 2>&1 | tee -a maxtel-$(date +%Y%m%d).log
```

Or if you're using systemd, `journalctl` handles log retention for you
automatically.

---

## See Also

- [Logging & Troubleshooting]({% link admin-logging-troubleshooting.md %})
  — the full logging and troubleshooting guide, including log symbols,
  rotation, and a general troubleshooting playbook
- [MaxTel]({% link maxtel.md %}) — overview, features, and getting started
- [Running MaxTel]({% link maxtel-running.md %}) — command-line options and
  operating modes
- [The Dashboard]({% link maxtel-dashboard.md %}) — UI panels and responsive
  layouts
- [Sysop Features]({% link maxtel-sysop-features.md %}) — snoop, config,
  kick, and restart
