---
layout: default
section: "display"
title: "Display Customization"
description: "Hidden configuration points in display_files.toml for area listings, headers, footers, and formats"
---

Beyond the obvious display files (login screens, menus, help pages), Maximus
has a number of less-visible configuration points that control how area
listings, headers, footers, date and time formats, and file listings are
rendered. These live in `config/general/display_files.toml` and are easy to
overlook — but they're some of the most impactful theming knobs.

---

## Area List Format Strings

When a caller browses file or message areas, Maximus renders each area using
a configurable format string. These support display codes and special
area-specific tokens.

### Message Area Listing

```toml
# config/general/display_files.toml

msg_header = "%x16%x01%x0fMessage Areas %x16%x01%x0d%xc4%xc4%xc4%xc4%xc4%xc4%xc4%xc4%xc4%xc4%xc4%xc4%xc4%xc4%x0a%x0a"
msg_format = "|pr%*|sy%-20#|tx ... |fi%n%x0a"
msg_footer = "%x0a|prLocation: |hi%b|07%x0a"
msg_area_list = ""
```

- **`msg_header`** — displayed once at the top of the area list. Use AVATAR
  hex codes (`%x16%x01...`) or pipe codes for colors and formatting.
- **`msg_format`** — rendered for each area. Tokens:
  - `%*` — area tag number
  - `%#` — area short name
  - `%-20#` — area name, left-padded to 20 characters
  - `%n` — area description
  - `%b` — current division/location breadcrumb
- **`msg_footer`** — displayed once at the bottom.
- **`msg_area_list`** — optional display file shown instead of the
  programmatic list. If set, it overrides the format-string-based listing
  entirely.

### File Area Listing

```toml
file_header = "%x16%x01%x0fFile Areas %x16%x01%x0d%xc4%xc4%xc4%xc4%xc4%xc4%xc4%xc4%xc4%xc4%xc4%xc4%xc4%xc4%x0a%x0a"
file_format = "|tx[|hd%-4t|tx]|pr %-20a |tx... %-n%x0a"
file_footer = "%x0a|prLocation: |hi%b|07%x0a"
file_area_list = ""
```

- **`file_format`** tokens:
  - `%t` / `%-4t` — area tag, left-padded to 4 characters
  - `%a` / `%-20a` — area name, left-padded to 20 characters
  - `%n` / `%-n` — area description
  - `%b` — current division/location breadcrumb

### Customizing the Look

These format strings accept full [display codes]({{ site.baseurl }}{% link display-codes.md %})
including theme colors. A quick retheme:

```toml
# Modern themed look
msg_format = "|br[|hk%-4*|br]|tx %-24#|dm ... |fi%n|cd%x0a"
file_format = "|br[|hk%-4t|br]|tx %-20a|dm ... |fi%-n|cd%x0a"
```

---

## Display File Screens

`display_files.toml` maps logical screen names to file paths. Each entry
points to a compiled `.bbs` display file (or leave empty to skip):

### Login & Session Screens

| Key | When Shown |
|-----|------------|
| `logo` | BBS logo / splash screen |
| `welcome` | Shown to returning callers at login |
| `rookie` | Shown to new callers (falls back to `welcome`) |
| `application` | New user application screen |
| `new_user1` | First new-user information screen |
| `new_user2` | Second new-user information screen |
| `bye_bye` | Logoff / goodbye screen |
| `bad_logon` | Shown after a failed login attempt |
| `day_limit` | Shown when daily time limit is reached |
| `time_warn` | Time remaining warning |
| `too_slow` | Shown when connection is too slow |
| `barricade` | Access denied / barricade screen |

### Help Screens

| Key | When Shown |
|-----|------------|
| `contents` | Help table of contents |
| `locate` | Locate/search help |
| `oped_help` | Full-screen editor help |
| `line_ed_help` | Line editor help |
| `replace_help` | Search-and-replace help |
| `scan_help` | Message scan/reader help |
| `header_help` | Message header entry help |
| `entry_help` | Message entry help |
| `xfer_baud` | File transfer baud rate information |

### System Screens

| Key | When Shown |
|-----|------------|
| `quote` | Random quotes file |
| `no_space` | Disk space warning |
| `no_mail` | No new mail notification |
| `shell_to_dos` | Shown before shell-to-OS |
| `back_from_dos` | Shown after return from shell |
| `not_found` | Generic "not found" screen |
| `not_configured` | Feature not configured |
| `protocol_dump` | Protocol listing screen |
| `tune` | Tune/music file |

### Example: Custom Login Flow

```toml
logo = "display/screens/my_logo"
welcome = "display/screens/my_welcome"
rookie = "display/screens/my_newuser_welcome"
bye_bye = "display/screens/my_goodbye"
quote = "display/screens/my_quotes"
```

---

## Date and Time Formats

These control how dates and times appear throughout the BBS:

```toml
time_format = "%H:%M:%S"
date_format = "%C-%D-%Y"
```

Format tokens follow C `strftime` conventions:

| Token | Meaning |
|-------|---------|
| `%H` | Hour (00–23) |
| `%M` | Minute (00–59) |
| `%S` | Second (00–59) |
| `%C` | Month (01–12) |
| `%D` | Day (01–31) |
| `%Y` | Year (2-digit) |

---

## File Name Format

The `fname_format` key points to a display file that controls how file
listings (in file areas) are rendered:

```toml
fname_format = "display/screens/fformat"
```

This display file defines the column layout for filename, size, date, and
description in file area listings.

---

## Quick Wins for Theming

Here are some high-impact changes you can make entirely in
`display_files.toml`:

1. **Retheme area listings** — change `msg_format` and `file_format` to use
   theme colors (`|pr`, `|hd`, `|fi`, `|br`) instead of hard-coded numerics.

2. **Add area list headers** — set `msg_header` and `file_header` to display
   a decorative banner with box-drawing characters and theme colors.

3. **Custom footers with breadcrumbs** — use `%b` in footers to show the
   caller's current position in the division hierarchy.

4. **Replace the login flow** — swap `logo`, `welcome`, `rookie`, and
   `bye_bye` with custom ANSI art screens.

5. **Fix the time format** — change `date_format` from the default
   `%C-%D-%Y` (US) to `%D-%C-%Y` (international) or any other layout.

---

## See Also

- [Display Files]({{ site.baseurl }}{% link display-files.md %}) — working with `.mec` source
  and the MECCA compiler
- [Display Codes]({{ site.baseurl }}{% link display-codes.md %}) — pipe codes and info codes
  used in format strings
- [Lightbar Customization]({{ site.baseurl }}{% link theming-lightbar.md %}) — lightbar mode
  for area lists
- [Theme Colors]({{ site.baseurl }}{% link theme-colors.md %}) — the semantic color system
- [Theming & Modding]({{ site.baseurl }}{% link theming-modding.md %}) — overview of all
  customization options
