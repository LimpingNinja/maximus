---
layout: default
section: "display"
title: "Lightbar Customization"
description: "Configuring lightbar mode for area lists and the message reader via display.toml"
---

Lightbar mode gives your callers a modern, arrow-key-driven selection
experience instead of typing area numbers or menu letters. It works in
file area selection, message area selection, and the message reader — each
configured independently in `config/general/display.toml`.

This page covers enabling and configuring lightbar behavior for **area lists
and the message reader**, including format strings, division hierarchy, and
keyboard controls.

> **Menus have their own lightbar settings.** Per-menu lightbar navigation
> (arrow-key highlight bar over the canned option list) is configured in
> each menu's `[custom_menu]` table, not in `display.toml`. See
> [Lightbar Menus]({{ site.baseurl }}{% link config-lightbar-menus.md %}) for a quick overview
> or [Canned & Bounded Menus]({{ site.baseurl }}{% link config-canned-bounded-menus.md %}) for
> the full deep-dive on boundaries, layout, and lightbar colors.

---

## Enabling Lightbar Mode

Each section in `display.toml` has a `lightbar_area` toggle:

```toml
# config/general/display.toml

[file_areas]
lightbar_area = true

[msg_areas]
lightbar_area = true

[msg_reader]
lightbar_area = true
```

Set to `false` to fall back to the classic scrolling list for that context.

---

## Lightbar Prompts

The `[general]` section controls whether interactive Yes/No and selection
prompts use lightbar-style highlighting:

```toml
[general]
lightbar_prompts = false
```

When enabled, prompts like "Continue? [Y/n]" use a highlight bar instead of
waiting for a keypress. This is independent of area lightbar mode.

---

## Highlight Style

### What Gets Highlighted

The `lightbar_what` key controls which part of the selected row is
highlighted:

| Value   | Behavior |
|---------|----------|
| `"row"` | Entire row is highlighted (default for areas) |
| `"full"` | Full row including padding (default for reader) |
| `"name"` | Only the area/item name is highlighted |

```toml
[file_areas]
lightbar_what = "row"

[msg_reader]
lightbar_what = "full"
```

### Highlight Colors

By default, the lightbar uses the theme's `lightbar_fg` and `lightbar_bg`
color slots (from `colors.toml`). You can override them per-context:

```toml
[file_areas]
lightbar_fore = ""     # Empty = use theme lightbar_fg
lightbar_back = ""     # Empty = use theme lightbar_bg

[msg_areas]
lightbar_fore = "f"    # Nibble value: white (0xf = 15)
lightbar_back = "1"    # Nibble value: blue  (0x1 = 17 as bg)
```

Values are either a theme color slot name or a single hex nibble (`0`–`f`)
for the 16-color palette.

---

## Reduce Area

The `reduce_area` setting controls how many lines of vertical padding to
reserve above and below the list for headers and footers:

```toml
[file_areas]
reduce_area = 5

[msg_areas]
reduce_area = 5
```

This determines the scrollable viewport height. A value of `5` means the
list gets `(screen_height - 5)` rows.

---

## Screen Boundaries

For precise control over where the lightbar list renders, you can set explicit
row/column boundaries. These are `[row, col]` pairs (1-based):

```toml
[file_areas]
# top_boundary = [3, 1]
# bottom_boundary = [20, 80]
```

If omitted, Maximus calculates boundaries from `reduce_area` and the
terminal dimensions.

### Header and Footer Anchors

You can pin the header and footer to specific screen positions:

```toml
[file_areas]
# header_location = [1, 1]
# footer_location = [22, 1]
```

These control where the area list header (area count, division name) and
footer (navigation prompt) are drawn. Useful when pairing with a custom
ANSI screen.

---

## Custom Screens

Each context can display a custom ANSI screen behind the lightbar list:

```toml
[file_areas]
custom_screen = "display/screens/file_area_bg"

[msg_areas]
custom_screen = ""
```

The screen is displayed **before** the list renders. Use it to provide a
decorative border, logo, or instructions around the lightbar area. The list
renders on top of it within the defined boundaries.

---

## Quick Reference

```toml
# config/general/display.toml — all lightbar settings

[general]
lightbar_prompts = false        # Lightbar-style Yes/No prompts

[file_areas]
lightbar_area = true            # Enable lightbar for file area selection
reduce_area = 5                 # Viewport padding (lines)
lightbar_what = "row"           # Highlight: row, full, or name
lightbar_fore = ""              # Override foreground (empty = theme)
lightbar_back = ""              # Override background (empty = theme)
# top_boundary = [3, 1]        # Optional explicit boundaries
# bottom_boundary = [20, 80]
# header_location = [1, 1]     # Optional header anchor
# footer_location = [22, 1]    # Optional footer anchor
custom_screen = ""              # Optional background ANSI screen

[msg_areas]
lightbar_area = true
reduce_area = 5
lightbar_what = "row"
lightbar_fore = ""
lightbar_back = ""
custom_screen = ""

[msg_reader]
lightbar_area = true
reduce_area = 5
lightbar_what = "full"
lightbar_fore = ""
lightbar_back = ""
```

---

## See Also

- [Lightbar Menus]({{ site.baseurl }}{% link config-lightbar-menus.md %}) — enabling lightbar
  navigation on menus
- [Canned & Bounded Menus]({{ site.baseurl }}{% link config-canned-bounded-menus.md %}) — full
  menu customization: boundaries, layout, lightbar colors, recipes
- [Theme Colors]({{ site.baseurl }}{% link theme-colors.md %}) — `lightbar_fg` and
  `lightbar_bg` theme slots
- [Display Customization]({{ site.baseurl }}{% link theming-display-customization.md %}) —
  area list format strings, headers, and footers
- [Theming & Modding]({{ site.baseurl }}{% link theming-modding.md %}) — overview of all
  customization options
