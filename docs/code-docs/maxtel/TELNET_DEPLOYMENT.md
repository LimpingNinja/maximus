# Maximus Telnet Deployment Guide

## Overview

Maximus 3.04a's `-pt<port>` option does **not** create a TCP/IP listener as one might expect. Instead, it creates a **UNIX domain socket** for inter-process communication. This document explains the architecture and how to properly deploy Maximus for telnet access.

## Understanding the Communications Architecture

### The Comm Driver System

Maximus uses a pluggable communications driver system defined in `comdll/`:

| Driver | File | Socket Type | Purpose |
|--------|------|-------------|---------|
| `IpCom*` | `ipcomm.c` | `AF_UNIX` | Inter-node IPC, inetd-style |
| `ModemCom*` | `modemcom.c` | Serial | Traditional modem/serial |
| `FdCom*` | `fdcomm.c` | `AF_INET` | **Real TCP** (not built!) |

When you run `max -pt5555`, the `tcpip` flag is set, which causes `common.c` to load the `IpCom*` driver:

```c
// common.c lines 18-34
if(tcpip) {
    CommApi.fComOpen = &IpComOpen;  // Uses AF_UNIX sockets!
    ...
}
```

### What `-pt<port>` Actually Does

The `ipcomm.c` driver creates:
- A UNIX domain socket at: `<cwd>/maxipc<port>` (e.g., `maxipc5555`)
- A lock file at: `<cwd>/maxipc<port>.lck`

This is visible when running:
```bash
$ netstat -an | grep maxipc
unix stream /path/to/maxipc5555
```

### Why UNIX Sockets?

The original design (circa 2003) assumed Maximus would run **behind a super-server** like inetd or xinetd. The workflow:

1. External daemon (inetd) listens on TCP port 23 (telnet)
2. When connection arrives, inetd spawns `max` with socket as stdin/stdout
3. Max uses the UNIX socket for **inter-node communication** (chat, paging)
4. The respawn model handles multiple concurrent sessions

## Deployment Options

### Option 1: xinetd/inetd (Recommended)

This is the originally intended deployment model.

#### xinetd Configuration

Create `/etc/xinetd.d/maximus`:

```
service maximus
{
    type            = UNLISTED
    socket_type     = stream
    protocol        = tcp
    port            = 2323
    wait            = no
    user            = maximus
    server          = /opt/maximus/bin/max
    server_args     = -n0 -b38400 etc/max.prm
    instances       = 10
    per_source      = 2
    log_on_success  = HOST PID
    log_on_failure  = HOST
    disable         = no
}
```

Key points:
- `-n0` enables dynamic node numbering (required for multi-user)
- `-b38400` sets the "baud rate" (cosmetic for telnet)
- `wait = no` allows concurrent connections
- xinetd handles the TCP listen/accept

#### inetd Configuration

Add to `/etc/inetd.conf`:

```
maximus stream tcp nowait maximus /opt/maximus/bin/max max -n0 -b38400 etc/max.prm
```

Then restart inetd:
```bash
kill -HUP $(cat /var/run/inetd.pid)
```

### Option 2: tcpser (For Retro/Modem Simulation)

tcpser emulates a Hayes modem over TCP, perfect for authentic BBS experience.

#### Installation

```bash
# macOS
brew install tcpser

# Linux (build from source)
git clone https://github.com/FozzTexx/tcpser.git
cd tcpser && make && sudo make install
```

#### Configuration

Create a startup script `run-maximus-tcpser.sh`:

```bash
#!/bin/bash
# Start tcpser listening on port 2323, connecting to max via pty

MAXDIR=/opt/maximus
cd $MAXDIR

# Create a pseudo-terminal pair and run max on it
tcpser -v 25232 -p 2323 -s 38400 -l 7 \
    -c "/opt/maximus/bin/max -w -n0 -pd1 etc/max.prm"
```

Options:
- `-v 25232` - Virtual modem speed
- `-p 2323` - TCP port to listen on
- `-s 38400` - Serial speed
- `-l 7` - Log level
- `-c` - Command to execute on connection

Users connect via telnet and see modem responses:
```
CONNECT 38400
```

### Option 3: socat (Simple TCP-to-Exec Bridge)

For quick testing or simple deployments:

```bash
#!/bin/bash
# Simple socat wrapper for Maximus

MAXDIR=/opt/maximus
PORT=2323

socat TCP-LISTEN:$PORT,fork,reuseaddr \
    EXEC:"$MAXDIR/bin/max -n0 -b38400 etc/max.prm",pty,setsid,ctty
```

For multiple persistent listeners:
```bash
# In a systemd service or screen session
while true; do
    socat TCP-LISTEN:2323,fork,reuseaddr \
        EXEC:"/opt/maximus/bin/max -n0 etc/max.prm",pty,setsid,ctty
done
```

### Option 4: systemd Socket Activation (Modern Linux)

Create `/etc/systemd/system/maximus.socket`:

```ini
[Unit]
Description=Maximus BBS Socket

[Socket]
ListenStream=2323
Accept=yes
MaxConnections=10

[Install]
WantedBy=sockets.target
```

Create `/etc/systemd/system/maximus@.service`:

```ini
[Unit]
Description=Maximus BBS Instance

[Service]
Type=simple
User=maximus
WorkingDirectory=/opt/maximus
ExecStart=/opt/maximus/bin/max -n0 -b38400 etc/max.prm
StandardInput=socket
StandardOutput=socket
StandardError=journal
```

Enable and start:
```bash
sudo systemctl enable maximus.socket
sudo systemctl start maximus.socket
```

## The Inter-Node IPC System

The UNIX socket (`maxipc<port>`) is used for **inter-node communication**:

- Node-to-node chat/paging
- Who's online queries
- Shared resource locking

This is separate from the telnet connection. Each max instance:
1. Receives user I/O via stdin/stdout (from inetd/socat/etc.)
2. Communicates with other nodes via the UNIX socket

## Environment Variables

Set these for proper operation:

```bash
export MAX_INSTALL_PATH=/opt/maximus
export MAXIMUS=/opt/maximus/etc/max.prm
export LD_LIBRARY_PATH=/opt/maximus/lib:$LD_LIBRARY_PATH
# macOS:
export DYLD_LIBRARY_PATH=/opt/maximus/lib:$DYLD_LIBRARY_PATH
```

## Testing Your Setup

1. Start the listener (via xinetd, socat, etc.)
2. Connect: `telnet localhost 2323`
3. Check logs: `tail -f /opt/maximus/log/max.log`
4. Verify node: Look for "Begin, v3.04a (task=N)" in log

## Troubleshooting

### "maxipc5555" file created but no TCP listener
This is expected! Use inetd/xinetd/socat as described above.

### Multiple nodes not communicating
Ensure all nodes use the same `MAX_INSTALL_PATH` so they find the same UNIX socket.

### Connection drops immediately
Check that the working directory is correct and `etc/max.prm` is accessible.

## Future Development

To add native TCP support, either:
1. Integrate `fdcomm.c` into the build (see `FDCOMM_ANALYSIS.md`)
2. Modify `ipcomm.c` to use `AF_INET` instead of `AF_UNIX`

---

*Document created for Maximus 3.04a - November 2025*
