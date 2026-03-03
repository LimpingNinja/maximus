---
layout: default
title: "Canned & Bounded Menus"
section: "configuration"
description: "Full reference for the [custom_menu] system: boundaries, layout, lightbar, and recipes"
---

Every menu TOML file in `config/menus/` can include a `[custom_menu]` table
that controls how Maximus renders the canned option list. This is the
mechanism that turns a plain text menu into a pixel-perfect, lightbar-driven
BBS experience — and it's entirely sysop-configurable.

This page is the authoritative reference for all `[custom_menu]` settings.
For a quick-start on enabling lightbar navigation specifically, see
[Lightbar Menus]({% link config-lightbar-menus.md %}).

---

## What Custom Menus Do

The guiding idea:

- If you want a fully custom look, **draw a menu screen** (ANSI/RIP/AVT).
- If you still want Maximus to print the option list, **tell it where**.
- Maximus remains the authority for which options exist, which are visible
  (privilege filtering via `OptionOkay()`), and what hotkeys do.

This is the **hybrid menu** pattern: art + a reliable option list that stays
correct as options change.

---

## Canned vs. Fully Drawn

### Hybrid Mode (default)

When `skip_canned_menu = false`, Maximus:

1. Displays the menu file (ANSI art) first — if one is configured and
   applies to the caller's help level.
2. Then prints the canned option list on top — inside your boundary
   rectangle if configured, or at the cursor position if not.

```toml
[custom_menu]
skip_canned_menu = false
```

This is the most common setup. Your ANSI art provides the frame; Maximus
fills in the options.

### Fully Drawn Mode

When `skip_canned_menu = true`, Maximus displays the menu file and **stops**
— the canned option list is not printed. Use this when your custom screen
already contains all the menu text and you just want Maximus to handle
input.

```toml
[custom_menu]
skip_canned_menu = true
```

> If no menu file exists (or doesn't apply to the caller's help level),
> Maximus falls back to canned rendering regardless of this setting.

---

## Boundaries

Boundaries define a rectangular area on screen where the canned option list
is placed. Coordinates are **1-based** and match the internal `Goto(row,col)`
behavior. The rectangle is inclusive.

```toml
[custom_menu]
top_boundary = [8, 8]        # [row, col] — top-left corner
bottom_boundary = [20, 61]   # [row, col] — bottom-right corner
```

The usable area is:

- **Width:** `bottom_col - top_col + 1`
- **Height:** `bottom_row - top_row + 1`

Think of it like reserving a box on screen:

```
+------------------------------------------------------+
| (your ANSI art / custom screen / header)             |
|                                                      |
|       top_boundary ->  [8, 8]                        |
|                 +-------------------------+          |
|                 | option list goes here   |          |
|                 | inside the rectangle    |          |
|                 +-------------------------+          |
|                           <- bottom_boundary [20,61] |
|                                                      |
+------------------------------------------------------+
```

If boundaries are not set or invalid (`top > bottom`, coordinates < 1),
Maximus falls back to normal unbounded canned rendering.

> **Bounded rendering is NOVICE-only.** REGULAR and EXPERT help levels
> always use the classic unbounded output path regardless of boundary
> settings.

---

## Title and Prompt Placement

### `show_title`

Controls whether the menu title line is printed in bounded mode.

```toml
[custom_menu]
show_title = true
title_location = [22, 32]
```

- If `title_location` is set, the title is printed at that position.
- If not set, it prints at the current cursor position (legacy behavior).
- In unbounded mode, the title is always printed regardless of this setting.

### `prompt_location`

For NOVICE menus, Maximus prints a selection prompt (typically "Select:").
Without explicit placement, the prompt can end up in awkward positions when
you have a drawn screen and bounded options.

```toml
[custom_menu]
prompt_location = [23, 1]
```

**Always set this** when using boundaries — it prevents the prompt from
overlapping your option area or disappearing off-screen.

---

## Option Width

The top-level `option_width` key (outside `[custom_menu]`) controls how wide
each option cell is in the canned list:

```toml
option_width = 14
```

This determines how many columns of options fit in your boundary. The number
of columns per row is `boundary_width / option_width` (minimum 1).

When lightbar is enabled, the effective cell width becomes
`option_width + (lightbar_margin × 2)`, so account for the margin when
sizing your boundary.

---

## Layout Controls

These are all optional. Start with defaults and enable one at a time.

### `option_spacing` (bool)

Adds an extra blank line between rows of options.

```toml
option_spacing = false    # default — tight rows
option_spacing = true     # extra vertical space
```

Visual comparison:

```
# option_spacing = false
A) First Option     B) Second Option
C) Third Option     D) Fourth Option

# option_spacing = true
A) First Option     B) Second Option

C) Third Option     D) Fourth Option
```

In bounded mode, spacing reduces how many rows fit in your boundary.

### `option_justify` (string)

Controls how the option text is aligned **within each cell**.

| Value | Behavior |
|-------|----------|
| `"left"` | `A) Option Text      ` (default) |
| `"center"` | `   A) Option Text   ` |
| `"right"` | `      A) Option Text` |

```toml
option_justify = "left"
```

The hotkey character and label justify together as a unit — there's no
separate hotkey positioning.

Use `"center"` when your drawn menu has a symmetrical look and you want each
cell to feel balanced. Use `"left"` for the most classic, legible style.

### `boundary_justify` (string)

Controls where the **entire option grid** sits inside the boundary rectangle.
This is not per-option — it moves the whole block.

Accepts one or two tokens:

**One token (horizontal only):**

| Value | Horizontal | Vertical default |
|-------|-----------|-----------------|
| `"left"` | Left | Top |
| `"center"` | Center | Center |
| `"right"` | Right | Top |

**Two tokens (horizontal + vertical):**

```toml
boundary_justify = "center center"
boundary_justify = "left top"
boundary_justify = "right bottom"
```

All nine combinations work: `left`/`center`/`right` × `top`/`center`/`bottom`.

Vertical justification only has a visible effect when there is extra vertical
space in your boundary (fewer option rows than boundary height).

### `boundary_layout` (string)

Controls the column distribution model inside the boundary.

#### `grid` (default)

Classic fixed-grid columns. Every row has the same column starting positions.
If the boundary width doesn't fit an exact number of columns,
`boundary_justify` can shift the grid.

```
|[A]      [B]      [C]|
|[D]      [E]      [F]|
```

#### `tight`

Like `grid`, but the last row is justified based on how many options are
actually in it. Useful when the last row has fewer items and you don't want
it stuck to the left.

```
|[A]      [B]      [C]|
|   [D]      [E]      |
```

#### `spread`

Distributes whitespace so the option list fills the boundary in both
dimensions — adds computed inter-column gaps **and** inter-row gaps.

```
|[A]          [B]          [C]|
|                              |
|[D]          [E]          [F]|
```

#### `spread_width`

Spreads options across the boundary width only (inter-column gaps). Rows step
normally with no extra vertical spacing.

```
|[A]          [B]          [C]|
|[D]          [E]          [F]|
```

#### `spread_height`

Spreads options across the boundary height only (inter-row gaps). Columns use
the normal grid placement.

```
|[A]      [B]      [C]|
|                      |
|[D]      [E]      [F]|
```

**Leftover handling:** Spread uses integer math. When space can't be evenly
divided, the remainder is distributed according to `boundary_justify`.

**Interaction with `option_spacing`:**
- `option_spacing = false` (sticky-first): vertical spread adds at most 1
  extra blank line per gap; remaining space becomes padding per vertical
  justification.
- `option_spacing = true` (spacing-first): vertical spread distributes
  remaining span evenly across gaps.

---

## Lightbar Settings

### `lightbar_menu` (bool)

Enables arrow-key navigation with a highlight bar over the canned option
list. **Requires valid boundaries** (`top_boundary` and `bottom_boundary`)
and NOVICE help level. Without boundaries, this setting is silently ignored
and Maximus falls back to the normal text menu.

```toml
[custom_menu]
lightbar_menu = true
```

### `lightbar_margin` (int)

Padding (in characters) on each side of every lightbar item.

```toml
lightbar_margin = 1    # default
```

The lightbar painter reserves and paints an effective width of
`option_width + (lightbar_margin × 2)`. So if `option_width = 14` and
`lightbar_margin = 1`, each cell occupies 16 columns.

If the highlight bar reaches into your border, either enlarge the boundary
or reduce `option_width` and/or `lightbar_margin`.

### `lightbar_color`

Controls the four color states used by the lightbar painter. Each is a
`["Foreground", "Background"]` pair using the standard 16-color DOS palette
names (case-insensitive):

```toml
[custom_menu.lightbar_color]
normal        = ["Light Gray", "Black"]
high          = ["Yellow", "Black"]
selected      = ["White", "Blue"]
high_selected = ["Yellow", "Blue"]
```

| State | What it colors |
|-------|---------------|
| `normal` | Non-selected option text |
| `high` | The hotkey character in a non-selected option |
| `selected` | The highlight bar (entire selected option) |
| `high_selected` | The hotkey character inside the highlight bar |

**Defaults** (when omitted):
- `normal`: Light Gray on Black (`0x07`)
- `selected`: Yellow on Blue (`0x1e`)
- `high` and `high_selected`: inherit from `normal`/`selected` if not set

**Available color names:**
Black, Blue, Green, Cyan, Red, Magenta, Brown, Light Gray, Dark Gray,
Light Blue, Light Green, Light Cyan, Light Red, Light Magenta, Yellow, White.

---

## Recipes

### Fully drawn menu (no canned list)

Your ANSI screen contains all the menu text. Maximus handles input only.

```toml
[custom_menu]
skip_canned_menu = true
```

### Classic bounded grid

The most predictable layout. Options in a fixed box, no spreading.

```toml
[custom_menu]
top_boundary = [8, 8]
bottom_boundary = [20, 61]
prompt_location = [23, 1]

boundary_layout = "grid"
boundary_justify = "left top"
option_spacing = false
option_justify = "left"
```

### Last row centered (tight)

When the last row has fewer options, center it instead of leaving it
left-anchored.

```toml
[custom_menu]
boundary_layout = "tight"
boundary_justify = "center top"
```

### Fill width only (spread_width)

Wide boundary, options distributed horizontally, rows near the top.

```toml
[custom_menu]
boundary_layout = "spread_width"
boundary_justify = "center top"
```

### Footer-style menu box (bottom aligned)

Options sit on the bottom edge of a large reserved area.

```toml
[custom_menu]
boundary_layout = "grid"
boundary_justify = "right bottom"
```

### Spread full (centered both axes)

Large boundary, options evenly distributed in both dimensions.

```toml
[custom_menu]
boundary_layout = "spread"
boundary_justify = "center center"
option_spacing = false
```

### Lightbar with custom colors

Full lightbar setup with the option grid centered.

```toml
[custom_menu]
skip_canned_menu = false
show_title = true
lightbar_menu = true
lightbar_margin = 1
top_boundary = [8, 8]
bottom_boundary = [20, 61]
title_location = [22, 32]
prompt_location = [23, 1]
option_spacing = false
option_justify = "left"
boundary_justify = "center center"
boundary_layout = "spread"

[custom_menu.lightbar_color]
normal        = ["Light Gray", "Black"]
high          = ["Yellow", "Black"]
selected      = ["White", "Blue"]
high_selected = ["Yellow", "Blue"]
```

---

## Full Key Reference

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `skip_canned_menu` | bool | `false` | Skip canned options when a menu file is displayed |
| `show_title` | bool | `true` | Show the menu title in bounded mode |
| `lightbar_menu` | bool | `false` | Enable lightbar navigation |
| `lightbar_margin` | int | `1` | Padding on each side of lightbar items |
| `top_boundary` | `[row, col]` | — | Top-left corner of option area (1-based) |
| `bottom_boundary` | `[row, col]` | — | Bottom-right corner of option area (1-based) |
| `title_location` | `[row, col]` | — | Where to print the menu title |
| `prompt_location` | `[row, col]` | — | Where to print the selection prompt |
| `option_spacing` | bool | `false` | Add blank rows between option rows |
| `option_justify` | string | `"left"` | Per-cell text alignment: `left`/`center`/`right` |
| `boundary_justify` | string | `"left top"` | Grid placement within boundary (H + V) |
| `boundary_layout` | string | `"grid"` | Column model: `grid`/`tight`/`spread`/`spread_width`/`spread_height` |
| `lightbar_color.normal` | `[FG, BG]` | `["Light Gray","Black"]` | Non-selected option color |
| `lightbar_color.high` | `[FG, BG]` | — | Hotkey color in non-selected options |
| `lightbar_color.selected` | `[FG, BG]` | `["Yellow","Blue"]` | Highlight bar color |
| `lightbar_color.high_selected` | `[FG, BG]` | — | Hotkey color inside highlight bar |

---

## Troubleshooting

### "My ANSI menu shows, but the option list / prompt is weird"

- Make sure your boundary coordinates are correct and large enough.
- Set `prompt_location` explicitly so "Select:" is always visible.
- Remember the boundary is inclusive — `bottom_boundary` is the final
  `[row, col]`, not a width/height.

### "The lightbar highlight bar overlaps my border"

Account for `lightbar_margin` consuming `lightbar_margin × 2` columns per
option cell. Either enlarge the boundary or reduce `option_width` and/or
`lightbar_margin`.

### "Vertical justification isn't doing anything"

Your boundary may be tight to the content height (no extra vertical space to
distribute). Make the boundary taller, or try `boundary_layout = "spread_height"`.

### "I enabled custom_menu and things changed more than I expected"

Start with the no-op settings and enable features one at a time:

```toml
[custom_menu]
option_spacing = false
option_justify = ""
boundary_justify = ""
boundary_layout = ""
```

Suggested safe progression:

1. Start with `grid` + explicit bounds
2. Add `prompt_location`
3. Add `boundary_justify` (horizontal)
4. Add the vertical token (`"left bottom"`, etc.)
5. Then experiment with `tight` and `spread*` modes

---

## See Also

- [Lightbar Menus]({% link config-lightbar-menus.md %}) — quick-start for
  enabling lightbar navigation
- [Menu System]({% link config-menu-system.md %}) — menu system overview
- [Menu Definitions]({% link config-menu-definitions.md %}) — TOML menu file
  format, option commands, and modifiers
- [Lightbar Customization]({% link theming-lightbar.md %}) — area-list and
  message reader lightbar settings in `display.toml`
- [Theming & Modding]({% link theming-modding.md %}) — overview of all
  customization options
