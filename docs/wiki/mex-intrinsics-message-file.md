---
layout: default
title: "Message & File Areas"
section: "mex"
description: "Message area navigation, file area search, download queues, and the menu_cmd() dispatch for invoking any Maximus command from MEX"
---

Message bases and file areas are the backbone of a BBS. Your callers read
and post messages, browse file listings, download files, and navigate between
areas. MEX gives you functions to search and select areas, inspect their
attributes, manage the download queue, and — the big one — invoke any
built-in Maximus menu command programmatically with `menu_cmd()`.

The global variables `marea` (current message area) and `farea` (current
file area) are always available after `#include <max.mh>`. The area search
functions let you iterate over all defined areas. And `menu_cmd()` with the
constants from `max_menu.mh` lets your script trigger things like "enter a
message," "download tagged files," or "change to file area" without
reimplementing any of it.

---

## On This Page

- [Quick Reference](#quick-reference)
- [Message Area Globals](#msg-globals)
- [Message Area Search](#msg-search)
- [File Area Globals](#file-globals)
- [File Area Search](#file-search)
- [Download Queue](#dl-queue)
- [menu_cmd() — Command Dispatch](#menu-cmd)
- [Menu Command Constants](#menu-constants)

---

## Quick Reference {#quick-reference}

### Message Areas

| Function | What It Does |
|----------|-------------|
| [`msgareafindfirst()`](#msgareafindfirst) | Start iterating message areas |
| [`msgareafindnext()`](#msgareafindnext) | Move to the next message area |
| [`msgareafindprev()`](#msgareafindprev) | Move to the previous message area |
| [`msgareafindclose()`](#msgareafindclose) | Close the message area search |
| [`msgareaselect()`](#msgareaselect) | Switch to a message area by tag |
| [`msg_area()`](#msg-area) | Display the message area change prompt |

### File Areas

| Function | What It Does |
|----------|-------------|
| [`fileareafindfirst()`](#fileareafindfirst) | Start iterating file areas |
| [`fileareafindnext()`](#fileareafindnext) | Move to the next file area |
| [`fileareafindprev()`](#fileareafindprev) | Move to the previous file area |
| [`fileareafindclose()`](#fileareafindclose) | Close the file area search |
| [`fileareaselect()`](#fileareaselect) | Switch to a file area by tag |
| [`file_area()`](#file-area) | Display the file area change prompt |

### Download Queue

| Function | What It Does |
|----------|-------------|
| [`tag_queue_file()`](#tag-queue) | Add a file to the download queue |
| [`tag_dequeue_file()`](#tag-dequeue) | Remove a file from the download queue |
| [`tag_queue_size()`](#tag-size) | Get number of files in the queue |
| [`tag_get_name()`](#tag-get) | Get filename and flags for a queued entry |

### Command Dispatch

| Function | What It Does |
|----------|-------------|
| [`menu_cmd()`](#menu-cmd) | Execute any Maximus menu command |
| [`display_file()`](#display-file-ref) | Display an ANSI/ASCII file (see [Display & I/O]({% link mex-intrinsics-display-io.md %})) |

---

## Message Area Globals {#msg-globals}

The `marea` global (`struct _marea`) describes the current message area:

| Field | Type | Description |
|-------|------|-------------|
| `marea.name` | `string` | Display name of the area |
| `marea.acs` | `string` | Access control string |
| `marea.path` | `string` | Path to the message base |
| `marea.tag` | `string` | Short tag/identifier |
| `marea.type` | `int` | Area type flags |
| `marea.attribs` | `int` | Area attribute flags (`MA_*` constants) |
| `marea.division` | `int` | Division number |

The `msg` global (`struct _msg`) describes the current message context:

| Field | Type | Description |
|-------|------|-------------|
| `msg.msgnum` | `long` | Current message number |
| `msg.high` | `long` | Highest message number in area |
| `msg.numrecs` | `long` | Total number of messages |
| `msg.direction` | `int` | Read direction (forward/backward) |

### Message Area Attribute Flags

| Constant | Meaning |
|----------|---------|
| `MA_PVT` | Area allows private messages |
| `MA_PUB` | Area allows public messages |
| `MA_READONLY` | Read-only area |
| `MA_NETMAIL` | NetMail area |
| `MA_ECHO` | EchoMail area |
| `MA_CONF` | Conference area |
| `MA_ALIAS` | Aliases allowed |
| `MA_REALNAME` | Real names required |

---

## Message Area Search {#msg-search}

{: #msgareafindfirst}
### msgareafindfirst(ma, name, flags)

Start searching message areas. Pass an empty string for `name` to match all
areas. Returns `0` on success:

```mex
int msgareafindfirst(ref struct _marea: ma, string: name, int: flags);
```

| Flag | Value | Meaning |
|------|-------|---------|
| `AFFO_NODIV` | 0 | All areas, regardless of division |
| `AFFO_DIV` | 1 | Only areas in the current division |

{: #msgareafindnext}
### msgareafindnext(ma)

Advance to the next message area:

```mex
int msgareafindnext(ref struct _marea: ma);
```

{: #msgareafindprev}
### msgareafindprev(ma)

Move to the previous message area:

```mex
int msgareafindprev(ref struct _marea: ma);
```

{: #msgareafindclose}
### msgareafindclose()

Close the message area search. Always call this when done:

```mex
void msgareafindclose();
```

{: #msgareaselect}
### msgareaselect(name)

Switch the current message area by tag. Returns `0` on success:

```mex
int msgareaselect(string: name);
```

```mex
if (msgareaselect("GENERAL") = 0)
  print("Switched to: " + marea.name + "\n");
```

{: #msg-area}
### msg_area()

Display the interactive message area selection prompt (same as the built-in
`Msg_Area` menu command):

```mex
void msg_area();
```

### List All Message Areas Example

```mex
struct _marea: ma;
char: nonstop;

nonstop := False;
reset_more(nonstop);

if (msgareafindfirst(ma, "", AFFO_NODIV) = 0)
{
  do
  {
    print(strpad(ma.tag, 15, ' ') + " " + ma.name + "\n");
    if (do_more(nonstop, COL_CYAN) = 0)
      goto done;
  }
  while (msgareafindnext(ma) = 0);
}

done:
msgareafindclose();
```

---

## File Area Globals {#file-globals}

The `farea` global (`struct _farea`) describes the current file area:

| Field | Type | Description |
|-------|------|-------------|
| `farea.name` | `string` | Display name of the area |
| `farea.acs` | `string` | Access control string |
| `farea.downpath` | `string` | Download path |
| `farea.uppath` | `string` | Upload path |
| `farea.filesbbs` | `string` | Path to FILES.BBS listing |
| `farea.tag` | `string` | Short tag/identifier |
| `farea.division` | `int` | Division number |
| `farea.attribs` | `int` | Area attribute flags |

---

## File Area Search {#file-search}

The file area search functions mirror the message area versions exactly:

{: #fileareafindfirst}
### fileareafindfirst(fa, name, flags)

```mex
int fileareafindfirst(ref struct _farea: fa, string: name, int: flags);
```

{: #fileareafindnext}
### fileareafindnext(fa)

```mex
int fileareafindnext(ref struct _farea: fa);
```

{: #fileareafindprev}
### fileareafindprev(fa)

```mex
int fileareafindprev(ref struct _farea: fa);
```

{: #fileareafindclose}
### fileareafindclose()

```mex
void fileareafindclose();
```

{: #fileareaselect}
### fileareaselect(name)

```mex
int fileareaselect(string: name);
```

{: #file-area}
### file_area()

Display the interactive file area selection prompt:

```mex
void file_area();
```

---

## Download Queue {#dl-queue}

The download queue (tag list) lets callers mark files for batch download.
Your MEX script can add, remove, inspect, and count queued files.

{: #tag-queue}
### tag_queue_file(filename, flags)

Add a file to the download queue:

```mex
int tag_queue_file(string: filename, int: flags);
```

{: #tag-dequeue}
### tag_dequeue_file(posn)

Remove a file from the download queue by position (0-based):

```mex
int tag_dequeue_file(int: posn);
```

{: #tag-size}
### tag_queue_size()

Returns the number of files currently in the download queue:

```mex
int tag_queue_size();
```

{: #tag-get}
### tag_get_name(posn, flags, filename)

Get the filename and flags for a queued entry:

```mex
int tag_get_name(int: posn, ref int: flags, ref string: filename);
```

```mex
int: i, count, flags;
string: fname;

count := tag_queue_size();
print("Download queue (" + itostr(count) + " files):\n");
for (i := 0; i < count; i := i + 1)
{
  tag_get_name(i, flags, fname);
  print("  " + fname + "\n");
}
```

---

## menu_cmd() — Command Dispatch {#menu-cmd}

This is the power tool. `menu_cmd()` executes any built-in Maximus menu
command from your MEX script:

```mex
void menu_cmd(int: cmdnum, string: args);
```

The `cmdnum` is one of the `MNU_*` constants from `max_menu.mh`. The `args`
string provides any arguments the command expects (pass `""` if none).

```mex
#include <max.mh>
#include <max_menu.mh>

void main()
{
  // Enter a new message
  menu_cmd(MNU_MSG_ENTER, "");

  // Display a file to the caller
  menu_cmd(MNU_DISPLAY_FILE, "bulletin.ans");

  // Run the goodbye sequence
  menu_cmd(MNU_GOODBYE, "");
}
```

{: #display-file-ref}
> **Note:** For displaying files, you can also use `display_file()` directly
> — see [Display & I/O]({% link mex-intrinsics-display-io.md %}#display-file).

---

## Menu Command Constants {#menu-constants}

These constants are defined in `max_menu.mh`. Include it when you use
`menu_cmd()`.

### General

| Constant | Value | Command |
|----------|-------|---------|
| `MNU_DISPLAY_FILE` | 102 | Display a file |
| `MNU_PRESS_ENTER` | 106 | Press Enter prompt |
| `MNU_KEY_POKE` | 107 | Stuff keys into input buffer |
| `MNU_CLEAR_STACKED` | 108 | Clear stacked input |
| `MNU_MENUPATH` | 110 | Change menu path |
| `MNU_CLS` | 111 | Clear screen |

### External Programs

| Constant | Value | Command |
|----------|-------|---------|
| `MNU_XTERN_ERLVL` | 201 | Run external program (errorlevel) |
| `MNU_XTERN_DOS` | 202 | Run external program (DOS shell) |
| `MNU_XTERN_RUN` | 203 | Run external program (direct) |

### System

| Constant | Value | Command |
|----------|-------|---------|
| `MNU_GOODBYE` | 301 | Log off the caller |
| `MNU_YELL` | 303 | Page sysop |
| `MNU_USERLIST` | 304 | Display user list |
| `MNU_VERSION` | 305 | Display version information |
| `MNU_USER_EDITOR` | 306 | User profile editor |
| `MNU_LEAVE_COMMENT` | 307 | Leave a comment for sysop |

### Messages

| Constant | Value | Command |
|----------|-------|---------|
| `MNU_SAME_DIRECTION` | 401 | Continue reading in same direction |
| `MNU_READ_NEXT` | 402 | Read next message |
| `MNU_READ_PREVIOUS` | 403 | Read previous message |
| `MNU_MSG_ENTER` | 404 | Enter a new message |
| `MNU_MSG_REPLY` | 405 | Reply to current message |
| `MNU_READ_NONSTOP` | 406 | Read nonstop |
| `MNU_READ_ORIGINAL` | 407 | Read original message |
| `MNU_READ_REPLY` | 408 | Read reply to current |
| `MNU_MSG_KILL` | 412 | Kill a message |
| `MNU_MSG_HURL` | 413 | Hurl a message to another area |
| `MNU_MSG_FORWARD` | 414 | Forward a message |
| `MNU_MSG_UPLOAD` | 415 | Upload a message |
| `MNU_MSG_XPORT` | 416 | Export a message |
| `MNU_READ_INDIVIDUAL` | 417 | Read a specific message by number |
| `MNU_MSG_CHECKMAIL` | 418 | Check for new personal mail |
| `MNU_MSG_CHANGE` | 419 | Change message attributes |
| `MNU_MSG_TAG` | 420 | Tag a message |
| `MNU_MSG_BROWSE` | 421 | Browse message headers |
| `MNU_MSG_CURRENT` | 422 | Display current message info |
| `MNU_MSG_AREA` | 428 | Change message area |
| `MNU_MSG_TRACK` | 429 | Track message replies |
| `MNU_MSG_DLOAD_ATTACH` | 430 | Download attached file |

### Files

| Constant | Value | Command |
|----------|-------|---------|
| `MNU_FILE_LOCATE` | 501 | Locate a file |
| `MNU_FILE_TITLES` | 502 | Display file titles |
| `MNU_FILE_VIEW` | 503 | View a file |
| `MNU_FILE_UPLOAD` | 504 | Upload a file |
| `MNU_FILE_DOWNLOAD` | 505 | Download a file |
| `MNU_FILE_RAW` | 506 | Raw file directory |
| `MNU_FILE_KILL` | 507 | Kill a file |
| `MNU_FILE_CONTENTS` | 508 | View archive contents |
| `MNU_FILE_HURL` | 509 | Hurl a file to another area |
| `MNU_FILE_OVERRIDE` | 510 | Override file path |
| `MNU_FILE_NEWFILES` | 511 | New files search |
| `MNU_FILE_TAG` | 512 | Tag a file for download |
| `MNU_FILE_AREA` | 513 | Change file area |

### User Settings (Change)

| Constant | Value | Command |
|----------|-------|---------|
| `MNU_CHG_CITY` | 601 | Change city |
| `MNU_CHG_PASSWORD` | 602 | Change password |
| `MNU_CHG_HELP` | 603 | Change help level |
| `MNU_CHG_WIDTH` | 605 | Change screen width |
| `MNU_CHG_LENGTH` | 606 | Change screen length |
| `MNU_CHG_MORE` | 608 | Toggle "More?" prompts |
| `MNU_CHG_VIDEO` | 609 | Change terminal type |
| `MNU_CHG_HOTKEYS` | 615 | Toggle hotkeys |
| `MNU_CHG_LANGUAGE` | 616 | Change language |
| `MNU_CHG_PROTOCOL` | 618 | Change default protocol |
| `MNU_CHG_ARCHIVER` | 620 | Change default archiver |

### Chat

| Constant | Value | Command |
|----------|-------|---------|
| `MNU_CHAT_WHO_IS_ON` | 801 | Who is online |
| `MNU_CHAT_PAGE` | 802 | Page another node |
| `MNU_CHAT_CB` | 803 | CB chat mode |
| `MNU_CHAT_TOGGLE` | 804 | Toggle chat availability |
| `MNU_CHAT_PVT` | 805 | Private chat |

---

## See Also

- [Standard Intrinsics]({% link mex-standard-intrinsics.md %}) — overview of
  all intrinsic categories
- [Display & I/O]({% link mex-intrinsics-display-io.md %}) — output, input,
  and file functions
- [User & Session]({% link mex-intrinsics-user-session.md %}) — user record
  access and session management
- [Variables & Types]({% link mex-language-vars-types.md %}#global-data) —
  the `marea`, `farea`, `msg` global structs
