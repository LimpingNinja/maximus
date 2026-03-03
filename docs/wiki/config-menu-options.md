---
layout: default
title: "Menu Options"
section: "configuration"
description: "Complete reference for every menu command — quick reference table and deep dives by category"
---

This is the complete reference for every command you can assign to a menu
option in Maximus. If you're looking for how menu files are structured and
what each key in an `[[option]]` table means, start with
[Menu Definitions]({% link config-menu-definitions.md %}).

Each command listed here is a value you can put in the `command` field of a
menu option. Command names are case-insensitive — `Display_Menu`,
`display_menu`, and `DISPLAY_MENU` all work.

---

## On This Page

- [Quick Reference Table](#quick-reference)
- [Navigation & Display](#navigation--display) — Display_Menu, Link_Menu,
  Return, Display_File, Cls, Menupath
- [Scripting & Input](#scripting--input) — MEX, Key_Poke, Clear_Stacked,
  Press_Enter, If
- [External Programs](#external-programs) — Xtern_Run, Xtern_Door32,
  Xtern_Dos, Xtern_Chain, Xtern_Concur, Xtern_Erlvl
- [Main Menu Commands](#main-menu-commands) — Goodbye, Yell, Userlist,
  Version, User_Editor, Leave_Comment, Climax
- [Message Commands](#message-commands) — Msg_Area, Msg_Enter, Msg_Reply,
  Msg_Browse, Read_Next, Read_Previous, ***Msg_NG_Read***, and more
- [Email Commands](#email-commands) — Msg_CheckEmail, Msg_Email_Compose,
  Msg_Email_Inbox
- [File Commands](#file-commands) — File_Area, File_Upload, File_Download,
  File_Titles, File_Locate, and more
- [User Settings](#user-settings) — Chg_Password, Chg_Video, Chg_Editor,
  Chg_Help, and more
- [Editor Commands](#editor-commands) — Edit_Save, Edit_Abort, Edit_List,
  Edit_Quote, and more
- [Chat Commands](#chat-commands) — Who_Is_On, Chat_Page, Chat_CB,
  Chat_Toggle, Chat_Pvt

---

## Quick Reference {#quick-reference}

Every available command at a glance. The **Arg?** column indicates whether
the command uses the `arguments` field.

| Command | Arg? | Category | Brief Description |
|---------|------|----------|-------------------|
| `Display_Menu` | ✓ | Navigation | Jump to another menu (flat) |
| `Link_Menu` | ✓ | Navigation | Nest into another menu (up to 8 deep) |
| `Return` | — | Navigation | Pop back from a `Link_Menu` call |
| `Display_File` | ✓ | Navigation | Show a display file (.bbs/.ans) |
| `Cls` | — | Navigation | Clear the screen |
| `Menupath` | ✓ | Navigation | Change the menu search directory |
| `MEX` | ✓ | Scripting | Run a MEX script |
| `Key_Poke` | ✓ | Scripting | Inject keystrokes into the input buffer |
| `Clear_Stacked` | — | Scripting | Flush the keyboard input buffer |
| `Press_Enter` | — | Scripting | Prompt "press enter to continue" |
| `If` | ✓ | Scripting | Conditional execution (internal) |
| `Xtern_Run` | ✓ | External | Run a stdio door (PTY bridging) |
| `Xtern_Door32` | ✓ | External | Run a Door32 door (socket passing) |
| `Xtern_Dos` | ✓ | External | Run a shell command via `system()` |
| `Xtern_Chain` | ✓ | External | Chain execution (replaces process) |
| `Xtern_Concur` | ✓ | External | Run concurrently (non-blocking) |
| `Xtern_Erlvl` | ✓ | External | Errorlevel exit (legacy DOS) |
| `Goodbye` | — | Main | Log off the system |
| `Yell` | — | Main | Yell for the sysop |
| `Userlist` | — | Main | Display the user list |
| `Version` | — | Main | Show Maximus version info |
| `User_Editor` | — | Main | Open the user editor (sysop) |
| `Leave_Comment` | — | Main | Leave a comment for the sysop |
| `Climax` | — | Main | Launch the CLiMax shell (sysop) |
| `Msg_Area` | — | Message | Change message area |
| `Msg_Enter` | — | Message | Compose a new message |
| `Msg_Reply` | — | Message | Reply to the current message |
| `Msg_Reply_Area` | ✓ | Message | Reply in a different area |
| `Msg_Browse` | — | Message | Browse/search/list messages |
| `Msg_List` | — | Message | Brief message listing |
| `Msg_Scan` | — | Message | Scan for new messages |
| `Msg_Inquire` | — | Message | Check for messages to you |
| `Msg_Tag` | — | Message | Tag areas of interest |
| `Msg_Checkmail` | — | Message | Run the mailchecker |
| `Msg_Current` | — | Message | Redisplay the current message |
| `Msg_Change` | — | Message | Modify an existing message |
| `Msg_Kill` | — | Message | Delete a message |
| `Msg_Hurl` | — | Message | Move a message to another area |
| `Msg_Forward` | — | Message | Forward a message to another user |
| `Msg_Upload` | — | Message | Create a message by uploading text |
| `Msg_Upload_QWK` | — | Message | Upload a QWK reply packet |
| `Msg_Xport` | — | Message | Export a message to a text file |
| `Msg_Restrict` | — | Message | Limit QWK downloads by date |
| `Msg_Track` | — | Message | Track message threads |
| `Msg_Download_Attach` | — | Message | Download file attaches |
| `Msg_Kludges` | — | Message | Toggle kludge line display |
| `Msg_Unreceive` | — | Message | Reset the "received" flag |
| `Msg_Edit_User` | — | Message | Edit the sender's user record |
| `Msg_ListTest` | — | Message | Diagnostic listing (internal) |
| `Read_Next` | — | Message | Read the next message |
| `Read_Previous` | — | Message | Read the previous message |
| `Read_Individual` | — | Message | Read a specific message number |
| `Read_Nonstop` | — | Message | Read all messages without pausing |
| `Read_Original` | — | Message | Jump to the thread original |
| `Read_Reply` | — | Message | Jump to the next reply in thread |
| `Same_Direction` | — | Message | Continue reading in same direction |
| `Msg_CheckEmail` | — | Email | Check the EMAIL area for new mail |
| `Msg_Email_Compose` | — | Email | Compose a new email |
| `Msg_Email_Inbox` | — | Email | List all email messages |
| `Msg_NG_Read` | — | Message | ***NEW*** Lightbar message index + FSR reader |
| `Msg_NG_Find` | — | Message | ***NEW*** Multi-area message finder (delegates to NG_Read) |
| `File_Area` | — | File | Change file area |
| `File_Titles` | — | File | List files with descriptions |
| `File_Locate` | — | File | Search for files across areas |
| `File_NewFiles` | — | File | Search for new files |
| `File_Upload` | — | File | Upload a file |
| `File_Download` | — | File | Download a file |
| `File_Tag` | — | File | Tag files for batch download |
| `File_Type` | — | File | View a text file's contents |
| `File_Contents` | — | File | View a compressed file's contents |
| `File_Raw` | — | File | Raw directory listing |
| `File_Kill` | — | File | Delete a file |
| `File_Hurl` | — | File | Move a file to another area |
| `File_Override` | — | File | Temporarily change download path |
| `Chg_City` | — | Settings | Change city |
| `Chg_Password` | — | Settings | Change password |
| `Chg_Phone` | — | Settings | Change phone number |
| `Chg_Help` | — | Settings | Change help level |
| `Chg_Nulls` | — | Settings | Change null character count |
| `Chg_Width` | — | Settings | Change screen width |
| `Chg_Length` | — | Settings | Change screen length |
| `Chg_Tabs` | — | Settings | Toggle tab expansion |
| `Chg_More` | — | Settings | Toggle "More?" prompts |
| `Chg_Video` | — | Settings | Change video mode |
| `Chg_Editor` | — | Settings | Toggle full-screen editor |
| `Chg_Clear` | — | Settings | Toggle screen clearing |
| `Chg_IBM` | — | Settings | Toggle high-bit character support |
| `Chg_RIP` | — | Settings | Toggle RIP graphics |
| `Chg_Hotkeys` | — | Settings | Toggle hotkey mode |
| `Chg_Userlist` | — | Settings | Toggle userlist visibility |
| `Chg_Protocol` | — | Settings | Set default transfer protocol |
| `Chg_FSR` | — | Settings | Toggle full-screen reader |
| `Chg_Archiver` | — | Settings | Set default QWK archiver |
| `Chg_Language` | — | Settings | Change language file |
| `Chg_Realname` | — | Settings | Change alias (legacy name) |
| `Edit_Save` | — | Editor | Save the message |
| `Edit_Abort` | — | Editor | Abort message entry |
| `Edit_List` | — | Editor | List message lines |
| `Edit_Edit` | — | Editor | Edit a single line |
| `Edit_Insert` | — | Editor | Insert a line |
| `Edit_Delete` | — | Editor | Delete a line |
| `Edit_Continue` | — | Editor | Continue/append to message |
| `Edit_To` | — | Editor | Edit the To: field |
| `Edit_From` | — | Editor | Edit the From: field |
| `Edit_Subj` | — | Editor | Edit the Subject: field |
| `Edit_Handling` | — | Editor | Edit message attributes |
| `Edit_Quote` | — | Editor | Quote from the original message |
| `Read_DiskFile` | — | Editor | Import a text file into message |
| `Who_Is_On` | — | Chat | Display online users |
| `Chat_Page` | — | Chat | Page another node for chat |
| `Chat_CB` | — | Chat | Enter CB-style group chat |
| `Chat_Toggle` | — | Chat | Toggle chat availability |
| `Chat_Pvt` | — | Chat | Enter private chat |

---

## Navigation & Display {#navigation--display}

These commands control what the caller sees and how they move between menus.

### `Display_Menu` — Jump to a menu

**Argument:** menu name (e.g., `"MESSAGE"`)

Switches to the named menu. This is a "flat" jump — the target menu doesn't
know how to return to the caller. If you need nested menus with automatic
return, use `Link_Menu` instead.

Use the `NoCls` modifier to prevent the screen from clearing before the new
menu is shown.

### `Link_Menu` — Nest into a menu

**Argument:** menu name

Like `Display_Menu`, but pushes the current menu onto a stack. The target
menu can use `Return` to pop back. Maximus supports up to **8 nested levels**
of `Link_Menu`. If you exceed this, a warning is logged and the call is
silently ignored.

Most boards use `Display_Menu` for the main hub navigation and `Link_Menu`
for sub-sections like `CHAT` where you want a clean "back to main" path.

### `Return` — Pop back from a nested menu

No argument. Returns to the menu that invoked the current one via
`Link_Menu`. If the current menu wasn't reached through `Link_Menu`, this
command does nothing.

### `Display_File` — Show a display file

**Argument:** file path (relative to display directory)

Shows a `.bbs` or `.ans` display file. Supports `%` translation characters in
the path — but note that in filenames, translation characters use `+` instead
of `%` (so `+b` for baud rate, not `%b`).

```toml
command = "Display_File"
arguments = "display/screens/bulletin"
```

### `Cls` — Clear the screen

No argument. Sends a clear-screen sequence to the caller's terminal.

### `Menupath` — Change menu search directory

**Argument:** new directory path

Temporarily changes the directory where Maximus looks for menu files. This is
an advanced feature for boards with multiple menu sets. The path change
persists until the next `Menupath` command or the session ends.

---

## Scripting & Input {#scripting--input}

Commands for running scripts, manipulating input, and controlling flow.

### `MEX` — Run a MEX script

**Argument:** path to MEX script (without `.vm` extension)

Executes a compiled MEX virtual machine program. MEX scripts can do nearly
anything — display dynamic content, interact with the user, modify user data,
query the message base. This is the primary extensibility mechanism.

```toml
command = "MEX"
arguments = "scripts/stats"
```

### `Key_Poke` — Inject keystrokes

**Argument:** keystrokes to inject

Inserts the specified characters into the caller's input buffer, as if they
typed them. Underscores are converted to spaces. Supports `%` translation
characters. Useful for pre-filling multi-step command sequences.

```toml
# Pre-fill Browse → All → Your → List
command = "Key_Poke"
arguments = "bayl"
```

You can also achieve the same effect by setting the `key_poke` field on any
other option — that field injects keystrokes before the command runs.

### `Clear_Stacked` — Flush the input buffer

No argument. Empties any queued keystrokes from the caller's input buffer.
Typically linked (via `NoDsp`) with another option to ensure a clean slate
before a command that prompts for input.

### `Press_Enter` — Pause prompt

No argument. Resets the text color to white and displays a "press enter to
continue" prompt. Typically used with `NoDsp` to create a pause between two
linked options.

### `If` — Conditional execution

**Argument:** condition expression

An internal flow-control mechanism. Used with the `Then` and `Else` modifiers
on subsequent options to create conditional menu logic. The `If` command
evaluates a condition; the next option marked `Then` runs only if the
condition was true, and the next option marked `Else` runs only if it was
false.

---

## External Programs {#external-programs}

Commands for launching door games, utilities, and other external programs.
For detailed setup guides, see [Door Games]({% link config-door-games.md %}).

### `Xtern_Run` — Run a stdio door

**Argument:** command line (supports `%` substitutions)

The workhorse door launcher. Forks a child process, sets up a PTY
(pseudo-terminal), redirects the child's stdio to the PTY slave, and runs a
relay loop between the caller's socket and the PTY master. The door program
reads stdin and writes stdout as if talking to a terminal.

See [Door Games — stdio Doors]({% link config-door-games.md %}#stdio-doors)
for a full setup walkthrough.

### `Xtern_Door32` — Run a Door32 door

**Argument:** command line (supports `%` substitutions)

**Aliases:** `Door32_Run`

Launches a door that speaks the Door32 protocol. Instead of PTY bridging,
Maximus passes the caller's TCP socket file descriptor directly to the door
process. The door reads the fd number from `door32.sys` and does its own I/O.
This is faster and cleaner than stdio bridging — no PTY overhead.

See [Door32 Support]({% link door32-support.md %}) for protocol details.

### `Xtern_Dos` — Run a shell command

**Argument:** command line

**Aliases:** `Xtern_OS2`, `Xtern_Shell`

Executes a command via `system()` — a plain fork+exec with no PTY or socket
bridging. The caller sees nothing from the command's output. This is for
server-side housekeeping tasks (running maintenance scripts, etc.), not for
interactive doors.

### `Xtern_Chain` — Chain execution

**Argument:** command line

Replaces the current Maximus process with the specified program using
`exec()`. There's no return — when the chained program exits, the session
ends. This is a system-level handoff mechanism, rarely used in normal
operation.

### `Xtern_Concur` — Run concurrently

**Argument:** command line

Forks and executes the command in the background, returning to the menu
immediately without waiting for the child to exit. Useful for kicking off
background tasks.

### `Xtern_Erlvl` — Errorlevel exit

**Argument:** command line

A legacy DOS mechanism that exits Maximus with an errorlevel, expecting a
batch file wrapper to run the door and restart Maximus. On UNIX, this is
converted to `Xtern_Run` behavior internally. You should use `Xtern_Run`
directly.

---

## Main Menu Commands {#main-menu-commands}

General-purpose commands typically found on the main menu.

### `Goodbye` — Log off

No argument. Ends the caller's session. Maximus runs the logout sequence
(displaying goodbye screens, updating user record, etc.) and drops the
connection.

### `Yell` — Yell for sysop

No argument. Plays an audible alert (if configured) to get the sysop's
attention. On a multi-node system, the yell is local to the node.

### `Userlist` — Display user list

No argument. Shows a paginated list of all users on the system. Users who
have disabled "In UserList" in their profile are hidden. Users in access
classes with the `Hide` flag are always hidden.

### `Version` — Show version info

No argument. Displays the Maximus version number, build info, and credits.
Prompts the caller to press enter afterward.

### `User_Editor` — Open the user editor

No argument. Launches the built-in user editor, allowing the sysop to browse
and modify user records. **Restrict to sysop-level access.**

### `Leave_Comment` — Comment to sysop

No argument. Opens the message editor addressed to the sysop, placing the
message in the area defined by the `comment_area` setting in your
configuration.

### `Climax` — CLiMax shell

No argument. Launches the CLiMax command-line interface, which provides a
Unix-shell-like environment for sysop tasks. Only available if Maximus was
compiled with `CLIMAX` support. **Restrict to sysop-level access.**

---

## Message Commands {#message-commands}

Commands for reading, writing, and managing messages. These are typically
found on the `MESSAGE` and `MSGREAD` menus.

### `Msg_Area` — Change message area

No argument. Prompts the caller to select a new message area from the area
list. The area list respects division hierarchy and ACS restrictions.

### `Msg_Enter` — Compose a new message

No argument. Opens the message editor (line editor or full-screen MaxEd,
depending on the caller's preference) to create a new message in the current
area. The caller is prompted for To:, Subject:, and message attributes before
entering the editor.

### `Msg_Reply` — Reply to current message

No argument. Opens the message editor pre-filled with reply headers (To: set
to the original sender, Subject: prefixed with "Re:"). The reply is placed in
the current area. Only available when a message is currently displayed.

### `Msg_Reply_Area` — Reply in a different area

**Argument:** area name, or `"."` to prompt

Like `Msg_Reply`, but places the reply in a different area. If the argument
is `"."`, the caller is prompted to choose an area. If an explicit area name
is given, the reply goes there directly.

### `Msg_Browse` — Browse messages

No argument. The Swiss Army knife of message reading. Opens the Browse
interface, which lets the caller select areas (current, tagged, all), message
types (all, personal, text search), and output mode (read, list, or QWK
pack). This is the command behind "read new mail" workflows.

### `Msg_List` — Brief message listing

No argument. Displays a compact one-line-per-message listing of the current
area, showing message number, from, to, subject, and date.

### `Msg_Scan` — Scan for new messages

No argument. Quickly scans the current area (or all areas) for new messages
since the caller's last visit.

### `Msg_Inquire` — Check for messages to you

No argument. Searches the current area for messages addressed to the caller.

### `Msg_Tag` — Tag areas of interest

No argument. Lets the caller select specific message areas to tag. Tagged
areas can be recalled later in `Msg_Browse` to read only those areas.

### `Msg_Checkmail` — Run the mailchecker

No argument. Scans all accessible areas for unread messages addressed to the
caller. Typically triggered automatically at login.

### `Msg_Current` — Redisplay current message

No argument. Re-displays the message that was last shown. Useful after
toggling kludge display or coming back from another command.

### `Msg_Change` — Modify an existing message

No argument. Allows the caller to edit a message they authored, provided it
hasn't been read by the addressee, scanned out as EchoMail, or sent as
NetMail. Users with the `MsgAttrAny` mail flag can modify any message.

### `Msg_Kill` — Delete a message

No argument. Deletes the current message. The message must be To: or From:
the caller. Users with the `ShowPvt` mail flag can delete any message.

### `Msg_Hurl` — Move a message

No argument. Moves a message from the current area to another area. **Sysop
only.**

### `Msg_Forward` — Forward a message

No argument. Sends a copy of the current message to another user.

### `Msg_Upload` — Create message by upload

No argument. Like `Msg_Enter`, but instead of using an editor, the caller
uploads an ASCII text file as the message body. Useful for posting prepared
text.

### `Msg_Upload_QWK` — Upload QWK reply packet

No argument. Accepts an uploaded `.rep` file from a QWK off-line reader and
imports the replies into the appropriate message areas.

### `Msg_Xport` — Export to text file

No argument. Exports the current message to an ASCII text file at a
caller-specified path. **Sysop only** — allows arbitrary file writes. To
print a message, specify `"prn"` as the filename.

### `Msg_Restrict` — QWK date restriction

No argument. Lets the caller set a cutoff date for QWK downloads — messages
older than the date are excluded. The restriction applies to the current
session only.

### `Msg_Track` — Track message threads

No argument. Follows the reply chain of the current message, showing the
thread history.

### `Msg_Download_Attach` — Download file attaches

No argument. Lists unreceived file attaches addressed to the caller and
prompts to download each one.

### `Msg_Kludges` — Toggle kludge display

No argument. Toggles display of FTN kludge lines (the `^A` control lines at
the top of messages that contain routing and control information). **Sysop
only.**

### `Msg_Unreceive` — Reset "received" flag

No argument. Clears the "received" attribute on the current message,
regardless of who sent it. **Sysop only.**

### `Msg_Edit_User` — Edit the sender's user record

No argument. Opens the user editor with an automatic search for the user
listed in the From: field of the current message. **Sysop only.**

### `Msg_ListTest` — Diagnostic listing

No argument. An internal diagnostic command that produces a verbose message
listing. Not intended for caller-facing menus.

### `Read_Next` — Read next message

No argument. Displays the next message in the current area, advancing the
lastread pointer.

### `Read_Previous` — Read previous message

No argument. Displays the previous message in the current area.

### `Read_Individual` — Read by number

No argument (the caller is prompted for the message number). Displays a
specific message by its number. Typically bound to digit keys (`0`–`9`) with
`NoDsp` so callers can type a number directly.

### `Read_Nonstop` — Read all without pausing

No argument. Displays all remaining messages in the area without pausing
between them. Direction follows the last `Read_Next` or `Read_Previous`.

### `Read_Original` — Jump to thread original

No argument. Follows the reply chain backward to display the original message
that started the current thread.

### `Read_Reply` — Jump to next reply

No argument. Follows the reply chain forward to display the next reply in the
thread.

### `Same_Direction` — Continue reading

No argument. Reads the next message in the same direction as the last
`Read_Next` or `Read_Previous` command. Typically bound to the `|` key with
`NoDsp`.

### `Msg_NG_Read` — ***NEW*** Lightbar message index + reader

No argument. The modern message reading experience. `Msg_NG_Read` replaces
the traditional Browse → Read flow with a Storm/Mystic-inspired workflow:

1. **Filter prompt** — the caller chooses how to slice the current area:
   **(F)orward**, **(N)ew**, **(B)y You**, **(Y)ours**, **(S)earch**, or
   **(#) From message number**.
2. **Lightbar message index** — a scrollable, pageable list of matching
   messages. Arrow keys navigate, Enter opens the full-screen reader (FSR),
   Esc returns to the menu. On TTY terminals, a plain columnar list with a
   "Read which #?" prompt is shown instead.
3. **Full-screen reader (FSR)** — the message body is displayed in a
   scrollable view with a hotkey bar. Left/Right moves between messages in
   the index, `[`/`]` follows thread links, and all the standard message
   commands (Reply, Kill, Forward, Export) are available via single keys.
4. **Area cycling** — pressing **G** (or reaching the end of an area) jumps
   to the next accessible message area without returning to the menu.

This is the recommended way to wire up message reading on new boards. Bind
it to `R` on your message menu. The legacy `Read_Next` / `Read_Previous` /
`Msg_Browse` commands continue to work alongside it.

```toml
[[option]]
command = "Msg_NG_Read"
arguments = ""
priv_level = "Demoted"
description = "%Read (NG index)"
```

### `Msg_NG_Find` — ***NEW*** Multi-area message finder

No argument. The multi-area counterpart to `Msg_NG_Read`. Currently
delegates to `Msg_NG_Read` for single-area operation — the multi-area scope
prompt (Current / Group / All) is planned for a future phase. Wire it up
now so your menus are ready when the scope prompt lands.

```toml
[[option]]
command = "Msg_NG_Find"
arguments = ""
priv_level = "Demoted"
description = "&Find (NG search)"
```

---

## Email Commands {#email-commands}

These commands operate on the dedicated **EMAIL area** — a private mailbox
separate from the normal message area hierarchy. The EMAIL area is configured
in your system settings; if none is configured, these commands silently do
nothing.

### `Msg_CheckEmail` — Check for new email

No argument. Scans the EMAIL area for unread messages addressed to the
caller (by name or alias) and presents them for reading. This is the primary
email experience, typically called at login before the general
`Msg_Checkmail` scan.

### `Msg_Email_Compose` — Compose a new email

No argument. Opens the message editor in the EMAIL area. Messages in the
EMAIL area are always private (the Email style implies `MA_PVT`).

### `Msg_Email_Inbox` — List all email

No argument. Shows all messages in the EMAIL area addressed to the caller —
both read and unread — in list format. Unlike `Msg_CheckEmail` (which only
shows unread), this shows everything.

---

## File Commands {#file-commands}

Commands for browsing, uploading, downloading, and managing files. Typically
found on the `FILE` menu.

### `File_Area` — Change file area

No argument. Prompts the caller to select a new file area.

### `File_Titles` — List files with descriptions

No argument. Displays all files in the current area with their name,
timestamp, size, and description (from `files.bbs`).

### `File_Locate` — Search for files

No argument. Searches all file areas for files matching a filename pattern or
description keyword. Also supports new-file searches.

### `File_NewFiles` — New file search

No argument. Searches all areas for files added since the caller's last
visit. Equivalent to the `[newfiles]` MECCA token.

### `File_Upload` — Upload a file

No argument. Receives a file from the caller using the selected transfer
protocol. Maximus checks the uploaded filename against `badfiles.bbs` — if it
matches, the upload is rejected and the file is deleted.

### `File_Download` — Download a file

No argument. Sends a file to the caller. Works with tagged files (from
`File_Tag`) or prompts for a filename.

### `File_Tag` — Tag files for download

No argument. Lets the caller mark files for batch download. Tagged files are
downloaded later using `File_Download`.

### `File_Type` — View a text file

No argument. Displays the contents of an ASCII text file in the current file
area. Also available as `File_View`.

### `File_Contents` — View archive contents

No argument. Lists the contents of a compressed file (ARC, ARJ, PAK, ZIP,
LZH).

### `File_Raw` — Raw directory listing

No argument. Shows a raw directory listing of the current file area,
including all files — not just those in `files.bbs`.

### `File_Kill` — Delete a file

No argument. Removes a file from the current area. Typically sysop-only.

### `File_Hurl` — Move a file

No argument. Moves a file from the current area to another file area.

### `File_Override` — Override download path

No argument. Temporarily changes the download path for the current area,
letting the sysop access any directory as if it were a file area. See also
`File_Raw`.

---

## User Settings {#user-settings}

Commands that let callers change their profile and preferences. Typically
found on the `CHANGE` menu.

### `Chg_City` — Change city

Prompts for a new city/location string.

### `Chg_Password` — Change password

Prompts for the current password, then a new one (twice for confirmation).

### `Chg_Phone` — Change phone number

Prompts for a new phone number.

### `Chg_Help` — Change help level

Cycles through Novice, Regular, and Expert help levels. This controls how
much menu chrome the caller sees.

### `Chg_Nulls` — Change null count

Sets the number of NUL characters sent after every line. A legacy serial
communication setting — modems sometimes needed padding. Usually `0` for
telnet callers.

### `Chg_Width` — Change screen width

Prompts for a new screen width in columns.

### `Chg_Length` — Change screen length

Prompts for a new screen length in rows.

### `Chg_Tabs` — Toggle tab expansion

Toggles whether Maximus sends literal tab characters or expands them to
spaces.

### `Chg_More` — Toggle "More?" prompts

Toggles the `[More Y,n,=]?` pause prompt that appears after each screenful
of output.

### `Chg_Video` — Change video mode

Lets the caller select between TTY, ANSI, and AVATAR video modes.

### `Chg_Editor` — Toggle full-screen editor

Toggles between the line editor and the full-screen MaxEd editor for message
composition.

### `Chg_Clear` — Toggle screen clearing

Toggles whether Maximus sends clear-screen sequences between menus.

### `Chg_IBM` — Toggle high-bit characters

Controls whether Maximus sends IBM extended ASCII characters (codes 128–255)
directly or translates them to 7-bit equivalents.

### `Chg_RIP` — Toggle RIP graphics

Toggles RIP (Remote Imaging Protocol) graphics mode. Enabling RIP also forces
ANSI graphics and screen clearing on.

### `Chg_Hotkeys` — Toggle hotkeys

Toggles hotkey mode. With hotkeys on, menu commands execute immediately on
keypress without waiting for Enter.

### `Chg_Userlist` — Toggle userlist visibility

Toggles whether the caller appears in the public user list. Even when
enabled, users with the `Hide` access flag are never shown.

### `Chg_Protocol` — Set default protocol

Lets the caller pick a default file transfer protocol, suppressing the
protocol selection prompt on future uploads/downloads.

### `Chg_FSR` — Toggle full-screen reader

Toggles the full-screen message reader. When enabled, ANSI/AVATAR users see
a formatted header with cursor-key navigation.

### `Chg_Archiver` — Set default archiver

Lets the caller pick a default compression method for QWK packets,
suppressing the archiver selection prompt.

### `Chg_Language` — Change language

Lets the caller select a different language file from those configured in the
language control file.

### `Chg_Realname` — Change alias

**Alias:** `Chg_Alias`

Prompts for a new alias. The new value must not conflict with any other
user's name or alias.

---

## Editor Commands {#editor-commands}

Commands used within the **line editor** and **editor menus** during message
composition. These are only valid on the `EDIT` menu — placing them on a
non-editor menu results in a "you can't get there from here" message.

### `Edit_Save` — Save the message

Saves the composed message to the message base and exits the editor.

### `Edit_Abort` — Abort message entry

Discards the message and exits the editor.

### `Edit_List` — List message lines

Displays the lines of the message being composed, with line numbers.

### `Edit_Edit` — Edit a line

Prompts for a line number, then lets the caller modify that line.

### `Edit_Insert` — Insert a line

Prompts for a position and inserts a new line before it.

### `Edit_Delete` — Delete a line

Prompts for a line number and removes it.

### `Edit_Continue` — Continue composing

In the line editor: appends more text to the message. In MaxEd: returns to
the full-screen editor display.

### `Edit_To` — Edit the To: field

Lets the caller change the message's addressee.

### `Edit_From` — Edit the From: field

Lets the caller change the sender name. Only meaningful in areas that allow
aliases or anonymous messages.

### `Edit_Subj` — Edit the Subject: field

Lets the caller change the subject line.

### `Edit_Handling` — Edit message attributes

Lets the caller modify message attributes (private, crash, hold, etc.).
**Sysop only.**

### `Edit_Quote` — Quote from original

Copies text from the message being replied to and inserts it into the current
message. Only available when replying.

### `Read_DiskFile` — Import a text file

Imports an ASCII text file from the local disk into the current message.
The caller specifies a full path, so this is **sysop only**.

---

## Chat Commands {#chat-commands}

Commands for the multinode chat system. Typically found on the `CHAT` menu.

### `Who_Is_On` — Display online users

No argument. Shows a table of all currently connected users across all nodes,
including their node number, user name, status, and current activity.

### `Chat_Page` — Page another node

No argument. Prompts for a node number and sends a chat request to that user.
The caller is placed into chat mode while waiting for a response.

### `Chat_CB` — CB-style group chat

No argument. Enters the CB simulator — a group chat where all participants
on the same "channel" see each other's messages in real time. Multiple
channels are supported.

### `Chat_Toggle` — Toggle chat availability

No argument. Toggles the caller's chat availability flag. When unavailable,
the caller won't receive page notifications from other nodes.

### `Chat_Pvt` — Private chat

No argument. Enters private chat mode for one-on-one conversation with
another node.

---

## Aliases

Some commands have aliases — alternate names that resolve to the same
function:

| Alias | Resolves To |
|-------|------------|
| `Door32_Run` | `Xtern_Door32` |
| `Xtern_OS2` | `Xtern_Dos` |
| `Xtern_Shell` | `Xtern_Dos` |
| `File_View` | `File_Type` |
| `Chg_Alias` | `Chg_Realname` |

---

## See Also

- [Menu Definitions]({% link config-menu-definitions.md %}) — how menu TOML
  files are structured
- [Menu System]({% link config-menu-system.md %}) — the parent overview
- [Door Games]({% link config-door-games.md %}) — door setup guides
- [Security & Access]({% link config-security-access.md %}) — privilege
  levels and flags for gating menu options
