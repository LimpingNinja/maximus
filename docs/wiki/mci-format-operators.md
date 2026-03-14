---
layout: default
title: "Format Operators"
section: "display"
description: "Formatting operators for padding, alignment, and character repetition"
---

Formatting operators control how the **next** value is displayed.
They are useful for creating aligned columns, padded fields, and repeated
characters in prompts and display files.

The "next value" consumed by a pending format operator can be any of:

- A standard `|XY` info code (e.g. `|UN`, `|BN`)
- A positional parameter (`|!1`–`|!F` or `|#1`–`|#F`)
- An inline string literal (`|{text}`)

**Note:** `##` is always a two-digit number (`00`–`99`), **or** a `|XY`
info code that resolves to a number (e.g. `|TH` for half-screen width).
When an info code is used, it is expanded first, and if the result is a
valid number between 0 and 99 it is used as the width parameter.

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

### Using Info Codes as Width

> **Info:** Anywhere a format operator expects a two-digit `##` width, you
> can substitute a `|XY` info code instead. The code is expanded first,
> and if the result is a number between 0 and 99 it is used as the width.
> If the code does not resolve to a valid number, the `$` is emitted as a
> literal character (the same fallback as a malformed operator).

This is particularly useful with the
[terminal geometry codes]({{ site.baseurl }}{% link mci-info-codes.md %}#terminal-geometry)
`|TW` (terminal width), `|TC` (center column), and `|TH` (half width),
which always produce two-digit numeric values. Any other info code that
happens to return a number will also work — for example `|US` (screen
length in lines) or `|TL` (time left in minutes, when ≤ 99).

The parser consumes `|XY` (3 characters) in place of `##` (2 characters);
everything else in the operator syntax stays the same. For operators that
take a fill character (`$r`, `$l`, `$c`, `$D`, `$X`), the fill character
follows the info code directly.

```
$R|TW|UN         right-pad user name to terminal width (space fill)
$r|TH-|UN        right-pad user name to half-width using '-'
$C|TW|BN         center BBS name within terminal width
$D|TW=           draw '=' across the full terminal width
$D|TH-           draw '-' for half the screen width
$X|TW.           fill with '.' up to the terminal width column
$T|TH|MD         trim area description to half-screen width
```

### Inline String Literals

> **Info:** The `|{text}` syntax lets you supply an arbitrary literal
> string as the value for a pending format operator. Everything between
> `{` and `}` is treated as the text to format. If no closing `}` is
> found, the `|` is emitted as a literal character.

This is useful when you want to pad, center, or trim a fixed string
without needing to define an info code for it:

```
$R30|{Welcome champ!}       "Welcome champ!                "
$C30|{MENU}                  "            MENU              "
$c40=|{[ Main Menu ]}        "============[ Main Menu ]=============="
$T10|{Hello, World!}         "Hello, Wor"
$r|TH-|{Section Title}       pad to half-width using '-'
```

Inline strings work anywhere a format operator expects its next value,
and can be freely combined with
[info code widths](#using-info-codes-as-width) in the same expression.

---

## Literal Dollar Sign

To display a literal `$` character, use `$$`:

```
Price: $$5.00
```

Displays: `Price: $5.00`

---

## See Also

- [Display Codes]({{ site.baseurl }}{% link display-codes.md %}) — overview and quick reference
- [Color Codes]({{ site.baseurl }}{% link mci-color-codes.md %}) — numeric and theme color codes
- [Info Codes]({{ site.baseurl }}{% link mci-info-codes.md %}) — dynamic data substitution codes
- [Positional Parameters]({{ site.baseurl }}{% link mci-positional-params.md %}) — language string parameters
- [Terminal Control]({{ site.baseurl }}{% link mci-terminal-control.md %}) — screen and cursor codes
