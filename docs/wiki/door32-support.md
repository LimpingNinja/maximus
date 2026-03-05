---
layout: default
title: "Door32 Support"
section: "configuration"
description: "Native Door32 protocol — how Maximus passes the caller's socket directly to door programs"
---

Door32 is the cleanest way to run door games on a modern telnet BBS. Unlike
the legacy approach (where Maximus creates a PTY and bridges data back and
forth), Door32 passes the caller's **actual TCP socket** directly to the door
program. The door reads and writes the socket itself — no middleman, no
translation, no latency from PTY bridging.

Maximus has native Door32 support built in. If you're setting up a new door
and it supports Door32, this is the method to use.

---

## How It Works

When you run a door with `xtern_door32` (or its alias `door32_run`), here's
what happens under the hood:

1. **Dropfiles are written.** Maximus writes all four dropfile formats to the
   node temp directory, including `door32.sys`. Line 2 of `door32.sys`
   contains the file descriptor number of the caller's TCP socket.

2. **The door is forked.** Maximus forks a child process for the door program.
   Unlike `xtern_run` (which sets up a PTY and redirects stdin/stdout to it),
   `xtern_door32` **preserves the session socket fd** in the child process
   without redirecting stdio.

3. **The door reads `door32.sys`.** The door opens `door32.sys`, reads the
   socket handle from line 2, and uses that fd directly for all caller I/O
   (reads, writes, telnet negotiation if needed).

4. **No PTY bridging.** Because the door talks to the socket directly, there's
   no PTY in the middle. This eliminates the select/read/write relay loop that
   `xtern_run` uses, reducing latency and avoiding terminal translation
   issues.

5. **Cleanup.** When the door exits, Maximus resumes the session, cleans up
   dropfiles, and re-displays the returning screen.

---

## Configuring a Door32 Door

### Menu Option

In your menu TOML file, use `xtern_door32` (or `door32_run`) as the command:

```toml
[[option]]
command = "xtern_door32"
arguments = "/opt/doors/lordlegacy/lord"
description = "Legend of the Red Dragon"
key = "L"
```

The `arguments` field is the command line passed to `/bin/sh -c`. You can
include arguments, environment variables, or anything else the shell
understands.

### Dropfile Path

Most Door32 doors expect to find `door32.sys` in a specific directory. You
have two options:

1. **Let the door find it in the node temp dir.** Pass the node temp path as
   an argument:

   ```toml
   arguments = "/opt/doors/mydoor/mydoor -d %n"
   ```

   The `%n` expands to the node temp directory path (where dropfiles live).

2. **Symlink or copy.** Some doors insist on a specific path. You can write a
   small wrapper script that copies or symlinks `door32.sys` to where the
   door expects it.

### The `door32.sys` File

For reference, here's what the door reads:

```
2           ← comm type: 0=local, 2=telnet/socket
5           ← socket file descriptor number
38400       ← baud rate (nominal)
MaximusNG BBS
1           ← user record number
John Doe    ← real name
John Doe    ← alias/handle
30          ← privilege level
45          ← minutes remaining
ANSI        ← emulation
1           ← node number
```

The critical field is **line 2** — the fd number. The door calls
`read(fd, ...)` and `write(fd, ...)` (or the equivalent in its language)
on this descriptor. The socket is already connected to the caller's telnet
client.

---

## Door32 vs. stdio Doors

| | Door32 (`xtern_door32`) | stdio (`xtern_run`) |
|---|---|---|
| **I/O method** | Door reads/writes the socket fd directly | Maximus creates a PTY; door uses stdin/stdout |
| **Latency** | Minimal — direct socket access | Slightly higher — PTY relay loop |
| **Telnet negotiation** | Door handles it (or ignores it) | Maximus strips/handles telnet sequences |
| **Compatibility** | Requires Door32-aware door | Works with any program that uses stdin/stdout |
| **Best for** | Modern doors, doors you compile yourself | Legacy doors, scripts, simple programs |

**Bottom line:** If the door supports Door32, use `xtern_door32`. If it's a
simple program that reads stdin and writes stdout, use `xtern_run` (see
[stdio doors]({{ site.baseurl }}{% link config-door-games.md %}#stdio-doors)).

---

## Writing a Door32 Door

If you're writing your own door (in C, Python, Rust, etc.), the Door32
protocol is straightforward:

1. Read `door32.sys` from the path passed as your first argument (or from the
   current directory).
2. Parse line 1 (comm type) and line 2 (socket fd).
3. If comm type is `0`, you're running locally — use stdin/stdout.
4. If comm type is `2`, use `read()` and `write()` on the fd from line 2.
5. Parse the remaining lines for user info, time limits, etc.
6. Exit cleanly when done — Maximus will resume the session.

A minimal C example:

```c
int fd = atoi(line2);  /* fd from door32.sys line 2 */
write(fd, "Hello, caller!\r\n", 16);
char buf[256];
int n = read(fd, buf, sizeof(buf));
```

---

## Troubleshooting

- **Door can't find `door32.sys`** — Make sure you're passing the node temp
  directory path. Check the log for `Created door32.sys in ...` to see where
  it was written.
- **Door writes to stdout instead of the socket** — The door isn't
  Door32-aware. Use `xtern_run` instead.
- **Garbled output** — Some doors expect raw TCP sockets but the connection
  includes telnet negotiation bytes. The door needs to handle (or strip)
  telnet IAC sequences. MaxTel handles WILL/WONT/DO/DONT negotiation before
  the session reaches Maximus, but some negotiation may still occur.
- **Door exits immediately** — Check file permissions on the door binary
  and the node temp directory. Also check the Maximus log for fork/exec
  errors.

---

## See Also

- [Door Games]({{ site.baseurl }}{% link config-door-games.md %}) — overview and stdio door
  setup
- [Dropfile Formats]({{ site.baseurl }}{% link dropfile-formats.md %}) — all four dropfile
  formats in detail
- [MaxTel]({{ site.baseurl }}{% link maxtel.md %}) — the telnet supervisor that manages
  the socket Maximus passes to doors
