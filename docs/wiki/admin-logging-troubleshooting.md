---
layout: default
title: "Logging & Troubleshooting"
section: "administration"
description: "Log files, verbosity levels, line symbols, rotation, and a practical troubleshooting playbook for when things go sideways"
---

Logs are your board's flight recorder. When everything's working, you
barely look at them. When something breaks — and it will — they're the
first thing you reach for. A good sysop doesn't need to read every line,
but they know where the logs are, what the symbols mean, and how to spot
the pattern that explains why node 3 keeps crashing at midnight.

This page covers the logging system in Maximus and MaxTel, how to read
and manage your log files, and a practical troubleshooting playbook for
the most common issues you'll run into.

---

## On This Page

- [Log Files & Locations](#log-files)
- [Log Verbosity](#verbosity)
- [Reading the Log](#reading)
- [Line Symbols](#symbols)
- [What to Look For](#what-to-look-for)
- [Log Rotation](#rotation)
- [Troubleshooting Playbook](#troubleshooting)
- [Common Issues](#common-issues)
- [MEX Script Logging](#mex-logging)

---

## Log Files & Locations {#log-files}

Maximus produces two main log files:

| Log | Default Location | Written By | Contains |
|-----|-----------------|------------|----------|
| **System log** | `log/max.log` | Maximus (per-node) | Session events, logins, errors, area changes, file transfers |
| **MaxTel log** | `maxtel.log` (working dir) | MaxTel | Node spawning, connections, assignments, supervisor events |

There's also a **startup log** (`max_startup.log` in the working
directory) that captures early initialization messages before the main
log file is open. If Maximus fails to start at all — before it even gets
to the point of opening its log — check this file.

### Configuring the System Log

The system log path is set in `maximus.toml`:

```toml
log_file = "log/max.log"
log_mode = "Trace"
```

The path is relative to `sys_path`. Set `log_file` to an empty string to
disable logging entirely (not recommended — you're flying blind without
it).

In a multi-node setup, each node writes to the same log file. Entries
include a node identifier so you can tell which node generated each line.

---

## Log Verbosity {#verbosity}

The `log_mode` setting in `maximus.toml` controls how much detail goes
into the log. Three named levels are available:

| Mode | What Gets Logged |
|------|-----------------|
| **Terse** | Critical events only — logins, logoffs, fatal errors |
| **Verbose** | Terse plus area changes, file transfers, menu navigation |
| **Trace** | Everything — individual commands, detailed state changes, debug info |

**Recommendation:** Run `Trace` unless your disk space is very tight.
The extra detail is invaluable when troubleshooting, and the log files
compress extremely well. You can always switch to `Verbose` or `Terse`
later if log size becomes an issue.

You can change the log mode in MaxCFG under **Setup → System Settings**
(the "Log Mode" toggle) or by editing `maximus.toml` directly. A restart
is required for the change to take effect.

For the full configuration reference, see
[Core Settings]({{ site.baseurl }}{% link config-core-settings.md %}).

---

## Reading the Log {#reading}

A typical log line looks like this:

```
+ 15 Jan 14:23:07 Max Hello World logged on at 38400 bps
```

The format is:

```
<symbol> <day> <month> <time> <node> <message>
```

| Field | Meaning |
|-------|---------|
| **Symbol** | A single character indicating the type/severity of the entry (see [below](#symbols)) |
| **Day Month** | Date of the entry (e.g., `15 Jan`) |
| **Time** | `HH:MM:SS` in local time |
| **Node** | Short node identifier (e.g., `Max` for single-node, or a task-specific abbreviation) |
| **Message** | The actual log content |

### Watching Logs Live

The most useful thing you can do while debugging is watch the log in
real time:

```bash
tail -f /var/max/log/max.log
```

Or if you want to watch both Maximus and MaxTel simultaneously:

```bash
tail -f /var/max/log/max.log /var/max/maxtel.log
```

This is especially useful when running MaxTel in headless mode where you
don't have the dashboard.

---

## Line Symbols {#symbols}

Every log line starts with a single-character symbol that tells you what
kind of entry it is. Learning to scan for these symbols is the fastest
way to find what you're looking for in a busy log.

| Symbol | Meaning | When It Appears |
|--------|---------|----------------|
| `!` | **Error / Critical** | Always logged regardless of log mode. Something went wrong. |
| `+` | **Major event** | Logins, logoffs, key session milestones. Appears at Terse and above. |
| `=` | **Standard activity** | Area changes, file operations. Appears at Verbose and above. |
| `:` | **Detail** | State transitions, internal checkpoints. Appears at Trace and above. |
| `~` | **Fine detail** | Individual operations, granular state. High verbosity only. |
| `#` | **Trace** | Very detailed tracing. High verbosity only. |
| `$` | **Deep trace** | The most verbose level of normal logging. |
| ` ` (space) | **Suppressed** | Almost never logged — only at the highest possible verbosity. |

### What This Means in Practice

- **Scanning for problems:** Look for `!` lines. These are errors and
  they're always logged. If something broke, there's usually a `!` line
  that says what.
- **Understanding a session:** Follow the `+` lines to see the flow —
  login, area changes, logoff.
- **Debugging specific behavior:** Switch to `Trace` mode and look at
  the `:` and `~` lines around the time of the issue.

Example — a normal session looks like:

```
+ 15 Jan 14:23:07 Max Hello World logged on at 38400 bps
= 15 Jan 14:23:12 Max Changed to message area: Local Chat
= 15 Jan 14:23:45 Max Read message #142
= 15 Jan 14:24:01 Max Changed to file area: Uploads
+ 15 Jan 14:25:30 Max Hello World logged off (normal)
```

A session with a problem:

```
+ 15 Jan 14:23:07 Max Hello World logged on at 38400 bps
= 15 Jan 14:23:12 Max Changed to message area: Local Chat
! 15 Jan 14:23:15 Max Error opening Squish base: local.sqd
! 15 Jan 14:23:15 Max File not found: /var/max/data/msgbase/local.sqd
+ 15 Jan 14:23:15 Max Hello World logged off (error)
```

---

## What to Look For {#what-to-look-for}

### Error Patterns

| Pattern | What It Usually Means |
|---------|-----------------------|
| `Error opening` | A file or database is missing, locked, or has bad permissions |
| `FATAL` | Something unrecoverable — the node is going down |
| `not found` | A path in your config points to something that doesn't exist |
| `permission denied` | File ownership or mode issue — check `chmod`/`chown` |
| `syscrash` | A node crashed previously and left stale state files |
| `MsgOpenApi` | The Squish message API failed to initialize |

### Timing Patterns

Sometimes the log entry itself is less informative than *when* it
appears:

- **Same error repeating every few seconds** — A node is crash-looping.
  Check MaxTel's log for the spawn command and try running Maximus
  manually.
- **Errors clustered around a specific time** — Check if a scheduled
  event (log rotation, backup, cron job) is interfering.
- **Errors only on one node** — Likely a per-node directory issue or a
  corrupted node state file. Check `run/node/<N>/`.

---

## Log Rotation {#rotation}

Logs grow forever if you let them. On an active multi-node board with
`Trace` logging, `max.log` can grow by several megabytes per day. You
need a rotation strategy.

### Using logrotate (Linux)

Create `/etc/logrotate.d/maximus`:

```
/var/max/log/max.log {
    daily
    rotate 14
    compress
    delaycompress
    missingok
    notifempty
    copytruncate
}
```

`copytruncate` is important — it copies the log and then truncates the
original in place, so Maximus doesn't need to be restarted or signaled
to reopen the file.

### Using newsyslog (macOS / FreeBSD)

Add to `/etc/newsyslog.conf`:

```
/var/max/log/max.log    644  14  *  @T00  JC
```

This rotates daily at midnight, keeps 14 compressed archives, and uses
the `copytruncate` equivalent (`C` flag).

### Manual Rotation Script

If you don't have `logrotate` or `newsyslog`:

```bash
#!/bin/bash
# rotate-logs.sh — Simple log rotation for Maximus
LOG="/var/max/log/max.log"
ARCHIVE="/var/max/log/archive"
DATE=$(date +%Y-%m-%d)

mkdir -p "${ARCHIVE}"

# Copy and truncate (no restart needed)
cp "${LOG}" "${ARCHIVE}/max-${DATE}.log"
> "${LOG}"

# Compress yesterday's log
gzip -f "${ARCHIVE}/max-$(date -d '-1 day' +%Y-%m-%d).log" 2>/dev/null

# Purge logs older than 30 days
find "${ARCHIVE}" -name "max-*.log.gz" -mtime +30 -delete
```

### MaxTel Log Rotation

MaxTel overwrites `maxtel.log` each time it starts. If you want to
preserve logs across restarts, redirect through `tee`:

```bash
maxtel -H 2>&1 | tee -a maxtel-$(date +%Y%m%d).log
```

Or if you're using systemd, `journalctl` handles retention for you
automatically.

---

## Troubleshooting Playbook {#troubleshooting}

When something goes wrong, resist the urge to start changing things.
Follow this sequence:

### 1. Check the Logs

Always start here. Open `max.log` and look for `!` lines around the
time of the problem:

```bash
grep '^!' /var/max/log/max.log | tail -20
```

If the problem is with MaxTel itself (nodes not spawning, connections
failing), check `maxtel.log` too.

If Maximus didn't even get far enough to write to its log, check
`max_startup.log` in the working directory.

### 2. Check Recent Changes

If it worked yesterday and doesn't today, what changed?

- **Config edits** — Did you save a TOML file recently? Check
  `git diff` if you're using version control on your config directory
  (and you should be — see
  [Backup & Recovery]({{ site.baseurl }}{% link admin-backup-recovery.md %})).
- **New MEX scripts** — A buggy script can crash a node.
- **OS updates** — Library changes, permission changes, firewall rule
  resets.
- **Disk space** — A full disk causes all sorts of mysterious failures.
  Check with `df -h`.

### 3. Reproduce It

Can you make it happen again? Try to identify:

- Does it happen for all users or just one?
- Does it happen on all nodes or just one?
- Does it happen in all areas or just one?
- Does it happen at a specific time or randomly?

Consistent bugs are easy to fix. Intermittent ones take patience.

### 4. Isolate It

Narrow the scope:

- **One node:** Check `run/node/<N>/` for stale files. Try deleting
  the node's state files and restarting.
- **One area:** Check the Squish base files for that area. Try
  `sqpack` to repack them.
- **One user:** Check their account in the
  [User Editor]({{ site.baseurl }}{% link maxcfg-user-editor.md %}). Look for the
  [BadLogon flag]({{ site.baseurl }}{% link admin-user-management.md %}#bad-logon).
- **All nodes:** It's probably a config problem or a system-level
  issue (disk, permissions, memory).

### 5. Change One Thing at a Time

The fastest way to make a problem worse is to change three things at
once and not know which one helped (or didn't). Make one change, test,
and verify before making another.

---

## Common Issues {#common-issues}

### Maximus Won't Start

**Symptoms:** MaxTel shows nodes stuck in "Starting", or `max` exits
immediately when run manually.

**Check:**
1. Run `max` manually from the BBS directory to see the error output
2. Check `max_startup.log` for early failures
3. Verify `maximus.toml` exists and is valid TOML (a missing comma or
   unclosed quote will kill it)
4. Make sure `user.db` exists in the expected location
5. Check file permissions — the BBS user needs read/write access

### Users Connect but See Nothing

**Symptoms:** Telnet connection succeeds but the caller gets a blank
screen.

**Check:**
1. PTY allocation — restrictive `/dev/pts` settings or container
   environments without proper devpts mount
2. Maximus configuration for socket/telnet mode
3. Permissions on the `max` binary and per-node directories

For more MaxTel-specific issues, see
[MaxTel Troubleshooting]({{ site.baseurl }}{% link maxtel-troubleshooting.md %}).

### Message Area Errors

**Symptoms:** Errors when entering a message area, reading messages, or
posting.

**Check:**
1. Do the Squish base files (`.sqd`, `.sqi`, `.sql`) exist for that area?
2. Are they readable/writable by the BBS user?
3. Try repacking: `sqpack <areaname>`
4. Check if the area definition in `config/areas/msg/areas.toml` has the
   correct path

### "No Memory" or Allocation Failures

**Symptoms:** `NoMem` messages in the log, or nodes crashing under load.

**Check:**
1. System memory with `free -h` (Linux) or `vm_stat` (macOS)
2. Per-process limits with `ulimit -a`
3. If running many nodes, each is a separate process — multiply
   accordingly

### Stale Node State

**Symptoms:** A node number shows as "in use" even though nobody is on
it. Or a node won't start because it thinks a previous session is still
active.

**Check:**
1. Look in `run/node/<N>/` for `active.bbs`, `ipc.bbs`, and lock files
2. If the node crashed, these files may be left behind
3. Stop MaxTel, delete the stale files, restart:
   ```bash
   rm -f /var/max/run/node/01/active.bbs
   rm -f /var/max/run/node/01/maxipc*
   ```

### Config Parse Errors

**Symptoms:** Maximus starts but behaves unexpectedly, or logs show
warnings about missing configuration keys.

**Check:**
1. TOML syntax — unclosed quotes, missing commas, wrong types
2. Use a TOML validator: `cat config/maximus.toml | python3 -c "import tomllib, sys; tomllib.load(sys.stdin.buffer); print('OK')"`
3. Check for typos in key names — TOML is case-sensitive

---

## MEX Script Logging {#mex-logging}

MEX scripts can write to the system log using the `log()` intrinsic:

```mex
log("=User searched for: " + searchterm);
```

The first character of the string is the log symbol — use `=` for
standard activity, `!` for errors, `:` for detail. This follows the
same symbol convention as the system log, so your script's entries
blend naturally with the rest of the log.

This is useful for:
- Tracking usage of custom features
- Debugging script behavior in production
- Audit trails for sensitive operations (file access, user changes)

---

## See Also

- [General Administration]({{ site.baseurl }}{% link admin-general.md %}) — the daily
  rhythm of running a board
- [Backup & Recovery]({{ site.baseurl }}{% link admin-backup-recovery.md %}) — protecting
  your log archives
- [Core Settings]({{ site.baseurl }}{% link config-core-settings.md %}) — `log_file` and
  `log_mode` configuration
- [MaxTel Troubleshooting]({{ site.baseurl }}{% link maxtel-troubleshooting.md %}) —
  MaxTel-specific issues with deeper detail on node problems
- [User Management]({{ site.baseurl }}{% link admin-user-management.md %}) — the BadLogon
  flag and user-related diagnostics
