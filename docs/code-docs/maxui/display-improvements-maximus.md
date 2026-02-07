# Display Improvements for Maximus (BBS Runtime)

This document proposes a set of **display/input UX improvements** for the Maximus runtime, inspired by the Notorious DoorKit (`notorious_doorkit`) APIs and behavior (notably: `forms.py`, `input.py`, `lightbar.py`, plus `screen.py`/`text.py` support layers).

The goal is to bring “classic door UX primitives” (bounded painted fields, editable forms, lightbar menus) to Maximus in a **reusable C library** that can be used by:

- Maximus core code
- MEX scripts (via new intrinsics)
- (optionally) external door integrations later

---

## Goals

- **Bounded input fields (painted fields)**
  - Fixed on-screen width (e.g. 40 columns)
  - Pre-painted field area (spaces or mask placeholders) using an explicit attribute
  - Cursor constrained inside the field region
  - Max-length enforcement distinct from width when needed
  - Support both:
    - **Inline prompt fields** (prompt + field at current cursor)
    - **Positioned fields** (x/y coordinates)

- **Form building mechanism (“EDIT SCREEN” style)**
  - Define a set of fields with:
    - coordinates, width
    - label
    - value
    - validation/normalization hooks
    - required fields
  - Navigation via arrows (spatial neighbor selection like DoorKit `AdvancedForm`)
  - Edit a field on Enter; save on Ctrl+S or via a configurable “ESC prompt”

- **Lightbar menus**
  - Classic vertical lightbar (`LightbarMenu` behavior)
  - Positioned/multi-column lightbar (`AdvancedLightbarMenu` behavior)
  - Hotkey markers and/or hotkey auto-assignment
  - Optional “hotkey format” rendering

- **MEX integration**
  - Expose these primitives as MEX intrinsics so scripts can build full-screen UIs without duplicating key loops.

---

## DoorKit Behaviors to Mirror (Summary)

These are the key behavioral contracts that DoorKit provides (and Maximus users expect from other BBS software):

### 1) Painted, bounded field editor (`edit_str` + `AdvancedForm`)

DoorKit’s `edit_str(value, fmt, row, col, normal_attr, highlight_attr, blank, flags)` provides:

- **A fixed-width editable region** at `(row, col)`
- **Normal vs focused attributes** (PC attribute byte 0-255)
- Field navigation return codes in “field mode” (previous/next)
- Basic cursor/edit keys: arrows, Home/End, Backspace/Delete

DoorKit’s `AdvancedForm` adds:

- Form-level redraw and focus management
- Label + input region separation
- Required field enforcement and optional splash message
- Field types:
  - plain text (optionally masked)
  - structured `format_mask` (“00/00/00”, “AA”, etc.)
  - toggle fields (`options` list)

### 2) Lightbars

DoorKit’s `LightbarMenu` / `AdvancedLightbarMenu` provide:

- A highlight bar using **different colors** for selected vs normal
- Navigation with arrows
- Optional “wrap”
- Hotkey support (explicit `{{X}}` marker + auto-assignment)
- Efficient redraw: only old + new rows

### 3) “PC attribute byte” model

DoorKit supports DOS-style attribute bytes (fg/bg + bright + blink) via `Attr` and `set_attrib()`.

Maximus already has the equivalent attribute byte model (`mdm_attr`, `attr_string`, AVATAR attribute set sequences, and ANSI translation via `avt2ansi`).

---

## Current Maximus Capabilities (What We Can Build On)

### Output primitives (remote)

Maximus already supports the core operations required to implement painted fields and lightbars:

- **Set attribute**
  - Via the AVATAR attribute set control (`attr_string` = `"\x16\x01%c"`) used throughout the output layer.
- **Goto XY**
  - `Goto(x, y)` macro in `src/max/core/max.h` expands to `Printf(goto_str, ...)`.
- **Fill rectangles / clear regions**
  - The output layer implements AVATAR “fill” and “clear” sequences and translates them for ANSI callers.
  - See `src/max/core/max_outr.c` handling of Fill/Clear which uses `fill_string` / `clear_string`.

Implication: we can paint a 40-wide field background **without needing ANSI-specific code**; we can paint via attribute-byte semantics and let the output layer translate.

### Input primitives (remote)

- `mdm_gets()` (`src/max/core/max_gets.c`) is Maximus’s line editor.
- It supports max-length and editing keys but it is **not field-bounded** and does not pre-paint a fixed-width region.

### MEX runtime

Existing MEX intrinsics cover:

- Output: `intrin_printstring`, `intrin_printchar`, `intrin_vidsync`, etc. (`src/max/mex_runtime/mexoutp.c`)
- Input: `intrin_input_str`, `intrin_input_ch`, `intrin_input_list` (`src/max/mex_runtime/mexinp.c`)
- Display files: `intrin_display_file` (declared in `mexint.h`)

There are **no general cursor/attribute/fill** intrinsics, and no form/lightbar abstractions.

---

## Proposed Improvements

### Improvement A: Add a reusable “UI widgets” library for Maximus runtime

**Where:**

- `src/max/display/` (recommended)

Rationale:

- It keeps UI widgets close to other runtime display logic.
- It avoids conflating raw input (`max_in.c`) with higher-level UI.

**New files (proposal):**

- `src/max/display/ui_attr.h` / `ui_attr.c`
- `src/max/display/ui_field.h` / `ui_field.c`
- `src/max/display/ui_form.h` / `ui_form.c`
- `src/max/display/ui_lightbar.h` / `ui_lightbar.c`

These should be written as pure runtime code (no MAXCFG dependence).

---

## Improvement B: Bounded (painted) field editor

### B1) Positioned field editor (core primitive)

A Maximus equivalent of DoorKit `edit_str()`.

**Proposed API (C-ish):**

```c
/* Return codes: compatible with DoorKit style */
#define UI_EDIT_ERROR     0
#define UI_EDIT_CANCEL    1
#define UI_EDIT_ACCEPT    2
#define UI_EDIT_PREVIOUS  3
#define UI_EDIT_NEXT      4

#define UI_EDIT_FLAG_ALLOW_CANCEL  0x0020
#define UI_EDIT_FLAG_FIELD_MODE    0x0002
#define UI_EDIT_FLAG_MASK          0x0004

int ui_edit_field(
    int row,
    int col,
    int width,          /* visible width */
    int max_len,        /* max data length (<= width for strict fields) */
    char *buf,
    int buf_cap,
    const ui_edit_field_style_t *style
);
```

**Behavior:**

- Paint `width` columns at `(row, col)` in `focus_attr` (editing state).
- Render `buf` (masked if desired in a higher-level wrapper) and pad with `fill_ch` to `width`.
- Cursor position constrained inside `[0, width]`.
- Enforce `max_len` on insertion.
- Handle:
  - Left/Right, Home/End
  - Backspace/Delete
  - Enter => accept
  - ESC => cancel if `UI_EDIT_FLAG_ALLOW_CANCEL`
- Tab/Down => next, Shift-Tab/Up => previous when `UI_EDIT_FLAG_FIELD_MODE`

Masking:

- When `UI_EDIT_FLAG_MASK` is set, the field displays `fill_ch` for each character typed.

**Implementation notes (Maximus-specific):**

- Use `Goto(row, col)` to position.
- Use `Printf(attr_string, focus_attr)` to set attribute.
- Use a small helper like `ui_fill_hline(row, col, width, fill_ch, attr)` (implemented using repeated `Putc` or an AVATAR fill sequence) to paint the field region.
- Avoid relying on `current_line/current_col` tracking for correctness; always re-position explicitly.

### B2) Inline prompt editor

A helper that edits a bounded field at the current cursor location:

```c
int ui_prompt_field(
    const char *prompt,
    int width,
    int max_len,
    char *buf,
    int buf_cap,
    const ui_prompt_field_style_t *style
);
```

This should:

- Output `prompt` (possibly via `Printf`/`Puts`)
- Determine the start position for the field (based on Maximus cursor tracking or by requiring caller to pass coordinates)
- Call `ui_edit_field()`

Recommendation: for robustness, prefer a signature where the caller provides `(row, col)` explicitly. Cursor tracking can drift in TTY/RIP edge-cases.

---

## Improvement C: Form building mechanism (“EDIT SCREEN”)

### C1) Data model

Mirror DoorKit’s `FormField` / `AdvancedForm`:

```c
typedef enum {
  UI_FIELD_TEXT = 0,
  UI_FIELD_MASKED,
  UI_FIELD_FORMAT_MASK,
  UI_FIELD_OPTIONS,
} ui_field_type_t;

typedef struct {
  const char *name;
  int x;
  int y;
  int width;

  const char *label;        /* drawn to the left of x */
  char *value;              /* caller-owned buffer */
  int value_cap;
  int max_length;

  ui_field_type_t type;

  /* options */
  const char **opt_values;
  const char **opt_labels;
  int opt_count;

  /* format mask */
  const char *format_mask;

  /* styling */
  byte label_attr;
  byte input_attr;
  byte focus_attr;

  /* behavior */
  int required;
  int (*validate)(const char *value);
  void (*normalize)(char *value, int cap);
  int hotkey;               /* optional */
} ui_form_field_t;

typedef struct {
  ui_form_field_t *fields;
  int field_count;
  int selected;
  int wrap;

  /* save behavior */
  int save_mode;            /* ctrl_s vs esc_prompt */

  /* optional required message */
  const char *required_splash;
  int required_x;
  int required_y;
  byte required_attr;
} ui_form_t;
```

### C2) Navigation and redraw behavior

- **Draw phase:**
  - Draw label (if any)
  - Draw the value padded to width in input_attr
- **Focus state:**
  - When selected but not editing: draw in focus_attr (or a separate “focused-but-not-editing” attr)
  - When editing: call `ui_edit_field()`

- **Spatial navigation:**
  - Use DoorKit’s “center point” selection algorithm:
    - Compute each field center `cx = x + (width - 1)/2`
    - For Up/Down/Left/Right, choose closest in that direction using (dy, abs(dx)) sorting

- **Hotkey jump:**
  - Optional: if a single key matches `field.hotkey`, set focus.

- **Saving:**
  - Ctrl+S saves (if in ctrl-s mode)
  - ESC either exits immediately or shows an ESC prompt (edit/save/exit)

This gives a direct Maximus analogue to DoorKit’s `AdvancedForm.run()`.

---

## Improvement D: Lightbar menus

### D1) Vertical lightbar (uniform positioning)

```c
typedef struct {
  const char **items;
  int count;
  int x;
  int y;
  int width;        /* 0 => auto */
  int justify;      /* left/center/right */
  byte normal_attr;
  byte selected_attr;
  byte hotkey_attr; /* 0 => use normal_attr/selected_attr */
  byte hotkey_highlight_attr; /* 0 => no special hotkey highlight when selected */
  int wrap;
  int enable_hotkeys;
  int show_brackets; /* 1 => show [X], 0 => show just X highlighted */
} ui_lightbar_menu_t;

int ui_lightbar_run(ui_lightbar_menu_t *m);
```

**Item passing / ownership:**

- `m->items` is an array of pointers to NUL-terminated strings.
- `m->count` is the number of items.
- The caller owns the array and the strings; `ui_lightbar_run()` must not free them.
- `ui_lightbar_run()` must not retain pointers beyond the call.

**Item parsing (recommended):**

- Support an explicit hotkey marker like `[X]` inside each string.
- The renderer should display the item with the marker stripped (e.g. `[Q]uit` -> `Quit`).
- Internally, `ui_lightbar_run()` should build a derived “render model” for the session (stack/local or temporary heap) so it does not re-parse strings on every keypress:
  - display text (marker stripped)
  - parsed hotkey (if present)
  - computed width/padding behavior

Behavior:

- Paint each row padded to width using `normal_attr`.
- Paint selected row using `selected_attr`.
- Redraw only previous + new selection.
- Enter returns index; ESC returns -1.
- Hide the terminal cursor during menu interaction; restore it on exit.

Hotkeys:

- Support explicit marker parsing similar to DoorKit `{{X}}` (or a Maximus-friendly equivalent like `[X]` marker).
- Optionally auto-assign a unique hotkey for each item.

### D2) Positioned/multi-column lightbar

Equivalent of DoorKit `AdvancedLightbarMenu`:

- Each item has its own `(x, y, width)`.
- Navigation in 2D using nearest-neighbor selection (center-based).
- Hide the terminal cursor during menu interaction; restore it on exit.

**Proposed C representation (recommended):**

Keep D1 simple, and use a separate item structure for positioned menus:

```c
typedef struct {
  const char *text;
  int x;
  int y;
  int width;
  int justify;
} ui_lightbar_item_t;
```

The caller owns `text` strings; the UI code treats them as read-only for the duration of the call.

### D3) Inline lightbar select prompt (Yes/No style)

A simple inline selection prompt that draws a short horizontal lightbar at the current cursor position.

Example:

- Prompt: `Create an account with this BBS?`
- Options: `Yes`, `No`

Behavior:

- Display the prompt text, followed by a lightbar with options.
- Left/Right changes selection.
- Enter accepts selection.
- ESC cancels.
- Hide the terminal cursor during selection; restore it on exit.

Return value:

- Returns the selected option's **index** (1-based).
- ESC cancels and returns `-1`.
- The selected hotkey is returned via an output parameter.

Example option list:

- `[O]ne`, `[T]wo`, `T[h]ree` returns `o`, `t`, or `h`.

Proposed API:

```c
int ui_select_prompt(
    const char *prompt,
    const char **options,
    int option_count,
    byte prompt_attr,
    byte normal_attr,
    byte selected_attr,
    int *out_key
);
```

`out_key` is the selected hotkey character (matching is case-insensitive). If the option contains a bracket hotkey marker like `[O]ne`, the returned key is the character inside the brackets (`o`). Otherwise, the key defaults to the first character of the option (`y` for `Yes`, `n` for `No`).

**Option passing / ownership:**

- `options` is an array of pointers to NUL-terminated strings.
- `option_count` is the number of options.
- The caller owns the array and strings; `ui_select_prompt()` must not free them.

**Option parsing:**

- Use the same `[X]` marker rules described in D1 for extracting a hotkey character.
- If no marker is present, the key defaults to the first character of the option.

## Improvement E: Low-level “attribute + fill” helper APIs

Even if the form/lightbar implementations are the primary goal, it’s valuable to expose a small set of primitives in one place so they are easy to reuse:

- `ui_set_attr(byte attr)`
- `ui_goto(int row, int col)`
- `ui_fill_rect(int row, int col, int width, int height, char ch, byte attr)`
- `ui_write_padded(int row, int col, int width, const char *s, byte attr)`

These should internally use Maximus’s existing output pipeline:

- `Printf(attr_string, attr)`
- `Goto(row, col)`
- `Putc`/`Puts`/`Printf` (or AVATAR fill sequences where appropriate)

---

## MEX Integration (Display Routines)

### Why this matters

Maximus already has basic input/output intrinsics for MEX, but MEX currently lacks the higher-level UI constructs needed to build modern “door-like” UIs inside Maximus.

Adding MEX intrinsics for forms/lightbars would allow:

- scripted config panels
- user profile editors
- interactive menus that feel consistent across Maximus and external doors

### Proposed new MEX intrinsics

Add these to `src/max/mex_runtime/` (new `mexui.c` or extend existing `mexoutp.c` / `mexinp.c`):

- **Cursor / attribute / fill**
  - `ui_goto(row, col)`
  - `ui_set_attr(attr_byte)`
  - `ui_fill_rect(row, col, width, height, ch, attr_byte)`

- **Bounded field editor**
  - `ui_edit_field(row, col, width, max_len, buf, style)`
    - `style` is a `struct ui_edit_field_style` passed by reference
    - Returns edited string
    - Return code is stored in `regs_2[0]` by the runtime

**UI declarations live in `maxui.mh`** (instead of `max.mh`).

- **Lightbar**
  - `ui_lightbar(items[], count, x, y, width, style) -> selected_index`
    - `style` is a `struct ui_lightbar_style` passed by reference
    - `style.out_hotkey` receives the selected hotkey (`0` if cancelled)

- **Inline select prompt**
  - `ui_select_prompt(prompt, options[], count, style) -> selected_index`
    - `style` is a `struct ui_select_prompt_style` passed by reference
    - `style.out_hotkey` receives the selected hotkey (`0` if cancelled)

- **Form runner (phase 2)**
  - `ui_form_run(form_descriptor) -> map/dict-like structure`

Implementation constraints:

- MEX is not naturally a “dict” language, so returning a form result likely means:
  - operate directly on a list of pass-by-reference strings
  - or return a delimited result string
  - or return an array-like structure if MEX supports it

A pragmatic first step is:

- implement `ui_edit_field()` and `ui_lightbar()` intrinsics
- then build “forms” at the MEX script level using those building blocks

### Return codes and conventions

Selection intrinsics return a **1-based index** on accept and `-1` on cancel.

---

## Where This Should Plug Into Maximus

- **Core runtime UI code:** `src/max/display/` (new `ui_*.c` as described)
- **Call sites (examples):**
  - user editor flows (profile editing)
  - sysop utilities
  - menu overlays
  - future “next-gen” UI screens

- **MEX:**
  - `src/max/mex_runtime/` new intrinsics bridging to `ui_*` APIs

---

## Compatibility and Risk Notes

- **TTY callers:**
  - Must degrade gracefully (painted field becomes “prompt + input” or simple echo).
- **RIP/AVATAR:**
  - Prefer using attribute-byte + AVATAR-like operations so the existing output layer continues to translate appropriately.
- **Cursor tracking:**
  - Rely on explicit `Goto()` calls while drawing/editing.
  - Avoid assuming internal `current_line/current_col` are always accurate after raw file displays or RIP sequences.

---

## Implementation Plan

### Phase 1 (foundation)

1. **Add `ui_attr` + `ui_draw` helpers**
   - `ui_set_attr`, `ui_goto`, `ui_fill_rect`, `ui_write_padded`

2. **Implement bounded field editor** (`ui_edit_field`)
   - returns accept/cancel/prev/next
   - supports width + max length
   - supports fill char + focus/normal attrs

3. **Expose minimal MEX intrinsics**
   - `intrin_ui_goto(row, col)`
   - `intrin_ui_set_attr(attr)`
   - `intrin_ui_fill_rect(row, col, width, height, ch, attr)`
   - `intrin_ui_edit_field(row, col, width, max_len, buf, style)`

### Phase 2 (menus)

4. **(D1) Implement vertical lightbar menu (uniform positioning)**
  - Implement `src/max/display/ui_lightbar.h` / `ui_lightbar.c`
  - Primary entrypoint: `int ui_lightbar_run(ui_lightbar_menu_t *m)`
  - Behavior requirements (from D1):
    - Paint each row padded to width using `normal_attr`
    - Paint selected row using `selected_attr`
    - Redraw only previous + new selection
    - Enter returns selected index; ESC returns -1
    - Arrow navigation + optional wrap
  - Hotkeys (from D1):
    - Parse explicit marker like `[X]` in item strings
    - Optional auto-assignment of unique hotkeys
    - Case-insensitive matching

5. **(D1) Expose MEX intrinsic for vertical lightbar**
  - `ui_lightbar(items[], count, x, y, width, style) -> selected_index`
  - Return convention:
    - Selected index in `regs_2[0]`
    - -1 for cancel

6. **(D3) Inline select prompt (Yes/No style)**
  - Implement `ui_select_prompt()` (as described in D3)
  - Expose MEX intrinsic:
    - `ui_select_prompt(prompt, options[], count, style) -> selected_index`

7. **(D2) Positioned/multi-column lightbar (optional in Phase 2, required before Phase 3 forms)**
  - Each item has its own `(x, y, width)`
  - 2D navigation using nearest-neighbor selection (center-based)

**Passing items (MEX vs C):**

- **MEX**
  - Use an `array [...] of string:` and pass the array to the intrinsic.
  - Runtime can marshal the array into a temporary `const char *items[]` + `count` and call the C implementation.
  - The C implementation should treat strings as read-only and never retain pointers beyond the call.

- **C**
  - Prefer an explicit-count API (already reflected in D1):
    - `const char **items` + `int count` stored in `ui_lightbar_menu_t`
    - Caller owns `items` and the strings; `ui_lightbar_run()` does not free them.
  - Internally, `ui_lightbar_run()` should build a derived array (stack/local or temporary heap) of parsed items:
    - Display text (with `[X]` markers stripped)
    - Parsed hotkey (if any)
    - Computed render width / padding behavior
  - For D2 (positioned) introduce a separate item struct (recommended):
    - `ui_lightbar_item_t { const char *text; int x; int y; int width; }`
    - Menu struct points to an array of `ui_lightbar_item_t` instead of `const char **items`
    - This keeps the vertical menu API small while enabling richer layouts.

### Phase 3 (forms)

6. **Implement `ui_form_t` and `ui_form_run()`**
   - spatial navigation
   - required fields
   - save modes

7. **MEX integration for forms**
   - Start with a “form = list of fields” API operating on pass-by-ref strings

### Phase 4 (advanced UX)

8. **Structured masks + option fields**
   - format mask editing
   - option/toggle cycling fields

9. **Positioned lightbar (multi-column)**

10. **(Optional) save-under / shadow screen**
   - A real screen buffer model (like DoorKit `ShadowScreen`) is powerful but more invasive.
   - Consider only after forms/lightbars are stable.

---

## Implementation Notes

### Phase 1 - Foundation (Completed)

**Date:** January 30, 2025

**Files Created:**
- `src/max/display/ui_field.h` - Header defining UI primitives and bounded field editor
- `src/max/display/ui_field.c` - Implementation of core UI library
- `src/max/mex_runtime/mexui.c` - MEX intrinsics for UI functions

**Files Modified:**
- `src/max/Makefile` - Added `ui_field.obj` and `mexui.obj` to build
- `src/max/mex_runtime/mexint.h` - Added declarations for UI intrinsics

**What Was Implemented:**

1. **Low-level UI primitives:**
   - `ui_set_attr(byte attr)` - Sets display attribute using Maximus AVATAR sequences
   - `ui_goto(int row, int col)` - Positions cursor using existing `Goto()` macro
   - `ui_fill_rect(int row, int col, int width, int height, char ch, byte attr)` - Fills rectangular region
   - `ui_write_padded(int row, int col, int width, const char *s, byte attr)` - Writes string padded to width

2. **Bounded field editor:**
   - `ui_edit_field()` - Full-featured line editor with:
     - Fixed on-screen width (can be smaller than max input length)
     - Separate normal and focus attributes
     - Password masking support (fill character)
     - Insert mode editing with cursor movement (left/right/home/end)
     - Backspace deletion
     - Return codes: `UI_EDIT_ACCEPT`, `UI_EDIT_CANCEL`, `UI_EDIT_PREVIOUS`, `UI_EDIT_NEXT`
     - Flag support for field navigation (tab/shift-tab) and ESC cancellation

3. **MEX intrinsics exposed:**
  - `intrin_ui_goto(row, col)` - Cursor positioning from MEX
  - `intrin_ui_set_attr(attr)` - Attribute setting from MEX
  - `intrin_ui_fill_rect(row, col, width, height, ch, attr)` - Rectangle fill from MEX
  - `intrin_ui_write_padded(row, col, width, s, attr)` - Write padded string from MEX
  - `intrin_ui_prompt_field(prompt, width, max_len, buf, style)` - Prompt+field from MEX (style struct)
  - `intrin_ui_edit_field(row, col, width, max_len, buf, style)` - Field editor from MEX (style struct)
   - All field functions return edited string via `MexReturnString()`
   - Return code is stored in `regs_2[0]` by the runtime

**How It Was Done:**

- Used existing Maximus primitives (`Printf`, `Putc`, `Goto`, `attr_string`, `Mdm_getcw`, `Mdm_keyp`, `vbuf_flush`)
- Followed Maximus coding conventions and header structure
- Integrated with existing MEX argument handling (`MexArgBegin`, `MexArgGetWord`, `MexArgGetString`, `MexArgEnd`)
- Fixed platform-specific key code conflicts (K_RETURN/K_ENTER, K_BS/K_VTDEL/K_DEL on UNIX)
- Character-by-character editing with incremental redraw for insert/backspace

**Outcome:**

- Clean compilation with no errors
- All files integrated into build system
- `max` binary built successfully with new UI library
- MEX intrinsics available for scripting (7 total UI functions)
- ✅ `ui_prompt_field()` wrapper implemented (including `UI_PROMPT_START_HERE/CR/CLBOL`)
- ✅ Tested with login username/password fields
- ✅ Comprehensive test suite created (`resources/m/uitest.mex`)
- ✅ Full API documentation created (`docs/code-docs/maxui/new_ui_mex_functions.md`)
- ✅ UI_* color constants added to `max.mh` for better MEX script readability

**Technical Notes:**

- The editor uses insert mode by default (characters inserted at cursor position)
- Field redraw is incremental for insert/backspace (redraws only the changed region of the field)
- Cursor position tracking is explicit (no reliance on global `current_col`)
- Password masking shows fill character for each input character
- Tab/Shift-Tab navigation only active when `UI_EDIT_FLAG_FIELD_MODE` is set
- ESC cancellation only active when `UI_EDIT_FLAG_ALLOW_CANCEL` is set
- All UI functions properly handle ANSI/AVATAR translation via Maximus output primitives
- `ui_set_attr()` emits AVATAR control sequences that are translated to ANSI for ANSI terminals

**Documentation:**

See `docs/code-docs/maxui/new_ui_mex_functions.md` for complete API reference, examples, and usage patterns for all UI MEX intrinsics.

---

## C API Notes (Current)

The authoritative C APIs for these UI primitives live in:

- `src/max/display/ui_field.h` (bounded field editor + prompt field)
- `src/max/display/ui_lightbar.h` (lightbar menus + select prompt)

Key C entrypoints:

```c
int ui_edit_field(
    int row,
    int col,
    int width,
    int max_len,
    char *buf,
    int buf_cap,
    const ui_edit_field_style_t *style
);

int ui_prompt_field(
    const char *prompt,
    int width,
    int max_len,
    char *buf,
    int buf_cap,
    const ui_prompt_field_style_t *style
);

int ui_lightbar_run_hotkey(ui_lightbar_menu_t *m, int *out_key);
int ui_lightbar_run_pos_hotkey(ui_lightbar_pos_menu_t *m, int *out_key);
```

Breaking behavior/API changes to call out:

- Field and prompt editors now take a **style struct** (`ui_edit_field_style_t` / `ui_prompt_field_style_t`) instead of many discrete style args.
- `ui_prompt_field_ex` was removed; its configuration is represented via fields in `ui_prompt_field_style_t`.
- Lightbar styles gained `hotkey_highlight_attr` so selected-item hotkeys can be colored independently.
- Lightbar styles gained `margin` which is added to the effective width.
- Select prompt styles gained `margin` and `separator`:
  - `margin` adds padding spaces on **both sides** of each option (default `0` = no padding)
  - `separator` is a string printed between options using `normal_attr` (default `""` = no separator)

---

## Next Implementation Steps

### Phase 2: Lightbar Menus (Completed)

This phase is now implemented:

- `ui_lightbar` (D1) vertical lightbar menu
- `ui_lightbar_pos` (D2) positioned / multi-column lightbar menu
- `ui_select_prompt` (D3) inline select prompt
- Hotkey marker parsing and single-character highlighting
- Bracket display modes (`UI_BRACKET_*`)
- Cursor hiding during interaction
- Style support for `hotkey_highlight_attr`

### Phase 3: Form Building (In Progress)

**Design Decision:** Struct-based approach for both C and MEX interfaces.

**C API Design:**
- `ui_form_field_t` struct with position, label, type, validation, and value storage
- `ui_form_style_t` for form-level defaults and save mode configuration
- `ui_form_run()` accepts array of fields, modifies values in-place, returns save/cancel status
- Field types: `UI_FIELD_TEXT`, `UI_FIELD_MASKED`, `UI_FIELD_FORMAT`, `UI_FIELD_OPTION`
- Save modes: `UI_FORM_SAVE_CTRL_S` (immediate), `UI_FORM_SAVE_ESC_PROMPT` (Edit/Save/Exit)

**MEX Interface Design:**
- `struct ui_form_field` with all field properties (name, label, x, y, width, type, value, etc.)
- `struct ui_form_style` for form configuration
- `ui_form_run(ref array of struct ui_form_field, int count, ref struct ui_form_style)` returns 1=saved, 0=cancelled
- Field values modified in-place in the array
- Sane defaults allow minimal field configuration (only set what you need)

**Implementation Phases:**

**Phase 3A: Basic form runner** (Current)
- Text fields only (`UI_FIELD_TEXT`)
- Spatial navigation (center-based nearest-neighbor for arrow keys)
- Tab/Shift-Tab sequential navigation
- Ctrl+S save mode
- Required field validation
- Format mask support (already implemented in `ui_edit_field`)

**Phase 3B: Field types**
- Masked fields (`UI_FIELD_MASKED` - password)
- Format mask fields (`UI_FIELD_FORMAT` - phone, date, etc.)
- Option/toggle fields (`UI_FIELD_OPTION` - lightbar-style selection)

**Phase 3C: Advanced features**
- ESC prompt save mode (Edit/Save/Exit)
- Hotkey field jumping
- Required field splash message
- Per-field color overrides

### Phase 4: Advanced Features (Optional)

- Structured mask editing with format enforcement
- Save-under / shadow screen buffer
- Multi-page forms with pagination
- Custom validation hooks and normalization

---
