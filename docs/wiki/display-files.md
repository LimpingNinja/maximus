---
layout: default
section: "display"
title: "Display Files"
description: "Display files, screen types, and the MECCA compiler"
---

Display files are the pre-built screens that Maximus shows to callers — login
banners, menu screens, help pages, and area listings. They are compiled from
`.mec` (MECCA) source into `.bbs` binary files that the BBS loads at runtime.

---

## How Display Files Work

When the BBS needs to show a screen (e.g., the main menu, a help page, or a
file area listing), it looks for a matching `.bbs` file in the display
directory. These files can contain:

- **ANSI art** — color graphics and box-drawing characters
- **Display codes** — [pipe codes]({% link display-codes.md %}) for dynamic
  content (user name, time left, area names)
- **MECCA control sequences** — conditional display, hotkeys, and screen
  positioning

## Display Directory

Display files live under the `display/` directory in your BBS installation:

```
display/
├── help/       # Help screens
├── screens/    # Menu screens, login, logoff, newuser
└── misc/       # Bulletins, notices, other display files
```

The display path is configured in `config/maximus.toml` under
`[paths].display_path`.

---

## Editing Display Files

1. **Edit the `.mec` source** in a text editor (with ANSI art support if
   needed).
2. **Compile with MECCA** to produce the `.bbs` binary:

   ```bash
   bin/mecca display/screens/main_menu.mec
   ```

3. **Reconnect or navigate to the screen again** — Maximus loads `.bbs` files
   fresh each time they're displayed, so there's no need to restart.

See [MECCA Language]({% link mecca-language.md %}) for the full token
reference. For compiler flags and usage, see
[MECCA Compiler]({% link mecca-compiler.md %}). For ANSI art workflows, see
[ANSI Art & RIP Graphics]({% link display-ansi-rip.md %}).

---

## See Also

- [MECCA Language]({% link mecca-language.md %}) — complete MECCA token
  reference for display files
- [MECCA Compiler]({% link mecca-compiler.md %}) — compiling `.mec` source to
  `.bbs`
- [Display Codes]({% link display-codes.md %}) — pipe codes, info codes, and
  formatting operators used in display files
- [Screen Types]({% link display-screen-types.md %}) — TTY, ANSI, and AVATAR
  terminal modes
- [ANSI Art & RIP Graphics]({% link display-ansi-rip.md %}) — art creation
  workflows and RIP graphics status
- [Directory Structure]({% link directory-structure.md %}) — where display
  files live in the installation tree
