---
layout: default
title: "Remembering Things: Dear Diary"
section: "mex"
description: "Lesson 6 — File I/O, persistence, and building a guestbook"
---

*Lesson 6 of Learning MEX*

---

> **What you'll build:** A guestbook — callers see the last few entries,
> add their own, and their words stick around for the next person who
> logs in. Persistence, finally.

## 3:25 AM

Your guessing game works. Your trivia game works. But here's the thing:
nobody will ever know. The moment the script exits, every variable
evaporates. The high score? Gone. The caller's brilliant answer? Dust.
It's like every session happens in a parallel universe that collapses the
instant it ends.

Real boards have *memory*. The guestbook remembers who signed it. The
quote-of-the-day file persists between calls. The high score table
survives a reboot. That memory lives in *files* — and this lesson teaches
your scripts to read them and write them.

## The File I/O Functions

MEX gives you a small, clean set of file operations. They work like
classic C file I/O but with a few MEX-specific touches.

### Opening a File

```c
int fd;
fd := open("guestbook.txt", IOPEN_READ);
```

`open()` takes a filename and a mode, and returns a **file descriptor** —
a small integer that represents the open file. You pass this descriptor to
every other file function. If the open fails (file doesn't exist, bad
permissions), it returns `-1`.

### File Modes

| Mode | What It Does |
|------|-------------|
| `IOPEN_READ` | Open for reading. File must exist. |
| `IOPEN_WRITE` | Open for writing. Creates or truncates. |
| `IOPEN_RW` | Open for reading and writing. |
| `IOPEN_APPEND` | Open for appending — writes go to the end. |
| `IOPEN_CREATE` | Create the file if it doesn't exist. |

You can combine modes with `+`:

```c
// Open for appending; create if it doesn't exist yet
fd := open("guestbook.txt", IOPEN_WRITE | IOPEN_APPEND | IOPEN_CREATE);
```

This is the most common pattern for files that grow over time — logs,
guestbooks, score tables. `IOPEN_WRITE` enables write access.
`IOPEN_APPEND` means every `writeln()` goes to the end of the file.
`IOPEN_CREATE` means the first caller to run the script creates the file
automatically. Combine flags with `|` (bitwise OR).

### Reading Lines

```c
int readln(int: fd, ref string: s);
```

`readln()` reads one line from the file into a string variable, stripping
the trailing newline. Returns `0` on success, non-zero on failure (usually
end of file).

The typical pattern — read every line in a loop:

```c
string: line;

while (readln(fd, line) <> -1)
{
  print(line, "\n");
}
```

That's a `while` loop from Lesson 5, driven by the return value of
`readln()`. Each iteration reads one line. When the file runs out,
`readln()` returns non-zero and the loop ends.

### Writing Lines

```c
int writeln(int: fd, string: s);
```

`writeln()` writes a string to the file and adds a newline. Returns `0`
on success.

```c
writeln(fd, "Sarah Chen was here.");
```

There's also `write()` if you need to write a specific number of bytes
without an automatic newline, and `read()` for reading raw bytes. But for
text files — which is most of what you'll do in MEX — `readln()` and
`writeln()` are all you need.

### Closing a File

```c
close(fd);
```

Always close your files when you're done. An unclosed file might not
flush its buffer, which means your last few writes could vanish. Close
it. Every time.

## The Guestbook

Here's the whole thing. Call it `guestbook.mex`:

```c
#include <max.mh>

#define GUESTBOOK "guestbook.txt"
#define MAX_SHOW  10

void show_entries()
{
  int: fd;
  int: count;
  string: line;

  fd := open(GUESTBOOK, IOPEN_READ);

  if (fd = -1)
  {
    print("|06No entries yet. You'll be the first!\n\n");
    return;
  }

  // Count total lines
  count := 0;

  while (readln(fd, line) <> -1)
    count := count + 1;

  close(fd);

  // Reopen and skip to the last MAX_SHOW entries
  fd := open(GUESTBOOK, IOPEN_READ);

  if (count > MAX_SHOW)
  {
    int: skip;
    skip := count - MAX_SHOW;

    while (skip > 0)
    {
      readln(fd, line);
      skip := skip - 1;
    }

    print("|03Showing last |15", MAX_SHOW, "|03 of |15",
          count, "|03 entries:\n\n");
  }
  else
    print("|03All |15", count, "|03 entries:\n\n");

  // Display the remaining lines
  while (readln(fd, line) <> -1)
    print("|07  ", line, "\n");

  close(fd);
  print("\n");
}

void add_entry()
{
  int: fd;
  string: message;
  string: entry;

  print("|14Leave a message (or just press Enter to skip):\n");
  input_str(message, INPUT_NLB_LINE, 0, 60, "|11> |15");

  if (message = "")
  {
    print("|06Maybe next time.\n\n");
    return;
  }

  // Build the entry: "Name -- message"
  entry := usr.name + " -- " + message;

  fd := open(GUESTBOOK, IOPEN_WRITE | IOPEN_APPEND | IOPEN_CREATE);

  if (fd = -1)
  {
    print("|12Error: couldn't open guestbook for writing.\n");
    return;
  }

  writeln(fd, entry);
  close(fd);

  print("|10Signed!|07 Thanks, ", usr.name, ".\n\n");
}

int main()
{
  print("\n|11═══════════════════════════════════════\n");
  print("|14  The Guestbook\n");
  print("|11═══════════════════════════════════════|07\n\n");

  show_entries();
  add_entry();

  return 0;
}
```

### What's New Here

**`#define` for filenames and constants.** `GUESTBOOK` and `MAX_SHOW` are
defined at the top so you can change them in one place. If you move the
guestbook file or want to show 20 entries instead of 10, one edit does it.

**Error handling with `fd = -1`.** Every `open()` call checks the return
value. If the file doesn't exist when reading, that's fine — we display a
"no entries yet" message. If it doesn't exist when writing, that's a real
error and we tell the caller.

**Two-pass reading.** The `show_entries()` function reads the file twice:
once to count lines, then again to display the last N entries. This is the
simplest approach — MEX files are small, and the double-read is
instantaneous. You `close()` after counting, then `open()` again for
display.

**`readln()` in a loop.** The `while (readln(fd, line) <> -1)` pattern is
the idiomatic way to process a text file in MEX. Each call to `readln()`
reads one line and returns the string length on success. When it hits
end-of-file, it returns `-1` and the loop exits.

**String concatenation for the entry.** The line
`entry := usr.name + " -- " + message` builds the guestbook entry from
the caller's name and their message, connected with a double dash. This is
the `+` operator doing string concatenation, same as you saw in Lesson 3.

**`IOPEN_WRITE | IOPEN_APPEND | IOPEN_CREATE`** is the golden mode for
files that grow. `WRITE` enables write access. `APPEND` means writes go to
the end — you never overwrite existing entries. `CREATE` means the file
springs into existence if it doesn't already exist. Together, they make a
file that "just works" whether it's the first call or the thousandth.

## Compile and Run

```
mex guestbook.mex
```

Wire it to a menu and test it. The first time you run it, you'll see
"No entries yet." Sign it. Run it again — your entry is there. Log in as
a different user, run it again — both entries show. The file persists
between calls, between sessions, between reboots.

That's the magic. Your board has a memory now.

## Other File Functions Worth Knowing

| Function | What It Does |
|----------|-------------|
| `read(fd, s, len)` | Read up to `len` bytes into string `s` |
| `write(fd, s, len)` | Write `len` bytes from string `s` |
| `seek(fd, pos, where)` | Move the read/write position |
| `tell(fd)` | Get the current position in the file |
| `fileexists(name)` | Returns true if the file exists (no open needed) |
| `filesize(name)` | Returns the file size in bytes |
| `remove(name)` | Delete a file |
| `rename(old, new)` | Rename a file |

`fileexists()` is especially handy — you can check for a file *before*
trying to open it, which makes your error handling cleaner:

```c
if (fileexists(GUESTBOOK))
  show_entries();
else
  print("|06No guestbook yet. Be the first!\n");
```

## A Word About Paths

File paths in MEX are relative to the Maximus working directory unless
you specify an absolute path. For a guestbook that should live in your
data directory, you might use something like:

```c
#define GUESTBOOK "data/guestbook.txt"
```

Or use `prm_string()` to get system paths from your configuration and
build the path dynamically. The important thing: know where your files
land, and make sure the directory exists before you try to write to it.

## What You Learned

- **`open()` and `close()`** — open a file by name and mode, get a file
  descriptor, close it when done.
- **`readln()` and `writeln()`** — line-oriented text I/O. The bread and
  butter of MEX file handling.
- **File modes** — `IOPEN_READ`, `IOPEN_WRITE | IOPEN_APPEND | IOPEN_CREATE`,
  and friends. Combine with `|`.
- **Error handling** — check for `fd = -1` after every `open()`.
- **The `while (readln(...) <> -1)` pattern** — the standard way to read
  a file line by line.
- **`fileexists()`** — check before you open, if you prefer.
- **`#define`** for constants keeps your code flexible and readable.

## Next

Your board can read and write files. It can remember things. But the
interface is still just `print()` and `input_str()` — text flowing down
the screen, line by line, like a teletype from 1972.

What if your menus could *move*? Highlight bars. Arrow key navigation.
The kind of UI that makes your board feel like software instead of
scrolling text.

Next lesson: the lightbar. It's where MEX gets fancy.

**[Lesson 7: "Press Any Key to Be Amazing" →]({{ site.baseurl }}{% link mex-learn-menus.md %})**
