---
layout: default
title: "MECCA Language"
section: "display"
description: "Complete reference for the MECCA display file language — colors, cursor control, conditionals, flow control, questionnaires, and every token Maximus understands"
permalink: /mecca-language/
---

If [pipe codes]({{ site.baseurl }}{% link display-codes.md %}) are the quick way to drop a
username or splash of color into a prompt, MECCA is the power tool. It's the
language behind every compiled display file on your board — the welcome
screen, the menu art, the new-user questionnaire, the sysop-only status
panel. A `.mec` source file is plain text with special tokens in square
brackets. The [MECCA compiler]({{ site.baseurl }}{% link mecca-compiler.md %}) turns it into a
`.bbs` binary that Maximus loads at runtime.

MECCA gives you color, cursor positioning, conditional display based on
privilege level or terminal type, flow control with labels and gotos, keyboard
input and questionnaire logging, and the ability to embed live system data —
the caller's name, time remaining, message counts, node status — directly
into any screen. One set of source files serves every terminal type: Maximus
automatically strips color and cursor codes for TTY callers, so you never
have to maintain separate ASCII and ANSI versions of the same screen.

This page is the complete language reference. If you're looking for how to
invoke the compiler itself, see
[MECCA Compiler]({{ site.baseurl }}{% link mecca-compiler.md %}). If you want pipe codes for
inline use in prompts and language strings, see
[Display Codes]({{ site.baseurl }}{% link display-codes.md %}).

---

## On This Page

- [How MECCA Works](#how-mecca-works)
- [Quick Reference](#quick-reference)
- [Colors](#colors)
- [Cursor Control & Video](#cursor-control)
- [User & Session Information](#user-info)
- [System & Node Information](#system-info)
- [Message & File Area Information](#area-info)
- [Conditional Display](#conditional-display)
- [Flow Control](#flow-control)
- [Questionnaires & Input](#questionnaires)
- [Privilege Controls](#privilege-controls)
- [Lock & Key Controls](#lock-key)
- [User State Tests](#user-state-tests)
- [File & External Operations](#file-operations)
- [Multinode Tokens](#multinode)
- [RIP Graphics (Legacy)](#rip-graphics)
- [Miscellaneous](#miscellaneous)
- [MECCA vs. Pipe Codes](#mecca-vs-pipe-codes)

---

## How MECCA Works {#how-mecca-works}

A MECCA source file is a plain text file — typically with a `.mec`
extension — that mixes literal display text with **tokens** enclosed in
square brackets. Anything outside the brackets is displayed to the caller
as-is. Anything inside is interpreted by the MECCA compiler and translated
into compact bytecodes that Maximus processes at runtime.

Here's a simple example. This `.mec` source:

```
[cls white]Welcome to [lightcyan][sys_name][white], [yellow][first][white]!
This is your [lightgreen][usercall][white] call to the system.
```

...compiles to a `.bbs` file that clears the screen, then displays a
personalized welcome in color — the system name in light cyan, the caller's
first name in yellow, their call count in light green, and everything else in
white. A caller on a plain TTY terminal sees the same text, just without the
colors.

### Token Syntax

Tokens are enclosed in `[` and `]`. They are **case-insensitive** — `[user]`,
`[USER]`, and `[UsEr]` all do the same thing. Spaces inside the brackets are
ignored, so `[  user  ]` works fine.

You can combine multiple tokens inside one pair of brackets, separated by
spaces:

```
[lightblue blink user]
```

is identical to:

```
[lightblue][blink][user]
```

To include a literal left bracket in your output, double it:

```
Want to check your mail [[Y,n]?
```

This displays `Want to check your mail [Y,n]?` without MECCA trying to parse
`Y,n` as a token.

You can also insert raw ASCII codes by putting the decimal value in brackets.
`[7]` produces a bell character; `[13]` produces a carriage return.

### Labels

MECCA supports named labels for flow control. A label definition uses a
leading slash:

```
[/my_label]
```

When you reference that label in a `[goto]`, you omit the slash:

```
[goto my_label]
```

Label names must start with a letter and contain only letters, numbers, and
underscores. They must be unique within the file and cannot collide with
existing token names. Labels can be forward-referenced — you can `[goto]` to
a label that appears later in the file.

### Compile Workflow

Edit your `.mec` file in any text editor, then compile it:

```bash
bin/mecca display/screens/welcome.mec
```

This produces `welcome.bbs` in the same directory. Maximus loads `.bbs` files
fresh each time they're displayed, so there's no need to restart — just
recompile and reconnect (or navigate to the screen again).

For RIP graphics, the compiler also recognizes `.mer` source files and
produces `.rbs` output files. See [RIP Graphics](#rip-graphics) for details.

---

## Quick Reference {#quick-reference}

Every MECCA token at a glance, grouped by category. Detailed descriptions,
examples, and usage notes follow in the sections below.

### Colors

| Token | Description |
|-------|-------------|
| `[black]` `[blue]` `[green]` `[cyan]` `[red]` `[magenta]` `[brown]` `[gray]` | Standard colors (foreground and background) |
| `[darkgray]` `[lightblue]` `[lightgreen]` `[lightcyan]` `[lightred]` `[lightmagenta]` `[yellow]` `[white]` | High-intensity colors (foreground only) |
| `[fg <color>]` | Change foreground without affecting background |
| `[bg <color>]` | Change background without affecting foreground |
| `[<fg> on <bg>]` | Set both foreground and background at once |
| `[blink]` | Enable blinking text (until next color token) |
| `[steady]` | Disable blinking |
| `[bright]` | Promote current foreground to high-intensity |
| `[dim]` | Demote current foreground to normal intensity |
| `[save]` | Save current color for later restoration |
| `[load]` | Restore the color saved by `[save]` |

### Cursor Control & Video

| Token | Description |
|-------|-------------|
| `[cls]` | Clear screen (resets color to cyan) |
| `[cleol]` | Clear to end of line |
| `[cleos]` | Clear from cursor to end of screen |
| `[locate <row> <col>]` | Move cursor to row, col (1-based) |
| `[up]` `[down]` `[left]` `[right]` | Move cursor one position |
| `[cr]` `[lf]` `[tab]` `[bs]` | Carriage return, line feed, tab, backspace |
| `[bell]` | Beep on the caller's terminal |
| `[sysopbell]` | Beep on the local console only |
| `[repeat]<c>[<n>]` | Repeat character `<c>` a total of `<n>` times |
| `[repeatseq <len>]<s>[<n>]` | Repeat a string `<s>` of length `<len>`, `<n>` times |
| `[pause]` | Pause for half a second |

### User & Session Information

| Token | Description |
|-------|-------------|
| `[user]` / `[name]` | Caller's full name |
| `[first]` / `[fname]` | Caller's first name |
| `[realname]` | Caller's real name (if alias is in use) |
| `[city]` | Caller's city |
| `[phone]` | Caller's phone number |
| `[date]` | Current date (`dd mmm yy`) |
| `[time]` | Current time (`hh:mm:ss`) |
| `[usercall]` | Caller's total calls (ordinal — "14th") |
| `[minutes]` | Minutes online in the last 24 hours |
| `[remain]` | Minutes remaining this session |
| `[length]` | Duration of this call in minutes |
| `[timeoff]` | Time the caller must be off by (includes newline) |
| `[dl]` / `[ul]` | KB downloaded / uploaded (lifetime + today) |
| `[netdl]` | Net downloads today (down − up) |
| `[ratio]` | Upload:download ratio |
| `[lastcall]` | Date of caller's last call |
| `[syscall]` | Total calls to this node (ordinal) |
| `[lastuser]` | Name of the last user on this node |
| `[expiry_date]` | Subscription expiration date (or "None") |
| `[expiry_time]` | Subscription time remaining (or "None") |

### System & Node Information

| Token | Description |
|-------|-------------|
| `[sys_name]` | System name |
| `[sysop_name]` | Sysop's full name |
| `[node_num]` | Current node number |

### Message Area Information

| Token | Description |
|-------|-------------|
| `[msg_carea]` | Current message area name |
| `[msg_cname]` | Current message area description |
| `[msg_darea]` | Current message area division |
| `[msg_sarea]` | Current message area (without division prefix) |
| `[msg_cmsg]` | Current message number |
| `[msg_hmsg]` | Highest message number in current area |
| `[msg_nummsg]` | Total messages in current area |
| `[alist_msg]` | Display the message area list |

### File Area Information

| Token | Description |
|-------|-------------|
| `[file_carea]` | Current file area name |
| `[file_cname]` | Current file area description |
| `[file_darea]` | Current file area division |
| `[file_sarea]` | Current file area (without division prefix) |
| `[alist_file]` | Display the file area list |

### NetMail Credits

| Token | Description |
|-------|-------------|
| `[netcredit]` | Caller's NetMail credit (cents) |
| `[netdebit]` | Caller's NetMail debit (cents) |
| `[netbalance]` | Net balance (credit − debit) |

### Privilege & Access

| Token | Description |
|-------|-------------|
| `[priv_level]` | Caller's numeric privilege level |
| `[priv_abbrev]` | Caller's class abbreviation (e.g. "SysOp") |
| `[priv_desc]` | Caller's class description |
| `[priv_up]` | Promote caller to next higher class |
| `[priv_down]` | Demote caller to next lower class |
| `[setpriv <key>]` | Set caller's privilege to a specific class |
| `[acs <string>]` / `[access <string>]` | Display rest of line if ACS passes |
| `[acsfile <string>]` / `[accessfile <string>]` | Display rest of file if ACS passes |

### Flow Control

| Token | Description |
|-------|-------------|
| `[goto <label>]` / `[jump <label>]` | Jump to a label |
| `[/label]` / `[label <name>]` | Define a label |
| `[quit]` | Exit current file (returns to caller if linked) |
| `[exit]` | Exit all files (including linked parents) |
| `[top]` | Jump back to the top of the current file |
| `[enter]` | "Press ENTER to continue" prompt |
| `[more]` | Display "More [Y,n,=]?" prompt |
| `[moreoff]` / `[moreon]` | Disable / enable automatic more prompts |
| `[ckoff]` / `[ckon]` | Disable / enable Ctrl-C/Ctrl-K abort |

### Questionnaire & Input

| Token | Description |
|-------|-------------|
| `[menu]<keys>` | Prompt for a single keypress from valid `<keys>` |
| `[choice]<c>` | Display rest of line if last `[menu]` response was `<c>` |
| `[readln]<desc>` | Read a line of input; log to questionnaire file |
| `[response]` | Display the last `[readln]` response |
| `[ifentered]<s>` | Display rest of line if last `[readln]` matched `<s>` |
| `[open]<f>` / `[sopen]<f>` | Open a questionnaire answer file |
| `[post]` | Write caller name, city, date/time to answer file |
| `[store]<desc>` | Write last `[menu]` response to answer file |
| `[write]<line>` | Write a line directly to the answer file |
| `[ansreq]` / `[ansopt]` | Require / allow optional answers |

### Conditional Tests

| Token | Description |
|-------|-------------|
| `[expert]` `[regular]` `[novice]` | Display rest of line by help level |
| `[hotkeys]` | Rest of line if caller has hotkeys enabled |
| `[ibmchars]` | Rest of line if caller supports high-bit characters |
| `[col80]` | Rest of line if caller's screen is ≥79 columns |
| `[islocal]` / `[isremote]` | Rest of line by connection type |
| `[b1200]` `[b2400]` `[b9600]` | Rest of line if baud ≥ threshold |
| `[notontoday]` | Rest of line if caller hasn't logged on today |
| `[permanent]` | Rest of line if caller is flagged permanent |
| `[tagged]` | Rest of line if caller has tagged files |
| `[maxed]` | Rest of line if caller is in the MaxEd editor |
| `[iffse]` / `[iffsr]` | Rest of line if full-screen editor/reader enabled |
| `[iftime <op> <hh>:<mm>]` | Rest of line based on time-of-day comparison |
| `[ifexist]<file>` | Rest of line if file exists on disk |
| `[iflang]<n>` | Rest of line if caller's language is `<n>` |
| `[iftask]<n>` | Rest of line if current node is `<n>` |
| `[incity]<s>` | Rest of line if `<s>` appears in caller's city |
| `[filenew]<f>` | Rest of line if file is newer than last logon |
| `[color]`…`[endcolor]` | Block displayed only to ANSI/AVATAR callers |
| `[nocolor]`…`[endcolor]` | Block displayed only to TTY callers |
| `[msg_nomsgs]` `[msg_nonew]` `[msg_noread]` | Message area state tests |
| `[msg_conf]` `[msg_echo]` `[msg_local]` `[msg_matrix]` | Message area type tests |
| `[msg_next]` / `[msg_prior]` | Reading direction tests |
| `[msg_fileattach]` | Rest of line if caller has pending file attaches |
| `[msg_attr <letter>]` | Rest of line if caller can set message attribute |
| `[msg_checkmail]` | Invoke the internal mail checker |
| `[chat_avail]` / `[chat_notavail]` | Rest of line by chat availability |

### Lock & Key

| Token | Description |
|-------|-------------|
| `[ifkey]<keys>` | Rest of line if caller has all specified keys |
| `[notkey]<keys>` | Rest of line if caller lacks all specified keys |
| `[keyon]<keys>` | Grant keys to the caller |
| `[keyoff]<keys>` | Revoke keys from the caller |

### File & External Operations

| Token | Description |
|-------|-------------|
| `[display]<f>` | Display a `.bbs` file (no return to current file) |
| `[link]<f>` | Display a `.bbs` file and return (up to 8 deep) |
| `[include <f>]` | Compiler directive: inline another `.mec` file |
| `[copy <f>]` | Compiler directive: copy raw bytes into output |
| `[comment <text>]` | Compiler directive: ignored comment |
| `[delete]<f>` | Delete a file from disk |
| `[download]<f>` | Initiate a file download |
| `[log]<s>` | Write a line to the system log |
| `[menupath]<p>` | Change the menu file search path |
| `[language]` | Invoke the language-change command |
| `[mex]<file>` | Run a MEX script |
| `[menu_cmd <cmd>]` | Invoke any menu command by name |
| `[dos]<cmd>` | Run an OS command |
| `[xtern_run]<cmd>` | Run external program (stdio) |
| `[xtern_dos]<cmd>` | Run external program (shell) |
| `[xtern_erlvl]<cmd>` | Run external program (errorlevel exit) |
| `[key_poke]<keys>` | Inject keystrokes into the input buffer |
| `[clear_stacked]` | Flush the keyboard stacking buffer |
| `[no_keypress]` / `[nostacked]` | Conditional: no pending input |
| `[onexit]<f>` | Set a file to display when the current file exits |
| `[newfiles]` | Invoke a new-files scan |
| `[leave_comment]` | Place caller in editor to leave a comment |
| `[quote]` | Display next quote from the quote file |
| `[hangup]` | Disconnect the caller |

### RIP Graphics (Legacy)

| Token | Description |
|-------|-------------|
| `[rip]`…`[endrip]` | Block displayed only to RIP callers |
| `[norip]`…`[endrip]` | Block displayed only to non-RIP callers |
| `[ripsend]<file>` | Send RIP scene/icon files to remote |
| `[ripdisplay]<file>` | Display a RIP scene/icon on remote |
| `[rippath]<path>` | Change the RIP file search directory |
| `[riphasfile]<name>[,<size>]` | Rest of line if remote has the file |
| `[textsize <cols> <rows>]` | Override assumed text window size |

---

## Colors {#colors}

Color is the soul of a BBS screen. MECCA gives you 16 foreground colors, 8
background colors, blinking, intensity toggling, and a save/restore
mechanism — everything you need to paint a screen without touching raw ANSI
escape codes.

### Setting Colors

The simplest way to change color is to drop the color name in brackets:

```
[yellow]This text is yellow. [white]This text is white.
```

For a foreground-on-background combination, use the `on` keyword:

```
[lightgreen on blue]This is light green text on a blue background.
```

Only the first eight standard colors can be used as backgrounds. The
high-intensity colors (`lightblue`, `yellow`, `white`, etc.) are
foreground-only.

### The Color Table

| Standard (FG & BG) | High-Intensity (FG only) |
|---------------------|--------------------------|
| `[black]` | `[darkgray]` |
| `[blue]` | `[lightblue]` |
| `[green]` | `[lightgreen]` |
| `[cyan]` | `[lightcyan]` |
| `[red]` | `[lightred]` |
| `[magenta]` | `[lightmagenta]` |
| `[brown]` | `[yellow]` |
| `[gray]` | `[white]` |

### Foreground and Background Independently

Sometimes you want to change just the foreground or just the background
without disturbing the other. That's what `[fg]` and `[bg]` are for:

```
[red on blue]Hello, [fg yellow]world!
```

This displays "Hello," in red on blue, then switches the foreground to yellow
while keeping the blue background. Without `[fg]`, a plain `[yellow]` token
would reset the background to black.

Similarly, `[bg green]` changes just the background:

```
[lightcyan on blue]Status: [bg green] OK [bg red] FAIL
```

### Bright and Dim

If you're already in a color and want to toggle intensity without restating
the whole color, use `[bright]` and `[dim]`:

```
[green]Normal green. [bright]Now light green. [dim]Back to normal green.
```

`[bright]` promotes a standard color to its high-intensity counterpart.
`[dim]` does the reverse. This is handy for alternating emphasis without
cluttering your source with full color names.

### Blinking

`[blink]` enables the blink attribute for subsequent text. A color token
always resets blink, so the order matters:

```
[green blink]This blinks.
[blink green]This does NOT blink — the color token came after.
```

To stop blinking without changing color, use `[steady]`:

```
[yellow blink]ALERT! [steady]Everything is fine now.
```

### Save and Load

When you're building a screen with a consistent background and need to
temporarily switch colors for a highlighted element, `[save]` and `[load]`
keep things tidy:

```
[yellow on blue save]Welcome to the board![cleol]
[lightred on green]*** NEW BULLETIN ***[cleol]
[load]Back to yellow on blue, no need to re-type it.[cleol]
```

`[save]` captures the current color state. `[load]` restores it. You get one
save slot — saving again overwrites the previous save.

### TTY Callers

If a caller has TTY mode (no ANSI or AVATAR graphics), Maximus automatically
strips all color and cursor codes from the output. Your colorized `.mec`
source works for everyone — ANSI callers see the colors, TTY callers see
clean text. You never need to maintain separate ASCII display files.

---

## Cursor Control & Video {#cursor-control}

Beyond color, MECCA gives you direct control over the caller's screen — clearing,
positioning, and drawing.

### Screen Clearing

`[cls]` clears the entire screen and resets the color to cyan. It's typically
the first token in any full-screen display file:

```
[cls white]
```

`[cleol]` clears from the cursor to the end of the current line. If the
background color is non-black, the cleared area inherits that color — which
is how you paint colored bars across the screen:

```
[locate 1 1][white on blue][repeat] [80][cleol]
```

`[cleos]` clears from the cursor to the end of the screen.

### Cursor Positioning

`[locate <row> <col>]` moves the cursor to an absolute position. Coordinates
are 1-based — `[locate 1 1]` is the top-left corner:

```
[locate 5 20]This text starts at row 5, column 20.
```

The single-step movement tokens `[up]`, `[down]`, `[left]`, and `[right]`
each move the cursor one position in the named direction. Combine them for
small adjustments without calculating absolute coordinates.

`[cr]` sends a carriage return (back to column 1), `[lf]` sends a line feed
(down one row), `[tab]` sends a tab, and `[bs]` sends a backspace (one
column left).

### Drawing Lines and Boxes

`[repeat]` outputs a single character multiple times. The character follows
the token directly, and the count goes in its own brackets:

```
[repeat]═[78]
```

This draws a double horizontal line 78 characters wide — perfect for screen
dividers.

`[repeatseq]` repeats a multi-character string. You specify the string length
as an argument to the token, then the string follows, with the repeat count
in brackets:

```
[repeatseq 3]═╦═[26]
```

This repeats the three-character pattern `═╦═` twenty-six times.

### Practical Example: A Header Bar

Here's a common pattern — a colored title bar across the top of the screen:

```
[cls]
[locate 1 1][white on blue][repeat] [80]
[locate 1 3][sys_name] — Node [node_num]
[locate 1 60][[remain] min left]
[locate 3 1][gray on black]
```

This clears the screen, paints a blue bar across row 1, places the system
name and node number at the left, the caller's remaining time at the right,
then resets to gray on black for the main content starting at row 3.

### Pausing and Sounds

`[pause]` halts display for half a second — useful for dramatic timing in
welcome screens or animations.

`[bell]` sends a beep to the caller's terminal. `[sysopbell]` beeps the
local console only — the caller doesn't hear it. This is useful for alerting
the sysop when a specific screen is displayed (like a chat request or
restricted area access).

---

## User & Session Information {#user-info}

These tokens embed live data about the current caller into your display
files. Every time the screen is shown, Maximus substitutes the current
values.

### Identity

| Token | Displays |
|-------|----------|
| `[user]` or `[name]` | Full name (e.g. "Kevin Morgan") |
| `[first]` or `[fname]` | First name only (e.g. "Kevin") |
| `[realname]` | Real name, if the caller is using an alias |
| `[city]` | City and state/province |
| `[phone]` | Phone number |

### Session Timing

| Token | Displays |
|-------|----------|
| `[date]` | Current date in `dd mmm yy` format |
| `[time]` | Current time in `hh:mm:ss` format |
| `[minutes]` | Minutes online in the last 24 hours |
| `[remain]` | Minutes remaining this session |
| `[length]` | Duration of this call (minutes) |
| `[timeoff]` | Time the caller must log off (includes trailing newline) |

### Activity

| Token | Displays |
|-------|----------|
| `[usercall]` | Total calls to this system, as an ordinal ("14th") |
| `[syscall]` | Total calls to this node, as an ordinal |
| `[lastcall]` | Date of the caller's previous call |
| `[lastuser]` | Name of the last user on this node |
| `[dl]` | Total KB downloaded (lifetime + today) |
| `[ul]` | Total KB uploaded (lifetime + today) |
| `[netdl]` | Net downloads today (today's down − today's up) |
| `[ratio]` | Upload:download ratio |

### Subscription & Credits

| Token | Displays |
|-------|----------|
| `[expiry_date]` | Subscription expiry date, or "None" |
| `[expiry_time]` | Subscription time remaining, or "None" |
| `[netcredit]` | NetMail credit in cents |
| `[netdebit]` | NetMail debit in cents |
| `[netbalance]` | Net balance (credit − debit) |

### Example: A Welcome Screen

```
[cls]
[lightcyan]Welcome back, [yellow][first][lightcyan]!
[gray]This is your [white][usercall][gray] call.
[gray]You have [white][remain][gray] minutes remaining.
[notontoday][lightgreen]This is your first call today — check the bulletins![gray]
[enter]
```

The `[notontoday]` line only appears if the caller hasn't logged on yet
today. The `[enter]` token pauses with a "Press ENTER to continue" prompt
before Maximus moves on.

---

## System & Node Information {#system-info}

| Token | Displays |
|-------|----------|
| `[sys_name]` | The system name as configured in `maximus.toml` |
| `[sysop_name]` | The sysop's full name |
| `[node_num]` | The current node number |

These are straightforward — use them to brand screens with your board's name
and give multinode callers context about which node they're on.

---

## Message & File Area Information {#area-info}

Display files can show context-sensitive information about whatever message
or file area the caller is currently in.

### Message Area

| Token | Displays |
|-------|----------|
| `[msg_carea]` | Area tag (e.g. "GENERAL") |
| `[msg_cname]` | Area description (e.g. "General Discussion") |
| `[msg_darea]` | Division name |
| `[msg_sarea]` | Area tag without division prefix |
| `[msg_cmsg]` | Current message number |
| `[msg_hmsg]` | Highest message number |
| `[msg_nummsg]` | Total messages in the area |
| `[alist_msg]` | Display the full message area list |

### File Area

| Token | Displays |
|-------|----------|
| `[file_carea]` | Area tag |
| `[file_cname]` | Area description |
| `[file_darea]` | Division name |
| `[file_sarea]` | Area tag without division prefix |
| `[alist_file]` | Display the full file area list |

### Example: Message Area Header

```
[white on blue][repeat] [80]
[locate 1 3]Messages: [lightcyan][msg_cname]
[locate 1 50][white]([msg_nummsg] msgs, #[msg_cmsg])
[white on black]
```

---

## Conditional Display {#conditional-display}

This is where MECCA becomes more than a pretty-printer. Conditional tokens
let you show or hide parts of a display file based on who's calling, what
terminal they have, what time it is, or what area they're in.

Most conditional tokens follow the same pattern: **display the rest of the
current line** only if the condition is true. If the condition fails, Maximus
skips to the next line.

### Terminal and Capability Tests

```
[color][lightcyan]You have ANSI graphics. Nice![endcolor]
[nocolor]You're on a plain TTY terminal. That's okay too.[endcolor]
[col80]This line only appears if your screen is at least 79 columns wide.
[ibmchars]You can see box-drawing characters: ╔═╗║ ║╚═╝
```

The `[color]`…`[endcolor]` and `[nocolor]`…`[endcolor]` pairs are block
constructs — they can span multiple lines. Everything else is line-scoped.

### Help Level Tests

```
[novice][white]Hint: Type 'H' for help at any menu prompt.
[regular][gray]Type '?' for a command summary.
[expert]
```

These test the caller's help level setting. The `[expert]` line with nothing
after it is a common pattern — experts see nothing extra.

### Time-of-Day Tests

`[iftime]` takes a comparison operator and a 24-hour time:

```
[iftime ge 06:00][iftime lt 12:00][yellow]Good morning, [first]!
[iftime ge 12:00][iftime lt 18:00][yellow]Good afternoon, [first]!
[iftime ge 18:00][yellow]Good evening, [first]!
```

You can chain multiple `[iftime]` tests on the same line — all must pass for
the line to display.

Supported operators: `eq`/`equal`, `ne`/`notequal`, `lt`/`below`,
`gt`/`above`, `le`/`be`, `ge`/`ae`.

### Connection and Location Tests

```
[islocal]You're on the local console — full sysop access.
[isremote][b9600]Nice fast connection!
[isremote][b2400][b9600 goto fast_user]You're on a slower link.
[/fast_user]
[incity]Toronto Welcome, fellow Torontonian!
```

`[b1200]`, `[b2400]`, and `[b9600]` test minimum baud rate — they display
the rest of the line if the caller's speed meets or exceeds the threshold.

`[incity]` does a substring search in the caller's city field. The search
string is separated from the rest of the line by a space.

### File and Session Tests

```
[filenew]display/misc/bulletin.bbs [display]display/misc/bulletin.bbs
[notontoday][lightgreen]First call today! Checking for new mail...[msg_checkmail]
[tagged][yellow]You have files tagged for download.
```

`[filenew]` compares a file's timestamp against the caller's last logon
date — if the file is newer, the rest of the line is displayed. This is the
classic "show the bulletin only if they haven't seen it" pattern.

### Task and Language Tests

```
[iftask]1 You are on Node 1.
[iftask]2 You are on Node 2.
[iflang]0 [display]display/screens/welcome_en.bbs
[iflang]1 [display]display/screens/welcome_de.bbs
```

`[iftask]` tests the current node number (decimal, space-separated from the
line content). `[iflang]` tests the caller's language index (0-based).

### Message Area Tests

These let you tailor message screens based on the area type or state:

```
[msg_matrix][yellow]This is a NetMail area — charges may apply.
[msg_echo][lightcyan]This is an EchoMail area — be aware of network rules.
[msg_local]This is a local-only area.
[msg_nomsgs]There are no messages in this area yet. Be the first to post!
[msg_nonew]No new messages since your last visit.
[msg_fileattach][yellow blink]You have file attaches waiting!
```

`[msg_conf]` tests for conference areas. `[msg_next]` and `[msg_prior]` test
the current reading direction.

`[msg_attr <letter>]` tests whether the caller has permission to set a
specific message attribute. This is how the message attribute selection screen
shows only the options the caller can actually use:

```
[msg_attr P]P  Toggle Private
[msg_attr C]C  Toggle Crash
[msg_attr A]A  Attach a file
```

---

## Flow Control {#flow-control}

MECCA's flow control is simple — labels and gotos — but it's enough to build
interactive screens, questionnaires, and mini-menus.

### Labels and Goto

Define a label with a leading slash. Jump to it with `[goto]`:

```
[/ask_again]
Do you want to play a game? [[Y,n]? [menu]YN
[choice]Y[link]display/misc/games.bbs
[choice]Y[goto ask_again]
[choice]N[quit]
```

This displays a prompt, waits for Y or N, links to the games screen if they
said yes, then loops back to ask again. If they said no, `[quit]` exits the
file.

Labels can be forward-referenced — you can `[goto]` a label that appears
later in the file:

```
Want a quote? [[y,n]? [menu]YN
[choice]Y[goto show_quote]
[choice]N[quit]
[/show_quote]
[quote quit]
```

### Quit and Exit

`[quit]` exits the current display file. If this file was invoked by a
`[link]` token in another file, control returns to the calling file.

`[exit]` exits all nested display files — it unwinds the entire `[link]`
stack and returns to whatever invoked the outermost file.

### Top

`[top]` jumps back to the beginning of the current file and starts over.
Use this for files that should loop until the caller explicitly leaves.

### Enter and More

`[enter]` displays "Press ENTER to continue" and waits. This is your
pause-between-screens token.

`[more]` displays the "More [Y,n,=]?" prompt. If the caller presses N,
Maximus stops displaying the current file.

`[moreoff]` disables the automatic more-prompt feature (useful for
full-screen displays where you don't want Maximus interrupting after 24
lines). `[moreon]` re-enables it.

### Abort Control

`[ckoff]` disables Ctrl-C and Ctrl-K checking, preventing the caller from
aborting the current file. `[ckon]` re-enables it. Use `[ckoff]` for screens
where aborting would leave the display in a messy state.

---

## Questionnaires & Input {#questionnaires}

MECCA includes a built-in questionnaire system — you can collect input from
callers and log it to a file, all from a display file. No MEX scripting
required.

### The Pattern

Most questionnaires follow this flow:

1. **Open a log file** with `[open]` — this is where answers are stored
2. **Post the header** with `[post]` — writes the caller's name, city, and
   timestamp
3. **Ask questions** with `[readln]` — each one reads a line of input and
   writes it to the log
4. Close the file (happens automatically when the display file ends)

### A Complete Example

```
[cls white]
[lightcyan]New User Survey[gray]
[repeat]─[40]

[open]data/surveys/newuser.txt
[post]

What is your favorite BBS? [readln]Fav_BBS
What brings you here? [readln]Reason
Rate your experience so far (1-5): [menu]12345
[store]Rating

[white]Thanks, [first]! Your answers have been recorded.
[enter quit]
```

`[open]` opens (or creates) the answer file. `[post]` writes a header block
with the caller's identity. Each `[readln]` prompts for a line of text and
writes it to the file, tagged with the optional one-word description (`Fav_BBS`,
`Reason`). `[menu]` collects a single keypress from the valid set, and
`[store]` writes that response to the file.

The answer file ends up looking like a readable log:

```
== Kevin Morgan, Toronto ON, 02 Mar 2026 18:15 ==
Fav_BBS: Maximus NG
Reason: Nostalgia and community
Rating: 5
```

### Menu and Choice

`[menu]` and `[choice]` are the backbone of interactive MECCA screens.
`[menu]` defines a set of valid keypress responses and waits for the caller
to press one. `[choice]` then branches based on what they pressed:

```
Pick a door: (T)rade Wars, (L)ORD, (Q)uit [menu]TLQ
[choice]T[menu_cmd xtern_run]tradewars
[choice]L[menu_cmd xtern_run]lord
[choice]Q[quit]
```

### Answer Requirements

By default, `[menu]` and `[readln]` require an answer — the caller can't
just press Enter to skip. Place `[ansopt]` before a question to make the
answer optional, and `[ansreq]` to switch back to requiring answers.

### Testing Responses

`[ifentered]` compares a string against the last `[readln]` response:

```
What's your favorite color? [readln]Color
[ifentered]blue [lightblue]Great taste!
[ifentered]red [lightred]Bold choice!
```

`[response]` displays the last `[readln]` response inline — it even works
across files, so one file can collect input and a linked file can reference it.

### Clearing Stacked Input

`[readln]` honors stacked commands by default — if the caller had extra
keystrokes buffered from a prior prompt, they'd be consumed. Drop a
`[clear_stacked]` right before `[readln]` to flush the buffer and force
fresh input:

```
[clear_stacked]
What is your real name? [readln]Name
```

---

## Privilege Controls {#privilege-controls}

### Displaying Privilege Info

| Token | Displays |
|-------|----------|
| `[priv_level]` | Numeric privilege level (e.g. "100") |
| `[priv_abbrev]` | Class abbreviation (e.g. "SysOp") |
| `[priv_desc]` | Class description (e.g. "System Operator") |

### Modifying Privilege

`[priv_up]` promotes the caller to the next higher class.
`[priv_down]` demotes them to the next lower class.
`[setpriv <key>]` sets the caller's privilege to a specific class identified
by its single-character key from the access control file.

These are powerful — use them carefully. A common pattern is a
"registration" questionnaire that promotes the caller after completing it:

```
[white]Registration complete. Upgrading your access...
[priv_up]
[white]You are now a [lightcyan][priv_desc][white]. Welcome aboard!
```

### ACS Testing

`[acs]` (or `[access]`) tests a full ACS string and displays the rest of the
line only if the caller passes:

```
[acs Sysop]You have sysop-level access.
[acs key/A]This line requires access key A.
```

`[acsfile]` (or `[accessfile]`) does the same but controls the rest of the
**entire file** — if the test fails, Maximus stops displaying the file
immediately.

---

## Lock & Key Controls {#lock-key}

Maximus uses a key-based access system where callers can be assigned
single-character keys (letters and digits). MECCA can test and manipulate
these keys.

### Testing Keys

`[ifkey]` displays the rest of the line if the caller has **all** of the
specified keys. The keys are separated from the line content by a space:

```
[ifkey]1A You have keys 1 and A. Welcome to the inner sanctum.
```

`[notkey]` is the inverse — the line displays only if the caller **lacks**
all of the specified keys:

```
[notkey]X You don't have key X. Apply in the sysop forum!
```

### Granting and Revoking Keys

`[keyon]` gives keys to the caller. `[keyoff]` takes them away. Both require
a space between the key list and the rest of the line:

```
[keyon]ABC Congratulations! You've been granted keys A, B, and C.
[keyoff]Z Key Z has been revoked.
```

This is how registration questionnaires and achievement systems work — answer
the right questions and your display file grants you keys that unlock new
areas of the board.

---

## User State Tests {#user-state-tests}

These conditional tokens test various attributes of the caller's session and
preferences:

| Token | Condition |
|-------|-----------|
| `[expert]` | Help level is Expert |
| `[regular]` | Help level is Regular |
| `[novice]` | Help level is Novice |
| `[hotkeys]` | Hotkeys are enabled |
| `[ibmchars]` | IBM (high-bit) character support is enabled |
| `[col80]` | Screen width is at least 79 columns |
| `[islocal]` | Caller is at the local console |
| `[isremote]` | Caller is remote |
| `[permanent]` | Caller is flagged as permanent (no auto-purge) |
| `[notontoday]` | Caller hasn't logged on today |
| `[tagged]` | Caller has files tagged for download |
| `[maxed]` | Caller is currently in the MaxEd editor |
| `[iffse]` | Full-screen editor is enabled |
| `[iffsr]` | Full-screen reader is enabled |
| `[b1200]` | Connected at 1200 bps or above |
| `[b2400]` | Connected at 2400 bps or above |
| `[b9600]` | Connected at 9600 bps or above |

All of these are line-scoped — they control whether the rest of the current
line is displayed.

---

## File & External Operations {#file-operations}

### Displaying Other Files

`[display]<f>` jumps to another `.bbs` file. Control does **not** return to
the current file — it's a one-way jump. External program translation
characters are supported in the filename (use `+` instead of `%`).

`[link]<f>` displays another `.bbs` file and then **returns** to the current
file where it left off. Up to 8 `[link]` calls can be nested.

`[onexit]<f>` sets a file to be displayed after the current file finishes.

### Compiler Directives

`[include <f>]` is a compile-time directive — MECCA reads the specified file
and processes it as if it were part of the current source. Use this for
shared headers or common screen elements.

`[copy <f>]` copies raw bytes from a file directly into the compiled output
without any parsing. The filename must be inside the brackets.

`[comment <text>]` is a compiler comment — ignored entirely. Use it to
document your `.mec` source:

```
[comment This section displays the welcome banner]
```

### Running External Programs

MECCA can launch external programs and commands directly from display files:

| Token | Behavior |
|-------|----------|
| `[dos]<cmd>` | Run an OS command via shell |
| `[xtern_run]<cmd>` | Run external program with stdio bridging |
| `[xtern_dos]<cmd>` | Run external program via `system()` |
| `[xtern_erlvl]<cmd>` | Run external program (legacy errorlevel exit) |
| `[mex]<file>` | Run a MEX script |
| `[menu_cmd <cmd>]<arg>` | Invoke any built-in menu command by name |

`[mex]` can pass arguments:

```
[mex]scripts/greeting.mex arg1 arg2
```

`[menu_cmd]` is the Swiss army knife — it can invoke any command from the
[Menu Options]({{ site.baseurl }}{% link config-menu-options.md %}) reference. For example:

```
[menu_cmd goodbye]
```

### Keyboard Injection

`[key_poke]<keys>` injects keystrokes into the input buffer as if the caller
had typed them. Use a `|` character for an empty response (just Enter). This
is how you automate multi-step operations:

```
[key_poke]|
[newfiles]
```

This skips the "enter date" prompt in the new-files scan by injecting a
blank response.

### File Operations

`[delete]<f>` deletes a file from disk. `[log]<s>` appends a line to the
system log — the first character of `<s>` is the log priority level:

```
[log]+User [user] accessed the restricted area
```

### Other Commands

| Token | What It Does |
|-------|-------------|
| `[newfiles]` | Invoke a new-files scan |
| `[leave_comment]` | Place the caller in the editor to leave a message to the sysop |
| `[quote]` | Display the next quote from the quote file |
| `[hangup]` | Disconnect the caller immediately |
| `[language]` | Invoke the Chg_Language command |
| `[menupath]<p>` | Change the menu file search path |
| `[tag_read]<f>` | Load tagged file list from a file |
| `[tag_write]<f>` | Save tagged file list to a file |
| `[tune]<name>` | Play a tune on the local console |

---

## Multinode Tokens {#multinode}

For boards with multiple nodes, MECCA provides a few tokens to communicate
across nodes and check caller availability.

`[apb]` sends an All-Points-Bulletin to every caller currently online. The
text can include external program translation characters:

```
[apb][yellow bell]%!User %n just logged on!%!
```

`[who_is_on]` executes the internal Who_Is_On command, displaying a list of
all active nodes and callers.

`[chat_avail]` and `[chat_notavail]` test whether the caller is available
for paging by other users:

```
[chat_avail][green]You are available for chat.
[chat_notavail][yellow]You are NOT available for chat.
```

---

## RIP Graphics (Legacy) {#rip-graphics}

MECCA includes tokens for RIP (Remote Imaging Protocol) graphics support.
These tokens compile, and the runtime has hooks to process them — but RIP
itself is a 1990s protocol that hasn't been actively maintained or tested in
Maximus NG. The display path and query system exist in the codebase, but
realistically, RIP support is frozen. If someone wants to revive it, the
hooks are there. For new boards, ANSI is the way to go.

That said, here's what the tokens do — because they're still in the language
and you might encounter them in legacy files.

### Conditional Display

`[rip]`…`[endrip]` wraps content that is only shown to callers with RIP
graphics enabled:

```
[rip]|1R00000000m_menu.rip[endrip]
```

`[norip]`…`[endrip]` is the inverse — content shown only to callers
**without** RIP:

```
[norip]You are not a RIP caller.[endrip]
```

RIP graphics are never shown on the local console, regardless of the
caller's setting.

### File Transfer

`[ripsend]<file>` sends RIP scene or icon files to the remote system for
storage. Files are looked up in the RIP path directory. Prefix with `@` to
send a list of files from a text file. Prefix with `!` to force re-querying
even if the file was already sent this session.

`[ripdisplay]<file>` tells the remote system to immediately display a RIP
scene or icon. If the remote doesn't have the file, it's sent automatically.

`[rippath]<path>` changes the directory where Maximus looks for RIP files.

### Checking Remote Files

`[riphasfile]<name>[,<size>]` displays the rest of the line only if the
remote system has a specific file (and optionally a matching file size).

### Window Size

`[textsize <cols> <rows>]` overrides the assumed text window size for
RIP-mode callers. Use `0` for either dimension to keep the default from the
user file.

### RIP Source Files

To create RIP-enabled display files, you have two paths:

1. Use a RIP editor to create `.rbs` files directly
2. Write `.mer` source files with MECCA tokens embedded in RIP sequences —
   the compiler produces `.rbs` output

When Maximus is about to display a `.bbs` file to a RIP-enabled caller, it
first checks for a `.rbs` file with the same base name. If found, the `.rbs`
is displayed instead.

---

## Miscellaneous {#miscellaneous}

A few tokens that don't fit neatly into the other categories:

| Token | What It Does |
|-------|-------------|
| `[comment <text>]` | Compiler comment — ignored in output |
| `[repeat]<c>[<n>]` | Repeat character `<c>` exactly `<n>` times |
| `[repeatseq <len>]<s>[<n>]` | Repeat string `<s>` (of length `<len>`) `<n>` times |
| `[pause]` | Half-second pause |
| `[bell]` | Beep on the caller's terminal |
| `[sysopbell]` | Beep on the local console only |
| `[tune]<name>` | Play a named tune on the local console |
| `[hangup]` | Disconnect the caller |
| `[tag_read]<f>` | Load tagged files from a file |
| `[tag_write]<f>` | Save tagged files to a file |
| `[delete]<f>` | Delete a file from disk |
| `[log]<s>` | Write to the system log |

---

## MECCA vs. Pipe Codes {#mecca-vs-pipe-codes}

Maximus has two display code systems, and they coexist:

**MECCA tokens** (`[token]`) are the display file language. They're compiled
into `.bbs` bytecodes by the MECCA compiler. They give you colors, cursor
control, conditionals, flow control, questionnaires, and live data
substitution. Use MECCA when you're building full-screen displays, menu
art, welcome screens, or anything that needs logic.

**Pipe codes** (`|XX`, `$XX`) are runtime-interpreted codes that work in
plain-text strings — menu titles, language file entries, MEX `print()`
calls, and also in `.bbs` files. They're simpler: colors, info codes, and
formatting operators. No conditionals, no flow control, no input.

The two systems complement each other:

| Feature | MECCA | Pipe Codes |
|---------|-------|------------|
| **Where** | `.mec` source → compiled `.bbs` files | Anywhere: prompts, menus, language strings, MEX, `.bbs` |
| **Colors** | Named tokens (`[yellow]`, `[lightcyan on blue]`) | Numeric (`\|14`, `\|17`) and theme (`\|pr`) |
| **Cursor control** | `[cls]`, `[locate]`, `[cleol]`, `[up]`/`[down]` | `\|CL`, `[Xnn`, `[Ynn` |
| **Live data** | `[user]`, `[remain]`, `[msg_carea]` | `\|UN`, `\|TL`, `\|MA` |
| **Conditionals** | Full: privilege, terminal, time, area, keys | None |
| **Flow control** | Labels, goto, quit, link | None |
| **Input** | `[menu]`, `[readln]`, questionnaire logging | None |
| **Formatting** | `[repeat]`, `[repeatseq]` | `$R`, `$C`, `$L`, `$D`, `$X` |

For new display files, MECCA remains the primary tool. For inline content in
config files and language strings, pipe codes are the natural choice. Both
are documented and maintained.

---

## See Also

- [MECCA Compiler]({{ site.baseurl }}{% link mecca-compiler.md %}) — command-line usage, flags,
  and error messages
- [Display Files]({{ site.baseurl }}{% link display-files.md %}) — where display files live and
  how they're loaded
- [Display Codes]({{ site.baseurl }}{% link display-codes.md %}) — pipe codes, info codes, and
  formatting operators
- [Screen Types]({{ site.baseurl }}{% link display-screen-types.md %}) — TTY, ANSI, and AVATAR
  terminal modes
- [ANSI Art & RIP Graphics]({{ site.baseurl }}{% link display-ansi-rip.md %}) — art workflows
  and RIP status
