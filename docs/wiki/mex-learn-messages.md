---
layout: default
title: "The Message Base: You've Got Mail (And We Can Read It)"
section: "mex"
description: "Lesson 8 — Message areas, area stats, and building a board dashboard"
---

*Lesson 8 of Learning MEX*

---

> **What you'll build:** A "Board Pulse" dashboard — a script that scans
> every message area on your board, counts the messages in each one, and
> presents a tidy summary. Your callers see what's active at a glance.

## 4:06 AM

Your board has menus, a guestbook, games, a profile card. It looks like
a real system now. But the thing that makes people *come back* to a BBS
isn't the games or the ASCII art. It's the messages. The conversations.
The arguments about text editors. The recipe threads that somehow appear
in the programming echo.

This lesson teaches your scripts to see the message base — not to replace
Maximus's built-in message reader (which does its job perfectly well), but
to *observe* it. How many messages are in each area? Which areas are
active? How many total posts does the board have? This is the kind of
information that turns a login screen from "welcome" to "welcome, and
here's what you missed."

## The Global Structs

When a user is logged in, Maximus maintains two global structs that
describe the current message context:

### `marea` — The Current Message Area

```c
struct _marea: marea;
```

| Field | Type | What It Holds |
|-------|------|--------------|
| `marea.name` | string | Area name (e.g., `"General"`) |
| `marea.descript` | string | Area description |
| `marea.path` | string | Path to the message base files |
| `marea.tag` | string | Echo tag (e.g., `"GENERAL"`) |
| `marea.type` | int | `MSGTYPE_SDM` (*.MSG) or `MSGTYPE_SQUISH` |
| `marea.attribs` | int | Flags: `MA_PVT`, `MA_PUB`, `MA_NET`, `MA_ECHO`, etc. |

### `msg` — The Current Message State

```c
struct _msg: msg;
```

| Field | Type | What It Holds |
|-------|------|--------------|
| `msg.current` | long | Current message number |
| `msg.high` | long | Highest message number in area |
| `msg.num` | long | Total messages in area |

These globals update automatically when you switch areas. That's the
key insight — you don't need to open message base files yourself. You
just switch areas and read the structs.

## Iterating Message Areas

To scan all areas on the board, use the find functions:

```c
struct _marea: ma;

if (msgareafindfirst(ma, "", AFFO_NODIV))
{
  do
  {
    // ma now contains info about an area
    print(ma.name, ": ", ma.descript, "\n");
  }
  while (msgareafindnext(ma));

  msgareafindclose();
}
```

**`msgareafindfirst(ma, "", AFFO_NODIV)`** starts the search. The empty
string means "match all areas." `AFFO_NODIV` skips division markers
(which are organizational separators, not real areas). Returns true if
an area was found.

**`msgareafindnext(ma)`** advances to the next area. Returns true if
another area exists, false when you've reached the end.

**`msgareafindclose()`** — always call this when done. It frees internal
state.

The `ma` struct is a *temporary copy* — it holds info about whichever
area the find cursor is pointing at. It doesn't change the user's
current area.

## Switching Areas and Reading Stats

To get the message count for an area, you need to *select* it:

```c
string: saved_area;

// Remember where we are
saved_area := marea.name;

// Switch to a different area
if (msgareaselect("General"))
{
  print("Messages in General: ", msg.num, "\n");
  print("Highest number: ", msg.high, "\n");
}

// Switch back
msgareaselect(saved_area);
```

**`msgareaselect(name)`** changes the user's current message area.
After this call, the global `marea` and `msg` structs reflect the new
area. Returns true on success.

**Always save and restore.** If your script switches areas for scanning
purposes, save `marea.name` before you start and restore it when you're
done. The caller shouldn't notice that your script was poking around
behind the scenes.

## Triggering Built-In Commands

MEX can invoke any of Maximus's built-in menu commands via `menu_cmd()`:

```c
menu_cmd(MNU_MSG_CHECKMAIL, "");
```

This runs the "check mail" command as if the caller had triggered it
from a menu. The constants are defined in `max_menu.mh`:

| Constant | What It Does |
|----------|-------------|
| `MNU_MSG_CHECKMAIL` | Check for personal mail |
| `MNU_MSG_ENTER` | Enter a new message |
| `MNU_READ_NEXT` | Read the next message |
| `MNU_READ_INDIVIDUAL` | Read a specific message by number |
| `MNU_MSG_BROWSE` | Browse/scan messages |
| `MNU_MSG_AREA` | Show the area-change menu |

This is powerful. Your scripts don't need to reimplement message reading —
they can *delegate* to the built-in reader for the heavy lifting and
handle the chrome, navigation, and presentation themselves.

## The Board Pulse Dashboard

Here's the full script. It scans every message area, tallies the stats,
and presents a summary. Call it `pulse.mex`:

```c
#include <max.mh>
#include <max_menu.mh>

#define MAX_AREAS 50

int main()
{
  struct _marea: ma;
  string: saved_area;
  string: area_name;
  int: total_areas;
  long: total_msgs;
  long: busiest_count;
  string: busiest_name;

  saved_area := marea.name;

  total_areas := 0;
  total_msgs := 0;
  busiest_count := 0;
  busiest_name := "(none)";

  print("\n|11═══════════════════════════════════════\n");
  print("|14  Board Pulse\n");
  print("|11═══════════════════════════════════════|07\n\n");

  print("|03Scanning message areas...\n\n");

  // Iterate every message area
  if (msgareafindfirst(ma, "", AFFO_NODIV))
  {
    do
    {
      // Try to select this area to get msg stats
      if (msgareaselect(ma.name))
      {
        total_areas := total_areas + 1;
        total_msgs := total_msgs + msg.num;

        // Print area line
        if (msg.num > 0)
        {
          print("|14  ", strpad(marea.descript, 28, ' '),
                " |15", strpadleft(ltostr(msg.num), 5, ' '),
                " |03msgs\n");
        }
        else
        {
          print("|08  ", strpad(marea.descript, 28, ' '),
                "     0 msgs\n");
        }

        // Track the busiest area
        if (msg.num > busiest_count)
        {
          busiest_count := msg.num;
          busiest_name := marea.descript;
        }
      }
    }
    while (msgareafindnext(ma));

    msgareafindclose();
  }

  // Restore the caller's original area
  msgareaselect(saved_area);

  // Summary
  print("\n|11═══════════════════════════════════════|07\n");
  print("|03  Areas:    |15", total_areas, "\n");
  print("|03  Messages: |15", total_msgs, "\n");
  print("|03  Busiest:  |15", busiest_name,
        " |03(|15", busiest_count, "|03)\n");
  print("|11═══════════════════════════════════════|07\n\n");

  // Offer to check mail
  print("|14Check your mail? |11[|15Y|11/|15N|11] ");

  if (input_ch(CINPUT_DISPLAY + CINPUT_ACCEPTABLE, "YyNn") = 'Y'
      or input_ch(0, "") = 'y')
  {
    // This won't work right — see explanation below
  }

  print("\n");
  return 0;
}
```

Wait — that mail check at the bottom has a bug. `input_ch()` consumes
the keypress on the first call, so calling it twice reads two different
keys. Here's the correct pattern:

```c
  int: ch;

  print("|14Check your mail? |11[|15Y|11/|15N|11] ");
  ch := input_ch(CINPUT_DISPLAY + CINPUT_ACCEPTABLE, "YyNn");

  if (ch = 'Y' or ch = 'y')
  {
    print("\n\n");
    menu_cmd(MNU_MSG_CHECKMAIL, "");
  }

  print("\n");
```

Store the result in a variable, *then* test it. This is a common mistake
with `input_ch()` — each call reads a new keypress.

### What's New Here

**Area iteration.** The `msgareafindfirst` / `msgareafindnext` /
`msgareafindclose` trio is the standard pattern for scanning areas.
Same pattern exists for file areas (`fileareafindfirst`, etc.).

**`msgareaselect()`** switches the current area, which updates both
`marea` and `msg` globals. Save and restore so the caller doesn't
notice.

**`strpad()` and `strpadleft()`** format strings to a fixed width.
`strpad("Hello", 10, ' ')` right-pads with spaces to 10 characters.
`strpadleft("42", 5, ' ')` left-pads — perfect for right-aligning
numbers in a column.

**`ltostr()`** converts a long to a string. Needed here because
`strpadleft()` takes a string, and `msg.num` is a long.

**`menu_cmd()`** invokes built-in Maximus commands. Include
`max_menu.mh` for the `MNU_*` constants. The second argument is a
string parameter — some commands use it (e.g., area name), others
ignore it (pass `""`).

**The `input_ch()` gotcha.** Each call to `input_ch()` reads one
keypress. If you need to test the result multiple ways, store it in a
variable first. Don't call `input_ch()` twice expecting the same key.

## Area Attributes

The `marea.attribs` field is a bitmask. Test individual flags with `&`:

```c
if (marea.attribs & MA_ECHO)
  print("This is an echomail area.\n");

if (marea.attribs & MA_NET)
  print("This is a netmail area.\n");

if (marea.attribs & MA_READONLY)
  print("This area is read-only.\n");
```

Useful flags:

| Flag | Meaning |
|------|---------|
| `MA_PVT` | Private messages allowed |
| `MA_PUB` | Public messages allowed |
| `MA_NET` | Netmail area |
| `MA_ECHO` | Echomail area |
| `MA_CONF` | Conference area |
| `MA_READONLY` | Read-only area |
| `MA_HIDDN` | Hidden from normal area list |

## The Caller Log

MEX also provides read access to the caller log via `call_open()`,
`call_read()`, `call_numrecs()`, and `call_close()`. Each record is a
`_callinfo` struct with the caller's name, city, login/logoff times,
files transferred, messages read and posted, and session flags.

```c
struct _callinfo: ci;
long: total;
long: i;

call_open();
total := call_numrecs();

// Show the last 5 callers
for (i := total - 5; i < total; i := i + 1)
{
  if (call_read(i, ci))
  {
    print(ci.name, " from ", ci.city,
          " — ", ci.posted, " msgs posted\n");
  }
}

call_close();
```

This is another "dashboard" data source — "who called recently?" is
the kind of information that makes a board feel alive.

## What You Learned

- **`marea` and `msg` globals** — area info and message counts, updated
  when you switch areas.
- **`msgareafindfirst/next/close`** — iterate all message areas on the
  board.
- **`msgareaselect()`** — switch the current area. Save and restore.
- **`menu_cmd()`** — trigger built-in Maximus commands from MEX.
  Include `max_menu.mh` for constants.
- **`strpad()` / `strpadleft()`** — fixed-width formatting for aligned
  columns.
- **Bitwise `&`** — test individual flags in attribute bitmasks.
- **Caller log** — `call_open/read/numrecs/close` for session history.
- **The `input_ch()` rule** — store the result, then test. Don't call
  twice.

## Next

Your board can show its own stats now. But what about the *rest of the
world*? What if your script could reach out over the network, fetch data
from an API, and bring it back to the terminal?

Next lesson: HTTP requests, JSON parsing, and bringing live data to
your board.

**[Lesson 9: "Phone Home" →]({{ site.baseurl }}{% link mex-learn-networking.md %})**
