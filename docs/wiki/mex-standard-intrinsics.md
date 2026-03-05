---
layout: default
title: "Standard Intrinsics"
section: "mex"
description: "Every built-in function MEX gives you — I/O, user data, areas, files, time, and more"
---

Intrinsics are the built-in functions that connect your MEX script to Maximus.
They're how you print to the screen, read the user record, scan message areas,
check the clock, and do file I/O. Without them, MEX is just a language that
can do math. With them, your script *is* the BBS.

Every intrinsic is declared in a header file — `max.mh` for the core set,
`maxui.mh` for the UI toolkit, `socket.mh` for networking, `json.mh` for
JSON processing. You `#include` what you need and the compiler handles the
rest.

This page is a map. Pick the category that matches what you're trying to do,
and it'll take you to the full reference with signatures, descriptions, and
examples.

---

## Finding the Right Function

Not sure where to look? Here's the cheat sheet:

| I want to... | Go to |
|--------------|-------|
| Print something to the screen | [Display & I/O](#display-io) |
| Read a keypress or get user input | [Display & I/O](#display-io) |
| Check who the caller is | [User & Session](#user-session) |
| Read or post messages | [Message & File Areas](#msg-file) |
| Navigate or search areas | [Message & File Areas](#msg-file) |
| Build a lightbar menu or form | [UI Primitives](#ui-prims) |
| Make an HTTP request | [Networking](#networking) |
| Parse or build JSON | [JSON](#json) |
| Read or write files on disk | [Display & I/O](#display-io) (file I/O section) |
| Generate random numbers | [Display & I/O](#display-io) (utility headers) |

---

## Reference Pages

{: #display-io}
### Display & I/O

[Full Reference →]({{ site.baseurl }}{% link mex-intrinsics-display-io.md %})

Output, input, file I/O, string manipulation, and the utility functions that
every script needs. This is the biggest category — it covers `print()`,
`getch()`, `input_str()`, `open()`/`read()`/`write()`, the `COL_*` color
constants, and all the string functions (`strlen`, `substr`, `strfind`,
`strtrim`, etc.).

**Header:** `max.mh` (always included)

{: #user-session}
### User & Session

[Full Reference →]({{ site.baseurl }}{% link mex-intrinsics-user-session.md %})

Everything about the current caller and the session they're in. The `usr`
struct gives you the caller's name, alias, city, privilege level, help level,
terminal type, and dozens of other fields. Session functions handle time
management, privilege checking, class lookups, and the caller log.

**Header:** `max.mh`

{: #msg-file}
### Message & File Areas

[Full Reference →]({{ site.baseurl }}{% link mex-intrinsics-message-file.md %})

Reading messages, scanning areas, navigating between areas, file area search,
download queue management, and the `menu_cmd()` dispatch that lets your script
invoke any menu command programmatically.

**Headers:** `max.mh`, `max_menu.mh` (for `menu_cmd()` constants)

{: #ui-prims}
### UI Primitives

[Full Reference →]({{ site.baseurl }}{% link mex-ui-primitives.md %})

Cursor positioning, color attributes, bounded input fields, lightbar menus
(vertical and 2D positioned), inline select prompts, multi-field forms, and
scrollable text viewers. This is the toolkit for building full-screen
interfaces.

**Header:** `maxui.mh`

{: #networking}
### Networking (Sockets & HTTP)

[Full Reference →]({{ site.baseurl }}{% link mex-sockets.md %})

Outgoing TCP connections, raw socket I/O, and the `http_request()` convenience
function for HTTP/HTTPS GET and POST. Includes timeout control so a dead
server can't hang your caller's session.

**Header:** `socket.mh`

{: #json}
### JSON Processing

[Full Reference →]({{ site.baseurl }}{% link mex-json.md %})

Parsing JSON strings, reading values with path accessors or cursor navigation,
building JSON from scratch, and the type constants. Backed by cJSON under
the hood.

**Header:** `json.mh`

---

## See Also

- [Language Guide]({{ site.baseurl }}{% link mex-language-guide.md %}) — the MEX language itself
  (types, operators, control flow, functions)
- [MEX Compiler]({{ site.baseurl }}{% link mex-compiler.md %}) — compiling and running scripts
- [Learning MEX]({{ site.baseurl }}{% link mex-learning.md %}) — the 10-lesson tutorial series
- [Examples & Recipes]({{ site.baseurl }}{% link mex-examples-recipes.md %}) — copy-paste
  starting points for common tasks
