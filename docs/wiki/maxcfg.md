---
layout: default
section: "configuration"
title: "MaxCFG"
description: "The Maximus BBS configuration tool"
---

MaxCFG is your one-stop shop for configuring Maximus BBS. System settings, menu
definitions, user accounts, message and file areas, language strings, color
themes — it's all here. You can do everything from the interactive TUI editor,
or use command-line flags for scripted/headless operations like converting
legacy configs.

---

## Two Ways to Use It

- **TUI Editor** — Run `maxcfg` with no arguments and you get a full
  interactive text-mode editor, modeled after classic BBS setup utilities. If
  you've used RemoteAccess Config or KBBS Setup, you'll feel right at home.
  See [TUI Editor Overview]({% link maxcfg-tui.md %}).

- **CLI Mode** — Pass flags for headless operations: converting legacy CTL
  files to TOML, applying delta overlays, exporting configs. Great for
  scripting and automation. See [CLI Usage]({% link maxcfg-cli.md %}).

---

## TUI Editor Pages

The TUI editor is broken into focused sections, each with its own page:

- **[TUI Editor Overview]({% link maxcfg-tui.md %})** — how the screen is
  laid out, navigation keys, field types, and how saving works
- **[Setup & Tools]({% link maxcfg-setup-tools.md %})** — your BBS identity,
  paths, login behavior, colors, FidoNet/matrix settings, security levels,
  protocols, and the Tools menu
- **[Menu Editor]({% link maxcfg-menu-editor.md %})** — design and preview
  your BBS menus with full ANSI art support, lightbar navigation, and a
  live preview that shows exactly what callers see
- **[Area Editor]({% link maxcfg-areas.md %})** — organize message and file
  areas into divisions, edit them via tree view or flat picklist
- **[User Editor]({% link maxcfg-user-editor.md %})** — browse, filter, and
  edit user accounts stored in the SQLite database
- **[Language Browser]({% link lang-editor.md %})** — search, filter, and
  live-edit every string your callers see, with MCI code preview

---

## Built-in Tools

MaxCFG also bundles a couple of standalone tools accessible from the CLI:

- **[Language Browser]({% link lang-editor.md %})** — also available from the
  TUI at **Content → Language Strings**. Search, filter, and edit language
  strings with live write-back to the TOML file.

- **[CTL → TOML Export]({% link legacy-ctl-to-toml.md %})** — one-way
  conversion of legacy `.ctl` configuration files into the modern TOML
  format. If you're migrating from classic Maximus, start here.

---

## Quick Start

```bash
# Launch the interactive editor
maxcfg

# Launch pointing at a specific BBS installation
maxcfg /opt/maximus

# Convert a legacy config to TOML
maxcfg --export-nextgen /path/to/max.ctl

# Apply a delta overlay to a language file
maxcfg --apply-delta english.toml
```

For the full CLI flag reference, see [CLI Usage]({% link maxcfg-cli.md %}).
