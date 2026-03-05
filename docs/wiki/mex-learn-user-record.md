---
layout: default
title: "Know Your Caller: Who Goes There?"
section: "mex"
description: "Lesson 3 — Peeking at the user record and building a caller profile card"
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

## Writing a Helper Function

The profile card is going to print a lot of labeled rows — "Name: Kevin",
"City: Portland", "Calls: 47" — and we don't want to repeat the same
color-code dance for every single one. This is where helper functions
earn their keep.

```c
void row(string: label, string: value)
{
  print("|03  ", label, ": |15", value, "|07\n");
}
```

Three lines. Takes a label and a value, prints them in a consistent
format — dark cyan label, bright white value, reset. Now instead of
hand-painting every row, we just call `row("Name", usr.name)`.

This is also a good time to notice: **helper functions go above `main()`**.
MEX requires functions to be declared before they're called. If `main()`
calls `row()`, then `row()` has to be defined first in the file.

## The Profile Card

Here's the whole thing. Call it `profile.mex`:

```c
#include <max.mh>

void row(string: label, string: value)
{
  print("|03  ", label, ": |15", value, "|07\n");
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
  // Header
  print("\n|11╔══════════════════════════════════════╗\n");
  print("|11║ |14Caller Profile              |11║\n");
  print("|11╠══════════════════════════════════════╣|07\n");

  // Identity
  row("Name",     usr.name);
  if (usr.alias <> "")
    row("Alias",  usr.alias);
  row("City",     usr.city);

  // Session
  print("|11║                                      ║|07\n");
  row("Calls",    uitostr(usr.times));
  row("Today",    uitostr(usr.call));
  row("Online",   uitostr(usr.time) + " min");
  row("Posted",   ltostr(usr.msgs_posted));
  row("Read",     ltostr(usr.msgs_read));

  // Transfer stats
  print("|11║                                      ║|07\n");
  row("Files up",   ultostr(usr.nup));
  row("Files down", ultostr(usr.ndown));
  row("KB up",      ultostr(usr.up));
  row("KB down",    ultostr(usr.down));

  // Settings
  print("|11║                                      ║|07\n");
  row("Video",    video_name(usr.video));
  row("Help",     help_name(usr.help));
  row("Screen",   itostr((int)usr.width) + "x" + itostr((int)usr.len));

  // Footer
  print("|11╚══════════════════════════════════════╝|07\n\n");

  return 0;
}
```

### What's New Here

**Multiple helper functions.** `row()` handles the formatting, while
`video_name()` and `help_name()` translate numeric constants into readable
strings. You'll write functions like these constantly — the user record
stores a lot of things as numbers that humans want to see as words.

**Functions that return values.** `video_name()` returns a `string`. The
`return` statement inside each branch hands back the appropriate label.
If none of the `if` conditions match, it falls through to `return "TTY"`
at the bottom — the default.

**Explicit type conversions.** `row()` expects two `string` arguments,
but fields like `usr.times` are `unsigned int` and `usr.msgs_posted` is
`long`. MEX won't silently coerce these — you need to convert them
explicitly:

| Function | Converts From |
|----------|---------------|
| `itostr()` | `int` → `string` |
| `uitostr()` | `unsigned int` → `string` |
| `ltostr()` | `long` → `string` |
| `ultostr()` | `unsigned long` → `string` |

So `row("Calls", uitostr(usr.times))` turns the number `47` into the
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
