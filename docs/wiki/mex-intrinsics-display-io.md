---
layout: default
title: "Display & I/O"
section: "mex"
description: "Output, input, file I/O, color, static data, and the utility functions every MEX script needs"
---

This is the kitchen-sink page — the functions that almost every MEX script
touches. Printing to the screen, reading keypresses, prompting for input,
changing colors, reading and writing files, and the handful of utility
functions that don't fit neatly anywhere else.

All of these live in `max.mh`, which you're already including. A few
convenience wrappers live in optional headers (`input.mh`, `intpad.mh`,
`rand.mh`) — those are noted where they appear.

---

## On This Page

- [Quick Reference](#quick-reference)
- [Output](#output)
- [Color & Display Codes](#color)
- [Input](#input)
- [More Prompt](#more-prompt)
- [File I/O](#file-io)
- [Filesystem](#filesystem)
- [Static Data](#static-data)
- [Random Numbers](#random)
- [Miscellaneous](#misc)

---

## Quick Reference {#quick-reference}

### Output

| Function | What It Does |
|----------|-------------|
| [`print()`](#print) | Print strings, numbers, and characters to the caller |
| [`display_file()`](#display-file) | Display an ANSI/ASCII/MECCA file with paging |
| [`vidsync()`](#vidsync) | Flush the output buffer |
| [`set_output()`](#set-output) | Enable/disable local or remote output |

### Input

| Function | What It Does |
|----------|-------------|
| [`getch()`](#getch) | Read a single keypress (no echo) |
| [`kbhit()`](#kbhit) | Check if a key is waiting |
| [`localkey()`](#localkey) | Read a local console keypress |
| [`input_str()`](#input-str) | Read a line or word with options |
| [`input_ch()`](#input-ch) | Read a character with options |
| [`input_list()`](#input-list) | Read from a list of valid responses |

### File I/O

| Function | What It Does |
|----------|-------------|
| [`open()`](#open) | Open a file |
| [`read()`](#read) | Read bytes from a file |
| [`readln()`](#readln) | Read a line from a file |
| [`write()`](#write) | Write bytes to a file |
| [`writeln()`](#writeln) | Write a line to a file |
| [`tell()`](#tell) | Get current file position |
| [`seek()`](#seek) | Move file position |
| [`close()`](#close) | Close a file |

### Filesystem

| Function | What It Does |
|----------|-------------|
| [`fileexists()`](#fileexists) | Check if a file exists |
| [`filesize()`](#filesize) | Get file size in bytes |
| [`filedate()`](#filedate) | Get file modification date |
| [`rename()`](#rename) | Rename a file |
| [`remove()`](#remove) | Delete a file |
| [`filecopy()`](#filecopy) | Copy a file |
| [`filefindfirst()`](#filefind) | Start a wildcard file search |
| [`filefindnext()`](#filefind) | Continue a wildcard file search |
| [`filefindclose()`](#filefind) | End a wildcard file search |

---

## Output {#output}

{: #print}
### print(...)

The workhorse. `print()` is variadic — it accepts any number of arguments
of any type and outputs them to the caller's terminal:

```mex
print("Hello, ", usr.name, "!\n");
print("You have ", usr.times, " calls on record.\n");
print("Time left: ", timeleft(), " seconds.\n");
```

Each argument is converted to its display form automatically — strings print
as-is, integers print as decimal, characters print as their character.

`print()` understands AVATAR display codes when you embed the `COL_*`
constants:

```mex
print(COL_YELLOW, "Warning: ", COL_GRAY, "disk space is low.\n");
```

{: #display-file}
### display_file(filename, nonstop)

Display an ANSI, ASCII, or MECCA file with automatic paging:

```mex
void display_file(string: filename, ref char: nonstop);
```

```mex
char: nonstop;
nonstop := False;
display_file("welcome.ans", nonstop);
```

The `nonstop` flag tracks whether the caller pressed `N` (continuous scroll)
at a "More?" prompt. Pass the same variable across multiple display calls
to maintain state.

{: #vidsync}
### vidsync()

Flushes the output buffer immediately. Normally Maximus batches output for
efficiency — call `vidsync()` when you need something to appear on screen
right now (e.g., before a timed pause):

```mex
print("Processing...");
vidsync();
sleep(2);
print(" done!\n");
```

{: #set-output}
### set_output(where)

Control whether output goes to the remote caller, the local console, both,
or neither:

```mex
int set_output(int: where);
```

| Constant | Value | Effect |
|----------|-------|--------|
| `DISABLE_NONE` | 0 | Output to both (default) |
| `DISABLE_LOCAL` | 1 | Suppress local console output |
| `DISABLE_REMOTE` | 2 | Suppress remote output |
| `DISABLE_BOTH` | 3 | Suppress all output |

Returns the previous state so you can restore it:

```mex
int: old_state;
old_state := set_output(DISABLE_REMOTE);
print("This only shows on the local console.\n");
set_output(old_state);
```

---

## Color & Display Codes {#color}

MEX provides two ways to set colors: AVATAR inline codes (for use with
`print()`) and pipe codes (for use in displayed files and lang strings).

### AVATAR Color Constants

These are string constants that emit AVATAR attribute-change sequences when
passed to `print()`:

| Constant | Color |
|----------|-------|
| `COL_BLACK` | Black |
| `COL_BLUE` | Blue |
| `COL_GREEN` | Green |
| `COL_CYAN` | Cyan |
| `COL_RED` | Red |
| `COL_MAGENTA` | Magenta |
| `COL_BROWN` | Brown |
| `COL_GRAY` | Gray (default) |
| `COL_DKGRAY` | Dark gray |
| `COL_LBLUE` | Light blue |
| `COL_LGREEN` | Light green |
| `COL_LCYAN` | Light cyan |
| `COL_LRED` | Light red |
| `COL_LMAGENTA` | Light magenta |
| `COL_YELLOW` | Yellow |
| `COL_WHITE` | White |

Combination constants: `COL_YELLOWONBLUE`, `COL_BLACKONGREEN`,
`COL_REDONGREEN`, `COL_WHITEONGREEN`.

### AVATAR Control Sequences

| Constant | Effect |
|----------|--------|
| `AVATAR_CLS` | Clear screen |
| `AVATAR_CLEOL` | Clear to end of line |
| `AVATAR_UP` | Cursor up one line |
| `AVATAR_DOWN` | Cursor down one line |
| `AVATAR_LEFT` | Cursor left one column |
| `AVATAR_RIGHT` | Cursor right one column |

```mex
print(AVATAR_CLS);                         // clear screen
print(COL_YELLOW, "Title\n", COL_GRAY);    // yellow heading
```

For full-screen positioning and attribute control, use the
[UI Primitives]({% link mex-ui-primitives.md %}) (`ui_goto`, `ui_set_attr`,
etc.).

---

## Input {#input}

{: #getch}
### getch()

Read a single character from the caller. Blocks until a key is pressed.
No echo — the character is not displayed:

```mex
char getch();
```

```mex
char: ch;
print("Press any key...");
ch := getch();
```

{: #kbhit}
### kbhit()

Check if a keypress is waiting in the input buffer. Returns the character
if one is available, or `0` if not. Non-blocking:

```mex
char kbhit();
```

```mex
if (kbhit())
  print("Key is waiting.\n");
```

{: #localkey}
### localkey()

Read a keypress from the local console (sysop keyboard), not the remote
caller. Useful for sysop-only controls:

```mex
char localkey();
```

{: #input-str}
### input_str(s, type, ch, max, prompt)

The full-featured line input function. Reads a string from the caller with
configurable behavior:

```mex
int input_str(ref string: s, int: type, char: ch, int: max, string: prompt);
```

| Parameter | Purpose |
|-----------|---------|
| `s` | Output: the string entered by the caller |
| `type` | `INPUT_*` flags ORed together |
| `ch` | Echo character (used with `INPUT_ECHO`), or `0` |
| `max` | Maximum input length |
| `prompt` | Prompt string (displayed before input) |

**Type flags** (pick one base mode):

| Flag | What It Does |
|------|-------------|
| `INPUT_LB_LINE` | Input a line, allowing stacked input |
| `INPUT_NLB_LINE` | Input a line, not allowing stacked input |
| `INPUT_WORD` | Input a word (stops at space) |

**Modifier flags** (OR with base mode):

| Flag | What It Does |
|------|-------------|
| `INPUT_ECHO` | Echo `ch` instead of actual character (for passwords) |
| `INPUT_NOECHO` | Disable all echo |
| `INPUT_NOLF` | Don't send linefeed at end |
| `INPUT_DEFAULT` | Pre-fill with current contents of `s` |
| `INPUT_SCAN` | Allow scan codes in string |
| `INPUT_NOCTRLC` | Don't allow Ctrl+C to redisplay prompt |

```mex
string: name;
input_str(name, INPUT_NLB_LINE, 0, 30, "Enter your name: ");
```

```mex
// Password input (echo asterisks)
string: pwd;
input_str(pwd, INPUT_NLB_LINE | INPUT_ECHO, '*', 20, "Password: ");
```

{: #input-ch}
### input_ch(type, options)

Read a single character with options:

```mex
int input_ch(int: type, string: options);
```

The `type` parameter uses `CINPUT_*` flags. The `options` string serves as
either a prompt (with `CINPUT_PROMPT`) or a list of acceptable characters
(with `CINPUT_ACCEPTABLE`).

```mex
int: ch;
ch := input_ch(CINPUT_PROMPT | CINPUT_ACCEPTABLE, "YyNn");
```

{: #input-list}
### input_list(list, type, help_file, invalid_response, prompt)

Present a list of valid choices and loop until the caller picks one:

```mex
int input_list(string: list, int: type, string: help_file,
               string: invalid_response, string: prompt);
```

```mex
int: choice;
choice := input_list("ABCQ", CINPUT_PROMPT, "",
                      "Invalid choice.", "[A,B,C,Q]? ");
```

### input.mh Helpers

The optional `input.mh` header provides two convenience functions:

- **`getchp()`** — like `getch()` but echoes the character
- **`readstr(s)`** — reads a full line with echo and backspace handling

```mex
#include <input.mh>

string: s;
readstr(s);
```

---

## More Prompt {#more-prompt}

{: #reset-more}
### reset_more(nonstop)

Reset the line counter for "More?" prompting:

```mex
void reset_more(ref char: nonstop);
```

{: #do-more}
### do_more(nonstop, colour)

Check if a "More [Y,n,=]?" prompt should be displayed. Returns `0` if the
caller said "No" (stop output), non-zero to continue:

```mex
int do_more(ref char: nonstop, string: colour);
```

```mex
char: nonstop;
int: i;

nonstop := False;
reset_more(nonstop);

for (i := 1; i <= 100; i := i + 1)
{
  print("Line " + itostr(i) + "\n");
  if (do_more(nonstop, COL_CYAN) = 0)
    return;   // caller said "no more"
}
```

---

## File I/O {#file-io}

MEX provides C-style file I/O with integer file descriptors.

{: #open}
### open(name, mode)

Open a file. Returns a file descriptor (positive integer), or `-1` on error:

```mex
int open(string: name, int: mode);
```

| Flag | Value | Purpose |
|------|-------|---------|
| `IOPEN_CREATE` | `0x01` | Create the file if it doesn't exist |
| `IOPEN_READ` | `0x02` | Open for reading |
| `IOPEN_WRITE` | `0x04` | Open for writing |
| `IOPEN_RW` | `0x06` | Open for reading and writing |
| `IOPEN_APPEND` | `0x08` | Append to end of file |
| `IOPEN_BINARY` | `0x80` | Binary mode (no CR/LF translation) |

OR flags together:

```mex
int: fd;
fd := open("guestbook.txt", IOPEN_CREATE | IOPEN_APPEND);
if (fd = -1)
{
  print("Cannot open file!\n");
  return;
}
```

{: #read}
### read(fd, s, len)

Read up to `len` bytes from a file into string `s`. Returns the number of
bytes actually read:

```mex
int read(int: fd, ref string: s, int: len);
```

{: #readln}
### readln(fd, s)

Read a line (up to the next newline) from a file. Returns `0` on success,
non-zero at end of file:

```mex
int readln(int: fd, ref string: s);
```

```mex
string: line;
while (readln(fd, line) = 0)
  print(line + "\n");
```

{: #write}
### write(fd, s, len)

Write `len` bytes from string `s` to a file:

```mex
int write(int: fd, ref string: s, int: len);
```

{: #writeln}
### writeln(fd, s)

Write a string followed by a newline to a file:

```mex
int writeln(int: fd, string: s);
```

```mex
writeln(fd, usr.name + " was here!");
```

{: #tell}
### tell(fd)

Returns the current file position as a `long`:

```mex
long tell(int: fd);
```

{: #seek}
### seek(fd, pos, where)

Move the file position:

```mex
long seek(int: fd, long: pos, int: where);
```

| Constant | Value | Meaning |
|----------|-------|---------|
| `SEEK_SET` | 0 | From beginning of file |
| `SEEK_CUR` | 1 | From current position |
| `SEEK_END` | 2 | From end of file |

{: #close}
### close(fd)

Close an open file. Always close your files — open file descriptors are a
limited resource:

```mex
int close(int: fd);
```

### Complete File I/O Example

```mex
// Append a guestbook entry
int: fd;
fd := open("guestbook.txt", IOPEN_CREATE | IOPEN_APPEND);
if (fd <> -1)
{
  writeln(fd, usr.name + " - " + stamp_string(usr.ludate));
  close(fd);
}
```

---

## Filesystem {#filesystem}

{: #fileexists}
### fileexists(filename)

Returns non-zero if the file exists:

```mex
int fileexists(string: filename);
```

{: #filesize}
### filesize(filename)

Returns the file size in bytes, or `-1` if the file doesn't exist:

```mex
long filesize(string: filename);
```

{: #filedate}
### filedate(filename, stamp)

Gets the modification date of a file:

```mex
int filedate(string: filename, ref struct _stamp: filedate);
```

{: #rename}
### rename(oldname, newname)

```mex
int rename(string: oldname, string: newname);
```

{: #remove}
### remove(filename)

```mex
int remove(string: filename);
```

{: #filecopy}
### filecopy(from, to)

```mex
int filecopy(string: fromname, string: toname);
```

{: #filefind}
### File Search (filefindfirst / filefindnext / filefindclose)

Wildcard file search using the `_ffind` struct:

```mex
int filefindfirst(ref struct _ffind: ff, string: filename, int: attribs);
int filefindnext(ref struct _ffind: ff);
void filefindclose(ref struct _ffind: ff);
```

```mex
struct _ffind: ff;
if (filefindfirst(ff, "*.txt", FA_NORMAL) = 0)
{
  do
  {
    print(ff.filename + "  " + ultostr(ff.filesize) + " bytes\n");
  }
  while (filefindnext(ff) = 0);
  filefindclose(ff);
}
```

The `attribs` parameter uses `FA_*` constants (`FA_NORMAL`, `FA_HIDDEN`,
`FA_SUBDIR`, etc.).

---

## Static Data {#static-data}

Static data persists across script invocations within the same session.
Use it for caching, counters, or passing data between scripts.

### Static Strings

```mex
int create_static_string(string: key);
int get_static_string(string: key, ref string: data);
int set_static_string(string: key, string: data);
int destroy_static_string(string: key);
```

```mex
// First run: create and set
create_static_string("last_area");
set_static_string("last_area", marea.name);

// Later: retrieve
string: saved_area;
if (get_static_string("last_area", saved_area) = 0)
  print("You were last in: " + saved_area + "\n");
```

### Static Data (Binary)

For structs and raw data:

```mex
int create_static_data(string: key, long: size);
int get_static_data(string: key, ref void: data);
int set_static_data(string: key, ref void: data);
int destroy_static_data(string: key);
```

All functions return `0` on success.

---

## Random Numbers {#random}

Include `rand.mh` for pseudo-random number generation:

```mex
#include <rand.mh>

srand(time());                    // seed with current time
int: roll;
roll := (rand() % 6) + 1;        // random 1-6
print("You rolled a " + itostr(roll) + "!\n");
```

| Function | What It Does |
|----------|-------------|
| `srand(int: seed)` | Set the random seed |
| `rand()` | Returns a random integer 0–32767 |

Always call `srand()` once before using `rand()`. Use `time()` as a
convenient seed value.

---

## Miscellaneous {#misc}

### System & Session

| Function | Signature | What It Does |
|----------|-----------|-------------|
| `shell()` | `int shell(int: method, string: cmd)` | Run an external command |
| `sleep()` | `void sleep(int: duration)` | Pause for `duration` seconds |
| `log()` | `void log(string: s)` | Write a message to the system log |
| `carrier()` | `int carrier()` | Returns non-zero if carrier is present |
| `ansi_detect()` | `int ansi_detect()` | Detect ANSI terminal capability |
| `rip_detect()` | `int rip_detect()` | Detect RIP graphics capability |

### Keyboard State

| Function | What It Does |
|----------|-------------|
| `keyboard(int: state)` | Enable (1) or disable (0) local keyboard |
| `iskeyboard()` | Returns current keyboard state |
| `snoop(int: state)` | Enable (1) or disable (0) local snoop display |
| `issnoop()` | Returns current snoop state |

### Terminal Dimensions

| Function | Returns |
|----------|---------|
| `term_length()` | Caller's terminal height in rows |
| `term_width()` | Caller's terminal width in columns |
| `screen_length()` | Physical screen height |
| `screen_width()` | Physical screen width |

### System Path

```mex
string prm_string(int: stringnum);
```

Returns system path strings. `prm_string(0)` returns the BBS system name.

---

## See Also

- [Standard Intrinsics]({% link mex-standard-intrinsics.md %}) — overview of
  all intrinsic categories
- [String Operations]({% link mex-language-string-ops.md %}) — string
  manipulation functions
- [UI Primitives]({% link mex-ui-primitives.md %}) — full-screen UI:
  positioning, fields, lightbars, forms
- [User & Session]({% link mex-intrinsics-user-session.md %}) — user record
  access and session management
