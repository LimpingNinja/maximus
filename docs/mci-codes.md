# Maximus NG — MCI / Display Codes (Design + Spec)

## Goals

- Add **Renegade-style pipe color codes** using the `|##` syntax.
- Add **Mystic-style formatting operators** (e.g., `$C##`, `$R##`, `$D##C`) that affect the *next* MCI expansion.
- Add **Mystic-style “BBS / User Information” MCI codes** (e.g., `|UN`, `|UH`) and map them to existing Maximus data.
- Provide a **global enable/disable mechanism** that does not require changing function signatures throughout the codebase.
- Keep implementation compatible with Maximus’ existing **AVATAR attribute stream** and the existing **AVATAR->ANSI conversion** (`avt2ansi`).

Non-goals (for the first pass):

- A full Mystic feature set (text boxes, pick lists, cursor save/restore, etc.).
  - **Note:** Theme color codes (`|xx`) are now implemented — see Feature 6.
- Replacing Maximus’ native display-file compiled codes (`DCMaximus`, AVATAR control bytes). Those remain supported.

## Terminology

- **Pipe colors**: `|##` where `##` is numeric, used to change colors.
- **Theme colors**: `|xx` where `x`/`x` are lowercase letters, resolved to pipe color strings from `colors.toml`.
- **MCI codes**: `|XY` where `X`/`Y` are typically letters, replaced with data (user name, time left, etc.).
- **Formatting operators**: `$...` operators (Mystic-style) that apply to the *next* MCI expansion.
- **AVATAR attribute**: Maximus’ “set attribute” control sequence `\x16\x01<attr>`.

## Where this belongs in Maximus (integration points)

### Output layer (character stream)

- `Putc()` / `Puts()` (in `src/max/core/max_out.c`) send output to:
  - `Mdm_putc()` / `Mdm_puts()` (remote)
  - `Lputc()` / `Lputs()` (local)

The output functions are where Maximus already interprets and translates:

- AVATAR control sequences
- ANSI escape sequences (raw pass-through state machine for ANSI users)
- RIP sequences

**Pipe color decoding belongs here** because it is fundamentally a small token sequence that can be translated into an AVATAR attribute change and then reuses existing behavior.

### Display / prompt layer (string expansion)

Mystic `$C/$R/$L/$D/$X...` formatting operators do *not* map cleanly to AVATAR control codes because they require:

- Capturing the *expanded value* of an MCI code
- Measuring its printable width (sometimes with control codes present)
- Emitting padding/duplication based on desired width and/or current column

That is a **string-level transformation** and should happen before data is emitted into the output character stream.

## Feature 1: Pipe Color Codes (`|##`)

### Syntax

- `|##` where `##` are two digits.
- `||` outputs a literal `|`.
- A solitary `|` with no processable code behind it outputs a literal `|`.
- A `|` followed by non-digits also outputs the literal `|` and continues normally.

### Semantics

Based on Mystic’s pipe colors:

- `|00`..`|15`: set **foreground** color to 0..15.
- `|16`..`|31`: set **background** color.

Maximus attributes are “PC text attributes”:

- low nibble: foreground (0..15)
- bits 4..6: background (0..7)
- bit 7: blink

#### Proposed mapping

- `|00..|15`:
  - `fg = code`
  - preserve background + blink
- `|16..|23`:
  - `bg = code - 16` (0..7)
  - preserve foreground + blink
- `|24..|31` (ICE / bright backgrounds vs blink):
  - **Default behavior (compatible)**: treat as background 0..7 and **set blink bit**.
    - `bg = code - 24`
    - `blink = 1`
  - Rationale: classic PC attribute model has no “bright background” separate from blink.
  - Future: add a configuration knob for “ICE colors” when/if an ANSI renderer supports it.

### Where to implement

Implement in:

- `src/max/core/max_outr.c` in `Mdm_putc()` (remote)
- `src/max/core/max_outl.c` in `Lputc()` (local)

This must be done in the “normal state” path (i.e., when not already inside an AVATAR escape parse state).

### Implementation sketch (state machine)

Maintain a small static parser state in each of `Mdm_putc` and `Lputc`:

- **state 0**: normal
  - when `ch == '|'` and pipe parsing enabled, go to state 1
- **state 1**: saw `|`
  - if next `ch == '|'`: output literal `|`, return to state 0
  - if next is digit: store first digit, go to state 2
  - else: output literal `|`, then re-process this `ch` as normal text (or output it directly), return to state 0
- **state 2**: saw `|` + first digit
  - if next is digit: parse `00..99`
    - if in supported range (00..31): emit AVATAR attr change
    - else: output literal `|` + first digit + this digit
  - else: output literal `|` + first digit, then re-process this `ch`

Emitting an attribute change should be done by injecting the AVATAR attribute sequence into the same function:

- emit `\x16` (AVATAR lead-in)
- emit `\x01` (attribute command)
- emit `<newattr>`

This will automatically:

- update `mdm_attr` / `curattr`
- convert to ANSI via `avt2ansi()` for ANSI callers

(Implementation detail: the existing AVATAR state machine already knows how to handle `\x16\x01<attr>`. We can either directly call the existing attribute handling path or simply output those bytes into the same state machine carefully.)

## Feature 2: Enable/Disable Without Widespread Signature Changes

### Requirement

We need a way to enable/disable MCI parsing (pipe colors and later MCI/format codes) without modifying every `Puts(...)`/`Printf(...)` call signature.

### Proposed solution: global parse flags + push/pop stack

Introduce a small module (new C file(s) or placed into an existing core module) managing:

- `uint32_t g_mci_parse_flags;`
- a small stack to support scoped overrides:
  - `MciPushFlags(uint32_t mask, uint32_t values)`
  - `MciPopFlags(void)`

Flags (initial):

- `MCI_PARSE_PIPE_COLORS`
- `MCI_PARSE_MCI_CODES`
- `MCI_PARSE_FORMAT_OPS`

Default behavior proposal:

- Enabled for system output (menus/prompt strings/display files)
- Disabled for “raw” outputs (raw ANSI art, message bodies) unless explicitly desired

This keeps call-site churn low because only a few high-level “entry points” need to push/pop behavior, e.g.:

- `.BBS` display-file display entry/exit
- message reader entry/exit
- raw ANSI display entry/exit

## Feature 3: Mystic Formatting Operators (`$...`)

### Supported operators (first pass)

From Mystic wiki:

- `$C##` center next MCI expansion within width `##`
- `$L##` left pad next expansion with spaces to width `##`
- `$R##` right pad next expansion with spaces to width `##`
- `$T##` trim next expansion to length `##`
- `$D##C` duplicate character `C` `##` times
- `$X##C` duplicate character `C` until cursor column `##`
- `$c##C` center next expansion using character `C`
- `$l##C` left pad next expansion using character `C`
- `$r##C` right pad next expansion using character `C`
- `|PD` pad the next MCI value with a single space (Mystic special-case)

### Key semantic: affects the *next* MCI expansion

These formatting operators do not directly print “a value” (except `$D`/`$X`), they modify the formatting of the next MCI code output.

### Where to implement

**Not in AVATAR conversion.**

These operators require measuring the expansion’s printable width. That means implementation must happen at a layer that has:

- The whole source string
- The expanded value
- Knowledge of the current cursor column (for `$X`)

Proposed approach:

- Implement `mci_expand()` that:
  - Scans an input string
  - Expands MCI codes to text
  - Applies any pending `$...` operators to the next expansion
  - Leaves pipe color tokens intact (or optionally normalizes them)
- The output of `mci_expand()` is then sent through `Puts(...)` where pipe colors are translated to AVATAR attributes.

Printable length measurement should ignore control sequences:

- AVATAR sequences (e.g., `\x16\x01<attr>`)
- ANSI sequences (optional; if present in expansions)

Existing helper `stravtlen()` may be leveraged for AVATAR sequences.

### Open question: cursor column for `$X`

`$X##C` depends on current column. Maximus tracks:

- `current_col` / `display_col` (remote-side counters)

We should define which one is authoritative for this feature. Proposal:

- Use `current_col` (the “actual terminal cursor” counter used by `Mdm_putc`).

## Feature 4: Mystic-Style BBS/User Information Codes (Mapping)

### General rules

- Codes are **case-sensitive**.
- Codes are `|XY` where `X` and `Y` are generally letters.
- Codes must not conflict with pipe colors (`|00..|31`).

### Data sources in Maximus

Primary:

- `struct _usr usr` (defined in `src/max/core/max_u.h`)
- Global `usrname` (defined in `max_v.h`): “Name/alias of current user”
- System name: `ngcfg_get_string_raw("maximus.system_name")`
- Sysop name: `ngcfg_get_string_raw("maximus.sysop")`

### Implemented mapping table

| Code | Meaning | Maximus source |
|---|---|---|
| `\|BN` | BBS name | `ngcfg_get_string_raw("maximus.system_name")` |
| `\|SN` | Sysop name | `ngcfg_get_string_raw("maximus.sysop")` |
| `\|UH` | User handle (alias) | `usr.alias` |
| `\|UN` | User name | `usrname` |
| `\|UR` | User real name | `usr.name` |
| `\|UC` | City/State | `usr.city` |
| `\|UP` | Home phone | `usr.phone` |
| `\|UD` | Data phone | `usr.dataphone` |
| `\|U#` | User number | `g_user_record_id` (DB record id, set at login) |
| `\|CS` | Total calls | `usr.times` |
| `\|CT` | Calls today | `usr.call` |
| `\|MP` | Total msg posts | `usr.msgs_posted` |
| `\|DK` | Downloaded KB total | `usr.down` |
| `\|FK` | Uploaded KB total | `usr.up` |
| `\|DL` | Downloaded files total | `usr.ndown` |
| `\|FU` | Uploaded files total | `usr.nup` |
| `\|DT` | Downloaded KB today | `usr.downtoday` |
| `\|TL` | Time left (minutes) | `timeleft()` |
| `\|US` | Screen lines | `usr.len` |
| `\|TE` | Terminal emulation | `usr.video` → `"TTY"`, `"ANSI"`, `"AVATAR"` |
| `\|DA` | Current date | `strftime("%d %b %y")` |
| `\|TM` | Time (HH:MM) | `strftime("%H:%M")` |
| `\|TS` | Time (HH:MM:SS) | `strftime("%H:%M:%S")` |
| `\|MB` | Message area name | `MAS(mah, name)` |
| `\|MD` | Message area description | `MAS(mah, descript)` |
| `\|FB` | File area name | `FAS(fah, name)` |
| `\|FD` | File area description | `FAS(fah, descript)` |

### Items for future consideration

- Flags (`FLxx`, `FDxx`) and "on/off prompts"
- IP address / host name / country codes (if tracked)

## Relationship to legacy MECCA / compiled display codes

Maximus already supports compiled “display file” control codes (e.g., `DCMaximus` / `DisplayMaxCode`) that operate on byte-level tokens inside `.BBS` files.

The new Mystic-like MCI system is a **runtime text expansion** layer.

Future work: if we want a fast “compiled” representation, we can add a preprocessor/compile step for display assets, but the first pass should be runtime and scoped.

The key takeaway: many “user/system info” tokens already exist in a canonical form.
MECCA compiles tokens into one of:

- AVATAR attribute/control bytes (ex: colors)
- Display “datacode” sequences interpreted by `DisplayDatacode()` (trigger byte `0x06`)
- Display “maxcode” sequences interpreted by `DisplayMaxCode()` (trigger byte `0x17`)

This matters because a Mystic-style code like `|UN` (user name) can be implemented by expanding to the *same underlying concept* (either direct data lookup, or emitting the equivalent compiled bytecode) so behavior matches the legacy ecosystem.

### Legacy MECCA token → compiled bytes → runtime meaning (selected)

These examples are taken from `src/utils/util/mecca_vb.h` (compile-time) and the corresponding runtime interpreters:

- `DisplayDatacode()` in `src/max/display/disp_dat.c` (trigger `0x06`)
- `DisplayMaxCode()` in `src/max/display/disp_max.c` (trigger `0x17`)

#### Data codes (trigger `0x06` → `DisplayDatacode`)

| MECCA token | Compiled bytes | Meaning at runtime |
|---|---|---|
| `[user]` / `[name]` | `0x06 0x02` | user name/alias (`usrname`) |
| `[city]` | `0x06 0x03` | user city/state (`usr.city`) |
| `[date]` | `0x06 0x04` | current date string (`strftime("%d %b %y")`) |
| `[usercall]` | `0x06 0x05` | user call count (ordinal) |
| `[first]` / `[fname]` | `0x06 0x06` | first name (`firstname`) |
| `[minutes]` | `0x06 0x0b` | minutes online past 24h + current call |
| `[length]` | `0x06 0x0c` | length of current call (minutes) |
| `[remain]` | `0x06 0x0f` | minutes remaining (`timeleft()`) |
| `[timeoff]` | `0x06 0x10` | time user must be off system |
| `[syscall]` | `0x06 0x11` | total system calls (ordinal) |
| `[time]` | `0x06 0x14` | current time string (`strftime("%H:%M:%S")`) |

#### Max codes (trigger `0x17` → `DisplayMaxCode`)

| MECCA token | Compiled bytes | Meaning at runtime |
|---|---|---|
| `[sys_name]` | `0x17 0x03` | BBS/system name (`ngcfg_get_string_raw("maximus.system_name")`) |
| `[sysop_name]` | `0x17 0x04` | sysop name (`ngcfg_get_string_raw("maximus.sysop")`) |
| `[phone]` | `0x17 0x10` | user phone (`usr.phone`) |
| `[realname]` | `0x17 0x12` | user real name (`usr.name`) |
| `[lastcall]` | `0x17 0x01` | last call date (`usr.ludate`, via `sc_time`) |
| `[lastuser]` | `0x17 0x0b` | last user (`bstats.lastuser`) |

#### Color tokens (compile to AVATAR attribute sequences)

MECCA color verbs compile directly to AVATAR attribute control sequences (examples):

- `[black]` → `0x16 0x01 0x10 0x80`
- `[blue]` → `0x16 0x01 0x10 0x81`
- `[lightred]` → `0x16 0x01 0x10 0x8c`
- `[blink]` → `0x16 0x02`

These are already interpreted by `Mdm_putc()` / `Lputc()` and converted to ANSI via `avt2ansi()` when needed.

### “Reversible to AVATAR” — yes and no

- **Yes (colors/attributes):** many compiled tokens are literally AVATAR control sequences.
- **No (most user/system info):** those are not AVATAR codes; they’re *display bytecodes* (`0x06`/`0x17` sequences) that the Maximus display engine interprets and then emits text (and/or control sequences).

So the right reuse strategy is:

- Reuse legacy semantics (same fields, same formatting) where possible.
- Don’t force everything into “AVATAR conversion”; user/system info is a higher layer than AVATAR.

Future work: if desired, we can implement a helper that expands a subset of Mystic MCI codes by emitting the equivalent `0x06`/`0x17` sequences and letting the existing interpreter do the rest.

Future work: if we want a fast “compiled” representation, we can add a preprocessor/compile step for display assets, but the first pass should be runtime and scoped.

## Testing / Validation (manual)

- Verify `||` prints `|`.
- Verify `|` at end of string prints `|`.
- Verify `|1` + non-digit prints `|1` and continues.
- Verify `|00..|15` changes foreground without altering background.
- Verify `|16..|23` changes background without altering foreground.
- Verify `|24..|31` sets blink + background 0..7 (per design).
- Verify behavior for:
  - ANSI user (ensures `avt2ansi` conversion still works)
  - AVATAR user (sees AVATAR attrs)
  - Local display (changes `curattr` correctly)

## Resolved Decisions

- **`|UH` / `|UN` / `|UR` semantics**: `|UH` = Handle (`usr.alias`), `|UN` = Username (`usrname`), `|UR` = Real Name (`usr.name`). Three distinct codes, no ambiguity.
- **Default-disabled subsystems**: Parsing is disabled via `MciPushParseFlags(MCI_PARSE_ALL, 0)` in: `ui_edit_field()`, `MagnEt()`, `Bored()`, `ShowMessageLines()`, and the description loop in `ShowFileEntry()`. These can be made opt-in later via config flags.

## Feature 5: Terminal Control Codes

Two-letter uppercase pipe codes that emit ANSI/AVATAR terminal control sequences.
These are processed in `MciExpand()` before the info code handler.

| Code | Meaning | Emits |
|---|---|---|
| `\|CL` | Clear screen | `\x0c` (CLS byte, display layer handles clear+home) |
| `\|BS` | Destructive backspace | `\x08 \x08` (back, space, back) |
| `\|CR` | Carriage return + line feed | `\r\n` |
| `\|CD` | Reset to default color | `ESC[0m` (ANSI SGR reset) |
| `\|SA` | Save cursor + attributes | `ESC 7` (DEC save) |
| `\|RA` | Restore cursor + attributes | `ESC 8` (DEC restore) |
| `\|SS` | Save screen | `ESC[?47h` (alternate screen buffer) |
| `\|RS` | Restore screen | `ESC[?47l` (main screen buffer) |
| `\|LC` | Load last color mode | *(stub — no-op)* |
| `\|LF` | Load last font | *(stub — no-op)* |

### Special codes

| Code | Meaning | Emits |
|---|---|---|
| `\|&&` | Cursor Position Report (DSR) | `ESC[6n` |

### Cursor control codes (bracket prefix)

| Code | Meaning | Emits |
|---|---|---|
| `[0` | Hide cursor | `ESC[?25l` |
| `[1` | Show cursor | `ESC[?25h` |
| `[A##` | Cursor up ## rows | `ESC[##A` |
| `[B##` | Cursor down ## rows | `ESC[##B` |
| `[C##` | Cursor forward ## cols | `ESC[##C` |
| `[D##` | Cursor back ## cols | `ESC[##D` |
| `[K` | Clear to end of line | `ESC[K` |
| `[L##` | Move to column ## and erase to EOL | `ESC[##G` + `ESC[K` |
| `[X##` | Move cursor to column ## | `ESC[##G` |
| `[Y##` | Move cursor to row ## | `ESC[##d` |

### Language string positional parameters

| Code | Meaning |
|---|---|
| `\|!1`..`\|!9` | Positional parameter 1–9 |
| `\|!A`..`\|!F` | Positional parameter 10–15 |

Expanded by `LangPrintf()` pass 1 before MCI format ops run.
Callers must zero-pad numeric values with `%02d` when the expanded
value feeds into `$D##C`, `[Y##`, `[X##`, or similar 2-digit fields.

## TOML Language File Rules

- **`\xHH` hex escapes** — libmaxcfg's TOML parser handles `\xHH` byte escapes (used for non-printable chars that have no MCI equivalent)
- **`\a` bell escape** — libmaxcfg's TOML parser handles `\a` (0x07 bell byte)
- All AVATAR constants converted to MCI by `lang_convert.c`:
  - `CLS` (0x0c) → `|CL`
  - `CLEOL` (0x16 0x07) → `[K`
  - Colors (0x16 0x01 NN) → `|##` pipe codes
  - Positions (0x16 0x08 R C) → `[Y##[X##`
  - RLE repeat (0x19 ch count) → `$D##C`
  - DSR (ESC[6n) → `|&&`

## Feature 6: Semantic Theme Color Codes (`|xx`)

### Design

Theme color codes use **two lowercase letters** — a namespace that was
previously unused in the MCI grammar:

- **Uppercase** two-letter codes = terminal control / info codes (`|CL`, `|UN`)
- **Digit** codes = raw numeric colors (`|00`–`|31`)
- **Lowercase** two-letter codes = semantic theme slots (`|tx`, `|pr`, etc.)

Each slot maps to a configured pipe color string (e.g., `"|07"` or `"|15|17"`)
stored in `colors.toml` under `[theme.colors]`.

### Slot Table

| Code | Key           | Purpose                            | Default |
|------|---------------|------------------------------------|---------|
| `tx` | text          | Normal body text                   | `\|07`   |
| `hi` | highlight     | Emphasized / important text        | `\|15`   |
| `pr` | prompt        | User-facing prompts                | `\|14`   |
| `in` | input         | User keystroke echo                | `\|15`   |
| `tf` | textbox_fg    | Text input field foreground        | `\|15`   |
| `tb` | textbox_bg    | Text input field background        | `\|17`   |
| `hd` | heading       | Section headings                   | `\|11`   |
| `lf` | lightbar_fg   | Lightbar selected foreground       | `\|15`   |
| `lb` | lightbar_bg   | Lightbar selected background       | `\|17`   |
| `er` | error         | Error messages                     | `\|12`   |
| `wn` | warning       | Warnings                           | `\|14`   |
| `ok` | success       | Confirmations / success            | `\|10`   |
| `dm` | dim           | De-emphasized / help text          | `\|08`   |
| `fi` | file_info     | File area descriptions             | `\|03`   |
| `sy` | sysop         | SysOp-only text                    | `\|13`   |
| `qt` | quote         | Quoted message text                | `\|09`   |
| `br` | border        | Box borders, dividers              | `\|01`   |
| `cd` | default       | Reset to theme default color       | `\|07`   |

### TOML Schema (`colors.toml`)

```toml
[theme.colors]
text         = "|07"
highlight    = "|15"
prompt       = "|14"
input        = "|15"
textbox_fg   = "|15"
textbox_bg   = "|17"
heading      = "|11"
lightbar_fg  = "|15"
lightbar_bg  = "|17"
error        = "|12"
warning      = "|14"
success      = "|10"
dim          = "|08"
file_info    = "|03"
sysop        = "|13"
quote        = "|09"
border       = "|01"
default      = "|07"
```

Values are arbitrary MCI pipe strings. Compound values (e.g. `"|15|17"`) are
supported — the expansion emits the full string, which then flows through
normal `|##` color processing in the output layer.

### Runtime Flow

1. **Startup**: `colors.toml` is loaded by `maxcfg_toml_load_file()` and
   the `[theme.colors]` section is parsed into a `MaxCfgThemeColors` struct
   via `maxcfg_theme_load_from_toml()`.

2. **Runtime pointer**: `g_mci_theme` (declared in `mci.h`, defined in
   `mci.c`) points to the loaded theme table. Set at startup.

3. **Expansion**: In `MciExpand()` (`src/max/display/mci.c`), when
   `MCI_PARSE_PIPE_COLORS` is set and the next three characters are
   `|` + lowercase + lowercase, the handler calls
   `maxcfg_theme_lookup(g_mci_theme, ch1, ch2)`.
   - If a matching slot is found, its stored pipe string is emitted into
     the output buffer. The `|##` codes within that string are then
     processed by the output layer (`Mdm_putc` state machine).
   - If no match is found, the `|xx` falls through to literal output.

4. **Preview**: `mci_preview.c` (maxcfg editor) has a parallel handler
   that resolves `|xx` codes against `g_theme_colors` and applies the
   resulting `|##` codes to the virtual screen attribute byte.

### Implementation Files

| File | Role |
|---|---|
| `src/libs/libmaxcfg/include/libmaxcfg.h` | `MaxCfgThemeSlot`, `MaxCfgThemeColors` structs, API declarations |
| `src/libs/libmaxcfg/src/libmaxcfg.c` | `maxcfg_theme_init`, `maxcfg_theme_load_from_toml`, `maxcfg_theme_lookup`, `maxcfg_theme_write_toml`, `maxcfg_ng_color_to_mci` |
| `src/max/display/mci.h` | `extern void *g_mci_theme` |
| `src/max/display/mci.c` | `\|xx` handler in `MciExpand()` |
| `src/apps/maxcfg/src/ui/mci_preview.c` | `\|xx` handler for preview rendering |
| `src/apps/maxcfg/src/ui/colorform.c` | `themeform_edit()` — Theme Colors editor form |
| `src/apps/maxcfg/src/ui/mci_helper.c` | Theme tab in MCI code helper popup |
| `src/apps/maxcfg/src/main.c` | Loads theme from `colors.toml` into `g_theme_colors` |

### maxcfg Integration

- **Setup → Global → Default Colors → Theme Colors**: scrollable form
  showing all 18 slots with their current MCI string values. F2/Enter
  opens the color picker; chosen fg/bg is converted to a `|NN` string
  via `maxcfg_ng_color_to_mci()`. Saved via `maxcfg_toml_override_set_string()`.

- **F3 MCI Helper**: a "Theme" tab lists all 18 `|xx` codes with descriptions.

### Parse Flag

Theme codes are gated by `MCI_PARSE_PIPE_COLORS` — the same flag that
controls `|##` numeric color codes. When pipe color parsing is disabled
(e.g., in the message editor or user input fields), theme codes are also
ignored.

## Open Decisions

- For `|24..|31`: should we offer a config toggle for "ICE backgrounds" vs "blink backgrounds"?
