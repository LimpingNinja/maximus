---
layout: default
title: "Menu Definitions"
section: "configuration"
description: "How a menu TOML file is structured — global properties, options, and modifiers"
---

Every menu your callers interact with is defined in a single TOML file under
`config/menus/`. The filename (minus `.toml`) becomes the menu's internal
name — `main.toml` is the `MAIN` menu, `message.toml` is the `MESSAGE` menu,
and so on.

This page walks through the anatomy of a menu file: what goes at the top,
what each key means, and how options are defined. For the complete list of
commands you can assign to an option, see
[Menu Options]({{ site.baseurl }}{% link config-menu-options.md %}).

---

## File Structure at a Glance

A menu TOML file has two sections:

1. **Global properties** — bare keys at the top of the file that define the
   menu's identity, display files, and layout hints.
2. **Options** — an array of `[[option]]` tables, each defining one command
   a caller can select.

Optionally, a `[custom_menu]` section can follow the options to enable
lightbar navigation and precise screen positioning (see
[Lightbar Menus]({{ site.baseurl }}{% link config-lightbar-menus.md %})).

Here's a minimal example:

```toml
name = "MAIN"
title = "Main menu (%t mins)"
menu_file = "display/screens/menu_main.ans"
menu_types = ["Novice"]
option_width = 14

[[option]]
command = "Display_Menu"
arguments = "MESSAGE"
priv_level = "Demoted"
description = "Message areas"

[[option]]
command = "Goodbye"
arguments = ""
priv_level = "Transient"
description = "Goodbye (log off)"
```

---

## Global Properties

These keys appear at the top level of the file (not inside any table). They
control how the menu is identified, what display files are shown, and how
auto-generated menus are formatted.

### `name` (string)

The menu's internal name. This is the identifier used in `Display_Menu` and
`Link_Menu` arguments from other menus. It should match the filename (without
`.toml`), in uppercase by convention.

```toml
name = "MAIN"
```

### `title` (string)

The title displayed to the caller at the top of the menu prompt. Supports
external program translation characters — `%t` expands to minutes remaining,
for example.

```toml
title = "Main menu (%t mins)"
```

### `header_file` (string)

Path to an optional display file shown **before** the menu, every time the
menu is displayed. Relative to your BBS display directory. Great for ANSI art
banners or MEX scripts (prefix with `:` for MEX).

```toml
header_file = "display/screens/msg_header.ans"
```

When using a MEX script as the header, the script receives `"1"` as its
argument the first time the menu is entered, and `"0"` on subsequent
redisplays.

### `header_types` (array of strings)

Controls which callers see the header file, based on their help level. Valid
values: `"Novice"`, `"Regular"`, `"Expert"`, `"RIP"`. If empty or omitted,
the header shows for everyone.

```toml
header_types = ["Novice", "Regular", "Expert"]
```

### `footer_file` (string)

Path to an optional display file shown **after** the menu body. Same rules as
`header_file`. Rarely used, but available if you want a persistent footer
banner.

### `footer_types` (array of strings)

Same as `header_types`, but for the footer file.

### `menu_file` (string)

Path to a custom display file that **replaces** the auto-generated menu
display. When present, Maximus shows this file instead of building a columnar
list from the option descriptions. This is how you create beautiful ANSI menu
screens.

```toml
menu_file = "display/screens/menu_main.ans"
```

If omitted, Maximus generates a simple multi-column layout from the option
descriptions — the classic BBS look.

### `menu_types` (array of strings)

Controls which callers see the custom menu file. Typically set to
`["Novice"]` so novice users get the pretty ANSI screen while experts see
just the prompt.

```toml
menu_types = ["Novice"]
```

### `menu_length` (int)

When using a custom `menu_file`, this tells Maximus how many lines the
display file occupies. Maximus uses this value to manage screen scrolling and
"More?" prompts when showing messages after the menu. Set to `0` if you don't
need explicit line tracking.

### `menu_color` (int)

An AVATAR color attribute number. When a caller presses a hotkey, Maximus
resets the text color to this value before echoing the keypress. This
prevents color bleed from ANSI art menu files. Set to `-1` to disable.

```toml
menu_color = -1
```

### `option_width` (int)

The column width used when auto-generating menu displays. Each option
description is padded to this width, and options are arranged in columns
across the screen. The default is 20. Set to `0` to use the default.

```toml
option_width = 14
```

---

## Options (`[[option]]`)

The heart of every menu. Each `[[option]]` table defines one selectable
command. Options appear in the order they're listed — this is the order
callers see them in auto-generated menus, and the order Maximus scans when
matching keypresses.

A menu can have up to **127 options**.

### `command` (string) — **required**

The command to execute when this option is selected. This is a command name
like `Display_Menu`, `Msg_Enter`, `File_Download`, or `Goodbye`. Command
names are case-insensitive.

For the full list of available commands and what each one does, see
[Menu Options]({{ site.baseurl }}{% link config-menu-options.md %}).

```toml
command = "Display_Menu"
```

### `arguments` (string)

The argument passed to the command. What this means depends on the command:

| Command Type | Arguments Meaning |
|-------------|-------------------|
| `Display_Menu` / `Link_Menu` | Name of the target menu |
| `Display_File` | Path to the display file |
| `MEX` | Path to the MEX script |
| `Xtern_Run` / `Xtern_Door32` | Command line to execute (supports `%` substitutions) |
| `Key_Poke` | Keystrokes to inject |
| `Msg_Reply_Area` | Area name, or `"."` to prompt |
| Most others | Not used (leave as `""`) |

```toml
arguments = "MESSAGE"
```

### `priv_level` (string) — **required**

The minimum privilege level required to see and use this option. This is an
access level name from your
[access levels]({{ site.baseurl }}{% link config-access-levels.md %}) — like `"Demoted"`,
`"Normal"`, `"Sysop"`, etc.

Can also include key-flag gating with a `/` separator: `"Normal/1C"` means
the caller needs Normal level **and** flags 1 and C set. See
[Privileges & Flags]({{ site.baseurl }}{% link config-privileges-flags.md %}) for details.

```toml
priv_level = "Demoted"
```

### `description` (string)

The user-facing text for this option. This serves double duty:

1. **Display text** — what callers see in the menu listing
2. **Hotkey** — the **first character** of the description becomes the
   selection key

So `"Message areas"` means pressing `M` selects this option. To use a
different hotkey, set the first character accordingly — `"?help"` triggers
on `?`, `"#Unreceive Msg"` triggers on `#`.

**Special prefixes:**

- **Backtick + number** (`` `75 ``) — binds to a keyboard scan code instead
  of a printable character. Scan code 75 is the left arrow key. These are
  always paired with `NoDsp` so they don't show in the menu listing.
- **`$`** — a non-selectable separator or prefix (not matched as a hotkey)

```toml
description = "Message areas"
```

### `key_poke` (string)

Keystrokes to inject into the input buffer **before** the command runs.
Maximus behaves as if the caller typed these keys. Underscores are converted
to spaces. Supports `%` translation characters.

This is useful for pre-filling command sequences:

```toml
# When the user presses B, inject "ayl" so Msg_Browse runs as
# Browse → All → Your → List
command = "Msg_Browse"
key_poke = "ayl"
description = "Browse new mail"
```

Leave as `""` when not needed.

### `modifiers` (array of strings)

Behavior and visibility flags that modify how the option works. Zero or more
of:

| Modifier | Effect |
|----------|--------|
| `NoDsp` | Hide from menu display but still respond to the hotkey. Essential for hidden shortcuts, scan-code bindings, and linked options. |
| `NoCls` | Don't clear the screen when chaining to another menu (only meaningful with `Display_Menu`). |
| `Ctl` | Write an Opus-compatible `.CTL` file before running an external program. Legacy; rarely needed. |
| `Reread` | Re-read the user record from disk after an external program exits. Use when a door game modifies user data. |
| `Stay` | Stay on the current menu after the command completes (don't return to the parent). |
| `Then` | Conditional: run this option only if the previous option succeeded. Implies `NoDsp`. |
| `Else` | Conditional: run this option only if the previous option failed. Implies `NoDsp`. |
| `RIP` | Only show to callers with RIP graphics enabled. |
| `NoRIP` | Only show to callers without RIP graphics. |
| `UsrLocal` | Only show to locally connected users (console). |
| `UsrRemote` | Only show to remote (telnet/modem) users. |
| `Local` | Only show when the current area has the Local style. |
| `Matrix` | Only show in NetMail-style areas. |
| `Echo` | Only show in EchoMail-style areas. |
| `Conf` | Only show in Conference-style areas. |

```toml
modifiers = ["NoDsp"]
```

---

## Putting It Together: A Real Example

Here's a trimmed version of the default `message.toml`, showing how globals,
visible options, hidden scan-code bindings, and area-type modifiers work
together:

```toml
name = "MESSAGE"
title = "MESSAGE (%t mins)"
header_file = ":scripts/headmsg"
header_types = ["Novice", "Regular", "Expert"]
menu_file = "display/screens/max_msg"
menu_types = ["Novice"]
option_width = 0

# Visible options — callers see these in the menu
[[option]]
command = "Msg_Area"
arguments = ""
priv_level = "Demoted"
description = "Area change"

[[option]]
command = "Read_Next"
arguments = ""
priv_level = "Demoted"
description = "Next message"

[[option]]
command = "Msg_Enter"
arguments = ""
priv_level = "Demoted"
description = "Enter message"

# Hidden scan-code binding — left arrow reads previous message
[[option]]
command = "Read_Previous"
arguments = ""
priv_level = "Demoted"
description = "`75"
modifiers = ["NoDsp"]

# Area-type-specific help — shows different help files depending
# on whether the caller is in a Local, NetMail, or Echo area
[[option]]
command = "Display_File"
arguments = "display/help/msg"
priv_level = "Demoted"
description = "?help"
modifiers = ["Local"]

[[option]]
command = "Display_File"
arguments = "display/help/mail"
priv_level = "Demoted"
description = "?help"
modifiers = ["Matrix"]

[[option]]
command = "Display_File"
arguments = "display/help/echo"
priv_level = "Demoted"
description = "?help"
modifiers = ["Echo"]
```

---

## See Also

- [Menu Options]({{ site.baseurl }}{% link config-menu-options.md %}) — every command you can
  assign to an option, with detailed descriptions
- [Lightbar Menus]({{ site.baseurl }}{% link config-lightbar-menus.md %}) — the
  `[custom_menu]` section for lightbar navigation and screen positioning
- [Canned & Bounded Menus]({{ site.baseurl }}{% link config-canned-bounded-menus.md %}) —
  auto-generated displays
- [Menu System]({{ site.baseurl }}{% link config-menu-system.md %}) — the parent overview
