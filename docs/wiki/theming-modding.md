---
layout: default
section: "display"
title: "Theming & Modding"
description: "Customizing the look, feel, and behavior of your BBS"
---

Maximus NG is designed to be deeply customizable. Every color, prompt, menu
screen, area listing format, and lightbar behavior can be tweaked without
touching source code. This section covers all the knobs.

---

## Theme Colors

The semantic color system lets you define named color slots (`|pr` for prompt,
`|er` for error, `|hd` for heading, etc.) in a single file and have them
applied everywhere — menus, prompts, status lines, area listings, and language
strings.

- [Theme Colors]({% link theme-colors.md %}) — how the color system works,
  the 25-slot reference, `colors.toml` configuration, and editing colors in
  maxcfg

---

## Language Strings

Every prompt, error message, status line, and menu label the BBS displays
comes from a language TOML file. You can customize every string your callers
see.

- [Language Files (TOML)]({% link lang-toml.md %}) — the language string
  system, positional parameters, parameter metadata, and the delta overlay
  architecture
- [Language Editor]({% link lang-editor.md %}) — editing strings
  interactively in maxcfg

---

## Lightbar Customization

Lightbar mode gives your callers a modern, arrow-key-driven selection
experience for area lists and the message reader. The `display.toml` settings
covered here control area-list and reader lightbar — menus have their own
per-menu lightbar settings under `[custom_menu]` (see
[Lightbar Menus]({% link config-lightbar-menus.md %}) in Configuration).

- [Lightbar Customization]({% link theming-lightbar.md %}) — enabling and
  configuring lightbar mode in `display.toml`, highlight styles, boundaries,
  and custom screens

---

## Display Customization

Beyond the obvious display files and menus, Maximus has a number of
less-visible configuration points that control how area listings, headers,
footers, date/time formats, and file listings are rendered. These live in
`display_files.toml` and are easy to overlook.

- [Display Customization]({% link theming-display-customization.md %}) — area
  list format strings, headers, footers, date/time formats, and other hidden
  configuration points in `display_files.toml`

---

## Display Files & Codes

Display files are the compiled screens (login banners, menus, help pages) and
display codes are the pipe sequences used inside them.

- [Display Codes]({% link display-codes.md %}) — pipe codes, info codes, and
  formatting operators
- [Display Files]({% link display-files.md %}) — working with `.mec` source,
  MECCA compilation, and ANSI art

---

## See Also

- [Color Codes]({% link mci-color-codes.md %}) — numeric pipe colors and
  theme color code reference
- [Configuration]({% link configuration.md %}) — core BBS settings
- [Directory Structure]({% link directory-structure.md %}) — where
  configuration and display files live
