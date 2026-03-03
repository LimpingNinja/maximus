---
layout: default
title: "Format Operators"
section: "display"
description: "Formatting operators for padding, alignment, and character repetition"
---

Formatting operators control how the **next** information code is displayed.
They are useful for creating aligned columns, padded fields, and repeated
characters in prompts and display files.

**Note:** `##` is always a two-digit number (`00`–`99`).

---

## Padding & Alignment

These operators affect the next `|XY` code that follows them:

| Operator  | Description                                        |
|-----------|----------------------------------------------------|
| `$C##`    | Center the next value within `##` columns (spaces) |
| `$L##`    | Left-pad the next value to `##` columns (spaces)   |
| `$R##`    | Right-pad the next value to `##` columns (spaces)  |
| `$T##`    | Trim the next value to `##` characters max         |
| `$c##C`   | Center within `##` columns using character `C`     |
| `$l##C`   | Left-pad to `##` columns using character `C`       |
| `$r##C`   | Right-pad to `##` columns using character `C`      |
| `\|PD`     | Prepend a single space to the next value           |

---

## Immediate Output

These produce output directly without needing a following code:

| Operator  | Description                                        |
|-----------|----------------------------------------------------|
| `$D##C`   | Duplicate character `C` exactly `##` times         |
| `$X##C`   | Duplicate character `C` until cursor reaches column `##` |

The difference between `$D` and `$X`: `$D` always produces the same number
of characters regardless of cursor position, while `$X` fills *up to* a
specific column — so the number of characters depends on where the cursor
currently is.

---

## Examples

```
|UN              translates to: "Kevin Morgan"
$R30|UN          translates to: "Kevin Morgan                  "
$C30|UN          translates to: "         Kevin Morgan          "
$L30|UN          translates to: "                  Kevin Morgan"
$D30-            translates to: "------------------------------"
$X30-            translates to: "------------------------------"  (from col 1)
|UN$X30.         translates to: "Kevin Morgan.................."
$c30.|UN         translates to: ".........Kevin Morgan.........."
$r30.|UN         translates to: "Kevin Morgan.................."
$l30.|UN         translates to: "..................Kevin Morgan"
```

---

## Literal Dollar Sign

To display a literal `$` character, use `$$`:

```
Price: $$5.00
```

Displays: `Price: $5.00`

---

## See Also

- [Display Codes]({% link display-codes.md %}) — overview and quick reference
- [Color Codes]({% link mci-color-codes.md %}) — numeric and theme color codes
- [Info Codes]({% link mci-info-codes.md %}) — dynamic data substitution codes
- [Positional Parameters]({% link mci-positional-params.md %}) — language string parameters
- [Terminal Control]({% link mci-terminal-control.md %}) — screen and cursor codes
