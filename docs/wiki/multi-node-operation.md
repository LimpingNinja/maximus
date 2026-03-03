---
layout: default
title: "Multi-Node Operation"
section: "administration"
description: "How Maximus runs multiple simultaneous BBS nodes"
permalink: /multi-node-operation/
---

Maximus supports up to 32 simultaneous nodes, each serving a different caller.
Every node is a completely independent Maximus process with its own PTY, its
own Unix socket, and its own working directory. MaxTel orchestrates all of
them — spawning nodes at startup, routing callers to free ones, and cleaning
up when sessions end.

If you've run a multi-line BBS before, the model will feel familiar: each
"line" is its own Maximus instance, and the supervisor (MaxTel) sits in front
managing traffic. The difference is that instead of modems and COM ports,
everything runs over TCP/IP with Unix domain sockets as the glue between
MaxTel and each node.

---

## How It Works — The Big Picture

Here's what happens from the moment MaxTel starts to when a caller connects:

1. **MaxTel starts and spawns nodes.** For each configured node (1 through
   *N*, where *N* is set by `-n`), MaxTel calls `forkpty()` to create a child
   process with its own pseudo-terminal. The child `exec`s the Maximus binary.

2. **Each node creates a Unix socket and waits.** The Maximus process binds a
   Unix domain socket at `run/node/<hex>/maxipc` and enters WFC (Waiting For
   Caller) mode — polling the socket for incoming connections.

3. **A caller telnets in.** MaxTel accepts the TCP connection on its listen
   port, finds a node in WFC state (by checking for a live socket file), and
   forks a **bridge process**.

4. **The bridge negotiates and connects.** The bridge process handles telnet
   negotiation with the caller (terminal type, ANSI detection, screen size),
   writes the results to a `termcap.dat` file for Maximus to read, then
   connects to the node's Unix socket.

5. **Data flows bidirectionally.** The bridge sits in a `select()` loop,
   shuttling bytes between the caller's TCP socket and the node's Unix
   socket. The caller sees a normal BBS session.

6. **The session ends.** When the caller disconnects (or the Maximus session
   exits), the bridge detects the closed connection and exits. MaxTel notices
   the bridge child exiting via `SIGCHLD`, marks the node as WFC again, and
   it's ready for the next caller.

---

## Per-Node Directory Layout

Each node gets its own directory under `run/node/`, named by hex node number:

```
run/node/
├── 01/          ← Node 1
│   ├── maxipc        Unix domain socket
│   ├── maxipc.lck    Lock file (present when a caller is connected)
│   ├── termcap.dat   Terminal capabilities (written by bridge)
│   ├── lastus.bbs    Current user info (written by Maximus)
│   └── bbstat.bbs    BBS statistics
├── 02/          ← Node 2
│   └── ...
└── 03/          ← Node 3
    └── ...
```

These directories are created on demand — you don't need to set them up
manually. MaxTel cleans up stale socket and lock files on startup.

For details on what each file does and how MaxTel uses them, see
[Node Management]({% link node-management.md %}).

---

## How Nodes Are Launched

When MaxTel spawns a node, it runs:

```
max -w -pt<N> -n<N> -b57600 -dl
```

| Flag | Purpose |
|------|---------|
| `-w` | WFC (Waiting For Caller) mode — Maximus waits for a connection instead of running local |
| `-pt<N>` | TCP/IP task mode — tells Maximus to create a Unix socket for node *N* |
| `-n<N>` | Node number — identifies this instance |
| `-b57600` | Baud rate (nominal; irrelevant for TCP but required by the comm layer) |
| `-dl` | Direct-connect local mode |

MaxTel also sets several environment variables before exec:

| Variable | Value |
|----------|-------|
| `MAXIMUS` | Absolute path to the Maximus base directory |
| `MAX_INSTALL_PATH` | Same as `MAXIMUS` |
| `LD_LIBRARY_PATH` (or `DYLD_LIBRARY_PATH` on macOS) | `<base>/bin/lib` |
| `MEX_INCLUDE` | `<base>/scripts/include` |

Each node process inherits these and uses them to find config files, shared
libraries, and MEX scripts.

---

## Node Lifecycle

Every node moves through a defined set of states:

```
INACTIVE ──→ STARTING ──→ WFC ──→ CONNECTED ──→ WFC (repeat)
                │                      │
                ▼                      ▼
             FAILED              STOPPING ──→ INACTIVE
```

| State | What's Happening |
|-------|-----------------|
| **Inactive** | Node isn't running. This is the initial state before spawn, or after a clean shutdown. |
| **Starting** | Maximus has been forked but hasn't created its socket yet. MaxTel polls for the socket file. |
| **WFC** | Socket exists, Maximus is listening. Ready for a caller. |
| **Connected** | A bridge process is active — a caller is online. |
| **Stopping** | Node is being shut down (kick, restart, or MaxTel exit). |
| **Failed** | Node crashed repeatedly within a short window. MaxTel backs off retries. See [Node Management]({% link node-management.md %}) for failure handling. |

The important thing to understand: **the Maximus process persists across
calls.** It doesn't exit after each caller — it returns to WFC and waits for
the next connection. What's created fresh for each call is the bridge process
that connects the caller's TCP socket to the node's Unix socket.

---

## Where to Go Next

- **[Node Management]({% link node-management.md %})** — node states, failure
  handling, per-node files, and how MaxTel tracks what's happening
- **[IPC & Isolation]({% link node-ipc-isolation.md %})** — the Unix socket
  architecture, bridge process details, PTY layer, and how nodes stay
  isolated from each other
- **[The Dashboard]({% link maxtel-dashboard.md %})** — watching all of this
  in real time
- **[Sysop Features]({% link maxtel-sysop-features.md %})** — snoop, kick,
  restart, and config
