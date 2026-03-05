---
layout: default
section: "display"
title: "Display Codes"
description: "Using display codes in Maximus BBS"
---

Maximus NG supports **display codes** — short sequences embedded in prompts,
display files, menu titles, and language strings that are replaced at runtime
with colors, user data, formatted text, or terminal commands.

If you have used display codes on Mystic BBS, Renegade, or similar systems,
you will feel right at home. Maximus uses the same `|` (pipe) prefix for
colors and information codes, and the same `$` prefix for formatting operators.

> **Related pages:**
> [Theme Colors]({{ site.baseurl }}{% link theme-colors.md %}) covers the semantic color system
> in depth. [Language Files (TOML)]({{ site.baseurl }}{% link lang-toml.md %}) explains how
> display codes interact with language strings and parameter substitution.

---

## How Display Codes Work

A display code starts with a **pipe character** (`|`) followed by two
characters that tell Maximus what to display. Codes are **case-sensitive**.

For example, if you place this in a prompt or display file:

```
Welcome to the BBS, |UN!
```

Maximus replaces `|UN` with the current user's name at runtime:

```
Welcome to the BBS, Kevin Morgan!
```

Display codes can be used in:

- **Display files** (`.bbs` / ANSI art)
- **Menu titles and option descriptions** (in TOML menu configs)
- **Language strings** (in `english.toml`)
- **MEX scripts** (via `print()`)

---

## Code Categories

Each category has its own reference page with full tables, examples, and usage
notes:

- **[Color Codes]({{ site.baseurl }}{% link mci-color-codes.md %})** — pipe color codes
  (`|00`–`|31`) for foreground, background, and blink; plus semantic theme
  color codes (`|xx`) that map to named slots in `colors.toml`.
- **[Info Codes]({{ site.baseurl }}{% link mci-info-codes.md %})** — two-letter uppercase codes
  (`|UN`, `|BN`, `|TL`, etc.) that expand to live user, system, area, and
  date/time data.
- **[Terminal Control]({{ site.baseurl }}{% link mci-terminal-control.md %})** — screen clear,
  cursor movement, save/restore, and other terminal manipulation codes.
- **[Format Operators]({{ site.baseurl }}{% link mci-format-operators.md %})** — `$`-prefix
  operators for padding, alignment, trimming, and character repetition.
- **[Positional Parameters]({{ site.baseurl }}{% link mci-positional-params.md %})** — `|!N`
  (early) and `|#N` (deferred) parameter slots used in language strings.

---

## Contexts Where Codes Are Disabled

Display codes are automatically disabled in certain contexts to prevent
user-authored content from being interpreted as codes:

- **Message editor** — text you type in the full-screen or line editor
- **Input fields** — bounded text input prompts
- **Message display** — message body text from other users
- **File descriptions** — uploaded file descriptions

System prompts, menus, status bars, and display files continue to process
all display codes normally.

---

## Quick Reference Card

| Category       | Syntax       | Example          | Result                    |
|----------------|--------------|------------------|---------------------------|
| Foreground     | `\|##`       | `\|14`           | Yellow text               |
| Background     | `\|##`       | `\|17`           | Blue background           |
| Theme color    | `\|xx`       | `\|pr`           | Theme prompt color        |
| User info      | `\|XY`       | `\|UN`           | User's name               |
| System info    | `\|XY`       | `\|BN`           | BBS name                  |
| Right-pad      | `$R##`       | `$R20\|UN`       | Name padded to 20 chars   |
| Center         | `$C##`       | `$C40\|BN`       | BBS name centered in 40   |
| Repeat char    | `$D##C`      | `$D40-`          | 40 dashes                 |
| Fill to column | `$X##C`      | `$X80.`          | Dots to column 80         |
| Clear screen   | `\|CL`       | `\|CL`           | Clears the screen         |
| Goto column    | `[X##`       | `[X01`           | Cursor to column 1        |
| Early param    | `\|!N`       | `\|!1`           | Early-expanded parameter  |
| Deferred param | `\|#N`       | `$R20\|#1`       | Format-op-wrapped param   |
| Literal pipe   | `\|\|`       | `\|\|`           | Displays `\|`             |
| Literal dollar | `$$`         | `$$`             | Displays `$`              |

---

## Compatibility Notes

Maximus NG display codes are designed to be broadly compatible with the
conventions established by Mystic BBS and Renegade:

- **Pipe color codes** (`|00`–`|31`) follow the same numbering as Mystic
  and Renegade.
- **Information codes** use two-letter Mystic-style mnemonics (`|UN`, `|BN`,
  `|TL`, etc.) rather than the older MECCA `[token]` syntax or ENiGMA½
  percent-prefix (`%UN`) syntax.
- **Formatting operators** (`$C`, `$R`, `$L`, `$D`, `$X`, `$T`, `$c`, `$l`,
  `$r`, `|PD`) match Mystic's formatting system.
- **Cursor codes** use bracket-prefix syntax (`[X##`, `[Y##`, `[A##`–`[D##`)
  consistent with Mystic.

Legacy Maximus MECCA display codes (compiled `.bbs` files using AVATAR
bytecodes) continue to work. The new pipe-based codes are an additional
layer that operates on plain-text strings at runtime.

---

## See Also

- [Color Codes]({{ site.baseurl }}{% link mci-color-codes.md %}) — numeric and theme color codes
- [Info Codes]({{ site.baseurl }}{% link mci-info-codes.md %}) — dynamic data substitution codes
- [Terminal Control]({{ site.baseurl }}{% link mci-terminal-control.md %}) — screen and cursor codes
- [Format Operators]({{ site.baseurl }}{% link mci-format-operators.md %}) — padding, alignment, repetition
- [Positional Parameters]({{ site.baseurl }}{% link mci-positional-params.md %}) — language string parameters
- [Theme Colors]({{ site.baseurl }}{% link theme-colors.md %}) — configuring the 25 semantic color slots
- [Language Files (TOML)]({{ site.baseurl }}{% link lang-toml.md %}) — language string system and display code usage
- [Language Editor]({{ site.baseurl }}{% link lang-editor.md %}) — editing language strings interactively in maxcfg
