---
layout: default
title: "Know Your Caller: Who Goes There?"
section: "mex"
description: "Lesson 3 -- Peeking at the user record and building a caller profile card"
---

*Lesson 3 of Learning MEX*

---

> **What you'll build:** A "caller profile card" that shows the user
> everything your board knows about them — name, location, stats, time
> remaining — formatted in an ANSI box.

## 2:38 AM

Your board talks. Your board listens. But right now it has the memory of
a goldfish — it knows things about the caller, but only because you
happened to use `usr.name` in a print statement. You haven't really
*looked* at what's in that structure.

Time to look.

The `usr` structure is Maximus's real-time dossier on whoever's connected.
It knows their name, where they're from, how many times they've called,
how long they've been online today, how many messages they've posted, their
screen dimensions, their access level, and a startling number of other
things. It's all just sitting there, waiting for a script to do something
interesting with it.

This lesson builds a profile card — a little vanity screen that shows the
caller their own stats in a tidy box. It's the kind of thing users will
check every login, like looking at their reflection in a window. And it
teaches you the most important data structure in MEX.

## The usr Struct — The Full Picture

You've already met `usr.name`, `usr.alias`, `usr.city`, and `usr.times`.
Here's the rest of the cast — the fields you'll actually use:

### Identity

| Field | Type | What It Holds |
|-------|------|--------------|
| `usr.name` | `string` | Full name |
| `usr.alias` | `string` | Alias (if set) |
| `usr.city` | `string` | City and state/province |
| `usr.sex` | `char` | `SEX_UNKNOWN`, `SEX_MALE`, or `SEX_FEMALE` |

### Access & Session

| Field | Type | What It Holds |
|-------|------|--------------|
| `usr.priv` | `unsigned int` | Privilege level |
| `usr.times` | `unsigned int` | Total calls to the system |
| `usr.call` | `unsigned int` | Calls *today* |
| `usr.time` | `unsigned int` | Minutes online today |
| `usr.help` | `char` | Help level: `HELP_NOVICE` (6), `HELP_REGULAR` (4), `HELP_EXPERT` (2) |
| `usr.hotkeys` | `char` | 1 if hotkeys are enabled |
| `usr.video` | `char` | `VIDEO_TTY` (0), `VIDEO_ANSI` (1), `VIDEO_AVATAR` (2) |

### Message & File Stats

| Field | Type | What It Holds |
|-------|------|--------------|
| `usr.msgs_posted` | `long` | Total messages posted |
| `usr.msgs_read` | `long` | Total messages read |
| `usr.up` | `unsigned long` | Kilobytes uploaded (all time) |
| `usr.down` | `unsigned long` | Kilobytes downloaded (all time) |
| `usr.nup` | `unsigned long` | Number of files uploaded |
| `usr.ndown` | `unsigned long` | Number of files downloaded (all time) |

### Screen

| Field | Type | What It Holds |
|-------|------|--------------|
| `usr.width` | `char` | Terminal width (columns) |
| `usr.len` | `char` | Terminal height (rows) |

There are more fields for subscriptions, protocols, keys, and dates — but
these are the ones that matter for 90% of scripting. You can always peek
at `max.mh` for the full list when you need something exotic.

## The Two-Phase Approach

Drawing a box with data inside sounds simple, but there's a subtle trap.
If you try to print each row as a single line — left border, label, value,
padding, right border — you need the right border to land at exactly the
right column every time. That means calculating padding widths based on
the label length, the value length, and the column the cursor is on. MCI
operators like `$X` can help, but they rely on the MCI layer accurately
tracking the cursor position through color codes and hex escapes, which
doesn't always work reliably.

There's a cleaner way: **draw the box first, then fill it in.**

### Phase 1 — Draw the frame

Print the entire box — top border, header, separator, 19 empty interior
rows, bottom border — all at once. Every interior row is just
`║` + 38 spaces + `║`. The borders are guaranteed to align because there's
no variable-length data on those rows.

### Phase 2 — Place data with AVATAR_GOTO

Once the frame is on screen, use `AVATAR_GOTO` to move the cursor back
*into* the box at specific (row, column) coordinates. Print the label
and value right there. The borders are already drawn — there's nothing to
pad, nothing to align.

`AVATAR_GOTO` is defined in `max.mh`:

```c
#define AVATAR_GOTO "\x16\x08"
```

To position the cursor at row `r`, column `c` (1-based):

```c
print(AVATAR_GOTO, (char)r, (char)c);
```

The two bytes after `\x16\x08` are the row and column encoded as raw
character values. This is an AVATAR terminal command — it works at the
byte level, completely independent of MCI processing. It's the same
mechanism used by `oneliner.mex` and other production scripts in the
Maximus distribution.

## Writing a Helper Function

The profile card is going to print a lot of labeled rows — "Name: Kevin",
"City: Portland", "Calls: 47" — and we don't want to repeat the same
positioning dance for every single one. This is where helper functions
earn their keep.

```c
void goto_rc(int: r, int: c)
{
  print(AVATAR_GOTO, (char)r, (char)c);
}

void field(int: r, string: label, string: value)
{
  goto_rc(r, DATA_COL);
  print("|03", label, ": |15", value, "|07");
}
```

`goto_rc()` wraps the AVATAR positioning into a readable call. `field()`
jumps to the target row, then prints the label in cyan and the value in
bright white. No padding, no border characters — the box frame is already
there from Phase 1.

This is also a good time to notice: **helper functions go above `main()`**.
MEX requires functions to be declared before they're called. If `main()`
calls `field()`, then `field()` has to be defined first in the file.

## The Profile Card

Here's the whole thing. Call it `profile.mex`:

```c
#include <max.mh>

#define BOX_WIDTH   40        // total columns: left ║ + 38 interior + right ║
#define BOX_INNER   38        // interior character count
#define DATA_COL     4        // col where labels start (2-space indent from ║)

// Position cursor at (row, col) -- 1-based
void goto_rc(int: r, int: c)
{
  print(AVATAR_GOTO, (char)r, (char)c);
}

// Draw the complete 23-row box frame
void draw_box()
{
  int: i;
  string: hbar, blank;

  // Build the horizontal bar: 38 ═ characters
  hbar := "";
  for (i := 0; i < BOX_INNER; i := i + 1)
    hbar := hbar + "\xCD";

  // Build the blank interior fill: 38 spaces
  blank := strpad("", BOX_INNER, ' ');

  // Row 1: top border  ╔═══╗
  print("|11\xC9", hbar, "\xBB\n");

  // Row 2: header      ║ Caller Profile ... ║
  print("\xBA |14Caller Profile", strpad("", 23, ' '), "|11\xBA\n");

  // Row 3: separator   ╠═══╣
  print("\xCC", hbar, "\xB9\n");

  // Rows 4-22: 19 empty interior rows  ║ (spaces) ║
  for (i := 0; i < 19; i := i + 1)
    print("\xBA", blank, "\xBA\n");

  // Row 23: bottom border  ╚═══╝
  print("\xC8", hbar, "\xBC|07\n");
}

// Print a labelled field at a given row inside the pre-drawn box
void field(int: r, string: label, string: value)
{
  goto_rc(r, DATA_COL);
  print("|03", label, ": |15", value, "|07");
}

string video_name(char: mode)
{
  if (mode = VIDEO_ANSI)
    return "ANSI";
  else if (mode = VIDEO_AVATAR)
    return "Avatar";

  return "TTY";
}

string help_name(char: level)
{
  if (level = HELP_NOVICE)
    return "Novice";
  else if (level = HELP_EXPERT)
    return "Expert";

  return "Regular";
}

int main()
{
  // Clear the screen — puts cursor at row 1, col 1
  print(AVATAR_CLS);

  // Phase 1: draw the complete box frame
  draw_box();

  // Phase 2: fill data with cursor positioning
  field(5,  "Name",       usr.name);
  field(6,  "Alias",      usr.alias);
  field(7,  "City",       usr.city);

  field(9,  "Calls",      uitostr(usr.times));
  field(10, "Today",      uitostr(usr.call));
  field(11, "Online",     uitostr(usr.time) + " min");
  field(12, "Posted",     ltostr(usr.msgs_posted));
  field(13, "Read",       ltostr(usr.msgs_read));

  field(15, "Files up",   ultostr(usr.nup));
  field(16, "Files down", ultostr(usr.ndown));
  field(17, "KB up",      ultostr(usr.up));
  field(18, "KB down",    ultostr(usr.down));

  field(20, "Video",  video_name(usr.video));
  field(21, "Help",   help_name(usr.help));
  field(22, "Screen", itostr((int)usr.width) + "x" + itostr((int)usr.len));

  // Move cursor below the box
  goto_rc(24, 1);
  print("|07\n");

  return 0;
}
```

### What's New Here

**AVATAR cursor positioning.** Instead of trying to pad every row to a
fixed width, the script draws the entire box frame first (Phase 1), then
uses `AVATAR_GOTO` to jump to exact screen coordinates and print data
(Phase 2). The `goto_rc()` helper wraps the low-level AVATAR escape
(`\x16\x08` + row byte + col byte) into a clean function call. This is
the same technique used by `oneliner.mex` and other production scripts.

**Two-phase rendering.** Phase 1 draws borders and empty interior using
simple loops — `for` loops that build strings of repeated characters
(`\xCD` for horizontal lines, spaces for blank rows). Phase 2 fills in
data at absolute positions. Because the frame is already on screen, the
data can be any length without breaking the layout.

**`strpad()` for fixed-width strings.** The `strpad("", 38, ' ')` call
produces exactly 38 spaces — used to fill the interior of each empty row.
Similarly, `strpad("", 23, ' ')` pads the header text to fill the row.
This is more reliable than MCI `$D` repeating inside `print()`.

**`AVATAR_CLS` for screen clearing.** Defined in `max.mh` as `"\x0c"`,
this clears the screen and resets the cursor to row 1, column 1 — giving
us a known starting position for the box.

**Multiple helper functions.** `field()` handles positioning and formatting,
`goto_rc()` wraps cursor movement, while `video_name()` and `help_name()`
translate numeric constants into readable strings. You'll write functions
like these constantly — the user record stores a lot of things as numbers
that humans want to see as words.

**Functions that return values.** `video_name()` returns a `string`. The
`return` statement inside each branch hands back the appropriate label.
If none of the `if` conditions match, it falls through to `return "TTY"`
at the bottom — the default.

**Explicit type conversions.** `field()` expects two `string` arguments,
but fields like `usr.times` are `unsigned int` and `usr.msgs_posted` is
`long`. MEX won't silently coerce these — you need to convert them
explicitly:

| Function | Converts From |
|----------|---------------|
| `itostr()` | `int` → `string` |
| `uitostr()` | `unsigned int` → `string` |
| `ltostr()` | `long` → `string` |
| `ultostr()` | `unsigned long` → `string` |

So `field(9, "Calls", uitostr(usr.times))` turns the number `47` into the
string `"47"` before passing it. For `char` fields like `usr.width`,
cast to `int` first: `itostr((int)usr.width)`.

Note that `print()` *can* accept mixed types directly — it handles
the conversion internally. But when you're calling your own functions
that declare `string` parameters, you need the explicit conversion.
This is one of MEX's sharp edges — get friendly with these four
functions early.

**The `else if` pattern.** MEX doesn't have a `switch` statement. For
multi-way branches, you chain `if` / `else if` / `else`. It's not as
pretty as a switch, but it works fine for small sets of values like video
modes and help levels.

**CP437 box characters.** All box-drawing characters use `\xHH` hex
escapes (`\xC9` for ╔, `\xBB` for ╗, `\xBA` for ║, `\xCD` for ═, etc.)
instead of UTF-8 multi-byte sequences, which ensures correct display on
CP437 terminals.

## Compile and Run

```
mex profile.mex
```

Wire it up and give it a spin. You'll see a box with everything the board
knows about you — call count, transfer stats, video mode, the lot. Log in
as different users and watch the numbers change.

## Why This Matters

The profile card itself is nice, but that's not really the point. The
point is that you now know how to read from the user record and act on
what you find.

That unlocks everything:

- **Conditional greetings** — "Welcome back!" for regulars, a tutorial
  for first-timers
- **Access-gated features** — hide sysop tools from normal callers by
  checking `usr.priv`
- **Time warnings** — "You have 5 minutes left" when `usr.time` gets
  close to the limit
- **Personalized menus** — show different options based on who's calling

The user record is the single most useful data source in MEX. Every
interesting script you write will read from it. Get comfortable here.

## What You Learned

- **The `usr` structure** has fields for identity, access, session stats,
  message/file stats, and screen settings — all live, all current.
- **Two-phase rendering** — draw the complete box frame first, then use
  cursor positioning to fill in data. This avoids padding/alignment issues.
- **AVATAR_GOTO** (`\x16\x08` + row + col) positions the cursor at
  absolute screen coordinates. It works at the byte level, independent of
  MCI processing.
- **Helper functions** reduce repetition. Declare them above `main()`,
  call them from anywhere below.
- **Functions can return values** — use `return` inside the function body.
  The return type goes before the function name.
- **Type conversion functions** — `itostr()`, `uitostr()`, `ltostr()`,
  `ultostr()` convert numbers to strings for function arguments.
  `print()` handles mixed types directly, but your own functions don't.
- **`else if` chains** are MEX's substitute for switch/case.
- **Constants like `VIDEO_ANSI` and `HELP_NOVICE`** are defined in
  `max.mh` — use them instead of raw numbers for readability.

## Next

You've got data. You've got output. You've got functions. But every
script you've written so far runs in a straight line — top to bottom, no
detours. Real programs make *choices*. They go left or right depending on
what's happening.

Next lesson: branching. The roads diverge. You get to pick.

**[Lesson 4: "Choose Your Own Adventure" →]({{ site.baseurl }}{% link mex-learn-decisions.md %})**
