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

**Note:** `##` is always two digits. To move to column 5, use `[X05`.

---

## Save & Restore

| Code   | Description                              |
|--------|------------------------------------------|
| `\|SA`  | Save cursor position and attributes      |
| `\|RA`  | Restore cursor position and attributes   |
| `\|SS`  | Save entire screen                       |
| `\|RS`  | Restore entire screen                    |

---

## Example

```
|CL|14Welcome!|CR|CR[X10|11This text starts at column 10.
```

Clears the screen, prints "Welcome!" in yellow, moves down two lines,
positions the cursor at column 10, then prints in light cyan.

---

## See Also

- [Display Codes]({% link display-codes.md %}) — overview and quick reference
- [Color Codes]({% link mci-color-codes.md %}) — numeric and theme color codes
- [Info Codes]({% link mci-info-codes.md %}) — dynamic data substitution codes
- [Format Operators]({% link mci-format-operators.md %}) — padding, alignment, repetition
