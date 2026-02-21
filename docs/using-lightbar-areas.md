# Using Lightbar Area Selection

Good news—you no longer have to squint at a scrolling list and type an area
name from memory. Maximus now has a full lightbar selection mode for both file
areas and message areas. Arrow keys, paging, division drill-in, a highlight
bar, and you press Enter to pick one. It's the area-change experience your
callers have been asking for since 1993.

This guide covers how to turn it on, what the configuration knobs do, and how
to make it look exactly the way you want.

---

## Table of Contents

1. [Enabling Lightbar Mode](#enabling-lightbar-mode)
2. [How It Looks](#how-it-looks)
3. [Keyboard Controls](#keyboard-controls)
4. [Configuration Reference](#configuration-reference)
   - [The Main Switch](#the-main-switch)
   - [List Height (reduce_area)](#list-height-reduce_area)
   - [Highlight Modes](#highlight-modes)
   - [Color Overrides](#color-overrides)
   - [Boundaries](#boundaries)
   - [Header and Footer Anchors](#header-and-footer-anchors)
   - [Custom Screens](#custom-screens)
5. [Format Strings](#format-strings)
   - [Header, Format, and Footer](#header-format-and-footer)
   - [The Breadcrumb Token (%b)](#the-breadcrumb-token-b)
   - [Message Area Tokens](#message-area-tokens)
6. [Language Strings](#language-strings)
7. [Division Hierarchy](#division-hierarchy)
8. [Tips and Recipes](#tips-and-recipes)
9. [Troubleshooting](#troubleshooting)
10. [See Also](#see-also)

---

## Enabling Lightbar Mode

Lightbar selection is controlled per area type in
`config/general/display.toml`. File areas and message areas each have their
own independent toggle:

```toml
[file_areas]
lightbar_area = true

[msg_areas]
lightbar_area = true
```

Set `lightbar_area = true` for the area type you want, restart, and you're
done. When disabled (`false`), the traditional scrolling list and prompt
behavior is preserved exactly—nothing changes for your callers.

You can enable one without the other. Running lightbar file areas with
traditional message areas (or vice versa) works fine.

---

## How It Looks

When a caller enters the area-change flow (the `File_area` or `Msg_area` menu
command, or typing `/`, `.`, `?`, or `=` at the area prompt), the screen
clears and they see something like this:

```
File Areas ──────────────

[div ] BBS Files            ... Top-level BBS file collections
[div ] Programming          ... Programming file categories
[div ] Retro                ... Retro computing collections
[area] uploads_staging      ... User upload staging
[area] general_files        ... General file area

Location: Root
Use cursor keys to highlight a selection, ENTER to select an area.
For more areas, type "/" for top level areas.
```

The highlight bar sits on one row. The caller moves it up and down, pages
through long lists, drills into divisions with Enter, and backs out with
Escape or `.` — all without typing a single area name.

---

## Keyboard Controls

These keys work identically for file areas and message areas.

### Navigation (handled by the lightbar primitive)

| Key | Action |
|-----|--------|
| **Up / Down** | Move the highlight bar |
| **PgUp / PgDn** | Jump a page at a time |
| **Home / End** | Jump to the first / last entry |
| **Enter** | Select the highlighted entry |
| **Escape** | Go back (see below) |

### Area-Specific Keys (handled by the area logic)

| Key | Action |
|-----|--------|
| **Enter** on a division | Drill into that division |
| **Enter** on an area | Select it and return |
| **/** or **\\** | Jump back to the root division |
| **.** | Go up one division level |
| **Q** / **q** | Exit the lightbar immediately |
| **Escape** (inside a division) | Go up one level |
| **Escape** (at root) | Exit the lightbar |

The short version: Enter drills in, Escape backs out, `/` goes home, `.` goes
up, `Q` quits. Once it clicks, your callers will never want to go back.

---

## Configuration Reference

All configuration lives in `config/general/display.toml`. File areas use the
`[file_areas]` section; message areas use `[msg_areas]`. The keys are
identical between the two—only the section name changes.

### The Main Switch

```toml
lightbar_area = true
```

Enables or disables lightbar mode for that area type. Default is `false`.

### List Height (reduce_area)

```toml
reduce_area = 5
```

Controls how many rows of vertical margin to reserve below the area list.
The list height is calculated as:

```
list_height = screen_rows - header_rows - reduce_area
```

A value of `5` leaves room for the footer (breadcrumb trail), the help text,
and a blank line or two of breathing room. If your custom screen uses more (or
fewer) lines below the list, adjust this number. Smaller means a taller list;
larger means more space below.

This value is ignored when both `top_boundary` and `bottom_boundary` are set
(see [Boundaries](#boundaries)).

### Highlight Modes

```toml
lightbar_what = "row"
```

Controls what the highlight bar covers when sitting on a row:

| Value | Behavior |
|-------|----------|
| `"row"` | Highlights the full row width within the list rectangle (default) |
| `"full"` | Highlights the entire terminal row from column 1 to screen width |
| `"name"` | Highlights only the area/division name within the formatted row |

`"row"` is the safe default and looks good with most format strings. `"name"`
is nice if your format string has a lot of decorative chrome around the name
and you want the highlight to feel more precise. `"full"` is the
sledgehammer—it paints the whole line, edge to edge.

### Color Overrides

```toml
lightbar_fore = ""
lightbar_back = ""
```

Override the foreground and background colors of the highlighted row. Values
can be a color name or a single hex nibble (`0`–`f`):

| Value | Color |
|-------|-------|
| `"0"` – `"7"` | Standard colors (black through white) |
| `"8"` – `"f"` | Bright colors |
| `"blue"`, `"red"`, etc. | Named colors |
| `""` (empty) | Use theme defaults |

When left empty, the selected foreground inherits the normal text color, and
the selected background uses the theme's lightbar background. This is usually
what you want. Override only if you need specific branding.

Examples:

```toml
# White on blue (classic BBS look)
lightbar_fore = "f"
lightbar_back = "1"

# Bright cyan on dark gray
lightbar_fore = "b"
lightbar_back = "8"
```

### Boundaries

```toml
# top_boundary = [3, 1]
# bottom_boundary = [20, 80]
```

Pin the list rectangle to exact screen coordinates. Values are `[row, col]`,
1-based. When set, the list is drawn inside this box and `reduce_area` is
ignored.

These are commented out by default, which means the list starts wherever the
header ends and extends down to `screen_rows - reduce_area`. That's the right
behavior for most setups. You only need boundaries if you're designing a
custom screen layout where the list must land in a specific rectangle.

**Rules:**
- Both values in each array must be greater than 0, or the boundary is ignored.
- If only one boundary is set, the other falls back to computed defaults.
- Values are clamped to screen dimensions—you can't position the list off-screen.
- If the result is nonsensical (bottom above top), everything falls back to safe defaults.

### Header and Footer Anchors

```toml
# header_location = [1, 1]
# footer_location = [22, 1]
```

Pin the header and footer to fixed screen positions. Like boundaries, these
are `[row, col]`, 1-based. When unset or invalid, the header and footer render
inline—header at the top of the cleared screen, footer below the list.

These are useful when you're using a custom screen and want the header or
footer to appear at a specific spot on that screen, rather than flowing with
the list.

### Custom Screens

```toml
custom_screen = ""
```

Display a custom `.ans`/`.bbs` screen file before the lightbar list is drawn.
The value is a bare filename (no extension, no path prefix). Maximus resolves
it against your `display_path` and tries standard extensions (`.ans`, `.bbs`,
etc.).

**When a custom screen is shown, the built-in header, footer, and help text
are all suppressed.** The custom screen owns the surrounding chrome—you're
responsible for drawing whatever context the caller needs. The lightbar list
renders on top of the screen within the computed (or configured) boundaries.

This is the power-user option. Most sysops won't need it. But if you want a
fully branded area selection screen with ANSI art and custom layout, this is
how.

Example:

```toml
# In display.toml:
custom_screen = "area_select"

# Maximus looks for:
#   <display_path>/area_select.ans
#   <display_path>/area_select.bbs
#   etc.
```

When using a custom screen, you'll almost certainly want to set `top_boundary`,
`bottom_boundary`, and possibly `header_location` / `footer_location` to
position everything precisely.

---

## Format Strings

The header, per-row format, and footer for each area type are defined in
`config/general/display_files.toml`. These control what the caller sees in
the list.

### Header, Format, and Footer

| Key | Area Type | Purpose |
|-----|-----------|---------|
| `file_header` | File areas | Rendered above the file area list |
| `file_format` | File areas | Rendered once per visible file area/division |
| `file_footer` | File areas | Rendered below the file area list |
| `msg_header` | Message areas | Rendered above the message area list |
| `msg_format` | Message areas | Rendered once per visible message area/division |
| `msg_footer` | Message areas | Rendered below the message area list |

The default formats look like this:

```toml
file_header = "%x16%x01%x0fFile Areas %x16%x01%x0d%xc4%xc4%xc4...%x0a%x0a"
file_format = "|tx[|hd%-4t|tx]|pr %-20a |tx... %-n%x0a"
file_footer = "%x0a|prLocation: |hi%b|07%x0a"

msg_header = "%x16%x01%x0fMessage Areas %x16%x01%x0d%xc4%xc4%xc4...%x0a%x0a"
msg_format = "%x16%x01%x0e%*%x16%x01%x0d%-20#%x16%x01%x07 ... %x16%x01%x03%n%x0a"
msg_footer = "%x0a|prLocation: |hi%b|07%x0a"
```

These use a mix of raw hex bytes (`%xHH`) for legacy AVATAR control codes and
MCI pipe codes (`|tx`, `|hi`, `|pr`). You can use either style—or both
together, as shown above.

The format tokens available in these strings are the same ones the legacy area
list used. The lightbar simply captures the formatted output per row and
displays it within the highlight bar.

### The Breadcrumb Token (%b)

The `%b` token in footer strings renders a breadcrumb trail showing where the
caller is in the division hierarchy:

| Context | Output |
|---------|--------|
| At root | `Root` |
| Inside "Echomail" | `Root -> Echomail` |
| Inside "Echomail.FidoNet" | `Root -> Echomail -> FidoNet` |

The default footer format places this after "Location:" in prompt color with
the breadcrumb in highlight color:

```toml
msg_footer = "%x0a|prLocation: |hi%b|07%x0a"
```

This gives your callers a clear sense of where they are when drilling into
nested divisions. If you don't use divisions, the breadcrumb just says "Root"
and stays out of the way.

### Message Area Tokens

Message area format strings support a few extra tokens:

| Token | Meaning |
|-------|---------|
| `%*` | Tag/new-mail indicator: `*` (new mail), `@` (tagged), or space |
| `%#` | Area name (padded) |
| `%n` | Area description |

The `%*` token is context-sensitive. In normal browsing, it shows `*` if the
area has unread mail. In tag mode, it shows `@` for tagged areas and a space
for untagged ones.

---

## Language Strings

The lightbar help text displayed below the list comes from the language file.
It uses the key `achg_lb_help` in the `f_area` heap:

```toml
[f_area]
achg_lb_help = { text = "|prUse cursor keys to highlight a selection, |hiENTER|pr to select an area.\nFor more areas, type \"|hi/|pr\" for top level areas.|cd" }
```

This string is shared between file areas and message areas. You can customize
it like any other language string—change the wording, colors, or add extra
hints. See [Using Language TOML Files](using-lang-toml.md) for details.

When a custom screen is active (`custom_screen` is set and found), this help
text is suppressed along with the header and footer.

---

## Division Hierarchy

Maximus organizes areas into a dot-separated hierarchy. A division is a
grouping node—it contains other areas and sub-divisions but isn't an area
you can read messages in or download files from.

```
Echomail                      ← division
  Echomail.FidoNet            ← sub-division
    Echomail.FidoNet.Chat     ← area (selectable)
    Echomail.FidoNet.BBS      ← area (selectable)
  Echomail.RetroNet           ← sub-division
    Echomail.RetroNet.General ← area (selectable)
Local                         ← division
  Local.Chat                  ← area (selectable)
  Local.Sysop                 ← area (selectable)
uploads_staging               ← area at root (no division)
```

In the lightbar, divisions show with a `[div]` tag and areas show with
`[area]`. Pressing Enter on a division drills in and shows its children.
Pressing `.` or Escape backs out one level. Pressing `/` jumps straight to
root no matter how deep you are.

The breadcrumb footer (`%b`) always reflects the current position in this
hierarchy.

---

## Tips and Recipes

**Start simple.** Enable lightbar mode with the defaults and see how it feels.
The out-of-the-box settings work well for most boards. Tweak from there.

**Adjust `reduce_area` first.** If the list feels too short, lower
`reduce_area`. If the footer or help text is getting clipped, raise it.
The default of `5` is a good starting point for 24-line terminals.

**Use `"name"` highlight mode for dense formats.** If your format string packs
a lot of info per row (type tag, name, description, status), the `"name"`
highlight mode draws attention to just the area name while keeping the
surrounding context readable.

**Match your footer colors to your theme.** The default footer uses `|pr`
(prompt) and `|hi` (highlight) semantic colors, which resolve through
`colors.toml`. If you're using the MaximusNG theme, they'll pick up your
theme colors automatically. If you're using legacy numeric colors, swap them
out:

```toml
# Legacy numeric colors instead of semantic
file_footer = "%x0a|14Location: |15%b|07%x0a"
```

**Custom screens are the nuclear option.** Don't reach for `custom_screen`
unless you have a specific ANSI layout in mind. When active, it suppresses the
built-in header, footer, and help text—you'll need to provide all of that
context yourself in the screen file, or your callers will be navigating blind.

**Test with both area types.** File and message areas use independent configs.
If you change `lightbar_what` or colors for one, remember to do the same for
the other (unless you want them to look different, which is also fine).

---

## Troubleshooting

**The list is too short / too tall**

Adjust `reduce_area`. Lower values give a taller list; higher values give more
room below for footer and help. If you're using explicit boundaries
(`top_boundary` / `bottom_boundary`), those override `reduce_area` entirely.

**Footer shows garbage instead of a breadcrumb**

Make sure your `msg_footer` or `file_footer` uses `%b` for the breadcrumb
token. If you see raw decoration characters instead of "Root -> ...", you may
have an older config file that predates the `%b` support for message areas.
Replace `msg_footer` with:

```toml
msg_footer = "%x0a|prLocation: |hi%b|07%x0a"
```

**Help text doesn't appear**

The `achg_lb_help` language string must be present in your language file. If
you're using a custom or migrated language file, check that the `[f_area]`
heap includes this key. Also, help text is suppressed when `custom_screen` is
active—that's by design.

**Custom screen doesn't show**

The `custom_screen` value is a bare filename without extension or path. It's
resolved against your `display_path` setting. Make sure the actual file exists
at `<display_path>/<custom_screen>.ans` (or `.bbs`, etc.). Check your
`display_path` in the Maximus config.

**Lightbar doesn't appear at all**

Double-check that `lightbar_area = true` is set in the correct section
(`[file_areas]` or `[msg_areas]`) of `config/general/display.toml`. The
lightbar only activates when the area-change flow is triggered through
menu commands or the `/`, `.`, `?`, `=` shortcuts.

**Colors look wrong on the highlighted row**

If `lightbar_fore` and `lightbar_back` are empty (the default), colors come
from the theme. Set explicit overrides if the theme defaults don't work well
with your format strings. Remember that the format string's own color codes
are replaced by the highlight color on the selected row.

---

## See Also

- [Using Display Codes](using-display-codes.md) — color codes, formatting
  operators, and terminal control
- [Using Language TOML Files](using-lang-toml.md) — customizing language
  strings including `achg_lb_help`
- [Using the maxcfg Language Editor](using-maxcfg-langed.md) — browse and edit
  language strings from inside maxcfg

---

*Arrow keys beat typing area names. Every time.*
