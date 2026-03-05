---
layout: default
section: "configuration"
title: "Configuration"
description: "TOML-based system configuration for Maximus BBS"
---

Maximus NG uses plain-text TOML files for everything that used to live
in compiled PRM/CTL binaries. You can edit them with any text editor,
diff them in Git, and deploy them without a recompile step. MaxCFG
gives you a full TUI if you'd rather point-and-click.

Here's what lives in this section:

- **[MaxCFG]({{ site.baseurl }}{% link maxcfg.md %})** — The configuration editor. A
  curses-based TUI for managing menus, areas, users, display files,
  and system settings. Includes a
  [CLI reference]({{ site.baseurl }}{% link maxcfg-cli.md %}) for batch operations.

- **[Core Settings]({{ site.baseurl }}{% link config-core-settings.md %})** — The
  fundamentals: BBS name, sysop identity, paths, session behavior,
  [login flow]({{ site.baseurl }}{% link config-session-login.md %}), and
  [equipment & comm]({{ site.baseurl }}{% link config-equipment-comm.md %}) settings.

- **[Menu System]({{ site.baseurl }}{% link config-menu-system.md %})** — How callers
  navigate your board. Menu
  [definitions]({{ site.baseurl }}{% link config-menu-definitions.md %}) and
  [options]({{ site.baseurl }}{% link config-menu-options.md %}),
  [lightbar menus]({{ site.baseurl }}{% link config-lightbar-menus.md %}), and
  [canned & bounded layouts]({{ site.baseurl }}{% link config-canned-bounded-menus.md %}).

- **[Security & Access]({{ site.baseurl }}{% link config-security-access.md %})** —
  Privilege levels, access flags, and the ACS (Access Control String)
  system that gates every area, menu option, and command.

- **[Door Games]({{ site.baseurl }}{% link config-door-games.md %})** — Running
  external programs, dropfile formats, and Door32 support.

- **[Legacy Migration]({{ site.baseurl }}{% link legacy-migration.md %})** — Moving
  from the old CTL/PRM/MAD pipeline to TOML. Conversion tools, delta
  overlays, and language file migration.
