# MAXTEL

## Telnet Supervisor for Maximus-CBCS

**Version 1.0**

Copyright © 2025 Kevin Morgan (Limping Ninja)  
https://github.com/LimpingNinja

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

---

## Table of Contents

1. [Introduction](#1-introduction)
   - [About MAXTEL](#11-about-maxtel)
   - [System Requirements](#12-system-requirements)
   - [Features](#13-features)
2. [Installation](#2-installation)
   - [Building MAXTEL](#21-building-maxtel)
   - [Directory Structure](#22-directory-structure)
   - [First-Time Setup](#23-first-time-setup)
3. [Running MAXTEL](#3-running-maxtel)
   - [Command Line Options](#31-command-line-options)
   - [Interactive Mode](#32-interactive-mode)
   - [Headless Mode](#33-headless-mode)
   - [Daemon Mode](#34-daemon-mode)
4. [The User Interface](#4-the-user-interface)
   - [Screen Layout](#41-screen-layout)
   - [User Stats Panel](#42-user-stats-panel)
   - [System Panel](#43-system-panel)
   - [Nodes Panel](#44-nodes-panel)
   - [Callers Panel](#45-callers-panel)
   - [Keyboard Commands](#46-keyboard-commands)
5. [Responsive Layouts](#5-responsive-layouts)
   - [Compact Mode](#51-compact-mode-80x25)
   - [Medium Mode](#52-medium-mode-100x30)
   - [Full Mode](#53-full-mode-132x40)
6. [Troubleshooting](#6-troubleshooting)
   - [Common Issues](#61-common-issues)
   - [Log Files](#62-log-files)

---

## 1. Introduction

Welcome to MAXTEL! This manual describes how to install, configure, and operate MAXTEL to manage telnet connections for your Maximus-CBCS installation on Unix and Linux systems.

### 1.1. About MAXTEL

Good news—you can now accept telnet callers on your Maximus BBS without extra hoops or external utilities. MAXTEL is a multi-node telnet supervisor that manages incoming connections and routes them to available Maximus BBS nodes. It provides a real-time ncurses-based interface that displays system status, active users, node activity, and recent callers.

Unlike traditional modem-based BBS setups, MAXTEL handles TCP/IP networking while presenting a familiar interface to both the SysOp and connecting users. When a user connects via telnet, MAXTEL assigns them to an available node and bridges their connection to the Maximus instance running on that node.

### 1.2. System Requirements

MAXTEL requires the following:

**Operating System**

A Unix-like operating system such as Linux, FreeBSD, or macOS. MAXTEL has been tested on modern Linux distributions and macOS.

**Maximus-CBCS**

A working Maximus-CBCS 3.x installation compiled for your platform. MAXTEL spawns Maximus instances for each node, so you should verify that Maximus runs correctly in local mode before proceeding.

**ncurses Library**

The ncurses library must be installed for the interactive user interface. Most Unix systems include this by default. For headless or daemon operation, ncurses is still required for compilation but is not used at runtime.

**Network Access**

The system must be able to bind to a TCP port (default 2323) to accept incoming connections.

### 1.3. Features

MAXTEL provides the following capabilities:

**Multi-Node Management**

Manage multiple simultaneous BBS nodes (1 to 32), each running its own Maximus instance.

**Real-Time Monitoring**

The ncurses interface displays live information about each node, including connection status, current user, activity, and session duration.

**Automatic Node Assignment**

When a caller connects, MAXTEL finds an available node and routes the connection. If all nodes are busy, the caller is informed and disconnected gracefully.

**Telnet Protocol Support**

MAXTEL performs telnet negotiation with connecting clients, including terminal type detection and ANSI capability checking.

**Caller History**

Recent callers are displayed in the interface, showing caller name, node number, call count, and connection time.

**System Statistics**

The interface displays system information including BBS name, sysop name, FTN address, uptime, peak usage, and activity levels.

**Responsive Interface**

The user interface adapts to different terminal sizes, from compact 80x25 displays to full-screen 132x60+ terminals.

**Headless Operation**

MAXTEL can run without a user interface for use in scripts, init systems, or as a background daemon.

---

## 2. Installation

### 2.1. Building MAXTEL

Good news—MAXTEL builds right alongside the rest of Maximus. To build, ensure you have a working Maximus build environment, then run:

```bash
make maxtel
```

To build and install MAXTEL to the `build/bin` directory:

```bash
make maxtel_install
```

The resulting binary will be placed in `build/bin/` alongside the other Maximus utilities.

### 2.2. Directory Structure

MAXTEL expects the following directory structure relative to the base Maximus installation:

| Path | Description |
|------|-------------|
| `bin/max` | The Maximus executable. Override with `-m`. |
| `etc/max.prm` | Compiled Maximus configuration. Override with `-c`. |
| `etc/callers.dat` | Caller log file for recent caller history. |
| `etc/stats.dat` | System statistics file. |
| `m/1`, `m/2`, etc. | Per-node directories for node-specific data. |

### 2.3. First-Time Setup

Before running MAXTEL for the first time:

1. **Verify Maximus works** — Run Maximus in local mode to confirm it operates correctly.

2. **Compile configuration** — Run SILT to compile your `max.prm` file. MAXTEL reads this for system information.

3. **Check permissions** — Ports below 1024 require root privileges on most Unix systems.

4. **Configure firewall** — Ensure incoming connections on your telnet port are allowed.

A typical first invocation:

```bash
cd /path/to/maximus
./build/bin/maxtel -p 2323 -n 4
```

This starts MAXTEL on port 2323 with 4 nodes available.

---

## 3. Running MAXTEL

### 3.1. Command Line Options

| Option | Description |
|--------|-------------|
| `-p PORT` | TCP port to listen on (default: 2323) |
| `-n NODES` | Number of nodes to manage, 1-32 (default: 4) |
| `-d PATH` | Base directory for Maximus (default: current directory) |
| `-m PATH` | Path to max executable (default: `./bin/max`) |
| `-c PATH` | Path to config directory (default: `etc/max`) |
| `-s SIZE` | Request terminal size, e.g., `132x60` |
| `-H` | Headless mode—no UI |
| `-D` | Daemon mode—fork to background (implies `-H`) |
| `-h` | Show help message |

**Examples:**

```bash
# Standard telnet port with 8 nodes
maxtel -p 23 -n 8

# Specify installation directory
maxtel -d /opt/maximus -p 2323

# Request larger terminal
maxtel -s 132x60
```

### 3.2. Interactive Mode

By default, MAXTEL runs in interactive mode with a full-screen ncurses interface. This is the typical way to monitor your BBS from a terminal or console.

In interactive mode, you can:

- Watch node status change in real-time as callers connect and disconnect
- View current user information for active sessions
- See recent caller history
- Monitor system statistics and uptime
- Use keyboard commands to manage nodes

To exit, press `Q`. MAXTEL will shut down all nodes gracefully before exiting.

### 3.3. Headless Mode

Headless mode (`-H`) runs MAXTEL without any user interface. This is useful when running from scripts, on headless servers, or under process supervisors like systemd.

```bash
maxtel -H -p 2323 -n 4
```

MAXTEL prints a startup message to stderr:

```
maxtel running in headless mode on port 2323 with 4 nodes
```

The program continues to accept connections, manage nodes, and log activity to `maxtel.log`. To stop headless MAXTEL, send SIGTERM or SIGINT:

```bash
kill $(pgrep maxtel)
```

### 3.4. Daemon Mode

Daemon mode (`-D`) runs MAXTEL as a background process, detached from the terminal. This mode implies headless operation and is suitable for system startup scripts.

```bash
maxtel -D -p 2323 -n 4
```

MAXTEL forks a child process, prints the PID, and the parent exits:

```
maxtel daemon started (PID 12345), port 2323
```

You can add MAXTEL to startup scripts:

```bash
# In .bashrc or rc.local
if ! pgrep -x maxtel > /dev/null; then
    /path/to/maxtel -D -p 2323 -n 4 -d /path/to/maximus
fi
```

Activity is logged to `maxtel.log`. To stop the daemon:

```bash
kill $(pgrep maxtel)
```

---

## 4. The User Interface

The MAXTEL interface is divided into several panels that display different aspects of system status. The layout adapts based on terminal size.

### 4.1. Screen Layout

The screen is organized as follows:

- **Header Bar** — Displays "MAXTEL v1.0", "Maximus Telnet Supervisor", and the listening port
- **Top Row** — User Stats (left) and System info (right)
- **Bottom Row** — Nodes (left) and Callers (right)
- **Status Bar** — Available keyboard commands and current layout mode

### 4.2. User Stats Panel

Displays information about the user on the selected node:

| Field | Description |
|-------|-------------|
| Name | User's registered name |
| City | User's city and state/province |
| Calls | Total calls to the BBS |
| Msgs | Messages posted/read |
| Up/Dn | Kilobytes uploaded/downloaded |
| Files | Files uploaded/downloaded |

If no user is logged into the selected node, the panel displays "(No user online)".

### 4.3. System Panel

Displays BBS information and status:

| Field | Description |
|-------|-------------|
| BBS / Sysop / FTN | System info from max.prm |
| Time | Current system time |
| Nodes | Total nodes configured |
| Online | Nodes with active users |
| Waiting | Nodes in WFC state |

In wider terminals, additional statistics appear: uptime, peak online, total users, messages written, and files downloaded.

In compact mode (80x25), the System panel uses tabs. Press `Tab` to switch between Info and Stats views.

### 4.4. Nodes Panel

Shows the status of each managed node:

| Column | Description |
|--------|-------------|
| Node | Node number |
| Status | WFC, Online, Starting, Stopping, or Inactive |
| User | Logged-in user, "\<waiting\>", or "Log-on" |
| Activity | Current activity (wider terminals only) |
| Time | Session duration (MM:SS) |

The selected node is highlighted. Use arrow keys to navigate. Scroll indicators appear when there are more nodes than fit on screen.

### 4.5. Callers Panel

Displays recent caller history:

| Column | Description |
|--------|-------------|
| Node | Node where call occurred |
| Calls | User's total call count |
| Name | Caller name/alias |
| Date/Time | Connection time (wider terminals) |
| City | Caller location (very wide terminals) |

The title indicates how many callers are shown (e.g., "Last 8"). The bottom border displays today's caller count.

### 4.6. Keyboard Commands

| Key | Action |
|-----|--------|
| `1`-`9` | Select node 1-9 |
| `↑` `↓` | Move node selection |
| `K` | Kick user on selected node |
| `R` | Restart selected node |
| `S` | Snoop mode (reserved) |
| `Tab` `←` `→` | Switch System tabs (compact mode) |
| `Q` | Quit MAXTEL |

---

## 5. Responsive Layouts

MAXTEL adapts its display to the terminal size. Three layout modes are supported.

### 5.1. Compact Mode (80x25)

For terminals 80 columns wide or narrower:

- System panel uses tabs (Info/Stats) — press `Tab` to switch
- Node columns are condensed (no Activity column)
- Caller names truncated to 14 characters
- Status bar shows `[Cmp]`

### 5.2. Medium Mode (100x30)

For terminals at least 100 columns wide:

- System panel shows Info and Stats side by side
- Callers display date/time of each call
- Additional space for longer names and values
- Status bar shows `[Med]`

### 5.3. Full Mode (132x40+)

For terminals at least 132 columns wide and 40 rows:

- Nodes show the Activity column
- Callers show city information
- All names and values display at full length
- More nodes and callers visible
- Status bar shows `[Full]`

### 5.4. Terminal Resizing

MAXTEL responds to terminal resize events (SIGWINCH). When you resize the terminal window, MAXTEL detects the new dimensions, selects the appropriate layout, and redraws the interface. No restart is required.

---

## 6. Troubleshooting

### 6.1. Common Issues

**"Failed to bind to port"**

MAXTEL cannot listen on the specified port. Common causes:
- Another program is using the port
- Ports below 1024 require root privileges
- Firewall blocking the port

Try a different port, check for conflicts with `netstat -an | grep LISTEN`, or run as root for privileged ports.

---

**Nodes stuck in "Starting"**

The Maximus executable may be failing to start. Verify the `-m` path points to a valid binary. Try running max manually. Check `maxtel.log` for errors.

---

**Users connect but see nothing**

There may be a PTY allocation or Maximus configuration issue. Ensure Maximus is configured for telnet/socket operation. Verify the max binary has execute permissions.

---

**Interface displays incorrectly**

The terminal may not be reporting its size correctly. Try the `-s` option to force a specific size. Ensure the terminal supports ncurses and standard ANSI escape sequences.

---

**Nodes restart continuously**

Maximus may be crashing on startup. Check `maxtel.log` for errors. Try running max manually with the same options to see error output.

### 6.2. Log Files

MAXTEL creates `maxtel.log` in the working directory (or the directory specified with `-d`). The log contains:

- Startup and shutdown messages
- Node spawn and termination events
- Connection attempts and assignments
- Error messages and warnings

To monitor the log in real-time:

```bash
tail -f maxtel.log
```

The log is overwritten each time MAXTEL starts. To preserve logs:

```bash
maxtel -H 2>&1 | tee -a maxtel-$(date +%Y%m%d).log
```

### 6.3. Getting Help

For additional assistance:

- Maximus-CBCS documentation for BBS configuration issues
- Project repository: https://github.com/LimpingNinja/maximus
- Source code: `maxtel/maxtel.c`

---

*Happy BBSing!*