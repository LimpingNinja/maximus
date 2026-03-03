---
layout: default
title: "Positional Parameters"
section: "display"
description: "Early and deferred parameter substitution in language strings"
---

Language strings (in `english.toml`) can contain **positional parameters**
that are filled in by the BBS at runtime. There are two expansion modes.

> For a full guide to the language string system, see
> [Language Files (TOML)]({% link lang-toml.md %}).

---

## Early Expansion (`|!N`)

| Code        | Parameter   |
|-------------|-------------|
| `\|!1`–`\|!9` | Parameters 1 through 9 |
| `\|!A`–`\|!F` | Parameters 10 through 15 |

Early-expansion parameters are substituted **before** formatting operators
run. Use `|!N` when the parameter value feeds *into* a formatting operator
as an argument — for example, providing the repeat count for `$D`:

```toml
# |!3 provides the number "75", which becomes the repeat count for $D
box_border = { text = "|br$D|!3\xC4" }
```

---

## Deferred Expansion (`|#N`)

| Code        | Parameter   |
|-------------|-------------|
| `\|#1`–`\|#9` | Parameters 1 through 9 |
| `\|#A`–`\|#F` | Parameters 10 through 15 |

Deferred-expansion parameters are substituted **during** formatting-operator
processing. Pending format ops (`$L`, `$R`, `$T`, `$C`) are applied to the
resolved value. Use `|#N` when a formatting operator should *wrap* the
parameter output:

```toml
# $L04 left-pads the node number to 4 columns
# $T28$R28 trims the username to 28 chars then right-pads to 28
who_row = { text = "|15 $L04|#2  $T28$R28|#1  |12|!3" }
```

---

## Mixing Both Modes

Both `|!N` and `|#N` share the same slot numbering. A single language string
can mix both — early slots are replaced first, deferred slots are resolved
later with format ops applied:

```toml
# |#1 = username (deferred, gets padded), |!3 = status (early, plain text)
user_line = { text = "$R20|#1 |!3" }
```

**Note:** Positional parameters are primarily used in language file strings.
Display files and menu titles typically use
[info codes]({% link mci-info-codes.md %}) (`|XY`) instead.

---

## See Also

- [Display Codes]({% link display-codes.md %}) — overview and quick reference
- [Format Operators]({% link mci-format-operators.md %}) — padding, alignment,
  and repetition operators that interact with deferred parameters
- [Language Files (TOML)]({% link lang-toml.md %}) — the language string system
- [Info Codes]({% link mci-info-codes.md %}) — dynamic data codes for display
  files and menus
