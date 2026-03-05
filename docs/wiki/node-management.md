---
layout: default
title: "Node Management"
section: "administration"
description: "Node states, per-node files, failure handling, and status tracking"
permalink: /node-management/
---

MaxTel keeps tabs on every node it manages — tracking state transitions,
detecting crashes, reading status files written by Maximus, and presenting it
all on the dashboard. This page covers the practical details of how that
works and what you'll see when things go right (and when they don't).

---

## Node States

Each node is always in one of six states. You'll see these in the Nodes panel
on the dashboard:

| State | Dashboard Shows | What It Means |
|-------|----------------|---------------|
| **Inactive** | *blank* | Node process isn't running. Initial state before spawn, or after shutdown. |
| **Starting** | `Starting` | Maximus has been forked but hasn't created its socket yet. MaxTel is polling for it. |
| **WFC** | `WFC` | Waiting For Caller. Socket is live, Maximus is listening. Ready for a connection. |
| **Online** | `Online` | A caller is connected and a bridge process is active. |
| **Stopping** | `Stopping` | Node is being shut down — either from a kick, restart, or MaxTel exit. |
| **Failed** | `FAILED` | Node crashed too many times in a short window. MaxTel has stopped trying. |

### Normal Flow

The happy path is a simple loop:

```
Starting → WFC → Online → WFC → Online → WFC → ...
```

Each time a caller disconnects, the node returns to WFC automatically. The
Maximus process stays alive — it doesn't restart between calls.

### How MaxTel Detects State Changes

- **Starting → WFC:** MaxTel polls for the node's socket file
  (`run/node/<hex>/maxipc`). When `stat()` succeeds, the node is ready.
- **WFC → Online:** MaxTel forks a bridge process and sets the state to
  Connected. The bridge handles the caller.
- **Online → WFC:** The bridge process exits (detected via `SIGCHLD`). MaxTel
  clears the username and marks the node as WFC.
- **Any → Stopping:** Triggered by kick (`K`), restart (`R`), or quit (`Q`).
  MaxTel sends `SIGTERM` to the Maximus process.

---

## Per-Node Files

Each node's directory (`run/node/<hex>/`) contains several files that MaxTel
and Maximus use to communicate. Here's what each one does:

### `maxipc` — The Unix Domain Socket

This is the main IPC channel. Maximus creates it at startup (`bind` +
`listen`), and the bridge process connects to it when a caller arrives. All
BBS session data flows through this socket.

MaxTel cleans up stale sockets on startup — if a previous run crashed and
left a socket file behind, it gets removed before the node spawns.

### `maxipc.lck` — Lock File

Created by Maximus when it accepts a connection on the Unix socket. Removed
when the connection closes. MaxTel uses this as a secondary indicator that a
node has an active caller, and cleans it up alongside the socket on
shutdown.

### `termcap.dat` — Terminal Capabilities

Written by the bridge process *before* it connects to the Unix socket.
Contains the results of telnet negotiation:

```
Telnet: 1
Ansi: 1
Rip: 0
Width: 132
Height: 50
```

Maximus reads this file at the start of each session (`Apply_Term_Caps`) to
configure the user's terminal mode — ANSI vs. TTY, screen dimensions, and
fullscreen reader support.

### `lastus.bbs` — Current User

Written by Maximus when a user logs in. Contains the user record (name,
alias, stats). MaxTel reads this to display the current username on the
dashboard. It only reads the file if its modification time is newer than the
connection start time, avoiding stale data from a previous session.

If your system uses the alias feature, MaxTel will display the alias instead
of the real name.

### `bbstat.bbs` — BBS Statistics

Written by Maximus with system-wide statistics (total calls, messages,
files, etc.). MaxTel reads this for the System panel on the dashboard. The
node 00 (or node 01 as fallback) version is used for global stats.

---

## Failure Handling

Nodes crash sometimes — maybe the config is broken, maybe a MEX script is
causing a segfault, maybe the binary isn't found. MaxTel handles this
gracefully with a failure tracking system.

### What Happens When a Node Crashes

1. MaxTel detects the child process exit via `SIGCHLD`
2. If the node was in STARTING or WFC state (i.e., it crashed before or
   between callers), MaxTel increments a **fail counter**
3. If the fail count exceeds the threshold within a time window, the node
   moves to **FAILED** state
4. In FAILED state, MaxTel uses exponential backoff before retrying — it
   won't hammer the system trying to respawn a broken node

### What FAILED Looks Like

On the dashboard, a failed node shows `FAILED` in red. The status bar or
node details may show the last error captured from the PTY output — MaxTel
buffers the last kilobyte of PTY output from each node specifically for
this purpose.

### Recovering a Failed Node

Use `R` (Restart) on a failed node to reset the fail counter and force an
immediate respawn attempt. This is the right thing to do after you've fixed
whatever caused the crash (config file, missing binary, permissions, etc.).

---

## How MaxTel Reads Node Status

MaxTel's main loop runs a status update cycle approximately every 100ms.
During each cycle it:

1. **Drains PTY output** from every node — this prevents Maximus from
   blocking if it writes to stdout while nobody is reading the master end
   of the PTY. The last 1KB of output is kept in a ring buffer per node
   (used for error extraction on crash).

2. **Checks for socket readiness** — for nodes in STARTING state, polls
   for the socket file to detect the transition to WFC.

3. **Reads `lastus.bbs`** — for connected nodes, reads the current user
   name from the per-node file (only if the file was modified after the
   connection started).

4. **Loads global stats** — reads `bbstat.bbs` for the System panel.

5. **Loads caller history** — reads `callers.dat` for the Callers panel.

6. **Refreshes the display** — redraws any panels that have changed.

This is why the dashboard feels live — it's polling at 10Hz and updating
only what changed.

---

## See Also

- **[Multi-Node Operation]({{ site.baseurl }}{% link multi-node-operation.md %})** — the big
  picture of how multi-node works
- **[IPC & Isolation]({{ site.baseurl }}{% link node-ipc-isolation.md %})** — Unix sockets,
  bridge processes, PTY layer, and process isolation
- **[Sysop Features]({{ site.baseurl }}{% link maxtel-sysop-features.md %})** — kick, restart,
  snoop, and config
- **[Troubleshooting]({{ site.baseurl }}{% link maxtel-troubleshooting.md %})** — common
  problems and log files
