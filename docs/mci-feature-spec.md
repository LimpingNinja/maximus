# Maximus NG — MCI Display Codes Feature Specification

## Overview

Maximus NG implements Renegade-style pipe color codes, Mystic-style formatting
operators, and Mystic-style BBS/user information MCI codes. These are processed
at runtime through the output pipeline and controlled by a global flag system
with push/pop scoping.

---

## Source Files

| File | Role |
|---|---|
| `src/max/display/mci.h` | Public API, flag definitions, function prototypes |
| `src/max/display/mci.c` | MCI expansion, stripping, flag stack implementation |
| `src/max/core/max_out.c` | Output pipeline integration (`Puts` → `MciMaybeExpandString`) |
| `src/max/core/max_outr.c` | Remote output — pipe color state machine in `Mdm_putc()` |
| `src/max/core/max_outl.c` | Local output — pipe color state machine in `Lputc()` |

---

## API

### Flag Constants

```c
#define MCI_PARSE_PIPE_COLORS 0x00000001UL  /* |## color codes in Mdm_putc/Lputc */
#define MCI_PARSE_MCI_CODES   0x00000002UL  /* |XY info codes in MciExpand */
#define MCI_PARSE_FORMAT_OPS  0x00000004UL  /* $... formatting ops in MciExpand */
#define MCI_PARSE_ALL         (MCI_PARSE_PIPE_COLORS | MCI_PARSE_MCI_CODES | MCI_PARSE_FORMAT_OPS)
```

### Strip Constants

```c
#define MCI_STRIP_COLORS  0x00000001UL  /* Strip |00..|31 */
#define MCI_STRIP_INFO    0x00000002UL  /* Strip |UN, |BN, etc. */
#define MCI_STRIP_FORMAT  0x00000004UL  /* Strip $C##, $D##C, |PD, etc. */
```

### Functions

#### `MciSetParseFlags`

```c
void MciSetParseFlags(unsigned long flags);
```

Set the global parse flags directly. Overwrites all flags.

#### `MciGetParseFlags`

```c
unsigned long MciGetParseFlags(void);
```

Return the current global parse flags.

#### `MciPushParseFlags`

```c
void MciPushParseFlags(unsigned long mask, unsigned long values);
```

Push the current flags onto the stack (max depth 16), then apply: `flags = (flags & ~mask) | (values & mask)`. Used to temporarily disable parsing in scoped contexts (editors, input fields, message display).

#### `MciPopParseFlags`

```c
void MciPopParseFlags(void);
```

Restore flags from the top of the stack.

#### `MciExpand`

```c
size_t MciExpand(const char *in, char *out, size_t out_size);
```

Expand a string containing MCI info codes and formatting operators. Pipe color
codes (`|##`) are **preserved** in the output for the downstream character-level
parser. Returns the number of bytes written (excluding NUL).

Behavior is gated by `g_mci_parse_flags`:
- `MCI_PARSE_MCI_CODES` — enables `|XY` info code expansion
- `MCI_PARSE_FORMAT_OPS` — enables `$...` formatting and `|PD`

Escape handling (always active):
- `||` → emits `||` (preserved for pipe color parser, which converts to literal `|`)
- `$$` → emits `$` (literal dollar)

#### `MciStrip`

```c
size_t MciStrip(const char *in, char *out, size_t out_size, unsigned long strip_flags);
```

Remove MCI sequences from a string. Intended for sanitizing user input where
MCI injection should be prevented. `strip_flags` is a bitmask of
`MCI_STRIP_COLORS`, `MCI_STRIP_INFO`, `MCI_STRIP_FORMAT`.

- `||` is always collapsed to a single `|`.
- Unrecognized sequences pass through unchanged.

---

## Output Pipeline

### Expansion (string level)

```
Puts(s)
  └─ MciMaybeExpandString(s)
       ├─ checks g_mci_parse_flags (MCI_CODES | FORMAT_OPS)
       ├─ fast-path: MciStringHasOps() scan for $, |PD, |[A-Z][A-Z], or |U#
       ├─ if no ops found: returns original string (zero-copy)
       └─ calls MciExpand() into a reusable heap buffer (szMciString)
  └─ Mdm_puts(expanded)   [remote]
  └─ Lputs(expanded)       [local]
```

Expansion happens **once**, in `Puts()` only. `Mdm_puts()` and `Lputs()` do
**not** re-expand. This prevents aliasing bugs where the shared buffer would be
used as both input and output.

### Pipe Color Parsing (character level)

```
Mdm_putc(ch) / Lputc(ch)
  state 0: normal
    ch == '|' → state 1
  state 1: saw '|'
    ch == '|' → emit literal '|', state 0
    ch is digit → store, state 2
    else → emit '|' + ch, state 0
  state 2: saw '|' + digit
    ch is digit → parse code 00..31
      valid → emit AVATAR attr (\x16\x01<attr>), state 0
      invalid → emit '|' + d1 + d2, state 0
    else → emit '|' + d1 + ch, state 0
```

Pipe color parsing is gated by `MCI_PARSE_PIPE_COLORS` in `g_mci_parse_flags`.

---

## Pipe Color Codes (`|##`)

### Foreground (`|00` – `|15`)

| Code | Color |
|---|---|
| `\|00` | Black |
| `\|01` | Blue |
| `\|02` | Green |
| `\|03` | Cyan |
| `\|04` | Red |
| `\|05` | Magenta |
| `\|06` | Brown |
| `\|07` | Light Gray |
| `\|08` | Dark Gray |
| `\|09` | Light Blue |
| `\|10` | Light Green |
| `\|11` | Light Cyan |
| `\|12` | Light Red |
| `\|13` | Light Magenta |
| `\|14` | Yellow |
| `\|15` | White |

Sets foreground color. Preserves current background and blink.

### Background (`|16` – `|23`)

| Code | Color |
|---|---|
| `\|16` | Black bg |
| `\|17` | Blue bg |
| `\|18` | Green bg |
| `\|19` | Cyan bg |
| `\|20` | Red bg |
| `\|21` | Magenta bg |
| `\|22` | Brown bg |
| `\|23` | Light Gray bg |

Sets background color (`code - 16`). Preserves foreground and blink.

### Blink + Background (`|24` – `|31`)

| Code | Color |
|---|---|
| `\|24` | Black bg + blink |
| `\|25` | Blue bg + blink |
| `\|26` | Green bg + blink |
| `\|27` | Cyan bg + blink |
| `\|28` | Red bg + blink |
| `\|29` | Magenta bg + blink |
| `\|30` | Brown bg + blink |
| `\|31` | Light Gray bg + blink |

Sets background color (`code - 24`) and enables blink bit. Preserves foreground.

---

## MCI Information Codes (`|XY`)

All codes are **case-sensitive** (two uppercase letters, except `|U#`).

| Code | Description | Data Source |
|---|---|---|
| `\|BN` | BBS / system name | `ngcfg_get_string_raw("maximus.system_name")` |
| `\|SN` | Sysop name | `ngcfg_get_string_raw("maximus.sysop")` |
| `\|UH` | User handle (alias) | `usr.alias` |
| `\|UN` | User name | `usrname` |
| `\|UR` | User real name | `usr.name` |
| `\|UC` | User city/state | `usr.city` |
| `\|UP` | User home phone | `usr.phone` |
| `\|UD` | User data phone | `usr.dataphone` |
| `\|U#` | User number | `g_user_record_id` (DB record id, set at login) |
| `\|CS` | Total calls (lifetime) | `usr.times` |
| `\|CT` | Calls today | `usr.call` |
| `\|MP` | Total messages posted | `usr.msgs_posted` |
| `\|DK` | Downloaded KB (total) | `usr.down` |
| `\|FK` | Uploaded KB (total) | `usr.up` |
| `\|DL` | Downloaded files (total) | `usr.ndown` |
| `\|FU` | Uploaded files (total) | `usr.nup` |
| `\|DT` | Downloaded KB today | `usr.downtoday` |
| `\|TL` | Time left (minutes) | `timeleft()` |
| `\|US` | User screen length | `usr.len` |
| `\|TE` | Terminal emulation | `usr.video` → `"TTY"`, `"ANSI"`, or `"AVATAR"` |
| `\|DA` | Current date | `strftime("%d %b %y")` |
| `\|TM` | Current time (HH:MM) | `strftime("%H:%M")` |
| `\|TS` | Current time (HH:MM:SS) | `strftime("%H:%M:%S")` |
| `\|MB` | Message area name | `MAS(mah, name)` |
| `\|MD` | Message area description | `MAS(mah, descript)` |
| `\|FB` | File area name | `FAS(fah, name)` |
| `\|FD` | File area description | `FAS(fah, descript)` |

Unrecognized `|XY` codes (two uppercase letters with no mapping) pass through
as literal text. `|U#` is a special case — the `#` character is not uppercase
but is recognized explicitly.

---

## Formatting Operators (`$...`)

Formatting operators are processed by `MciExpand()` when `MCI_PARSE_FORMAT_OPS`
is set.

### Operators That Modify the Next MCI Code

These set pending state that is applied to the next `|XY` expansion:

| Operator | Description |
|---|---|
| `$C##` | Center next expansion within `##` columns (space-padded) |
| `$L##` | Left-pad next expansion to `##` columns (space-padded) |
| `$R##` | Right-pad next expansion to `##` columns (space-padded) |
| `$T##` | Trim next expansion to `##` visible characters |
| `$c##C` | Center next expansion within `##` columns using char `C` |
| `$l##C` | Left-pad next expansion to `##` columns using char `C` |
| `$r##C` | Right-pad next expansion to `##` columns using char `C` |
| `\|PD` | Prepend a single space to the next expansion |

`##` is always a two-digit number (`00`–`99`). Padding/trim calculations use
visible character width, ignoring AVATAR control sequences and pipe color codes.

### Immediate Output Operators

These produce output directly without requiring a subsequent MCI code:

| Operator | Description |
|---|---|
| `$D##C` | Emit character `C` repeated `##` times |
| `$X##C` | Emit character `C` until cursor reaches column `##` |

`$X` uses `current_col` (the remote-side cursor column counter) as the
reference position.

---

## Escape Sequences

| Input | Output | Notes |
|---|---|---|
| `\|\|` | `\|` | MciExpand preserves `\|\|`; pipe parser converts to literal `\|` |
| `$$` | `$` | MciExpand converts directly to `$` |

---

## Contexts Where Parsing Is Disabled

The following contexts push `MCI_PARSE_ALL → 0` at entry and pop at exit,
preventing all MCI/pipe/formatting interpretation of user-authored content:

| Context | File | Function |
|---|---|---|
| Bounded input fields | `src/max/display/ui_field.c` | `ui_edit_field()` |
| Full-screen editor (MaxEd) | `src/max/msg/maxed.c` | `MagnEt()` |
| Line editor (BORED) | `src/max/core/max_bor.c` | `Bored()` |
| Message body display | `src/max/msg/m_read.c` | `ShowMessageLines()` |
| File description display | `src/max/core/max_fbbs.c` | `ShowFileEntry()` (description loop) |

System chrome (status bars, headers, menus, prompts) continues to use AVATAR
sequences directly and is unaffected by the push/pop.

---

## Visible Width Calculation

`MciExpand` tracks cursor column internally (`cur_col`, seeded from
`current_col`) for `$X` calculations. Visible width helpers skip:

- AVATAR control sequences (`\x16` + 2 bytes)
- Pipe color sequences (`|` + 2 digits, codes 00–31)
- Escaped pipes (`||` counts as 1 visible character)

Used by: `mci_visible_len()` (length measurement) and `mci_apply_trim()`
(truncation to visible width).

---

## Default State

On startup, all three parse flags are **enabled**:

```c
g_mci_parse_flags = MCI_PARSE_PIPE_COLORS | MCI_PARSE_MCI_CODES | MCI_PARSE_FORMAT_OPS;
```

This means display files, menu prompts, language strings, and MEX `print()`
output all receive full MCI expansion and pipe color parsing by default.
