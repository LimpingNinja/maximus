---
layout: default
title: "Lightbar Menus"
section: "configuration"
description: "Enabling arrow-key lightbar navigation on BBS menus"
---

Lightbar menus add arrow-key navigation to the canned option list that
Maximus prints for NOVICE-mode callers. Instead of reading a list and typing
a hotkey, the caller sees a highlight bar they can move with the arrow keys
and press Enter to select.

Lightbar mode is configured **per-menu** in the `[custom_menu]` table inside
each menu's TOML file (`config/menus/*.toml`). It is separate from the
area-list lightbar settings in `display.toml` — see
[Lightbar Customization]({{ site.baseurl }}{% link theming-lightbar.md %}) for those.

---

## Basic Setup

Lightbar navigation requires three things:

1. **`lightbar_menu = true`** — turns on the highlight bar.
2. **Valid boundaries** — `top_boundary` and `bottom_boundary` must both be
   set with valid coordinates. Lightbar uses positioned rendering, so it
   needs to know the rectangle.
3. **NOVICE help level** — the caller must be in NOVICE mode. REGULAR and
   EXPERT always use the classic text path.

Without valid boundaries, `lightbar_menu = true` is silently ignored and
Maximus falls back to the normal canned menu (text list + single-keystroke
input).

### Minimum Working Example

```toml
# config/menus/main.toml
option_width = 14

[custom_menu]
lightbar_menu = true
top_boundary = [8, 8]
bottom_boundary = [20, 61]
```

That's enough. Maximus will render the canned option list inside the boundary
with arrow-key navigation, hotkey detection, and Enter to select.

### Recommended Additions

- **`lightbar_margin`** — padding (in characters) on each side of every
  lightbar item. Default is `1`. The effective cell width becomes
  `option_width + (lightbar_margin × 2)`.
- **`prompt_location`** — where the "Select:" prompt is drawn. Without it,
  the prompt prints inline after the last option, which can look odd with a
  bounded layout. Setting this prevents overlap.

```toml
[custom_menu]
lightbar_menu = true
lightbar_margin = 1
top_boundary = [8, 8]
bottom_boundary = [20, 61]
prompt_location = [23, 1]
```

---

## Advanced: Colors and Layout

### Lightbar Colors

The lightbar painter supports four color states, each specified as a
`["Foreground", "Background"]` pair using DOS color names:

```toml
[custom_menu.lightbar_color]
normal        = ["Light Gray", "Black"]
high          = ["Yellow", "Black"]
selected      = ["White", "Blue"]
high_selected = ["Yellow", "Blue"]
```

- **`normal`** — non-selected option text
- **`high`** — the hotkey character in a non-selected option
- **`selected`** — the highlight bar (selected option)
- **`high_selected`** — the hotkey character inside the highlight bar

If omitted, `normal` defaults to Light Gray on Black and `selected` defaults
to Yellow on Blue.

### Layout Controls

The `[custom_menu]` table also supports layout options that control how the
option grid is arranged within the boundary:

- **`option_justify`** — text alignment within each cell: `"left"`,
  `"center"`, or `"right"`
- **`boundary_justify`** — where the option grid sits inside the boundary:
  `"center center"`, `"right bottom"`, etc.
- **`boundary_layout`** — column distribution model: `"grid"`, `"tight"`,
  `"spread"`, `"spread_width"`, or `"spread_height"`
- **`option_spacing`** — adds a blank row between option rows

These are covered in full detail — with visual examples, recipes, and
troubleshooting — on the
[Canned & Bounded Menus]({{ site.baseurl }}{% link config-canned-bounded-menus.md %}) page.

---

## See Also

- [Canned & Bounded Menus]({{ site.baseurl }}{% link config-canned-bounded-menus.md %}) — full
  deep-dive on `[custom_menu]`: boundaries, layout modes, lightbar colors,
  recipes, and troubleshooting
- [Menu System]({{ site.baseurl }}{% link config-menu-system.md %}) — menu system overview
- [Menu Definitions]({{ site.baseurl }}{% link config-menu-definitions.md %}) — TOML menu file
  format and option commands
- [Lightbar Customization]({{ site.baseurl }}{% link theming-lightbar.md %}) — area-list and
  message reader lightbar settings in `display.toml`
- [Theme Colors]({{ site.baseurl }}{% link theme-colors.md %}) — `lightbar_fg` and
  `lightbar_bg` theme slots used by area lightbars
