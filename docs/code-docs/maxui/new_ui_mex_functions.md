# UI MEX Intrinsics Reference

This document describes the UI (User Interface) intrinsics available to MEX scripts in Maximus BBS. These functions provide low-level control over screen positioning, attributes, and bounded input fields.

## Table of Contents

1. [Headers](#headers)
2. [API Usage (C)](#api-usage-c)
3. [API Usage (MEX)](#api-usage-mex)
2. [Low-Level Primitives](#low-level-primitives)
3. [Field Editing Functions](#field-editing-functions)
4. [Lightbar and Select Prompt](#lightbar-and-select-prompt)
5. [Constants](#constants)
6. [Examples](#examples)

---

## Headers

UI-related constants, structures, and function prototypes are declared in:

```c
#include <maxui.mh>
```

Core/non-UI intrinsics remain in:

```c
#include <max.mh>
```

---

## API Usage (C)

This section describes how to use the underlying C APIs that the MEX intrinsics wrap.

### ScrollingRegion (C)

- **[header]** `#include "ui_scroll.h"`
- **[lifecycle]**
  - `ui_scrolling_region_style_default(&style);`
  - `ui_scrolling_region_init(&r, x, y, w, h, max_lines, &style);`
  - `ui_scrolling_region_free(&r);`
- **[append]**
  - `ui_scrolling_region_append(&r, "line...", UI_SCROLL_APPEND_FOLLOW);`
- **[render]**
  - `ui_scrolling_region_render(&r);`
- **[input loop]**
  - Read a key with `ui_read_key()`.
  - Pass it to `ui_scrolling_region_handle_key(&r, key)`.
  - If it returns `1`, re-render.
  - If it returns `0`, treat the key as a “command” key for the caller.

### TextBufferViewer (C)

- **[header]** `#include "ui_scroll.h"`
- **[lifecycle]**
  - `ui_text_viewer_style_default(&style);`
  - `ui_text_viewer_init(&v, x, y, w, h, &style);`
  - `ui_text_viewer_free(&v);`
- **[set buffer]**
  - `ui_text_viewer_set_text(&v, text);`
- **[render]**
  - `ui_text_viewer_render(&v);`
- **[two input patterns]**
  - **[pattern A: handle_key]** caller does `key = ui_read_key();` and calls `ui_text_viewer_handle_key(&v, key)`.
  - **[pattern B: read_key]** caller does `key = ui_text_viewer_read_key(&v);`.
    - returns `0` if the viewer consumed a navigation key (and re-rendered)
    - returns the key code for non-navigation keys (caller command)

### Overlays / snapshot-restore (C)

- **[shadow buffer]** implemented in `ui_shadowbuf.h/.c`.
- **[widget helpers]**
  - `ui_scrolling_region_overlay_push()` / `ui_scrolling_region_overlay_pop()`
  - `ui_text_viewer_overlay_push()` / `ui_text_viewer_overlay_pop()`

---

## API Usage (MEX)

This section describes:

- **[intrinsic API surface]** what functions exist in `maxui.mh`
- **[usage patterns]** how to structure your MEX input loop so “command” keys pass through

### Intrinsic surface (ScrollingRegion / TextBufferViewer)

Headers:

```c
#include <max.mh>
#include <maxui.mh>
```

Keys:

- **[ui_read_key()]** returns integer key codes consistent with C (`keys.h`)

ScrollingRegion:

- **[style]** `ui_scroll_region_style_default(ref style)`
- **[create/destroy]** `ui_scroll_region_create(key, x, y, w, h, max_lines, style)` / `ui_scroll_region_destroy(key)`
- **[append]** `ui_scroll_region_append(key, text, flags)`
- **[render]** `ui_scroll_region_render(key)`
- **[key handling]** `ui_scroll_region_handle_key(key, keycode)` (returns non-zero if consumed)

TextBufferViewer:

- **[style]** `ui_text_viewer_style_default(ref style)`
- **[create/destroy]** `ui_text_viewer_create(key, x, y, w, h, style)` / `ui_text_viewer_destroy(key)`
- **[set text]** `ui_text_viewer_set_text(key, text)`
- **[render]** `ui_text_viewer_render(key)`
- **[key handling]** `ui_text_viewer_handle_key(key, keycode)`
- **[key helper]** `ui_text_viewer_read_key(key)`

### MEX pattern A: focus routing (non-blocking)

Use this when you want a page with multiple interactive regions (ex: feed + viewer + command bar).

```c
int: focus;  // 0=region, 1=viewer
int: k;

focus := 0;

while (1)
{
  k := ui_read_key();

  if (k = 27)  // ESC
    return;

  if (k = 't' or k = 'T')
    focus := 1 - focus;
  else if (focus = 0)
  {
    if (ui_scroll_region_handle_key("sr", k))
      ui_scroll_region_render("sr");
    else
    {
      // command keys for caller
    }
  }
  else
  {
    if (ui_text_viewer_handle_key("tv", k))
      ui_text_viewer_render("tv");
    else
    {
      // command keys for caller
    }
  }
}
```

### MEX pattern B: viewer-driven command passthrough

Use this when the viewer is the primary UI and you want the “scroll keys are internal, everything else is a command” behavior.

```c
int: k;

while (1)
{
  k := ui_text_viewer_read_key("tv");
  if (k = 0)
    ;
  else if (k = 27)
    return;
  else
  {
    // command keys (R, [, ], etc.)
  }
}
```

### Rebuilding MEX scripts

After editing `resources/m/*.mex`, rebuild the VM bytecode:

```sh
cd resources/m
../../build/bin/mex uitest.mex
cp -f uitest.vm ../../build/m/uitest.vm
```

---

## Low-Level Primitives

### ui_goto(row, col)

Position the cursor at the specified screen coordinates.

**Signature:**
```c
void ui_goto(int: row, int: col);
```

**Parameters:**
- `row` - Row number (1-based)
- `col` - Column number (1-based)

**Example:**
```c
ui_goto(10, 5);  // Move cursor to row 10, column 5
print("Hello!");
```

---

### ui_set_attr(attr)

Set the current display attribute (color/style).

**Signature:**
```c
void ui_set_attr(int: attr);
```

**Parameters:**
- `attr` - Attribute byte (foreground + background * 16)

**Example:**
```c
ui_set_attr(UI_YELLOWONBLUE);  // Yellow text on blue background
print("Highlighted text");
```

**Note:** Use the `UI_*` color constants from `maxui.mh` for readability.

---

### ui_fill_rect(row, col, width, height, ch, attr)

Fill a rectangular area with a character and attribute.

**Signature:**
```c
void ui_fill_rect(int: row, int: col, int: width, int: height, char: ch, int: attr);
```

**Parameters:**
- `row` - Starting row (1-based)
- `col` - Starting column (1-based)
- `width` - Width in columns
- `height` - Height in rows
- `ch` - Fill character
- `attr` - Display attribute

**Example:**
```c
// Draw a cyan header bar across the top
ui_fill_rect(1, 1, 80, 1, ' ', UI_CYAN);
```

---

### ui_write_padded(row, col, width, s, attr)

Write a string padded to a specific width with a given attribute.

**Signature:**
```c
void ui_write_padded(int: row, int: col, int: width, string: s, int: attr);
```

**Parameters:**
- `row` - Row position (1-based)
- `col` - Column position (1-based)
- `width` - Total width to pad to
- `s` - String to write
- `attr` - Display attribute

**Example:**
```c
// Center a title in an 80-column header
ui_write_padded(1, 1, 80, "[ Main Menu ]", UI_CYAN);
```

---

## Field Editing Functions

### ui_edit_field(row, col, width, max_len, buf, style)

Low-level bounded field editor with full control over positioning and appearance.

**Signature:**
```c
string ui_edit_field(int: row, int: col, int: width, int: max_len, string: buf, ref struct ui_edit_field_style: style);
```

**Style struct:**
```c
struct ui_edit_field_style
{
  int: normal_attr;
  int: focus_attr;
  char: fill_ch;
  int: flags;
  string: format_mask;
};
```

**Returns:**
- Edited string content
- Result code in `regs_2[0]` (see `UI_EDIT_*` constants)

**Editing keys:**
- Left/Right/Home/End move within the field.
- Backspace deletes the character to the left of the cursor.
- Delete removes the character under the cursor.

**Example:**
```c
string: username;
struct ui_edit_field_style: es;

ui_edit_field_style_default(es);
es.normal_attr := UI_GRAY;
es.focus_attr := UI_YELLOWONBLUE;
es.fill_ch := ' ';
es.flags := 0;

ui_goto(5, 1);
print("Username: ");
username := ui_edit_field(5, 11, 20, 30, "", es);
```

---

### ui_prompt_field(prompt, width, max_len, buf, style)

Display a prompt and edit field in one call. The prompt is displayed at the current cursor position.

**Signature:**
```c
string ui_prompt_field(string: prompt, int: width, int: max_len, string: buf, ref struct ui_prompt_field_style: style);
```

**Style struct:**
```c
struct ui_prompt_field_style
{
  int: prompt_attr;
  int: field_attr;
  char: fill_ch;
  int: flags;
  int: start_mode;
  string: format_mask;
};
```

**Returns:**
- Edited string content
- Result code in `regs_2[0]`

**Editing keys:**
- Key handling is shared with `ui_edit_field`.

**Example:**
```c
string: email;
struct ui_prompt_field_style: ps;

ui_prompt_field_style_default(ps);
ps.prompt_attr := UI_YELLOW;
ps.field_attr := UI_YELLOWONBLUE;
ps.fill_ch := ' ';
ps.flags := 0;
ps.start_mode := UI_PROMPT_START_HERE;

email := ui_prompt_field("Email address: ", 40, 60, "", ps);
```

---

## Constants

### Edit Result Codes

Returned in `regs_2[0]` after field editing:

```c
#define UI_EDIT_ERROR     0  // Error occurred
#define UI_EDIT_CANCEL    1  // User cancelled (ESC)
#define UI_EDIT_ACCEPT    2  // User accepted (ENTER)
#define UI_EDIT_PREVIOUS  3  // User pressed UP/SHIFT-TAB
#define UI_EDIT_NEXT      4  // User pressed DOWN/TAB
#define UI_EDIT_NOROOM    5  // Not enough screen space
```

### Edit Flags

```c
#define UI_EDIT_FLAG_ALLOW_CANCEL  0x0020  // Allow ESC to cancel
#define UI_EDIT_FLAG_FIELD_MODE    0x0002  // Field mode (vs line mode)
#define UI_EDIT_FLAG_MASK          0x0004  // Mask input with fill_ch
```

---

## Implementation Notes

- UI widgets share centralized key decoding in the display layer (see `ui_read_key()` in the C implementation) so arrow keys and common escape sequences behave consistently across forms, lightbars, and field editors.

---

## Missing Functionality (Deferred)

- Robust longest-match ANSI escape decoding across many terminal variants.
- Time-bounded ESC parsing (treating bare ESC as a keypress vs. the start of an escape sequence based on a short timeout).

### Prompt Start Modes

```c
#define UI_PROMPT_START_HERE   0  // Start at current cursor position
#define UI_PROMPT_START_CR     1  // Start at beginning of line (CR)
#define UI_PROMPT_START_CLBOL  2  // Clear to BOL, then start
```

### Color Attributes

Integer attribute values for use with UI functions:

```c
// Foreground colors (on black background)
#define UI_BLACK      0x00
#define UI_BLUE       0x01
#define UI_GREEN      0x02
#define UI_CYAN       0x03
#define UI_RED        0x04
#define UI_MAGENTA    0x05
#define UI_BROWN      0x06
#define UI_GRAY       0x07
#define UI_DKGRAY     0x08
#define UI_LBLUE      0x09
#define UI_LGREEN     0x0a
#define UI_LCYAN      0x0b
#define UI_LRED       0x0c
#define UI_LMAGENTA   0x0d
#define UI_YELLOW     0x0e
#define UI_WHITE      0x0f

// Common foreground+background combinations
#define UI_YELLOWONBLUE   0x1e  // Yellow on blue
#define UI_BLACKONGREEN   0x20  // Black on green
#define UI_REDONGREEN     0x2c  // Red on green
#define UI_WHITEONGREEN   0x2f  // White on green
```

**Note:** Attribute byte format is `foreground + (background * 16)`. For example:
- White on blue = 15 + (1 * 16) = 31 = 0x1f
- Yellow on blue = 14 + (1 * 16) = 30 = 0x1e

---

## Examples

### Simple Login Form

```c
void main()
{
  string: username;
  string: password;
  struct ui_edit_field_style: es;
  
  print(AVATAR_CLS);
  
  // Draw header
  ui_fill_rect(1, 1, 80, 1, ' ', UI_CYAN);
  ui_write_padded(1, 1, 80, "[ System Login ]", UI_CYAN);
  
  // Username field
  ui_goto(5, 1);
  ui_set_attr(UI_YELLOW);
  print("Username: ");
  ui_edit_field_style_default(es);
  es.normal_attr := UI_GRAY;
  es.focus_attr := UI_YELLOWONBLUE;
  es.fill_ch := ' ';
  es.flags := 0;
  username := ui_edit_field(5, 11, 20, 30, "", es);
  
  // Password field (masked)
  ui_goto(7, 1);
  ui_set_attr(UI_YELLOW);
  print("Password: ");
  ui_edit_field_style_default(es);
  es.normal_attr := UI_GRAY;
  es.focus_attr := UI_YELLOWONBLUE;
  es.fill_ch := '*';
  es.flags := UI_EDIT_FLAG_MASK;
  password := ui_edit_field(7, 11, 20, 20, "", es);
  
  // Display result
  ui_goto(10, 1);
  if (strlen(username) > 0 && strlen(password) > 0)
    print("Login successful!\n");
  else
    print("Login cancelled.\n");
}
```

### Registration Form with Prompt Fields

```c
void main()
{
  string: name;
  string: email;
  string: city;
  struct ui_prompt_field_style: ps;
  
  print(AVATAR_CLS);
  ui_set_attr(UI_LGREEN);
  print("User Registration\n");
  print("=================\n\n");
  
  // Use prompt_field for simple fields
  ui_prompt_field_style_default(ps);
  ps.prompt_attr := UI_YELLOW;
  ps.field_attr := UI_YELLOWONBLUE;
  ps.fill_ch := ' ';
  ps.flags := 0;
  ps.start_mode := UI_PROMPT_START_HERE;
  name := ui_prompt_field("Full Name: ", 30, 50, "", ps);

  ui_prompt_field_style_default(ps);
  ps.prompt_attr := UI_YELLOW;
  ps.field_attr := UI_YELLOWONBLUE;
  ps.fill_ch := ' ';
  ps.flags := 0;
  ps.start_mode := UI_PROMPT_START_HERE;
  email := ui_prompt_field("Email: ", 40, 60, "", ps);
  
  // Use fill_ch with underscores as a visual guide
  ui_prompt_field_style_default(ps);
  ps.prompt_attr := UI_YELLOW;
  ps.field_attr := UI_YELLOWONBLUE;
  ps.fill_ch := '_';
  ps.flags := 0;
  ps.start_mode := UI_PROMPT_START_HERE;
  city := ui_prompt_field("City: ", 25, 30, "", ps);
  
  print("\n\nThank you, ", name, "!\n");
}
```

### Multi-Field Form with Navigation

```c
void main()
{
  string: field1, field2, field3;
  int: result;
  struct ui_edit_field_style: es;
  
  print(AVATAR_CLS);
  
  // Field 1
  ui_goto(5, 1);
  print("Field 1: ");
  ui_edit_field_style_default(es);
  es.normal_attr := UI_GRAY;
  es.focus_attr := UI_YELLOWONBLUE;
  es.fill_ch := ' ';
  es.flags := 0;
  field1 := ui_edit_field(5, 10, 20, 30, "", es);
  result := regs_2[0];
  
  if (result == UI_EDIT_CANCEL)
  {
    print("\nCancelled.\n");
    return;
  }
  
  // Field 2
  ui_goto(7, 1);
  print("Field 2: ");
  ui_edit_field_style_default(es);
  es.normal_attr := UI_GRAY;
  es.focus_attr := UI_YELLOWONBLUE;
  es.fill_ch := ' ';
  es.flags := 0;
  field2 := ui_edit_field(7, 10, 20, 30, "", es);
  
  // Field 3
  ui_goto(9, 1);
  print("Field 3: ");
  ui_edit_field_style_default(es);
  es.normal_attr := UI_GRAY;
  es.focus_attr := UI_YELLOWONBLUE;
  es.fill_ch := ' ';
  es.flags := 0;
  field3 := ui_edit_field(9, 10, 20, 30, "", es);
  
  print("\n\nResults:\n");
  print("Field 1: '", field1, "'\n");
  print("Field 2: '", field2, "'\n");
  print("Field 3: '", field3, "'\n");
}
```

---

## Implementation Notes

- All UI functions automatically handle ANSI/AVATAR translation based on the user's terminal type
- Field editors support insert/overwrite modes, backspace, delete, and cursor movement
- The `normal_attr` vs `focus_attr` distinction allows visual feedback for which field is active
- Masked fields (with `UI_EDIT_FLAG_MASK`) display the `fill_ch` instead of actual input
- Result codes in `regs_2[0]` allow scripts to handle cancel/navigation events
- All row/col coordinates are 1-based (not 0-based)
- `ui_lightbar` hides the terminal cursor during menu interaction and restores it on exit

---

## See Also

- `display-improvements-maximus.md` - Overall UI improvement design document
- `maxui.mh` - MEX UI header file with UI constants/structs/prototypes
- `ui_field.h` / `ui_field.c` - C implementation of UI primitives
- `mexui.c` - MEX intrinsic wrappers for UI functions

---

## Lightbar and Select Prompt

### Bracket display

Both `ui_lightbar` and `ui_select_prompt` style structs have a `show_brackets` field:

```c
#define UI_BRACKET_NONE      0
#define UI_BRACKET_SQUARE    1
#define UI_BRACKET_ROUNDED   2
#define UI_BRACKET_CURLY     3
```

### ui_lightbar(items, count, x, y, width, style)

Displays a vertical lightbar menu.

**Signature:**
```c
int ui_lightbar(ref array [1..] of string: items,
               int: count,
               int: x,
               int: y,
               int: width,
               ref struct ui_lightbar_style: style);
```

**Style struct:**
```c
struct ui_lightbar_style
{
  int: justify;
  int: wrap;
  int: enable_hotkeys;
  int: show_brackets;
  int: normal_attr;
  int: selected_attr;
  int: hotkey_attr;
  int: hotkey_highlight_attr;
  int: margin;
  int: out_hotkey;
};
```

**Hotkey coloring:**
- When an item is **not selected**, the hotkey character uses `hotkey_attr` if non-zero.
- When an item **is selected**, the hotkey character uses `hotkey_highlight_attr` if non-zero.
- If the relevant hotkey attribute is `0`, the hotkey character uses the row's normal/selected attribute.

**Margin:**
- `margin` (default `0`): extra columns added to the effective width
  - For `ui_lightbar`: added to the computed or specified menu width
  - For `ui_lightbar_pos`: added to each item's computed or specified width

**Returns:**
- 1-based selected index on accept
- `-1` on cancel

**Hotkey output:**
- `style.out_hotkey` receives the selected hotkey character (numeric codepoint)
- `0` if cancelled

### ui_lightbar_pos(items, count, style)

Displays a positioned/multi-column lightbar menu (each item has its own `x/y/width/justify`).

**Signature:**
```c
int ui_lightbar_pos(ref array [1..] of struct ui_lightbar_item: items,
                   int: count,
                   ref struct ui_lightbar_style: style);
```

**Item struct:**
```c
struct ui_lightbar_item
{
  string: text;
  int: x;
  int: y;
  int: width;
  int: justify;
};
```

**Navigation:**
- Arrow keys move selection using a nearest-neighbor heuristic in 2D space.

**Returns / output:**
- 1-based selected index on accept
- `-1` on cancel
- `style.out_hotkey` is set to selected hotkey (0 if cancelled)

### ui_select_prompt(prompt, options, count, style)

Displays an inline selection prompt (Yes/No style).

**Signature:**
```c
int ui_select_prompt(string: prompt,
                    ref array [1..] of string: options,
                    int: count,
                    ref struct ui_select_prompt_style: style);
```

**Style struct:**
```c
struct ui_select_prompt_style
{
  int: prompt_attr;
  int: normal_attr;
  int: selected_attr;
  int: hotkey_attr;
  int: show_brackets;
  int: margin;
  string: separator;
  int: default_index;
  int: out_hotkey;
};
```

**Returns:**
- 1-based selected index on accept
- `-1` on cancel

**Hotkey output:**
- `style.out_hotkey` receives the selected hotkey character (numeric codepoint)
- `0` if cancelled

**Default selection:**
- `default_index` (default `0`): 1-based index of the option to pre-select
  - `0` = first option is pre-selected (same as `1`)
  - `1` = first option, `2` = second, etc.
  - Values out of range are clamped to the first option

**Margin and separator:**
- `margin` (default `0`): adds padding spaces on **both sides** of each option when highlighted
  - `margin=0`: highlight covers exactly the option text (e.g. `Yes`)
  - `margin=1`: adds 1 space on each side (e.g. ` Yes `)
- `separator` (default `""`): string printed between options, rendered with `normal_attr`
  - Example: `separator=" or "` produces `Yes or No`

**Cursor behavior:**
- `ui_select_prompt` hides the terminal cursor during selection and restores it on exit.

### ui_form_run(fields, count, style)

Runs an interactive form with multiple input fields.

**Signature:**
```c
int ui_form_run(ref array [1..] of struct ui_form_field: fields,
               int: count,
               ref struct ui_form_style: style);
```

**Field struct:**
```c
struct ui_form_field
{
  string: name;
  string: label;
  int: x;
  int: y;
  int: width;
  int: max_len;
  int: field_type;
  char: hotkey;
  int: required;
  int: label_attr;
  int: normal_attr;
  int: focus_attr;
  string: format_mask;
  string: value;
};
```

**Style struct:**
```c
struct ui_form_style
{
  int: label_attr;
  int: normal_attr;
  int: focus_attr;
  int: save_mode;
  int: wrap;
  string: required_msg;
  int: required_x;
  int: required_y;
  int: required_attr;
};
```

**Returns:**
- `1` if saved
- `0` if cancelled
- `-1` on error

**Navigation / keys:**
- Arrow keys move selection using a nearest-neighbor heuristic in 2D space.
- `Tab` / `Shift-Tab` move sequentially.
- `Enter` edits the current field.
- `Ctrl+S` saves.
- `Esc` cancels.

**Cursor behavior:**
- While navigating the form, `ui_form_run` hides the terminal cursor.
- While actively editing a field (after `Enter`), the cursor is shown so the user can see the insert position.
- The cursor is restored on exit.

**Editing keys:**
- While editing a field, left/right arrow keys move within the field (they do not change the selected field).

**Required fields:**
- `required=1` enforces a non-empty field on save.
- For `UI_FIELD_TEXT`, whitespace-only input is treated as empty.
- For `UI_FIELD_FORMAT`, required means the raw value must contain at least the number of mask positions (clamped to `max_len`).

**Value output:**
- On save, `fields[i].value` is updated in-place.

**Format fields:**
- `UI_FIELD_FORMAT` stores the *raw* value (e.g. digits only) in `value`.

**Defaults:**
- Use `ui_form_style_default(style)` to initialize defaults.
