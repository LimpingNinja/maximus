---
layout: default
title: "Color Codes"
section: "display"
description: "Pipe color codes and semantic theme color codes"
---

Color codes change text and background colors using the pipe character
followed by a two-digit number or two lowercase letters.

---

## Foreground Colors (`|00` – `|15`)

These set the text (foreground) color without changing the background.

| Code   | Color          |
|--------|----------------|
| `\|00`  | Black          |
| `\|01`  | Blue           |
| `\|02`  | Green          |
| `\|03`  | Cyan           |
| `\|04`  | Red            |
| `\|05`  | Magenta        |
| `\|06`  | Brown          |
| `\|07`  | Light Gray     |
| `\|08`  | Dark Gray      |
| `\|09`  | Light Blue     |
| `\|10`  | Light Green    |
| `\|11`  | Light Cyan     |
| `\|12`  | Light Red      |
| `\|13`  | Light Magenta  |
| `\|14`  | Yellow         |
| `\|15`  | White          |

---

## Background Colors (`|16` – `|23`)

These set the background color without changing the foreground.

| Code   | Background     |
|--------|----------------|
| `\|16`  | Black          |
| `\|17`  | Blue           |
| `\|18`  | Green          |
| `\|19`  | Cyan           |
| `\|20`  | Red            |
| `\|21`  | Magenta        |
| `\|22`  | Brown          |
| `\|23`  | Light Gray     |

---

## Blink + Background (`|24` – `|31`)

These set the background color and enable the blink attribute. On terminals
that support ICE colors instead of blinking, these produce bright backgrounds.

| Code   | Background + Blink |
|--------|--------------------|
| `\|24`  | Black + blink      |
| `\|25`  | Blue + blink       |
| `\|26`  | Green + blink      |
| `\|27`  | Cyan + blink       |
| `\|28`  | Red + blink        |
| `\|29`  | Magenta + blink    |
| `\|30`  | Brown + blink      |
| `\|31`  | Light Gray + blink |

---

## Examples

```
|14Hello, |15|17World|16!
```

Produces: **yellow** "Hello, " followed by **white on blue** "World" then
returns to a **black background** for "!".

### Literal Pipe

To display a literal `|` character, use `||`:

```
Type || to see a pipe symbol.
```

Displays: `Type | to see a pipe symbol.`

---

## Theme Color Codes (`|xx`)

Theme color codes are **two lowercase letters** that refer to a named semantic
color slot defined in `colors.toml`. Instead of hard-coding numeric color
values throughout your display files and prompts, you can use theme codes and
change the entire BBS color scheme from one file.

When Maximus encounters a theme code like `|pr`, it looks up the slot in the
loaded theme and substitutes the configured pipe color string (e.g., `|14`
for yellow, or `|15|17` for white on blue). The substituted codes then flow
through normal color processing.

### Slot Reference

| Code   | Slot Name     | Purpose                              | Default  |
|--------|---------------|--------------------------------------|----------|
| `\|tx`  | text          | Normal body text                     | `\|07`    |
| `\|hi`  | highlight     | Emphasized / important text          | `\|15`    |
| `\|pr`  | prompt        | User-facing prompts                  | `\|14`    |
| `\|in`  | input         | User keystroke echo                  | `\|15`    |
| `\|tf`  | textbox_fg    | Text input field foreground          | `\|15`    |
| `\|tb`  | textbox_bg    | Text input field background          | `\|17`    |
| `\|hd`  | heading       | Section headings                     | `\|11`    |
| `\|lf`  | lightbar_fg   | Lightbar selected item foreground    | `\|15`    |
| `\|lb`  | lightbar_bg   | Lightbar selected item background    | `\|17`    |
| `\|er`  | error         | Error messages                       | `\|12`    |
| `\|wn`  | warning       | Warning messages                     | `\|06`    |
| `\|ok`  | success       | Confirmations / success messages     | `\|10`    |
| `\|dm`  | dim           | De-emphasized text, help text        | `\|08`    |
| `\|fi`  | file_info     | File area descriptions               | `\|03`    |
| `\|sy`  | sysop         | SysOp-only text                      | `\|13`    |
| `\|qt`  | quote         | Quoted message text                  | `\|09`    |
| `\|br`  | border        | Box borders, dividers                | `\|01`    |
| `\|hk`  | hotkey        | Hotkey characters                    | `\|14`    |
| `\|ac`  | accent        | Accent / decorative elements         | `\|11`    |
| `\|ds`  | disabled      | Disabled or unavailable items        | `\|08`    |
| `\|nf`  | info          | Informational notices                | `\|10`    |
| `\|nt`  | notice        | System notices                       | `\|13`    |
| `\|dv`  | divider       | Horizontal rules, separators         | `\|05`    |
| `\|la`  | label         | Field labels                         | `\|03`    |
| `\|cd`  | default       | Reset to the theme's default color   | `\|16\|07` |

> For full details on configuring slots, creating custom themes, and editing
> colors interactively, see [Theme Colors]({% link theme-colors.md %}).

### Examples

```
|pr Enter your name: |in
```

Displays the prompt in the theme's prompt color, then switches to the input
color for the user's typed response.

```
|hd=== Message Area List ===|tx
```

Displays the heading in the heading color, then returns to normal text color.

```
|er ERROR: |tx File not found.
```

Displays "ERROR:" in the error color, then the description in normal text.

---

## See Also

- [Display Codes]({% link display-codes.md %}) — overview and quick reference
- [Theme Colors]({% link theme-colors.md %}) — full theme configuration guide
- [Info Codes]({% link mci-info-codes.md %}) — dynamic data substitution codes
- [Terminal Control]({% link mci-terminal-control.md %}) — screen and cursor codes
