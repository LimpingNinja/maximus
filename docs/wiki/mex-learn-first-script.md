---
layout: default
title: "Your First Script: Hello, Stranger"
section: "mex"
description: "Lesson 1 — Go beyond hello world and greet your callers by name"
---

*Lesson 1 of Learning MEX*

---

> **What you'll build:** A script that greets callers by name, in color,
> and actually feels like your board is paying attention.

## 2:07 AM

You got "Hello, world" working an hour ago. It compiled. It ran. The words
appeared on the screen. You felt a brief, irrational surge of triumph, the
kind that only happens when you make a computer do a thing it wasn't doing
before.

And then you stared at it.

*Hello, world.*

That's it? That's the whole experience? Two words and a blinking cursor?
Your board already says "Hello" when people log in. It says it in a `.bbs`
file that you didn't write, in colors you didn't choose, in a font that
hasn't changed since 1993. Adding another "Hello" to the pile is not
exactly revolutionary.

But what if your board knew who it was talking to?

## The usr Structure

Every time someone's connected to your board, Maximus keeps a live snapshot
of who they are in a structure called `usr`. It's always there, always
current, and your MEX scripts can read it whenever they want.

The interesting fields for right now:

| Field | Type | What It Holds |
|-------|------|--------------|
| `usr.name` | `string` | The caller's full name |
| `usr.alias` | `string` | Their alias, if they set one |
| `usr.city` | `string` | City and state/province |
| `usr.times` | `unsigned int` | How many times they've called |

There are dozens more — video mode, help level, privilege level, screen
dimensions, message stats — but these four are enough to make something
that feels personal. We'll meet the rest in
[Lesson 3]({{ site.baseurl }}{% link mex-learn-user-record.md %}).

## Your First Real Script

Create a file called `greeter.mex` in your scripts directory:

```c
#include <max.mh>

int main()
{
  print("\n|11Hello, |15", usr.name, "|11.\n\n");
  print("|03You've called |14", usr.times, "|03 time");

  if (usr.times <> 1)
    print("s");

  print(".|07\n\n");

  return 0;
}
```

Let's walk through this line by line — not because it's complicated, but
because every line here introduces something new.

### Line 1: The Include

```c
#include <max.mh>
```

Same as always. This is the line that gives you access to `print()`,
`usr`, and everything else Maximus-specific. It goes at the top. Always.

### Lines 3–4: main() and the Opening Brace

```c
int main()
{
```

Entry point. Maximus starts here. The `{` opens the function body, and
all your variable declarations would go right after it (we don't need any
local variables in this script, but that's where they'd live — see
[Script Structure]({{ site.baseurl }}{% link mex-script-structure.md %}) if you need a
refresher).

### Line 5: print() With Variables

```c
  print("\n|11Hello, |15", usr.name, "|11.\n\n");
```

This is the moment. You're not printing a static string anymore — you're
printing *three* arguments, separated by commas:

1. `"\n|11Hello, |15"` — a newline, set color to cyan (`|11`), print
   "Hello, ", switch to bright white (`|15`)
2. `usr.name` — the caller's actual name. Not a string literal.
   A variable. Pulled live from the user record.
3. `"|11.\n\n"` — back to cyan, print a period, two newlines.

`print()` takes as many arguments as you hand it, separated by commas,
and prints them all in order. Strings, variables, numbers — it doesn't
care. It just sends them to the terminal.

When a caller named **Sarah Chen** runs this script, she sees:

```
Hello, Sarah Chen.
```

In cyan and white. On her terminal. At 2 AM. And for a split second, the
board feels like it's talking to *her*.

### Lines 6–9: A Little Grammar

```c
  print("|03You've called |14", usr.times, "|03 time");

  if (usr.times <> 1)
    print("s");

  print(".|07\n\n");
```

`usr.times` is a number — how many times this caller has connected. We
print it in yellow (`|14`) sandwiched between dark cyan (`|03`) text.

Then a quick grammar check: if they've called more than once (or zero
times, somehow), we add an "s" to make it "times" instead of "time."
The `<>` operator means "not equal" in MEX — another one of those
Pascal-isms.

The result for a caller who's visited 47 times:

```
You've called 47 times.
```

For a first-timer:

```
You've called 1 time.
```

It's a tiny detail. Your callers will notice.

### Line 11: Return

```c
  return 0;
```

Done. Control goes back to Maximus, and the caller sees whatever menu
they were on.

## The Color Codes

You saw `|11`, `|15`, `|14`, `|03`, `|07` in the script. These are
**pipe color codes** — the same ones you use in ANSI display files, menu
descriptions, and everywhere else in Maximus. They work inside MEX
strings exactly the same way.

The short version:

| Code | Color |
|------|-------|
| `|01` – `|07` | Dark colors (blue, green, cyan, red, magenta, yellow/brown, white/gray) |
| `|08` – `|15` | Bright colors (dark gray through bright white) |
| `|16` – `|23` | Background colors |

`|07` resets to plain gray — it's the "go back to normal" code. You'll
use it a lot at the end of colored output so the next thing on screen
isn't accidentally bright magenta.

For the full reference, see
[Display Codes]({{ site.baseurl }}{% link display-codes.md %}).

## Compile and Run

Compile it:

```
mex greeter.mex
```

Wire it to a menu option in your menu TOML:

```toml
[[option]]
command = "MEX"
arguments = "greeter"
priv_level = "Demoted"
description = "%Greeter"
```

Log in and press the hotkey. You should see your name, your call count,
correct pluralization, and a whole lot of color.

## Going Further

This script is fine. It works. But it's a *greeting* — the kind of thing
that belongs on a welcome screen, not buried behind a menu option. So
let's think about where this actually goes.

You could wire this script as your board's login display by pointing the
`Uses Logo` path at it (prefix with `:` to tell Maximus it's a MEX
script, not a `.bbs` file). Now every caller sees a personalized welcome
the moment they connect.

Or you could keep building. Add the caller's city:

```c
  print("|03Calling from |14", usr.city, "|07\n");
```

Show their alias if they have one:

```c
  if (usr.alias <> "")
    print("|03Also known as |14", usr.alias, "|07\n");
```

Wrap the whole thing in a box made of line-drawing characters. Put an
ASCII art banner above it. Make it yours.

That's the point. MEX isn't a thing you *learn* and then *use*. It's a
thing you *use* and then *learn more* because you wanted to do something
cooler. The learning is a side effect of the building.

## What You Learned

- **`print()` takes multiple arguments** — strings, variables, numbers,
  separated by commas. It prints them all in sequence.
- **`usr.name`** (and `usr.city`, `usr.alias`, `usr.times`, and many
  more) — live data about whoever's online right now.
- **Pipe color codes** work inside MEX strings — same syntax as display
  files.
- **`<>` means "not equal"** — MEX's Pascal heritage showing through.
- **The edit-compile-test loop is fast** — save, `mex greeter.mex`,
  reload, done.

## Next

Your board can talk now. But it's doing all the talking. In the next
lesson, you'll teach it to *listen*.

**[Lesson 2: "Are You Still There?" →]({{ site.baseurl }}{% link mex-learn-input-output.md %})**
