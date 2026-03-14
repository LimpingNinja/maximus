---
layout: default
title: "Terminal Control"
section: "display"
description: "Terminal control codes for screen manipulation and cursor movement"
---

Terminal control codes send ANSI escape sequences for screen manipulation.
They require an ANSI-capable terminal (most modern terminals support these).

---

## Screen & Color

| Code   | Description                              |
|--------|------------------------------------------|
| `\|CL`  | Clear the screen                         |
| `\|CD`  | Reset color to default (ANSI reset)      |
| `\|CR`  | Carriage return + line feed (new line)   |
| `\|BS`  | Destructive backspace                    |

---

## Cursor Movement

| Code    | Description                              |
|---------|------------------------------------------|
| `[X##`  | Move cursor to column `##`               |
| `[Y##`  | Move cursor to row `##`                  |
| `[A##`  | Move cursor up `##` rows                 |
| `[B##`  | Move cursor down `##` rows               |
| `[C##`  | Move cursor forward `##` columns         |
| `[D##`  | Move cursor back `##` columns            |
| `[K`    | Clear from cursor to end of line         |
| `[0`    | Hide cursor                              |
| `[1`    | Show cursor                              |
| `[<`    | Move cursor to beginning of line (col 1) |
| `[>`    | Move cursor to end of line (last column) |
| `[H`    | Move cursor to center of line            |

**Note:** `##` is always two digits. To move to column 5, use `[X05`.

> **Info:** Anywhere a cursor code expects `##`, you can substitute an
> `|XY` info code that resolves to a number. For example, `|[X|TC` moves
> the cursor to the center column, and `|[C|TH` moves forward by half
> the screen width. See
> [terminal geometry codes]({{ site.baseurl }}{% link mci-info-codes.md %}#terminal-geometry)
> for useful numeric codes.

The `|[<`, `|[>`, and `|[H` codes use the user's terminal width to
determine the target column. If the width is unset, **80 columns** is
assumed. `|[H` uses `(width + 1) / 2` — the same formula as the `|TC`
info code.

---

## Save & Restore

| Code   | Description                              |
|--------|------------------------------------------|
| `\|SA`  | Save cursor position and attributes      |
| `\|RA`  | Restore cursor position and attributes   |
| `\|SS`  | Save entire screen                       |
| `\|RS`  | Restore entire screen                    |

---

## Display File Embedding

| Code           | Description                                   |
|----------------|-----------------------------------------------|
| `\|DF{path}`   | Display a `.bbs`/`.ans` file or run a MEX script inline |

The `|DF{path}` code embeds a display file (or MEX script) directly
into a language string or prompt.  When the MCI engine encounters this
code, it outputs any preceding text, then calls the display-file
subsystem for the given path, and finally continues with any text that
follows.

- **Display files** — the path is resolved the same way as a menu
  `Display_File` command: extensions (`.bbs`, `.ans`) are tried
  automatically and the file contents are rendered with full MCI
  processing.
- **MEX scripts** — prefix the path with `:` to invoke a MEX script
  instead, e.g. `|DF{:my_script}`.
- If no closing `}` is found, the `|` is emitted as a literal
  character.

```
|DF{display/screens/welcome}       show welcome.bbs / welcome.ans
|DF{:login_script}                 run login_script.mex
|14Header |DF{display/hdr/bar}|14 Footer   embed a file between text
```

> **Note:** `|DF{path}` is handled at the output layer, before MCI
> expansion of the surrounding text.  It cannot be used as a value for
> format operators (`$R`, `$C`, etc.) — it is a side-effect code, not
> a value producer.

---

## Example

```
|CL|14Welcome!|CR|CR[X10|11This text starts at column 10.
```

Clears the screen, prints "Welcome!" in yellow, moves down two lines,
positions the cursor at column 10, then prints in light cyan.

---

## See Also

- [Display Codes]({{ site.baseurl }}{% link display-codes.md %}) — overview and quick reference
- [Color Codes]({{ site.baseurl }}{% link mci-color-codes.md %}) — numeric and theme color codes
- [Info Codes]({{ site.baseurl }}{% link mci-info-codes.md %}) — dynamic data substitution codes
- [Format Operators]({{ site.baseurl }}{% link mci-format-operators.md %}) — padding, alignment, repetition
