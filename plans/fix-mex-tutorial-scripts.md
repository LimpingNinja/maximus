# Fix Plan: MEX Tutorial Scripts (Lessons 2, 3, 6, 7, 10)

**Date:** 2026-03-07
**Status:** **Completed** — 2026-03-07
**Scope:** Bug fixes in MEX tutorial `.mex` scripts and their corresponding wiki `.md` documentation

---

## Summary

Five of the ten MEX tutorial scripts have bugs ranging from UTF-8 encoding issues on CP437 terminals to incorrect `shell()` script references and box-drawing alignment problems. This plan documents each bug, its root cause, and the specific line-by-line fixes required across both the `.mex` source files and the wiki `.md` code blocks.

**Files affected:** 15 total (5 `.mex` in `resources/m/learn/`, 5 `.mex` in `resources/install_tree/scripts/learn/`, 5 `.md` in `docs/wiki/`)

---

## Research Findings: MEX Script Chaining via shell()

### The shell() Function

From `docs/legacy/max_mex.txt` (lines 9012-9076):

```
Prototype   int shell(int: method, string: cmd);
Return Val. The return value of the program, or -1 if the program
            could not be executed.
```

`shell()` invokes an external program. The `method` parameter accepts `IOUTSIDE_RUN`, `IOUTSIDE_DOS`, or `IOUTSIDE_REREAD` constants (combinable via bitwise OR).

### The `:` Prefix Syntax for MEX Scripts

From `docs/legacy/max_mex.txt` (lines 369-379):

> "To indicate to Maximus that it is to run a MEX program instead of a .bbs file, simply add a colon (":") to the beginning of the filename."

Example from the documentation:

```
Uses Logo   :M\Test
```

> "instead of displaying the standard \max\misc\logo.bbs file, Maximus would instead run the \max\m\test.vm program."

This confirms that `shell(0, ":scriptname")` is the correct way to chain MEX scripts. The `:` prefix tells Maximus to treat the argument as a MEX `.vm` script name (without the `.vm` extension). When the sub-script finishes, control returns to the calling script.

The wiki documentation for Lesson 7 (`docs/wiki/mex-learn-menus.md` line 275-278) also confirms this:

> `shell(0, ":profile")` runs another MEX script from within your script. The `:` prefix tells Maximus to treat the argument as a MEX script name (without the `.vm` extension). When the sub-script finishes, control returns to your menu loop.

### What Happens When the Script Doesn't Exist

From the documentation (lines 329-332): "If the file is not found, an appropriate error message will be displayed (and also placed in the system log)." The `shell()` function returns `-1` if the program could not be executed.

The implementation in `src/max/mex_runtime/mexxtrn.c` (line 89) passes the command to `Outside()` in `src/max/core/max_xtrn.c`, which handles the actual execution. A missing `.vm` file results in an error that can disrupt the BBS session.

### File I/O Documentation Notes

From `docs/legacy/max_mex.txt` (lines 8386-8393):

- `IOPEN_APPEND` (0x08): Append to the end of the file
- `IOPEN_CREATE` (0x01): Create the file if it does not exist, or truncate if it does

**Important:** `IOPEN_CREATE` alone *truncates* an existing file. When combined with `IOPEN_APPEND`, the append flag takes precedence for write position, and the create flag only creates the file if missing (no truncation occurs). The flag combination `IOPEN_APPEND + IOPEN_CREATE` = `0x09` has no bit overlap and is correct.

`writeln()` (lines 10292-10307): Returns the number of characters written. If less than `strlen(s)`, only a partial write occurred (disk full or I/O error). Automatically appends a newline.

---

## Lesson 2: Input & Output -- Em-Dash Encoding

**Files:**
- `resources/m/learn/learn-input-output.mex`
- `docs/wiki/mex-learn-input-output.md`

### Issue

UTF-8 em-dash characters (U+2014, 3-byte sequence `0xE2 0x80 0x94`) display as garbled text on CP437 BBS terminals. The CP437 codepage has no mapping for this character, so each UTF-8 byte is individually interpreted as a CP437 glyph.

### Affected Lines -- `learn-input-output.mex`

| Line | Current Text | Proposed Text |
|------|-------------|---------------|
| 1 | `// learn-input-output.mex --- MEX Learn Tutorial: Lesson 2 - Input and Output` | `// learn-input-output.mex -- MEX Learn Tutorial: Lesson 2 - Input and Output` |
| 19 | `print("\|03Quick --- three questions. No wrong answers.\n\n");` | `print("\|03Quick -- three questions. No wrong answers.\n\n");` |
| 49 | `print("Yes --- and honestly, you already are.\n");` | `print("Yes -- and honestly, you already are.\n");` |
| 51 | `print("No --- but you're calling one at 2 AM, so...\n");` | `print("No -- but you're calling one at 2 AM, so...\n");` |

### Affected Lines -- `mex-learn-input-output.md`

| Line | Current Text | Proposed Text |
|------|-------------|---------------|
| 6 | `description: "Lesson 2 --- Input, output, and the art of conversation at 2400 baud"` | `description: "Lesson 2 -- Input, output, and the art of conversation at 2400 baud"` |
| 127 | `print("\|03Quick --- three questions. No wrong answers.\n\n");` | `print("\|03Quick -- three questions. No wrong answers.\n\n");` |
| 157 | `print("Yes --- and honestly, you already are.\n");` | `print("Yes -- and honestly, you already are.\n");` |
| 159 | `print("No --- but you're calling one at 2 AM, so...\n");` | `print("No -- but you're calling one at 2 AM, so...\n");` |

**Note:** Em-dashes in wiki *prose* (outside code blocks) are fine -- those render in web browsers which handle UTF-8. Only code blocks and frontmatter need fixing since they represent what appears on a CP437 terminal.

---

## Lesson 3: User Record -- Box-Drawing Display Broken

**Files:**
- `resources/m/learn/learn-user-record.mex`
- `docs/wiki/mex-learn-user-record.md`

### Issue

The profile card uses CP437 box-drawing characters for a framed display. Three problems exist:

1. **Missing side borders on data rows.** The `row()` function at lines 7-10 prints data as `|03  label: |15 value |07\n` -- no `0xBA` vertical borders on either side. The top/bottom frame, header, and separator lines all have `0xBA` borders, but the actual data rows hang outside the box.

2. **Misaligned header.** Line 36 reads:
   ```
   print("|11\xBA |14Caller Profile              |11\xBA\n");
   ```
   The top frame at line 35 is `0xC9` + 38x `0xCD` + `0xBB` = 40 visible characters. Between the two `0xBA` on the header line, there should be 38 visible characters. Currently there are only 29 (1 space + 14 chars + 14 trailing spaces), making the right `0xBA` fall 9 positions short.

3. **Blank separator rows have correct `0xBA` borders** (lines 46, 53, 60) but the data rows between them do not, breaking the visual box.

### Fix Approach: MCI Format Operators + CP437 Hex Escapes

Instead of manually typing 38 `═` characters or using a padding loop in MEX, this fix uses **MCI format operators** — the same display code system documented in the wiki. This makes the script both shorter and a better tutorial, since it teaches MCI codes alongside MEX programming.

**MCI operators used (from [Format Operators](docs/wiki/mci-format-operators.md)):**

| Operator | Type | Description |
|----------|------|-------------|
| `$D##C` | Immediate output | Duplicate character `C` exactly `##` times |
| `$X##C` | Immediate output | Fill character `C` until cursor reaches column `##` |
| `\|##` | Color code | Set foreground (`00`-`15`) or background (`16`-`23`) color |

**Why these operators work here:**
- `$D` and `$X` are "immediate output" operators — they produce characters directly without requiring a following `\|XY` info code. This is critical because our row content comes from MEX string variables, not MCI info codes.
- Padding operators like `$R##` affect only the *next* `\|XY` expansion, so they can't pad arbitrary MEX string content. `$X##C` solves this by filling based on actual cursor position, automatically handling variable-width content.
- MCI codes are processed when output through `print()` — confirmed in [Display Codes](docs/wiki/display-codes.md:46): "Display codes can be used in MEX scripts (via `print()`)".
- `\xHH` hex escapes in MEX string literals produce single CP437 bytes that `$D` and `$X` can use as the fill character.

**CP437 Box-Drawing Hex Reference:**

| Character | CP437 Hex | Name |
|-----------|-----------|------|
| `\xC9` | `0xC9` | Double top-left corner (`╔`) |
| `\xCD` | `0xCD` | Double horizontal line (`═`) |
| `\xBB` | `0xBB` | Double top-right corner (`╗`) |
| `\xBA` | `0xBA` | Double vertical line (`║`) |
| `\xCC` | `0xCC` | Double left T-junction (`╠`) |
| `\xB9` | `0xB9` | Double right T-junction (`╣`) |
| `\xC8` | `0xC8` | Double bottom-left corner (`╚`) |
| `\xBC` | `0xBC` | Double bottom-right corner (`╝`) |

### Box Geometry

The frame is 40 visible characters wide: `\xC9` + 38x `\xCD` + `\xBB`. All content between the side `\xBA` borders must be exactly 38 visible characters (padded with spaces as needed). Column 39 (0-based) is the rightmost printable position before the newline.

### How `$D` and `$X` Replace Manual Repetition and Padding

**Horizontal lines — `$D38\xCD`:**

The old code manually typed 38 `═` characters per line (or used UTF-8 multi-byte equivalents). The MCI `$D##C` operator replaces this with a 7-character sequence:

```
OLD: \xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB
NEW: \xC9$D38\xCD\xBB
```

The MCI layer sees `$D`, parses the two-digit count `38`, takes the next byte (`\xCD` = CP437 `═`) as the character, and emits 38 copies.

**Content padding — `$X39 `:**

The old `row()` function had no borders. The v1 fix used a manual padding loop (`while (i < pad) { print(" "); ... }`). The MCI `$X##C` operator eliminates that entirely:

```
OLD (v1 fix):  pad := BOX_INNER - 2 - strlen(label) - 2 - strlen(value);
               i := 0; while (i < pad) { print(" "); i := i + 1; }

NEW:           $X39    (fills spaces up to column 39 automatically)
```

`$X` checks the current cursor column. After printing `\xBA` + content of any length, `$X39 ` fills spaces up to column 39 (0-based), so the closing `\xBA` always lands at column 40. No length calculation needed.

**Blank separator rows — `$X39 `:**

Same technique: print `\xBA`, then `$X39 ` fills 38 spaces, then `\xBA`:

```
OLD: print("|11\xBA                                      \xBA|07\n");
NEW: print("|11\xBA$X39 \xBA|07\n");
```

### Before / After — `learn-user-record.mex`

#### BEFORE (current code)

```c
// learn-user-record.mex — MEX Learn Tutorial: Lesson 3 - User Record
// Source: docs/wiki/mex-learn-user-record.md
// This script accompanies the Maximus wiki tutorial series.

#include <max.mh>

void row(string: label, string: value)
{
  print("|03  ", label, ": |15", value, "|07\n");
}

string video_name(char: mode)
{
  if (mode = VIDEO_ANSI)
    return "ANSI";
  else if (mode = VIDEO_AVATAR)
    return "Avatar";

  return "TTY";
}

string help_name(char: level)
{
  if (level = HELP_NOVICE)
    return "Novice";
  else if (level = HELP_EXPERT)
    return "Expert";

  return "Regular";
}

int main()
{
  // Header
  print("\n|11╔══════════════════════════════════════╗\n");
  print("|11║ |14Caller Profile              |11║\n");
  print("|11╠══════════════════════════════════════╣|07\n");

  // Identity
  row("Name",     usr.name);
  if (usr.alias <> "")
    row("Alias",  usr.alias);
  row("City",     usr.city);

  // Session
  print("|11║                                      ║|07\n");
  row("Calls",    uitostr(usr.times));
  row("Today",    uitostr(usr.call));
  row("Online",   uitostr(usr.time) + " min");
  row("Posted",   ltostr(usr.msgs_posted));
  row("Read",     ltostr(usr.msgs_read));

  // Transfer stats
  print("|11║                                      ║|07\n");
  row("Files up",   ultostr(usr.nup));
  row("Files down", ultostr(usr.ndown));
  row("KB up",      ultostr(usr.up));
  row("KB down",    ultostr(usr.down));

  // Settings
  print("|11║                                      ║|07\n");
  row("Video",    video_name(usr.video));
  row("Help",     help_name(usr.help));
  row("Screen",   itostr((int)usr.width) + "x" + itostr((int)usr.len));

  // Footer
  print("|11╚══════════════════════════════════════╝|07\n\n");

  return 0;
}
```

**Problems:** (1) `row()` has no `║` borders — data rows hang outside the box. (2) Header has only 29 visible chars between `║` borders (off by 9). (3) UTF-8 multi-byte box chars risk encoding issues on CP437 terminals. (4) 38 manually-typed `═` characters per horizontal line.

#### AFTER (proposed code with MCI operators)

```c
// learn-user-record.mex -- MEX Learn Tutorial: Lesson 3 - User Record
// Source: docs/wiki/mex-learn-user-record.md
// This script accompanies the Maximus wiki tutorial series.
//
// Demonstrates: user record fields, helper functions, MCI display codes
//   - $D##C  repeats character C exactly ## times  (immediate output)
//   - $X##C  fills character C up to column ##      (immediate output)
//   - |##    sets foreground/background color
//   - \xHH   CP437 hex byte in string literals

#include <max.mh>

void row(string: label, string: value)
{
  // MCI does the padding: $X39 fills spaces to column 39 so the
  // closing \xBA border always aligns at column 40.
  print("|11\xBA|03  " + label + ": |15" + value + "$X39 |11\xBA|07\n");
}

string video_name(char: mode)
{
  if (mode = VIDEO_ANSI)
    return "ANSI";
  else if (mode = VIDEO_AVATAR)
    return "Avatar";

  return "TTY";
}

string help_name(char: level)
{
  if (level = HELP_NOVICE)
    return "Novice";
  else if (level = HELP_EXPERT)
    return "Expert";

  return "Regular";
}

int main()
{
  // Header — $D38\xCD repeats the ═ character 38 times
  print("\n|11\xC9$D38\xCD\xBB\n");
  print("|11\xBA |14Caller Profile$X39 |11\xBA\n");
  print("|11\xCC$D38\xCD\xB9|07\n");

  // Identity
  row("Name",     usr.name);
  if (usr.alias <> "")
    row("Alias",  usr.alias);
  row("City",     usr.city);

  // Session separator — $X39 fills spaces to align the right border
  print("|11\xBA$X39 \xBA|07\n");
  row("Calls",    uitostr(usr.times));
  row("Today",    uitostr(usr.call));
  row("Online",   uitostr(usr.time) + " min");
  row("Posted",   ltostr(usr.msgs_posted));
  row("Read",     ltostr(usr.msgs_read));

  // Transfer stats separator
  print("|11\xBA$X39 \xBA|07\n");
  row("Files up",   ultostr(usr.nup));
  row("Files down", ultostr(usr.ndown));
  row("KB up",      ultostr(usr.up));
  row("KB down",    ultostr(usr.down));

  // Settings separator
  print("|11\xBA$X39 \xBA|07\n");
  row("Video",    video_name(usr.video));
  row("Help",     help_name(usr.help));
  row("Screen",   itostr((int)usr.width) + "x" + itostr((int)usr.len));

  // Footer
  print("|11\xC8$D38\xCD\xBC|07\n\n");

  return 0;
}
```

### Key Changes Summary

| Area | Before | After |
|------|--------|-------|
| Comment line 1 | em-dash (`—`) | `--` double hyphen |
| Comment block | No MCI docs | Documents `$D`, `$X`, `\|##`, `\xHH` |
| `row()` function | No borders, no padding, multi-arg `print()` | Single `print()` with `\xBA` borders + `$X39 ` MCI padding |
| Horizontal lines | 38 manually-typed `═` per line (UTF-8 multi-byte) | `$D38\xCD` — 7-char MCI sequence per line |
| Header text line | 29 visible chars between borders (off by 9) | `$X39 ` auto-pads to correct width |
| Blank separator rows | 38 manually-typed spaces between `\xBA` borders | `$X39 ` auto-fills spaces |
| Box chars encoding | Direct UTF-8 multi-byte chars | `\xHH` hex escapes for CP437 single-byte chars |
| `#define BOX_INNER` | Not present (was in v1 fix) | Not needed — `$X39` handles alignment implicitly |
| Padding loop in `row()` | Not present (was manual loop in v1 fix) | Not needed — `$X39 ` replaces the entire loop |

### Why MCI Operators Are Better Than a Manual Padding Loop

The previous fix approach (v1) added a manual padding loop to `row()`:

```c
// v1 approach (replaced):
pad := BOX_INNER - 2 - strlen(label) - 2 - strlen(value);
i := 0;
while (i < pad) { print(" "); i := i + 1; }
```

This had several downsides for a tutorial script:
1. **Fragile width math** — if a value contained MCI color codes, `strlen()` would count the non-printing bytes, producing incorrect padding
2. **Verbose** — 5 extra lines of boilerplate per function
3. **Doesn't teach MCI** — misses the opportunity to demonstrate Maximus display codes

The MCI approach (`$X39 `) is:
1. **Cursor-aware** — `$X` checks the actual terminal cursor column (which ignores non-printing color codes), so padding is always correct
2. **One token** — replaces 5 lines with 4 characters
3. **Educational** — teaches `$D` and `$X` alongside MEX, which is the whole point of the tutorial series

### Why Hex Escapes Instead of Direct Unicode

The current `.mex` file contains UTF-8-encoded box-drawing characters (e.g., top-left corner = 3 bytes: `0xE2 0x95 0x94`). While these render correctly in the user's current setup, using `\xHH` hex escapes (e.g., `\xC9` for the top-left corner) is more robust because:

- Each character is exactly 1 byte (matching CP437's expectations)
- The MEX compiler handles `\xHH` escapes in string literals
- This approach is already demonstrated in `learn-menus.mex` line 14: `ui_fill_rect(1, 1, 40, 1, 0xCD, ...)`
- It eliminates any risk of a text editor re-encoding the file and breaking the characters
- `$D##C` and `$X##C` take a single byte as the `C` argument — `\xCD` (1 byte) works; a UTF-8 multi-byte sequence would not

### Affected Lines -- `mex-learn-user-record.md`

The wiki doc contains the same code in a fenced code block (around lines 111-177). The entire code block must be replaced with the MCI-based version shown above. Additionally:

| Line | Current Text | Proposed Text |
|------|-------------|---------------|
| 5 | `description: "Lesson 3 --- Peeking at the user record..."` | `description: "Lesson 3 -- Peeking at the user record..."` |

The prose paragraph discussing the `row()` helper function (around lines 88-96) should be updated to explain:
- The `$X39 ` MCI operator that handles padding automatically
- The `$D38\xCD` operator that generates horizontal lines
- How `$X` is cursor-position-aware and ignores non-printing color codes
- Link to the [Format Operators](docs/wiki/mci-format-operators.md) wiki page for further reading

---

## Lesson 6: File I/O -- Em-Dash Corrupts Written Data

**Files:**
- `resources/m/learn/learn-file-io.mex`
- `docs/wiki/mex-learn-file-io.md`

### Issue

The guestbook file is created and data *is* written, but entries appear garbled on CP437 terminals due to an em-dash in the entry format string.

### Root Cause Analysis

1. **Em-dash in entry string (PRIMARY).** Line 76:
   ```c
   entry := usr.name + " --- " + message;
   ```
   The em-dash (U+2014) is a 3-byte UTF-8 sequence. When written to file and read back on a CP437 terminal, the bytes render as garbled characters, making entries appear corrupted.

2. **IOPEN flag combination (VERIFIED OK).** From `max_mex.txt` lines 8386-8393:
   - `IOPEN_APPEND` = `0x08`
   - `IOPEN_CREATE` = `0x01`
   - Combined: `0x09` -- distinct bit flags with no overlap. The arithmetic is correct.

3. **File I/O mechanics (VERIFIED OK).** From `max_mex.txt` lines 10292-10307: `writeln()` writes the string and appends a newline automatically. It returns the number of characters written -- if less than `strlen(s)`, a partial write occurred (disk full). The file I/O functions themselves are not causing data loss.

4. **File path (VERIFIED OK).** The relative path `"guestbook.txt"` resolves to the Maximus working directory. No issue here.

**Conclusion:** The "file created but no data written" report is actually "file created, data written, but data *appears* garbled due to em-dash encoding." The fix is simply replacing the em-dash.

### Proposed Fix

Replace the em-dash with `--` in the entry format string.

### Affected Lines -- `learn-file-io.mex`

| Line | Current Text | Proposed Text |
|------|-------------|---------------|
| 1 | `// learn-file-io.mex --- MEX Learn Tutorial: Lesson 6 - File I/O` | `// learn-file-io.mex -- MEX Learn Tutorial: Lesson 6 - File I/O` |
| 75 | `// Build the entry: "Name --- message"` | `// Build the entry: "Name -- message"` |
| 76 | `entry := usr.name + " --- " + message;` | `entry := usr.name + " -- " + message;` |

### Affected Lines -- `mex-learn-file-io.md`

| Line | Current Text | Proposed Text |
|------|-------------|---------------|
| 6 | `description: "Lesson 6 --- File I/O, persistence..."` | `description: "Lesson 6 -- File I/O, persistence..."` |
| 196 | `// Build the entry: "Name --- message"` | `// Build the entry: "Name -- message"` |
| 197 | `entry := usr.name + " --- " + message;` | `entry := usr.name + " -- " + message;` |
| 249 | `entry := usr.name + " --- " + message` (prose) | `entry := usr.name + " -- " + message` |

---

## Lesson 7: Menus -- shell() Calls Reference Wrong Script Names

**Files:**
- `resources/m/learn/learn-menus.mex`
- `docs/wiki/mex-learn-menus.md`

### Issue

All menu options except "Exit" crash the BBS. Selecting options 1-4 causes an immediate failure because the `shell()` calls reference non-existent MEX script names.

### Root Cause

The `shell()` calls use short names that don't match the actual `learn-*` naming convention. The scripts *do exist* -- they just have different names:

| Line | Current Call | Expects `.vm` | Actual Script Name | Actual `.vm` |
|------|-------------|--------------|-------------------|-------------|
| 59 | `shell(0, ":profile")` | `profile.vm` | `learn-user-record.mex` (Lesson 3) | `learn-user-record.vm` |
| 61 | `shell(0, ":guestbook")` | `guestbook.vm` | `learn-file-io.mex` (Lesson 6) | `learn-file-io.vm` |
| 63 | `shell(0, ":trivia")` | `trivia.vm` | `learn-decisions.mex` (Lesson 4) | `learn-decisions.vm` |
| 65 | `shell(0, ":guess")` | `guess.vm` | `learn-loops.mex` (Lesson 5) | `learn-loops.vm` |

The `shell(0, ":name")` syntax is correct per the documentation (see Research Findings above). The `:` prefix tells Maximus to look for a compiled `.vm` file with that name. When the file doesn't exist, `shell()` returns `-1` and Maximus displays an error, which can crash or hang the session.

### Proposed Fix

Update the `shell()` calls to reference the correct script names. No structural changes needed -- the `shell()` mechanism and `:` prefix syntax are working correctly; only the script names were wrong.

### Affected Lines -- `learn-menus.mex`

| Line | Current Text | Proposed Text |
|------|-------------|---------------|
| 1 | `// learn-menus.mex --- MEX Learn Tutorial: Lesson 7 - Menus` | `// learn-menus.mex -- MEX Learn Tutorial: Lesson 7 - Menus` |
| 13 | `// You could use '---' directly --- this just shows that hex codes work.` | `// You could use the char directly -- this just shows that hex codes work.` |
| 59 | `shell(0, ":profile");` | `shell(0, ":learn-user-record");` |
| 61 | `shell(0, ":guestbook");` | `shell(0, ":learn-file-io");` |
| 63 | `shell(0, ":trivia");` | `shell(0, ":learn-decisions");` |
| 65 | `shell(0, ":guess");` | `shell(0, ":learn-loops");` |

### Before / After Code (lines 58-67)

**Before:**
```c
    if (choice = 1)
      shell(0, ":profile");
    else if (choice = 2)
      shell(0, ":guestbook");
    else if (choice = 3)
      shell(0, ":trivia");
    else if (choice = 4)
      shell(0, ":guess");
    else if (choice = 5 or choice = 0)
      choice := 0;
```

**After:**
```c
    if (choice = 1)
      shell(0, ":learn-user-record");
    else if (choice = 2)
      shell(0, ":learn-file-io");
    else if (choice = 3)
      shell(0, ":learn-decisions");
    else if (choice = 4)
      shell(0, ":learn-loops");
    else if (choice = 5 or choice = 0)
      choice := 0;
```

### Affected Lines -- `mex-learn-menus.md`

| Line | Current Text | Proposed Text |
|------|-------------|---------------|
| 5 | `description: "Lesson 7 --- Lightbar menus..."` | `description: "Lesson 7 -- Lightbar menus..."` |

The code block in the wiki (around lines 182-251) needs the same four `shell()` name corrections and two em-dash fixes as the `.mex` file.

The prose section around lines 275-279 that explains `shell()`:

**Before:**
> `shell(0, ":profile")` runs another MEX script from within your script. The `:` prefix tells Maximus to treat the argument as a MEX script name (without the `.vm` extension). When the sub-script finishes, control returns to your menu loop.

**After:**
> `shell(0, ":learn-user-record")` runs another compiled MEX script from within your script. The `:` prefix tells Maximus to treat the argument as a MEX `.vm` script name (without the extension) -- see `docs/legacy/max_mex.txt` section 10.5.3. The script name must match the compiled `.vm` filename exactly. In this tutorial, all scripts follow the `learn-*` naming convention. When the sub-script finishes, control returns to your menu loop.

**Note:** The "why it crashes" explanation is that the original short names (`profile`, `guestbook`, `trivia`, `guess`) didn't match the `learn-*` naming convention used when the tutorial scripts were created. The `.vm` files simply didn't exist under those names.

---

## Lesson 10: Mini-Game -- High Scores Don't Display Correctly

**Files:**
- `resources/m/learn/learn-mini-game.mex`
- `docs/wiki/mex-learn-mini-game.md`

### Issue

After multiple playthroughs, high scores appear to not save. New scores are appended correctly to the file, but only the oldest entries are ever displayed.

### Root Cause Analysis

1. **`show_scores()` reads the FIRST 5 lines only (PRIMARY).** Lines 35-41:
   ```c
   while (readln(fd, line) = 0 and count < 5)
   {
     print("|03  ", line, "\n");
     count := count + 1;
   }
   ```
   This reads from the beginning of the file and stops after 5 entries. Since `save_score()` uses `IOPEN_APPEND`, new scores go to the END of the file. After 5 games, the display is permanently stuck showing the first 5 scores. It *looks* like scores aren't saving, but they are -- they're just never displayed.

   Compare with `learn-file-io.mex` (Lesson 6) which correctly implements a two-pass read to show the LAST N entries (lines 24-57). The mini-game should use the same pattern.

2. **Em-dash in score entry string.** Line 52:
   ```c
   entry := usr.name + " --- " + itostr(moves) + " moves";
   ```
   Same UTF-8 encoding issue as Lessons 2 and 6. Scores display with garbled characters.

3. **IOPEN flag combination (VERIFIED OK).** Same as Lesson 6 -- `IOPEN_APPEND + IOPEN_CREATE` = `0x09`, valid bit combination.

4. **No write confirmation.** The `save_score()` function doesn't verify the write succeeded. If `open()` returns -1, it silently fails. The caller sees "Score saved!" regardless (line 350 in `main()` prints this unconditionally after calling `save_score()`).

### Proposed Fix

#### Fix 1: Replace em-dash with `--`

**`learn-mini-game.mex`:**

| Line | Current Text | Proposed Text |
|------|-------------|---------------|
| 1 | `// learn-mini-game.mex --- MEX Learn Tutorial: Lesson 10 - Mini Game` | `// learn-mini-game.mex -- MEX Learn Tutorial: Lesson 10 - Mini Game` |
| 52 | `entry := usr.name + " --- " + itostr(moves) + " moves";` | `entry := usr.name + " -- " + itostr(moves) + " moves";` |

#### Fix 2: Rewrite `show_scores()` to display the LAST 5 entries

Replace lines 23-45 with a two-pass approach matching the pattern from Lesson 6:

**Before (lines 23-45):**
```c
void show_scores()
{
  int: fd;
  int: count;
  string: line;

  fd := open(SCORE_FILE, IOPEN_READ);

  if (fd = -1)
    return;

  print("|11--- High Scores ---|07\n");
  count := 0;

  while (readln(fd, line) = 0 and count < 5)
  {
    print("|03  ", line, "\n");
    count := count + 1;
  }

  close(fd);
  print("|11-------------------|07\n\n");
}
```

**After:**
```c
void show_scores()
{
  int: fd;
  int: count;
  string: line;

  fd := open(SCORE_FILE, IOPEN_READ);

  if (fd = -1)
    return;

  // First pass: count total lines
  count := 0;

  while (readln(fd, line) = 0)
    count := count + 1;

  close(fd);

  if (count = 0)
    return;

  // Second pass: skip to the last 5 entries
  fd := open(SCORE_FILE, IOPEN_READ);

  if (count > 5)
  {
    int: skip;
    skip := count - 5;

    while (skip > 0)
    {
      readln(fd, line);
      skip := skip - 1;
    }
  }

  print("|11--- High Scores ---|07\n");

  while (readln(fd, line) = 0)
    print("|03  ", line, "\n");

  close(fd);
  print("|11-------------------|07\n\n");
}
```

#### Fix 3: Make `save_score()` return success/failure

**Before (lines 47-61):**
```c
void save_score()
{
  int: fd;
  string: entry;

  entry := usr.name + " -- " + itostr(moves) + " moves";

  fd := open(SCORE_FILE, IOPEN_APPEND + IOPEN_CREATE);

  if (fd <> -1)
  {
    writeln(fd, entry);
    close(fd);
  }
}
```

**After:**
```c
int save_score()
{
  int: fd;
  string: entry;

  entry := usr.name + " -- " + itostr(moves) + " moves";

  fd := open(SCORE_FILE, IOPEN_APPEND + IOPEN_CREATE);

  if (fd = -1)
    return 0;

  writeln(fd, entry);
  close(fd);
  return 1;
}
```

And update the call site in `main()` (around line 349-350):

**Before:**
```c
        save_score();
        print("|10Score saved!|07\n\n");
```

**After:**
```c
        if (save_score())
          print("|10Score saved!|07\n\n");
        else
          print("|12Could not save score.|07\n\n");
```

#### Fix 4: Em-dash in comment lines throughout the file

| Line | Current Text | Proposed Text |
|------|-------------|---------------|
| 263 | `// Escape pressed --- confirm quit` | `// Escape pressed -- confirm quit` |
| 337 | `// Walk through the exit --- you win!` | `// Walk through the exit -- you win!` |

### Affected Lines -- `mex-learn-mini-game.md`

| Line | Current Text | Proposed Text |
|------|-------------|---------------|
| 6 | `description: "Lesson 10 --- The capstone..."` | `description: "Lesson 10 -- The capstone..."` |
| 109 | `while (readln(fd, line) = 0 and count < 5)` | Full two-pass replacement (see code block above) |
| 124 | `entry := usr.name + " --- " + itostr(moves) + " moves";` | `entry := usr.name + " -- " + itostr(moves) + " moves";` |
| 335 | `// Escape pressed --- confirm quit` | `// Escape pressed -- confirm quit` |
| 409 | `// Walk through the exit --- you win!` | `// Walk through the exit -- you win!` |

The wiki code block (lines 76-442) needs the same `show_scores()` replacement and `save_score()` return-value changes as the `.mex` file.

The prose at line 509 (`show_scores() reads the first five lines`) should be updated to say it reads the *last* five lines.

---

## Cross-Cutting: UTF-8 Em-Dash Replacement Summary

The em-dash (U+2014) to `--` replacement is needed across all affected files. This table summarizes every instance:

### In `.mex` files (affects terminal display)

| File | Line(s) | Context |
|------|---------|---------|
| `learn-input-output.mex` | 1, 19, 49, 51 | Comment + 3 print strings |
| `learn-user-record.mex` | 1 | Comment only (rest of file is rewritten) |
| `learn-file-io.mex` | 1, 75, 76 | Comment + entry format string |
| `learn-menus.mex` | 1, 13 | Comments only |
| `learn-mini-game.mex` | 1, 52, 263, 337 | Comment + score entry string + 2 comments |

### In `.md` wiki files (affects code blocks only -- prose em-dashes are OK)

| File | Line(s) | Context |
|------|---------|---------|
| `mex-learn-input-output.md` | 6, 127, 157, 159 | Frontmatter + 3 code block lines |
| `mex-learn-user-record.md` | 5 | Frontmatter |
| `mex-learn-file-io.md` | 6, 196, 197, 249 | Frontmatter + code block + prose code reference |
| `mex-learn-menus.md` | 5 | Frontmatter |
| `mex-learn-mini-game.md` | 6, 124, 335, 409 | Frontmatter + code block lines |

---

## Install Tree Copies

The `resources/install_tree/scripts/learn/` directory contains copies of all tutorial scripts. These must be updated to match their counterparts in `resources/m/learn/`:

| Install Tree File | Mirrors |
|-------------------|---------|
| `resources/install_tree/scripts/learn/learn-input-output.mex` | `resources/m/learn/learn-input-output.mex` |
| `resources/install_tree/scripts/learn/learn-user-record.mex` | `resources/m/learn/learn-user-record.mex` |
| `resources/install_tree/scripts/learn/learn-file-io.mex` | `resources/m/learn/learn-file-io.mex` |
| `resources/install_tree/scripts/learn/learn-menus.mex` | `resources/m/learn/learn-menus.mex` |
| `resources/install_tree/scripts/learn/learn-mini-game.mex` | `resources/m/learn/learn-mini-game.mex` |

---

## Complete File Modification Checklist

### MEX Scripts (`resources/m/learn/`)

- [ ] `learn-input-output.mex` -- Replace 4 em-dashes with `--`
- [ ] `learn-user-record.mex` -- Rewrite: use MCI `$D38\xCD` for horizontal lines, `$X39 ` for content padding in `row()` and separator rows, `\xBA` borders, `\xHH` hex escapes for box-drawing, fix em-dash in comment
- [ ] `learn-file-io.mex` -- Replace 3 em-dashes with `--` (comment + entry format)
- [ ] `learn-menus.mex` -- Fix 4 `shell()` calls to use correct `learn-*` script names, fix 2 em-dashes
- [ ] `learn-mini-game.mex` -- Rewrite `show_scores()` for two-pass read, change `save_score()` to return int, fix 4 em-dashes, add write verification at call site

### Install Tree Copies (`resources/install_tree/scripts/learn/`)

- [ ] `learn-input-output.mex` -- Mirror changes from above
- [ ] `learn-user-record.mex` -- Mirror changes from above
- [ ] `learn-file-io.mex` -- Mirror changes from above
- [ ] `learn-menus.mex` -- Mirror changes from above
- [ ] `learn-mini-game.mex` -- Mirror changes from above

### Wiki Documentation (`docs/wiki/`)

- [ ] `mex-learn-input-output.md` -- Fix 4 em-dashes in frontmatter + code block
- [ ] `mex-learn-user-record.md` -- Replace code block with MCI-based CP437 version (using `$D`/`$X` operators), fix frontmatter em-dash, update prose about `row()` function and MCI format operators
- [ ] `mex-learn-file-io.md` -- Fix 4 em-dashes in frontmatter + code block + prose
- [ ] `mex-learn-menus.md` -- Fix 4 `shell()` names + 2 em-dashes in code block, fix frontmatter em-dash, update `shell()` prose to explain `learn-*` naming
- [ ] `mex-learn-mini-game.md` -- Replace `show_scores()` and `save_score()` in code block, fix 4 em-dashes, update prose about score reading

**Total files modified: 15**
