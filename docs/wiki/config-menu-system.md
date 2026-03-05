---
layout: default
title: "Menu System"
section: "configuration"
description: "How Maximus menus work — files, navigation, help levels, and display"
---

Menus are the backbone of the BBS experience. Every screen your callers see,
every command they can run, every door they can launch — it all flows through
the menu system. Maximus gives you complete control over what appears, who
can see it, and how it behaves.

---

## The Big Picture

A Maximus BBS is organized as a set of **named menus**. Each menu is a TOML
file in `config/menus/` — one file per menu. The default installation ships
with ten menus that cover the basics:

| Menu | Purpose |
|------|---------|
| `main` | Top-level hub after login |
| `message` | Message area commands |
| `msgread` | In-message reading commands |
| `file` | File area commands |
| `change` | User preference settings |
| `edit` | Line editor commands |
| `chat` | Multinode chat commands |
| `reader` | Off-line reader (QWK) commands |
| `sysop` | Sysop-only tools |
| `mex` | Sample MEX script launchers |

You can add as many menus as you like — just drop a new `.toml` file in the
menus directory and reference it from another menu using `Display_Menu` or
`Link_Menu`.

---

## How Callers Navigate

When a caller logs in, Maximus loads the menu specified by the `first_menu`
setting in your [Core Settings]({{ site.baseurl }}{% link config-core-settings.md %}) (usually
`MAIN`). From there, everything is driven by menu options:

- **`Display_Menu`** — jump to another menu (flat; no automatic return)
- **`Link_Menu`** — nest into another menu (up to 8 levels deep; `Return`
  pops back)
- **`Return`** — pop back from a `Link_Menu` call

This means your menu tree can be as flat or as deep as you want. Most boards
use a simple hub-and-spoke: `MAIN` links to `MESSAGE`, `FILE`, `CHANGE`, and
`CHAT`, each of which uses `Return` to get back.

---

## Help Levels and Display Files

Maximus supports three **help levels** — Novice, Regular, and Expert — and
each menu can show different content depending on the caller's level:

- **Novice** — the full experience: header file, menu display file, and
  generated option list all shown
- **Regular** — typically just the menu display file (no generated list)
- **Expert** — minimal display, just the prompt

In practice, **Novice is what you almost always want.** New callers default
to Novice, and the vast majority never change it. If you're only going to
tag your display files with one qualifier, make it `Novice` — that way your
ANSI art and custom menus actually get seen.

Each menu can have up to three display files:

| File | When It Shows |
|------|--------------|
| **Header** | Before the menu, every time (good for ANSI art banners) |
| **Menu** | The main menu screen (replaces the auto-generated option list) |
| **Footer** | After the menu (rarely used, but available) |

Each display file can be tagged with help-level qualifiers (`Novice`,
`Regular`, `Expert`, `RIP`) so it only shows for the right audience. If no
qualifier is given, it shows for everyone.

When no custom menu file is provided, Maximus auto-generates a simple
columnar display from the option descriptions — the classic yellow-and-gray
BBS look.

---

## Custom Menus and Lightbar Navigation

Beyond the standard display, Maximus supports **custom menus** with precise
screen positioning and **lightbar navigation** — a highlighted selector bar
that moves between options with arrow keys, just like a modern TUI. This is
configured through the `[custom_menu]` section of a menu TOML file.

For the full details on lightbar menus and bounded display areas, see
[Lightbar Menus]({{ site.baseurl }}{% link config-lightbar-menus.md %}).

---

## Where to Go Next

- **[Menu Definitions]({{ site.baseurl }}{% link config-menu-definitions.md %})** — how a menu
  TOML file is structured: global properties, option keys, and what each
  field means
- **[Menu Options]({{ site.baseurl }}{% link config-menu-options.md %})** — the complete
  reference for every command you can put on a menu, grouped by category
- **[Lightbar Menus]({{ site.baseurl }}{% link config-lightbar-menus.md %})** — custom screen
  positioning, boundary boxes, and arrow-key navigation
- **[Canned & Bounded Menus]({{ site.baseurl }}{% link config-canned-bounded-menus.md %})** —
  auto-generated displays and how they interact with custom layouts
