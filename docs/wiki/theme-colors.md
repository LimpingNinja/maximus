---
layout: default
section: "display"
title: "Theme Colors"
description: "Semantic color theming system for Maximus BBS"
---

Maximus NG uses a **semantic color system** — instead of hard-coding numeric
pipe colors (`|14`, `|12`) throughout your display files and language strings,
you use named **theme codes** (`|pr` for prompt, `|er` for error, `|hd` for
heading). All 25 slots are defined in a single file, `colors.toml`, and
changing a slot there changes it everywhere.

---

## How It Works

1. You write display files and language strings using theme codes:

   ```
   |pr Enter your name: |in
   |er ERROR: |tx File not found.
   |hd=== Message Areas ===|cd
   ```

2. At runtime, Maximus looks up each theme code in the loaded color
   configuration and substitutes the actual pipe color string. For example,
   `|pr` might resolve to `|14` (yellow) and `|er` to `|12` (light red).

3. The substituted codes flow through normal color processing — ANSI escape
   sequences are sent to the terminal.

This means you can retheme your entire BBS by editing one file.

---

## Configuration: `colors.toml`

Theme colors are defined in `config/general/colors.toml` under the
`[theme.colors]` section:

```toml
[theme.colors]
text         = "|07"
highlight    = "|15"
prompt       = "|14"
input        = "|15"
heading      = "|11"
lightbar_fg  = "|15"
lightbar_bg  = "|17"
error        = "|12"
warning      = "|06"
success      = "|10"
dim          = "|08"
file_info    = "|03"
sysop        = "|13"
quote        = "|09"
border       = "|01"
hotkey       = "|14"
accent       = "|11"
disabled     = "|08"
info         = "|10"
notice       = "|13"
divider      = "|05"
label        = "|03"
default      = "|16|07"
textbox_fg   = "|15"
textbox_bg   = "|17"
```

Values can be any valid pipe color string, including compound codes for
foreground + background:

```toml
lightbar_fg = "|15|17"   # White text on blue background
```

---

## Slot Reference

Every slot has a two-letter code, a descriptive name, and a default value.
See also [Color Codes]({% link mci-color-codes.md %}#slot-reference) for the
same table in the Display Codes context.

| Code | Name | Default | Purpose |
|------|------|---------|---------|
| `\|tx` | text | `\|07` | Normal body text |
| `\|hi` | highlight | `\|15` | Emphasized text |
| `\|pr` | prompt | `\|14` | User-facing prompts |
| `\|in` | input | `\|15` | Input fields |
| `\|tf` | textbox_fg | `\|15` | Textbox foreground |
| `\|tb` | textbox_bg | `\|17` | Textbox background |
| `\|hd` | heading | `\|11` | Section headings |
| `\|lf` | lightbar_fg | `\|15` | Lightbar foreground |
| `\|lb` | lightbar_bg | `\|17` | Lightbar background |
| `\|er` | error | `\|12` | Error messages |
| `\|wn` | warning | `\|06` | Warnings |
| `\|ok` | success | `\|10` | Success confirmations |
| `\|dm` | dim | `\|08` | De-emphasized text |
| `\|fi` | file_info | `\|03` | File metadata |
| `\|sy` | sysop | `\|13` | Sysop-only indicators |
| `\|qt` | quote | `\|09` | Quoted text |
| `\|br` | border | `\|01` | Borders and frames |
| `\|hk` | hotkey | `\|14` | Hotkey letters |
| `\|ac` | accent | `\|11` | Accent color |
| `\|ds` | disabled | `\|08` | Disabled/unavailable items |
| `\|nf` | info | `\|10` | Informational notices |
| `\|nt` | notice | `\|13` | System notices |
| `\|dv` | divider | `\|05` | Divider lines |
| `\|la` | label | `\|03` | Labels |
| `\|cd` | default | `\|16\|07` | Reset to theme default (use at end of strings) |

---

## Editing in MaxCFG

Theme colors can be edited interactively in **maxcfg** under
**Setup → Global → Default Colors → Theme Colors**. The editor shows a live
preview of each slot with its current pipe color value.

---

## The `|cd` Reset Convention

Themed language strings should end with `|cd` to reset the color back to the
theme's default, preventing color bleed into the next output. Exceptions:

- Strings ending with `|in` (input follows — the input color takes over)
- Strings that chain directly into another theme color
- Standalone color variables (e.g., `menu_name_col`)

---

## Theme Codes in Display Files

Theme codes work everywhere display codes work:

- **Display files** (`.bbs` / ANSI art)
- **Menu titles and option descriptions** (in TOML menu configs)
- **Language strings** (in `english.toml`)
- **MEX scripts** (via `print()`)

Using theme codes instead of numeric colors means your ANSI art and display
files automatically adapt when the theme changes.

---

## See Also

- [Color Codes]({% link mci-color-codes.md %}) — numeric pipe colors and
  theme code slot table
- [Display Codes]({% link display-codes.md %}) — pipe codes, info codes, and
  formatting operators
- [Theming & Modding]({% link theming-modding.md %}) — overview of all
  customization options
