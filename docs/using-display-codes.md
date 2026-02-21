# Using Display Codes

Maximus NG supports **display codes** — short sequences embedded in prompts,
display files, menu titles, and language strings that are replaced at runtime
with colors, user data, formatted text, or terminal commands.

If you have used display codes on Mystic BBS, Renegade, or similar systems,
you will feel right at home. Maximus uses the same `|` (pipe) prefix for
colors and information codes, and the same `$` prefix for formatting operators.

---

## How Display Codes Work

A display code starts with a **pipe character** (`|`) followed by two
characters that tell Maximus what to display. Codes are **case-sensitive**.

For example, if you place this in a prompt or display file:

```
Welcome to the BBS, |UN!
```

Maximus replaces `|UN` with the current user's name at runtime:

```
Welcome to the BBS, Kevin Morgan!
```

Display codes can be used in:

- **Display files** (`.bbs` / ANSI art)
- **Menu titles and option descriptions** (in TOML menu configs)
- **Language strings** (in `english.toml`)
- **MEX scripts** (via `print()`)

---

## Color Codes (`|##`)

Color codes change the text color using the pipe character followed by a
two-digit number. Colors follow the standard 16-color PC/DOS palette.

### Foreground Colors (`|00` – `|15`)

These set the text (foreground) color without changing the background.

| Code   | Color          |
|--------|----------------|
| `\|00`  | Black          |
| `\|01`  | Blue           |
| `\|02`  | Green          |
| `\|03`  | Cyan           |
| `\|04`  | Red            |
| `\|05`  | Magenta        |
| `\|06`  | Brown          |
| `\|07`  | Light Gray     |
| `\|08`  | Dark Gray      |
| `\|09`  | Light Blue     |
| `\|10`  | Light Green    |
| `\|11`  | Light Cyan     |
| `\|12`  | Light Red      |
| `\|13`  | Light Magenta  |
| `\|14`  | Yellow         |
| `\|15`  | White          |

### Background Colors (`|16` – `|23`)

These set the background color without changing the foreground.

| Code   | Background     |
|--------|----------------|
| `\|16`  | Black          |
| `\|17`  | Blue           |
| `\|18`  | Green          |
| `\|19`  | Cyan           |
| `\|20`  | Red            |
| `\|21`  | Magenta        |
| `\|22`  | Brown          |
| `\|23`  | Light Gray     |

### Blink + Background (`|24` – `|31`)

These set the background color and enable the blink attribute. On terminals
that support ICE colors instead of blinking, these produce bright backgrounds.

| Code   | Background + Blink |
|--------|--------------------|
| `\|24`  | Black + blink      |
| `\|25`  | Blue + blink       |
| `\|26`  | Green + blink      |
| `\|27`  | Cyan + blink       |
| `\|28`  | Red + blink        |
| `\|29`  | Magenta + blink    |
| `\|30`  | Brown + blink      |
| `\|31`  | Light Gray + blink |

### Examples

```
|14Hello, |15|17World|16!
```

Produces: **yellow** "Hello, " followed by **white on blue** "World" then
returns to a **black background** for "!".

### Literal Pipe

To display a literal `|` character, use `||`:

```
Type || to see a pipe symbol.
```

Displays: `Type | to see a pipe symbol.`

---

## Theme Color Codes (`|xx`)

Theme color codes are **two lowercase letters** that refer to a named semantic
color slot defined in `colors.toml`. Instead of hard-coding numeric color
values throughout your display files and prompts, you can use theme codes and
change the entire BBS color scheme from one file.

When Maximus encounters a theme code like `|pr`, it looks up the slot in the
loaded theme and substitutes the configured pipe color string (e.g., `|14`
for yellow, or `|15|17` for white on blue). The substituted codes then flow
through normal color processing.

### Slot Reference

| Code   | Slot Name     | Purpose                              | Default  |
|--------|---------------|--------------------------------------|----------|
| `\|tx`  | text          | Normal body text                     | `\|07`    |
| `\|hi`  | highlight     | Emphasized / important text          | `\|15`    |
| `\|pr`  | prompt        | User-facing prompts                  | `\|14`    |
| `\|in`  | input         | User keystroke echo                  | `\|15`    |
| `\|tf`  | textbox_fg    | Text input field foreground          | `\|15`    |
| `\|tb`  | textbox_bg    | Text input field background          | `\|17`    |
| `\|hd`  | heading       | Section headings                     | `\|11`    |
| `\|lf`  | lightbar_fg   | Lightbar selected item foreground    | `\|15`    |
| `\|lb`  | lightbar_bg   | Lightbar selected item background    | `\|17`    |
| `\|er`  | error         | Error messages                       | `\|12`    |
| `\|wn`  | warning       | Warning messages                     | `\|14`    |
| `\|ok`  | success       | Confirmations / success messages     | `\|10`    |
| `\|dm`  | dim           | De-emphasized text, help text        | `\|08`    |
| `\|fi`  | file_info     | File area descriptions               | `\|03`    |
| `\|sy`  | sysop         | SysOp-only text                      | `\|13`    |
| `\|qt`  | quote         | Quoted message text                  | `\|09`    |
| `\|br`  | border        | Box borders, dividers                | `\|01`    |
| `\|hk`  | hotkey        | Hotkey characters                    | `\|14`    |
| `\|cd`  | default       | Reset to the theme's default color   | `\|07`    |

### Configuration

Theme colors are defined in `colors.toml` under the `[theme.colors]` section:

```toml
[theme.colors]
text         = "|07"
highlight    = "|15"
prompt       = "|14"
input        = "|15"
heading      = "|11"
lightbar_fg  = "|15"
lightbar_bg  = "|17"
error        = "|12"
warning      = "|14"
success      = "|10"
```

Values can be any valid pipe color string, including compound codes:

```toml
lightbar_fg = "|15|17"   # White text on blue background
```

Theme colors can also be edited interactively in **maxcfg** under
**Setup → Global → Default Colors → Theme Colors**.

### Examples

```
|pr Enter your name: |in
```

Displays the prompt in the theme's prompt color, then switches to the input
color for the user's typed response.

```
|hd=== Message Area List ===|tx
```

Displays the heading in the heading color, then returns to normal text color.

```
|er ERROR: |tx File not found.
```

Displays "ERROR:" in the error color, then the description in normal text.

---

## Information Codes (`|XY`)

Information codes are two **uppercase letters** (or special characters) that
are replaced with live data about the BBS, the current user, or the system.

### User Information

| Code   | Description                    |
|--------|--------------------------------|
| `\|UN`  | User name                      |
| `\|UH`  | User handle (alias)            |
| `\|UR`  | User real name                 |
| `\|UC`  | User city / state              |
| `\|UP`  | User home phone                |
| `\|UD`  | User data phone                |
| `\|U#`  | User number (account ID)       |
| `\|US`  | User screen length (lines)     |
| `\|TE`  | Terminal emulation (TTY, ANSI, AVATAR) |

### Call & Activity Statistics

| Code   | Description                    |
|--------|--------------------------------|
| `\|CS`  | Total calls (lifetime)         |
| `\|CT`  | Calls today                    |
| `\|MP`  | Total messages posted          |
| `\|DK`  | Total download KB              |
| `\|FK`  | Total upload KB                |
| `\|DL`  | Total downloaded files         |
| `\|FU`  | Total uploaded files           |
| `\|DT`  | Downloaded KB today            |
| `\|TL`  | Time left (minutes)            |

### System Information

| Code   | Description                    |
|--------|--------------------------------|
| `\|BN`  | BBS / system name              |
| `\|SN`  | Sysop name                     |

### Area Information

| Code   | Description                    |
|--------|--------------------------------|
| `\|MB`  | Current message area name      |
| `\|MD`  | Current message area description |
| `\|FB`  | Current file area name         |
| `\|FD`  | Current file area description  |

### Date & Time

| Code   | Description                    |
|--------|--------------------------------|
| `\|DA`  | Current date (DD Mon YY)       |
| `\|TM`  | Current time (HH:MM)          |
| `\|TS`  | Current time (HH:MM:SS)       |

### Example

```
|14Welcome to |15|BN|14, |15|UN|14!
You have |15|TL|14 minutes remaining.
Current message area: |11|MB
```

Produces something like:

```
Welcome to Maximus BBS, Kevin!
You have 60 minutes remaining.
Current message area: General
```

---

## Formatting Operators (`$...`)

Formatting operators control how the **next** information code is displayed.
They are useful for creating aligned columns, padded fields, and repeated
characters in prompts and display files.

**Note:** `##` is always a two-digit number (`00`–`99`).

### Padding & Alignment

These operators affect the next `|XY` code that follows them:

| Operator  | Description                                        |
|-----------|----------------------------------------------------|
| `$C##`    | Center the next value within `##` columns (spaces) |
| `$L##`    | Left-pad the next value to `##` columns (spaces)   |
| `$R##`    | Right-pad the next value to `##` columns (spaces)  |
| `$T##`    | Trim the next value to `##` characters max         |
| `$c##C`   | Center within `##` columns using character `C`     |
| `$l##C`   | Left-pad to `##` columns using character `C`       |
| `$r##C`   | Right-pad to `##` columns using character `C`      |
| `\|PD`     | Prepend a single space to the next value           |

### Immediate Output

These produce output directly without needing a following code:

| Operator  | Description                                        |
|-----------|----------------------------------------------------|
| `$D##C`   | Duplicate character `C` exactly `##` times         |
| `$X##C`   | Duplicate character `C` until cursor reaches column `##` |

The difference between `$D` and `$X`: `$D` always produces the same number
of characters regardless of cursor position, while `$X` fills *up to* a
specific column — so the number of characters depends on where the cursor
currently is.

### Examples

```
|UN              translates to: "Kevin Morgan"
$R30|UN          translates to: "Kevin Morgan                  "
$C30|UN          translates to: "         Kevin Morgan          "
$L30|UN          translates to: "                  Kevin Morgan"
$D30-            translates to: "------------------------------"
$X30-            translates to: "------------------------------"  (from col 1)
|UN$X30.         translates to: "Kevin Morgan.................."
$c30.|UN         translates to: ".........Kevin Morgan.........."
$r30.|UN         translates to: "Kevin Morgan.................."
$l30.|UN         translates to: "..................Kevin Morgan"
```

### Literal Dollar Sign

To display a literal `$` character, use `$$`:

```
Price: $$5.00
```

Displays: `Price: $5.00`

---

## Terminal Control Codes

These codes send terminal control sequences for screen manipulation. They
require an ANSI-capable terminal (most modern terminals support these).

### Screen & Color

| Code   | Description                              |
|--------|------------------------------------------|
| `\|CL`  | Clear the screen                         |
| `\|CD`  | Reset color to default (ANSI reset)      |
| `\|CR`  | Carriage return + line feed (new line)   |
| `\|BS`  | Destructive backspace                    |

### Cursor Movement

| Code    | Description                              |
|---------|------------------------------------------|
| `[X##`  | Move cursor to column `##`               |
| `[Y##`  | Move cursor to row `##`                  |
| `[A##`  | Move cursor up `##` rows                 |
| `[B##`  | Move cursor down `##` rows               |
| `[C##`  | Move cursor forward `##` columns         |
| `[D##`  | Move cursor back `##` columns            |
| `[K`    | Clear from cursor to end of line         |
| `[0`    | Hide cursor                              |
| `[1`    | Show cursor                              |

**Note:** `##` is always two digits. To move to column 5, use `[X05`.

### Save & Restore

| Code   | Description                              |
|--------|------------------------------------------|
| `\|SA`  | Save cursor position and attributes      |
| `\|RA`  | Restore cursor position and attributes   |
| `\|SS`  | Save entire screen                       |
| `\|RS`  | Restore entire screen                    |

### Example

```
|CL|14Welcome!|CR|CR[X10|11This text starts at column 10.
```

Clears the screen, prints "Welcome!" in yellow, moves down two lines,
positions the cursor at column 10, then prints in light cyan.

---

## Language String Parameters (`|!N` and `|#N`)

Language strings (in `english.toml`) can contain **positional parameters**
that are filled in by the BBS at runtime. There are two expansion modes:

### Early Expansion (`|!N`)

| Code        | Parameter   |
|-------------|-------------|
| `\|!1`–`\|!9` | Parameters 1 through 9 |
| `\|!A`–`\|!F` | Parameters 10 through 15 |

Early-expansion parameters are substituted **before** formatting operators
run. Use `|!N` when the parameter value feeds *into* a formatting operator
as an argument — for example, providing the repeat count for `$D`:

```toml
# |!3 provides the number "75", which becomes the repeat count for $D
box_border = { text = "|br$D|!3\xC4" }
```

### Deferred Expansion (`|#N`)

| Code        | Parameter   |
|-------------|-------------|
| `\|#1`–`\|#9` | Parameters 1 through 9 |
| `\|#A`–`\|#F` | Parameters 10 through 15 |

Deferred-expansion parameters are substituted **during** formatting-operator
processing. Pending format ops (`$L`, `$R`, `$T`, `$C`) are applied to the
resolved value. Use `|#N` when a formatting operator should *wrap* the
parameter output:

```toml
# $L04 left-pads the node number to 4 columns
# $T28$R28 trims the username to 28 chars then right-pads to 28
who_row = { text = "|15 $L04|#2  $T28$R28|#1  |12|!3" }
```

### Mixing Both Modes

Both `|!N` and `|#N` share the same slot numbering. A single language string
can mix both — early slots are replaced first, deferred slots are resolved
later with format ops applied:

```toml
# |#1 = username (deferred, gets padded), |!3 = status (early, plain text)
user_line = { text = "$R20|#1 |!3" }
```

**Note:** Positional parameters are primarily used in language file strings.
Display files and menu titles typically use information codes (`|XY`) instead.

---

## Contexts Where Codes Are Disabled

Display codes are automatically disabled in certain contexts to prevent
user-authored content from being interpreted as codes:

- **Message editor** — text you type in the full-screen or line editor
- **Input fields** — bounded text input prompts
- **Message display** — message body text from other users
- **File descriptions** — uploaded file descriptions

System prompts, menus, status bars, and display files continue to process
all display codes normally.

---

## Quick Reference Card

| Category       | Syntax       | Example          | Result                    |
|----------------|--------------|------------------|---------------------------|
| Foreground     | `\|##`       | `\|14`           | Yellow text               |
| Background     | `\|##`       | `\|17`           | Blue background           |
| Theme color    | `\|xx`       | `\|pr`           | Theme prompt color        |
| User info      | `\|XY`       | `\|UN`           | User's name               |
| System info    | `\|XY`       | `\|BN`           | BBS name                  |
| Right-pad      | `$R##`       | `$R20\|UN`       | Name padded to 20 chars   |
| Center         | `$C##`       | `$C40\|BN`       | BBS name centered in 40   |
| Repeat char    | `$D##C`      | `$D40-`          | 40 dashes                 |
| Fill to column | `$X##C`      | `$X80.`          | Dots to column 80         |
| Clear screen   | `\|CL`       | `\|CL`           | Clears the screen         |
| Goto column    | `[X##`       | `[X01`           | Cursor to column 1        |
| Early param    | `\|!N`       | `\|!1`           | Early-expanded parameter  |
| Deferred param | `\|#N`       | `$R20\|#1`       | Format-op-wrapped param   |
| Literal pipe   | `\|\|`       | `\|\|`           | Displays `\|`             |
| Literal dollar | `$$`         | `$$`             | Displays `$`              |

---

## Compatibility Notes

Maximus NG display codes are designed to be broadly compatible with the
conventions established by Mystic BBS and Renegade:

- **Pipe color codes** (`|00`–`|31`) follow the same numbering as Mystic
  and Renegade.
- **Information codes** use two-letter Mystic-style mnemonics (`|UN`, `|BN`,
  `|TL`, etc.) rather than the older MECCA `[token]` syntax or ENiGMA½
  percent-prefix (`%UN`) syntax.
- **Formatting operators** (`$C`, `$R`, `$L`, `$D`, `$X`, `$T`, `$c`, `$l`,
  `$r`, `|PD`) match Mystic's formatting system.
- **Cursor codes** use bracket-prefix syntax (`[X##`, `[Y##`, `[A##`–`[D##`)
  consistent with Mystic.

Legacy Maximus MECCA display codes (compiled `.bbs` files using AVATAR
bytecodes) continue to work. The new pipe-based codes are an additional
layer that operates on plain-text strings at runtime.
