---
layout: default
title: "Info Codes"
section: "display"
description: "Information codes for dynamic data substitution"
---

Information codes are two **uppercase letters** (or special characters) that
are replaced with live data about the BBS, the current user, or the system.

---

## User Information

| Code   | Description                    |
|--------|--------------------------------|
| `\|UN`  | User name                      |
| `\|UH`  | User handle (alias)            |
| `\|UR`  | User real name                 |
| `\|UC`  | User city / state              |
| `\|UP`  | User home phone                |
| `\|UD`  | User data phone                |
| `\|U#`  | User number (account ID)       |
| `\|US`  | User screen length (lines)     |
| `\|TE`  | Terminal emulation (TTY, ANSI, AVATAR) |

---

## Call & Activity Statistics

| Code   | Description                    |
|--------|--------------------------------|
| `\|CS`  | Total calls (lifetime)         |
| `\|CT`  | Calls today                    |
| `\|MP`  | Total messages posted          |
| `\|DK`  | Total download KB              |
| `\|FK`  | Total upload KB                |
| `\|DL`  | Total downloaded files         |
| `\|FU`  | Total uploaded files           |
| `\|DT`  | Downloaded KB today            |
| `\|TL`  | Time left (minutes)            |

---

## System Information

| Code   | Description                    |
|--------|--------------------------------|
| `\|BN`  | BBS / system name              |
| `\|SN`  | Sysop name                     |

---

## Area Information

| Code   | Description                    |
|--------|--------------------------------|
| `\|MB`  | Current message area name      |
| `\|MD`  | Current message area description |
| `\|FB`  | Current file area name         |
| `\|FD`  | Current file area description  |

---

## Terminal Geometry

| Code   | Description                                      |
|--------|--------------------------------------------------|
| `\|TW`  | Terminal width in columns (2-digit, e.g. `80`)   |
| `\|TC`  | Center column of screen (2-digit)                |
| `\|TH`  | Half-screen width (2-digit)                      |

These codes expose the current user's terminal width as zero-padded
two-digit numbers so they can be used both for display and as dynamic
width parameters in [format operators]({{ site.baseurl }}{% link mci-format-operators.md %}).

### How the values are calculated

All three derive from the user's configured terminal width (`usr.width`).
If the width is unset or zero, **80 columns** is assumed.

| Code   | Formula              | 80-col result | 97-col result |
|--------|----------------------|:-------------:|:-------------:|
| `\|TW`  | width (clamped ≤ 99) | `80`          | `97`          |
| `\|TC`  | (width + 1) / 2      | `40`          | `49`          |
| `\|TH`  | width / 2            | `40`          | `48`          |

`|TC` and `|TH` differ only on **odd** widths: `|TC` rounds up (giving
the true center column in a 1-based coordinate system), while `|TH`
truncates (giving a pure half-width count).

### Use as format-operator widths

Because these codes resolve to numbers, they can replace the `##` width
parameter in any format operator. See
[Format Operators — Using Info Codes as Width]({{ site.baseurl }}{% link mci-format-operators.md %}#using-info-codes-as-width)
for details.

```
$D|TW=           draw '=' across the full terminal width
$r|TH-|UN        right-pad user name to half-width using '-'
[X|TC             move cursor to the center column
```

---

## Date & Time

| Code   | Description                    |
|--------|--------------------------------|
| `\|DA`  | Current date (DD Mon YY)       |
| `\|TM`  | Current time (HH:MM)          |
| `\|TS`  | Current time (HH:MM:SS)       |

---

## Example

```
|14Welcome to |15|BN|14, |15|UN|14!
You have |15|TL|14 minutes remaining.
Current message area: |11|MB
```

Produces something like:

```
Welcome to Maximus BBS, Kevin!
You have 60 minutes remaining.
Current message area: General
```

Info codes can be combined with
[format operators]({{ site.baseurl }}{% link mci-format-operators.md %}) for aligned output:

```
$R30|UN          translates to: "Kevin Morgan                  "
```

---

## Inline String Literals

The `|{text}` syntax provides an inline string literal that the MCI
engine treats as a value — just like an info code — but containing
arbitrary fixed text instead of dynamic data.

```
|{Hello, World!}
```

Produces: `Hello, World!`

On its own, `|{text}` is simply a pass-through — the delimiters `|{`
and `}` are stripped and the content is emitted directly.  Where it
becomes useful is as the target of a
[format operator]({{ site.baseurl }}{% link mci-format-operators.md %}#inline-string-literals):

```
$R30|{Welcome champ!}     "Welcome champ!                "
$C|TW|{=== MENU ===}      center text across terminal width
$T10|{Hello, World!}       "Hello, Wor"
```

If no closing `}` is found, the `|` is emitted as a literal character.

---

## See Also

- [Display Codes]({{ site.baseurl }}{% link display-codes.md %}) — overview and quick reference
- [Color Codes]({{ site.baseurl }}{% link mci-color-codes.md %}) — numeric and theme color codes
- [Format Operators]({{ site.baseurl }}{% link mci-format-operators.md %}) — padding, alignment, repetition
- [Terminal Control]({{ site.baseurl }}{% link mci-terminal-control.md %}) — screen and cursor codes
