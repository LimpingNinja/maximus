---
layout: default
title: "IPC & Isolation"
section: "administration"
description: "Unix sockets, bridge processes, PTY layer, and node isolation"
permalink: /node-ipc-isolation/
---

This page is the "under the hood" reference for how MaxTel and Maximus talk
to each other, how caller data gets from a TCP socket to a BBS session, and
why nodes can't step on each other. You don't need to know any of this to
*run* a BBS — but if you're debugging connection problems, writing custom
integrations, or just curious about the plumbing, this is where it lives.

---

## The Unix Domain Socket

Every node communicates through a **Unix domain socket** — a local-only IPC
channel that looks like a file on disk but behaves like a network socket.
There's no TCP overhead, no network stack involved. It's just two processes
talking through the kernel.

### How It's Created

When Maximus starts on a node (spawned by MaxTel with `-pt<N>`), the comm
layer (`ipcomm.c`) does the following:

1. Creates a `SOCK_STREAM` socket with `AF_UNIX`
2. Builds the socket path: `$MAXIMUS/run/node/<hex>/maxipc`
3. Ensures the node directory exists (created on demand)
4. Removes any stale socket file from a previous run
5. Calls `bind()` to attach to the path, then `listen(1)` with a backlog
   of one

At this point, Maximus is sitting in WFC mode — polling the socket for an
incoming connection. MaxTel detects this by checking for the socket file's
existence with `stat()`.

### How Connections Are Accepted

This is where it gets interesting. Maximus uses a comm abstraction layer
that was originally designed for serial modems. The concept of "carrier
detect" (DCD) maps directly onto socket acceptance:

- `IpComIsOnline()` is polled during Maximus's idle loop (the same way a
  modem driver would check for a ring signal)
- When `select()` indicates a pending connection on the listening socket,
  Maximus calls `accept()`
- On successful accept, it sets `fDCD = TRUE` — the BBS equivalent of
  "carrier detected, we have a caller"
- It writes a lock file (`maxipc.lck`) to signal the active connection
- From this point on, all reads and writes go through the accepted socket
  descriptor

When the caller disconnects, Maximus detects the dead socket (read returns
0 or error), sets `fDCD = FALSE` (carrier lost), and returns to WFC.

---

## The Bridge Process

The bridge is the middleman between the caller's TCP connection and the
node's Unix socket. MaxTel forks a dedicated child process for each
incoming connection — one bridge per caller, one caller per node.

### What the Bridge Does

When a caller telnets in and MaxTel finds a free node, the bridge process
runs through this sequence:

1. **Telnet negotiation** — sends IAC commands to detect the client's
   capabilities:
   - Terminal type (via TTYPE)
   - Screen dimensions (via NAWS — Negotiate About Window Size)
   - ANSI support (via probing)
   - Falls back to ANSI cursor-position detection if NAWS isn't supported

2. **Writes `termcap.dat`** — stores the negotiation results in the node's
   directory so Maximus can read them at session start:
   ```
   Telnet: 1
   Ansi: 1
   Rip: 0
   Width: 132
   Height: 50
   ```

3. **Connects to the Unix socket** — opens a `SOCK_STREAM` connection to
   the node's `maxipc` socket path

4. **Enters the bridge loop** — a tight `select()` loop that shuttles data
   in both directions:
   - Bytes from the TCP socket → Unix socket (caller input to Maximus)
   - Bytes from the Unix socket → TCP socket (Maximus output to caller)

5. **Exits when either side closes** — if the caller drops the TCP
   connection or Maximus closes the Unix socket, the bridge detects it and
   exits cleanly

### Bridge Lifecycle

```
MaxTel accepts TCP connection
  └→ fork() — child becomes the bridge
       ├→ Telnet negotiation with caller
       ├→ Write termcap.dat
       ├→ Connect to node's Unix socket
       ├→ select() loop: TCP ↔ Unix socket
       └→ _exit(0) when either side closes

MaxTel parent:
  ├→ Closes its copy of the TCP fd
  ├→ Records the bridge PID
  ├→ Sets node state to Connected
  └→ Detects bridge exit via SIGCHLD → node back to WFC
```

The bridge process also closes MaxTel's listening socket (`listen_fd`) so
that if MaxTel dies unexpectedly, the port isn't held open by orphaned
bridge children.

---

## The PTY Layer

Every Maximus node runs on a **pseudo-terminal (PTY)** — a kernel-level
terminal emulation pair. MaxTel creates this with `forkpty()`, which gives:

- **Master fd** — held by MaxTel. Used for snoop mode and PTY draining.
- **Slave fd** — inherited by the Maximus child process as its stdin/stdout.

### Why PTYs?

Maximus was originally written for real serial terminals and modems. It
expects to be talking to something that looks like a terminal — with a
controlling TTY, terminal I/O settings, and the ability to detect screen
dimensions. A PTY provides all of this transparently.

### PTY Draining

MaxTel reads from each node's master PTY fd on every status update cycle
(~10Hz). This is critical — if nobody reads the master end, the PTY buffer
fills up and Maximus blocks on writes. The drained output is kept in a 1KB
ring buffer per node, used to extract error messages when a node crashes.

### Snoop Mode and the PTY

When you press `S` to snoop a node, MaxTel reads from the PTY master (same
as draining, but now it writes the output to your terminal) and writes your
keystrokes to the PTY master (which Maximus reads as input). This is how
you see exactly what the caller sees and can type into the session. See
[Sysop Features]({{ site.baseurl }}{% link maxtel-sysop-features.md %}) for the full snoop
mode reference.

---

## Process Isolation

Each node is a fully independent process. There is no shared memory between
nodes, no message passing, no inter-node signaling. The only shared
resources are:

| Resource | How Contention Is Handled |
|----------|--------------------------|
| User database | File-level locking |
| Message bases | File-level locking (Squish/JAM) |
| File areas | Read-only access to file lists |
| Config files | Read at startup, not modified at runtime |
| `callers.dat` | Append-only with file locking |

This means a crash on one node has zero impact on other nodes. A runaway
MEX script on node 3 can't corrupt node 5's session. The worst that happens
is contention on shared files, which the file locking handles.

### What Each Node Gets

Every node process has:

- Its own **PID** — independently killable
- Its own **PTY** — isolated terminal I/O
- Its own **Unix socket** — separate IPC channel
- Its own **working directory** — `run/node/<hex>/`
- Its own **environment** — inherited from MaxTel at spawn time
- Its own **bridge process** (when a caller is connected)

MaxTel tracks all of these per node and can manage them independently —
kick one node without affecting others, restart a single node, or snoop on
one while the rest run undisturbed.

---

## Putting It All Together

Here's the full data path for a single caller session:

```
Caller's telnet client
  │
  │  TCP socket (internet)
  ▼
MaxTel (listen on port 2323)
  │
  │  fork() → bridge process
  ▼
Bridge process
  │  ┌─ Telnet negotiation
  │  ├─ Write termcap.dat
  │  └─ Connect to Unix socket
  │
  │  Unix domain socket (local)
  ▼
Maximus node process (on PTY)
  │
  │  Reads termcap.dat → configures terminal
  │  Accepts Unix socket → sets carrier
  │  Runs BBS session
  │  Caller disconnects → carrier lost → back to WFC
  ▼
Bridge exits → MaxTel detects via SIGCHLD → node marked WFC
```

---

## See Also

- **[Multi-Node Operation]({{ site.baseurl }}{% link multi-node-operation.md %})** — the big
  picture, lifecycle, and how nodes are launched
- **[Node Management]({{ site.baseurl }}{% link node-management.md %})** — states, per-node
  files, and failure handling
- **[Sysop Features]({{ site.baseurl }}{% link maxtel-sysop-features.md %})** — snoop mode
  uses the PTY layer described here
- **[Troubleshooting]({{ site.baseurl }}{% link maxtel-troubleshooting.md %})** — when the
  plumbing breaks
