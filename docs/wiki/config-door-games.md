---
layout: default
title: "Door Games"
section: "configuration"
description: "Running external door games and programs — overview, stdio doors, and configuration"
---

Door games are one of the defining features of the BBS experience. Whether
it's a classic like Legend of the Red Dragon, a text adventure, or a custom
utility you wrote yourself, Maximus can launch external programs, pass them
caller information through dropfiles, and resume the session when the door
exits.

This page covers the basics of how doors work in Maximus, what you need to
know about modern vs. legacy doors, and how to set up the most common door
type — **stdio doors** that read stdin and write stdout. For the more advanced
**Door32 protocol** (direct socket-passing), see
[Door32 Support]({{ site.baseurl }}{% link door32-support.md %}). For details on the dropfile
formats Maximus generates, see
[Dropfile Formats]({{ site.baseurl }}{% link dropfile-formats.md %}).

---

## How Doors Work

When a caller selects a menu option that launches a door, Maximus:

1. **Writes dropfiles** — four formats (`Dorinfo1.Def`, `Door.Sys`,
   `Chain.Txt`, `door32.sys`) are written to the node temp directory. These
   tell the door who the caller is, how much time they have, and how to
   communicate.

2. **Displays a "leaving" screen** — if configured, a display file is shown
   to the caller before the door launches.

3. **Forks and executes the door** — the command line from the menu option's
   `arguments` field is passed to `/bin/sh -c`. How I/O is handled depends on
   the execution method (see below).

4. **Waits for exit** — Maximus monitors the child process, watches for
   carrier loss, and bridges data between the caller and the door (for
   stdio-based methods).

5. **Resumes the session** — when the door exits, Maximus cleans up, displays
   a "returning" screen if configured, and drops the caller back into the
   menu.

---

## A Note About Legacy 16-bit Doors

If you're coming from the DOS BBS world, you may have a library of 16-bit
door games compiled for real-mode DOS. **These will not run on modern UNIX
systems.** There's no 16-bit x86 execution environment on Linux, FreeBSD, or
macOS.

Your options for classic DOS doors:

- **Find a recompiled or rewritten version.** Many popular doors (LORD,
  TradeWars, BRE, Usurper) have been rewritten or ported for modern
  platforms. Check the retro BBS community.
- **Use DOSBox or DOSEMU.** You can wrap a DOS door in a DOSBox instance and
  launch it as a stdio door. This works but adds complexity and overhead.
- **Use a Door32-native reimplementation.** Some doors have been updated to
  support Door32 natively, which is the cleanest integration.

The bottom line: if you can find a native Linux/BSD binary or a Door32-aware
version of a door, use that. If you must run the original DOS binary, a
DOSBox wrapper is the way to go.

---

## Execution Methods

Maximus supports several ways to launch external programs. Each is a menu
option command:

| Command | Alias | What It Does |
|---------|-------|-------------|
| `xtern_run` | — | Fork + exec with PTY bridging (stdio door) |
| `xtern_door32` | `door32_run` | Fork + exec with direct socket fd passing |
| `xtern_dos` | `xtern_os2`, `xtern_shell` | Fork + exec via `system()` (shell command) |
| `xtern_concur` | — | Fork + exec, runs concurrently (non-blocking) |
| `xtern_chain` | — | Chain execution — replaces the current process |
| `xtern_erlvl` | — | Errorlevel exit (legacy DOS only; converted to `xtern_run` on UNIX) |

For most doors, you'll use one of two:

- **`xtern_run`** — for stdio doors (the most common case)
- **`xtern_door32`** — for Door32-aware doors (the best case)

The others are specialized: `xtern_dos` is for shell commands where you don't
need PTY bridging, `xtern_concur` launches something in the background,
`xtern_chain` replaces Maximus entirely (used for system-level handoffs), and
`xtern_erlvl` is a legacy DOS mechanism that doesn't apply on UNIX.

---

## stdio Doors {#stdio-doors}

A **stdio door** is any program that reads from stdin and writes to stdout.
This is the simplest and most common type of door on a UNIX system. It could
be a compiled binary, a Python script, a Bash script, or anything else that
uses standard I/O.

When you use `xtern_run`, Maximus:

1. Opens a **PTY** (pseudo-terminal) pair — a master fd and a slave fd.
2. Forks a child process.
3. In the child: redirects stdin, stdout, and stderr to the slave fd, then
   execs the door program.
4. In the parent: runs a **relay loop** that copies data between the caller's
   TCP socket (via MaxTel) and the PTY master fd.

The result: the door program thinks it's talking to a terminal. The caller
thinks they're talking to the BBS. Maximus sits in the middle, shuttling
bytes back and forth.

### Setting Up a stdio Door

**Step 1: Place the door binary or script.**

Put your door program somewhere accessible. A common convention:

```
/opt/maximus/doors/mygame/mygame
```

Make sure the binary is executable:

```bash
chmod +x /opt/maximus/doors/mygame/mygame
```

**Step 2: Add a menu option.**

In your menu TOML file (e.g., `config/menus/doors.toml`), add an option:

```toml
[[option]]
command = "xtern_run"
arguments = "/opt/maximus/doors/mygame/mygame %n"
description = "My Awesome Game"
key = "M"
```

The `arguments` field is the full command line. `%n` expands to the node temp
directory path, so the door can find its dropfiles.

**Step 3: Configure the door (if needed).**

Many doors need to know where to find their dropfile. Common patterns:

```toml
# Door reads DOOR.SYS from the node temp dir
arguments = "/opt/doors/mygame/mygame -d %n/Door.Sys"

# Door reads from a specific path — use a wrapper
arguments = "/opt/doors/mygame/run.sh %n"
```

A typical wrapper script (`run.sh`):

```bash
#!/bin/bash
DROPDIR="$1"
cp "$DROPDIR/Door.Sys" /opt/doors/mygame/
cd /opt/doors/mygame
./mygame
```

**Step 4: Test locally.**

Before exposing the door to callers, test it from a local console session:

1. Start Maximus locally: `bin/runbbs.sh -c`
2. Navigate to the door menu and launch it.
3. Check the log (`log/max.log`) for any fork/exec errors.

### Menu Option Modifiers

You can add modifiers to control door behavior:

```toml
[[option]]
command = "xtern_run"
arguments = "/opt/doors/mygame/mygame %n"
description = "My Awesome Game"
key = "M"
modifiers = ["Reread"]
```

| Modifier | What It Does |
|----------|-------------|
| `Reread` | Re-read the user record from disk after the door exits (useful if the door modifies user data) |
| `Ctl` | Write an Opus-compatible `.CTL` file for the door |
| `NoFix` | Don't adjust lastread pointers before launching |

### Command-Line Substitutions

The `arguments` field supports `%` substitutions:

| Code | Expands To |
|------|-----------|
| `%n` | Node temp directory path (where dropfiles are) |
| `%p` | COM port number |
| `%b` | Baud rate |
| `%t` | Time remaining (minutes) |
| `%f` | First name |
| `%l` | Last name |
| `%u` | Full user name |

---

## Security Considerations

- **Privilege gating:** Use `priv_level` on the menu option to restrict who
  can run doors:

  ```toml
  [[option]]
  command = "xtern_run"
  arguments = "/opt/doors/mygame/mygame"
  priv_level = 30
  ```

- **File permissions:** Doors run as the same user that runs Maximus. Make
  sure door binaries and data directories have appropriate permissions.
  Don't run Maximus as root.

- **Carrier watch:** Maximus monitors the connection while a door runs. If
  the caller drops carrier, Maximus sends SIGTERM to the door process,
  waits 250ms, then sends SIGKILL. This prevents zombie doors from
  consuming resources.

- **Time enforcement:** The caller's remaining time is passed to the door
  via dropfiles. Well-behaved doors enforce this; badly-behaved ones don't.
  If `strict_xfer` is enabled in
  [Session & Login]({{ site.baseurl }}{% link config-session-login.md %}), Maximus will kill
  the door when time runs out.

---

## See Also

- [Dropfile Formats]({{ site.baseurl }}{% link dropfile-formats.md %}) — all four dropfile
  formats, field-by-field
- [Door32 Support]({{ site.baseurl }}{% link door32-support.md %}) — native socket-passing
  for Door32-aware doors
- [Core Settings]({{ site.baseurl }}{% link config-core-settings.md %}) — `temp_path`,
  `doors_path` configuration
- [Session & Login]({{ site.baseurl }}{% link config-session-login.md %}) — `strict_xfer`
  and timeout settings
