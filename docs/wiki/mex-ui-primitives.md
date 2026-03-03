---
layout: default
title: "UI Primitives"
section: "mex"
description: "Screen control, field editors, lightbar menus, forms, and scrollable viewers"
permalink: /mex-ui-primitives/
---

This is where your BBS stops looking like a stream of text and starts looking
like *software*. Cursor-addressed screens. Color-blocked headers. Bounded
input fields that scroll when the text gets long. Lightbar menus you navigate
with arrow keys. Multi-field forms with Tab navigation and validation.
Scrollable text viewers for help screens and log windows.

All of this lives in `maxui.mh` and works alongside `max.mh`:

```mex
#include <max.mh>
#include <maxui.mh>
```

Everything here works with ANSI and AVATAR terminals automatically. You
don't need to emit raw escape sequences — the display layer handles
translation based on the caller's terminal type. You paint screens, the
system worries about the protocol.

---

## On This Page

- [Quick Reference](#quick-reference)
- [Low-Level Screen Primitives](#low-level)
- [Field Editors](#field-editors)
- [Lightbar Menus](#lightbar-menus)
- [Forms](#forms)
- [Scrolling Regions & Text Viewers](#scrolling)
- [Constants Reference](#constants)
- [Complete Examples](#examples)

---

## Quick Reference {#quick-reference}

### Screen Primitives

| Function | What It Does |
|----------|-------------|
| [`ui_goto()`](#ui-goto) | Move cursor to a screen position |
| [`ui_set_attr()`](#ui-set-attr) | Set foreground + background color attribute |
| [`ui_fill_rect()`](#ui-fill-rect) | Fill a rectangle with a character and color |
| [`ui_write_padded()`](#ui-write-padded) | Write a padded string at a position |
| [`ui_read_key()`](#ui-read-key) | Read a keypress (handles arrow/function keys) |

### Field Editors

| Function | What It Does |
|----------|-------------|
| [`ui_edit_field()`](#ui-edit-field) | Positioned, styled text input field |
| [`ui_prompt_field()`](#ui-prompt-field) | Prompt string + inline edit field |

### Lightbar & Selection

| Function | What It Does |
|----------|-------------|
| [`ui_lightbar()`](#ui-lightbar) | Vertical lightbar menu |
| [`ui_lightbar_pos()`](#ui-lightbar-pos) | Positioned / multi-column lightbar |
| [`ui_select_prompt()`](#ui-select-prompt) | Inline Yes/No style selection |

### Forms

| Function | What It Does |
|----------|-------------|
| [`ui_form_run()`](#ui-form-run) | Multi-field form with navigation and validation |

### Scrolling & Viewing

| Function | What It Does |
|----------|-------------|
| [`ui_scroll_region_*()`](#scroll-region) | Append-only scrollable text panel |
| [`ui_text_viewer_*()`](#text-viewer) | Read-only paging text viewer |

---

## Low-Level Screen Primitives {#low-level}

These are the building blocks. Most scripts won't use them directly — the
higher-level widgets (fields, lightbars, forms) call them internally. But
they're available when you need pixel-perfect control.

{: #ui-goto}
### ui_goto(row, col)

Move the cursor to an absolute screen position.

```mex
void ui_goto(int: row, int: col);
```

Coordinates are **1-based** — `ui_goto(1, 1)` is the top-left corner.

```mex
ui_goto(10, 5);
print("Hello!");   // prints at row 10, column 5
```

{: #ui-set-attr}
### ui_set_attr(attr)

Set the current display attribute (foreground + background color).

```mex
void ui_set_attr(int: attr);
```

The attribute byte is `foreground + (background * 16)`. Use the `UI_*`
constants from `maxui.mh` for readability:

```mex
ui_set_attr(UI_YELLOWONBLUE);
print("Highlighted text");
```

{: #ui-fill-rect}
### ui_fill_rect(row, col, width, height, ch, attr)

Fill a rectangular screen area with a character and attribute.

```mex
void ui_fill_rect(int: row, int: col, int: width, int: height,
                  char: ch, int: attr);
```

```mex
// Draw a cyan header bar across the top row
ui_fill_rect(1, 1, 80, 1, ' ', UI_CYAN);
```

{: #ui-write-padded}
### ui_write_padded(row, col, width, s, attr)

Write a string padded to a fixed width with a given attribute. Useful for
labels, headers, and status bars.

```mex
void ui_write_padded(int: row, int: col, int: width, string: s, int: attr);
```

```mex
ui_write_padded(1, 1, 80, "[ Main Menu ]", UI_CYAN);
```

{: #ui-read-key}
### ui_read_key()

Read a single keypress. Returns an integer key code consistent with the C
layer (`keys.h`). Handles arrow keys, function keys, and escape sequences
transparently.

```mex
int: k;
k := ui_read_key();
```

All the higher-level widgets use this internally for their input loops.

---

## Field Editors {#field-editors}

Field editors give you bounded, styled text input — the kind you'd use for
login forms, registration screens, or any prompt where you want control over
position, width, and appearance.

{: #ui-edit-field}
### ui_edit_field

Low-level field editor with full control over positioning and style.

```mex
string ui_edit_field(int: row, int: col, int: width, int: max_len,
                     string: buf, ref struct ui_edit_field_style: style);
```

| Parameter | Purpose |
|-----------|---------|
| `row`, `col` | Screen position (1-based) |
| `width` | Visible width on screen |
| `max_len` | Maximum input length (can exceed `width` — field scrolls) |
| `buf` | Initial value (pre-fill) |
| `style` | Style struct (see below) |

**Returns** the edited string. The result code is stored in `regs_2[0]`.

**Style struct:**

```mex
struct ui_edit_field_style
{
  int: normal_attr;     // Attribute when field is unfocused
  int: focus_attr;      // Attribute when field is active
  char: fill_ch;        // Background fill character (' ', '_', etc.)
  int: flags;           // UI_EDIT_FLAG_* constants
  string: format_mask;  // Format mask for structured input
};
```

Initialize with `ui_edit_field_style_default(es)` before customizing.

**Editing keys:** Left/Right/Home/End move within the field. Backspace
deletes left, Delete removes under cursor. Enter accepts, Escape cancels,
Tab/Down advance to next field, Shift-Tab/Up go to previous.

**Example:**

```mex
string: username;
struct ui_edit_field_style: es;

ui_edit_field_style_default(es);
es.normal_attr := UI_GRAY;
es.focus_attr  := UI_YELLOWONBLUE;
es.fill_ch     := ' ';
es.flags       := 0;

ui_goto(5, 1);
print("Username: ");
username := ui_edit_field(5, 11, 20, 30, "", es);
```

{: #ui-prompt-field}
### ui_prompt_field

Combines a prompt string and an edit field in one call. The prompt is
displayed at the current cursor position, then the field follows.

```mex
string ui_prompt_field(string: prompt, int: width, int: max_len,
                       string: buf,
                       ref struct ui_prompt_field_style: style);
```

**Style struct:**

```mex
struct ui_prompt_field_style
{
  int: prompt_attr;    // Color for the prompt text
  int: field_attr;     // Color for the input field
  char: fill_ch;       // Background fill character
  int: flags;          // UI_EDIT_FLAG_* constants
  int: start_mode;     // Where to position (see prompt start modes)
  string: format_mask; // Format mask
};
```

**Example:**

```mex
string: email;
struct ui_prompt_field_style: ps;

ui_prompt_field_style_default(ps);
ps.prompt_attr := UI_YELLOW;
ps.field_attr  := UI_YELLOWONBLUE;
ps.fill_ch     := ' ';
ps.start_mode  := UI_PROMPT_START_HERE;

email := ui_prompt_field("Email address: ", 40, 60, "", ps);
```

---

## Lightbar Menus {#lightbar-menus}

Lightbar menus let your caller select from a list using arrow keys. The
selected item is highlighted with a distinct attribute — the classic BBS
lightbar look.

{: #ui-lightbar}
### ui_lightbar — Vertical List

```mex
int ui_lightbar(ref array [1..] of string: items, int: count,
                int: x, int: y, int: width,
                ref struct ui_lightbar_style: style);
```

Returns the **1-based** selected index on accept, or `-1` on cancel (Escape).

**Style struct:**

```mex
struct ui_lightbar_style
{
  int: justify;              // Text justification
  int: wrap;                 // Wrap around at ends
  int: enable_hotkeys;       // Allow single-key selection
  int: show_brackets;        // Bracket style (see constants)
  int: normal_attr;          // Unselected item color
  int: selected_attr;        // Highlighted item color
  int: hotkey_attr;          // Hotkey letter color (unselected)
  int: hotkey_highlight_attr;// Hotkey letter color (selected)
  int: margin;               // Extra padding columns
  int: out_hotkey;           // Output: selected hotkey character
};
```

**Hotkey coloring:** The first uppercase letter in each item string becomes
the hotkey. When unselected, it renders with `hotkey_attr`. When selected,
it uses `hotkey_highlight_attr`. Set either to `0` to use the row's
normal/selected attribute instead.

**Example:**

```mex
string: items[3];
struct ui_lightbar_style: ls;
int: choice;

items[1] := "Read Messages";
items[2] := "File Areas";
items[3] := "Goodbye";

ui_lightbar_style_default(ls);
ls.normal_attr   := UI_GRAY;
ls.selected_attr := UI_YELLOWONBLUE;
ls.wrap          := 1;

choice := ui_lightbar(items, 3, 5, 10, 20, ls);
```

{: #ui-lightbar-pos}
### ui_lightbar_pos — Positioned/Multi-Column

For menus where items aren't in a simple vertical list — each item has its
own screen position and width.

```mex
int ui_lightbar_pos(ref array [1..] of struct ui_lightbar_item: items,
                    int: count,
                    ref struct ui_lightbar_style: style);
```

**Item struct:**

```mex
struct ui_lightbar_item
{
  string: text;
  int: x;
  int: y;
  int: width;
  int: justify;
};
```

Arrow keys navigate using a **nearest-neighbor heuristic** in 2D space —
pressing right jumps to the closest item to the right, pressing down finds
the closest below. This works naturally for grid layouts, L-shaped menus,
and scattered button arrangements.

{: #ui-select-prompt}
### ui_select_prompt — Inline Selection

For Yes/No style prompts that appear inline with text:

```mex
int ui_select_prompt(string: prompt,
                     ref array [1..] of string: options, int: count,
                     ref struct ui_select_prompt_style: style);
```

**Style struct:**

```mex
struct ui_select_prompt_style
{
  int: prompt_attr;      // Prompt text color
  int: normal_attr;      // Unselected option color
  int: selected_attr;    // Highlighted option color
  int: hotkey_attr;      // Hotkey color in options
  int: show_brackets;    // Bracket style
  int: margin;           // Padding on each side when highlighted
  string: separator;     // String between options (e.g., " or ")
  int: default_index;    // 1-based pre-selected option (0 = first)
  int: out_hotkey;       // Output: selected hotkey character
};
```

The cursor is hidden during selection and restored on exit.

---

## Forms {#forms}

{: #ui-form-run}
`ui_form_run` manages a complete multi-field form with Tab/Shift-Tab
navigation, field validation, hotkeys, and save/cancel handling.

```mex
int ui_form_run(ref array [1..] of struct ui_form_field: fields,
                int: count, ref struct ui_form_style: style);
```

Returns `1` if saved (Ctrl+S), `0` if cancelled (Escape), `-1` on error.

**Field struct:**

```mex
struct ui_form_field
{
  string: name;          // Field identifier
  string: label;         // Display label
  int: x;               // Field X position
  int: y;               // Field Y position
  int: width;            // Visible width
  int: max_len;          // Max input length
  int: field_type;       // UI_FIELD_TEXT or UI_FIELD_FORMAT
  char: hotkey;          // Direct-jump hotkey
  int: required;         // 1 = must be non-empty to save
  int: label_attr;       // Label color (0 = use form default)
  int: normal_attr;      // Field color unfocused (0 = form default)
  int: focus_attr;       // Field color focused (0 = form default)
  string: format_mask;   // Mask for formatted fields
  string: value;         // Current/output value
};
```

**Style struct:**

```mex
struct ui_form_style
{
  int: label_attr;       // Default label color
  int: normal_attr;      // Default field color
  int: focus_attr;       // Default focus color
  int: save_mode;        // Save behavior
  int: wrap;             // Wrap navigation at edges
  string: required_msg;  // Message shown for empty required fields
  int: required_x;       // Position for required message
  int: required_y;
  int: required_attr;    // Color for required message
};
```

**Navigation:** Arrow keys use 2D nearest-neighbor (like `ui_lightbar_pos`).
Tab/Shift-Tab move sequentially. Enter edits the selected field. Ctrl+S
saves. Escape cancels.

**Cursor behavior:** While navigating between fields, the cursor is hidden.
While actively editing a field (after pressing Enter), the cursor is shown
so the user sees the insert position.

**Required fields:** Fields with `required=1` must be non-empty to save.
For text fields, whitespace-only input counts as empty. On save, values are
updated in-place in the `fields` array.

---

## Scrolling Regions & Text Viewers {#scrolling}

These are the heavy-hitters for content display — scrollable text panels
that handle their own navigation internally.

{: #scroll-region}
### ScrollingRegion

An append-only scrollable text panel. Lines are added over time and the
region auto-scrolls to follow new content (like a log viewer or chat
window).

```mex
// Lifecycle
ui_scroll_region_style_default(ref style);
ui_scroll_region_create(key, x, y, w, h, max_lines, style);
ui_scroll_region_destroy(key);

// Content
ui_scroll_region_append(key, text, flags);

// Display
ui_scroll_region_render(key);

// Input
ui_scroll_region_handle_key(key, keycode);  // returns non-zero if consumed
```

The `key` is a string identifier (like `"sr"`) — you can have multiple
regions active simultaneously.

{: #text-viewer}
### TextBufferViewer

A read-only text viewer with paging. Set the full text once, then let the
user scroll through it.

```mex
// Lifecycle
ui_text_viewer_style_default(ref style);
ui_text_viewer_create(key, x, y, w, h, style);
ui_text_viewer_destroy(key);

// Content
ui_text_viewer_set_text(key, text);

// Display
ui_text_viewer_render(key);

// Input (two patterns)
ui_text_viewer_handle_key(key, keycode);  // returns non-zero if consumed
ui_text_viewer_read_key(key);             // returns 0 if consumed, key if not
```

### Input Loop Patterns

**Pattern A — Focus routing (multiple widgets):**

Use when you have multiple interactive regions on one screen (e.g., a
scrolling feed + a viewer + a command bar):

```mex
int: focus;  // 0=region, 1=viewer
int: k;

focus := 0;

while (1)
{
  k := ui_read_key();

  if (k = 27)  // ESC
    return;

  if (k = 't' or k = 'T')
    focus := 1 - focus;         // toggle focus
  else if (focus = 0)
  {
    if (ui_scroll_region_handle_key("sr", k))
      ui_scroll_region_render("sr");
  }
  else
  {
    if (ui_text_viewer_handle_key("tv", k))
      ui_text_viewer_render("tv");
  }
}
```

**Pattern B — Viewer-driven (single widget):**

Use when the viewer is the primary UI. Scroll keys are consumed internally;
everything else passes through as a command:

```mex
int: k;

while (1)
{
  k := ui_text_viewer_read_key("tv");
  if (k = 0)
    ;                            // viewer consumed it
  else if (k = 27)
    return;
  else
  {
    // command keys (R, [, ], etc.)
  }
}
```

---

## Constants Reference {#constants}

### Color Attributes

Integer attribute values for `ui_set_attr` and style structs:

```mex
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
#define UI_YELLOWONBLUE   0x1e
#define UI_BLACKONGREEN   0x20
#define UI_REDONGREEN     0x2c
#define UI_WHITEONGREEN   0x2f
```

**Formula:** `foreground + (background * 16)`. For example, white on blue =
`0x0f + (0x01 * 16)` = `0x1f`.

### Edit Result Codes

Returned in `regs_2[0]` after field editing:

| Constant | Value | Meaning |
|----------|-------|---------|
| `UI_EDIT_ERROR` | 0 | Error occurred |
| `UI_EDIT_CANCEL` | 1 | User pressed Escape |
| `UI_EDIT_ACCEPT` | 2 | User pressed Enter |
| `UI_EDIT_PREVIOUS` | 3 | User pressed Up/Shift-Tab |
| `UI_EDIT_NEXT` | 4 | User pressed Down/Tab |
| `UI_EDIT_NOROOM` | 5 | Not enough screen space |

### Edit Flags

| Constant | Value | Purpose |
|----------|-------|---------|
| `UI_EDIT_FLAG_ALLOW_CANCEL` | `0x0020` | Allow Escape to cancel |
| `UI_EDIT_FLAG_FIELD_MODE` | `0x0002` | Field mode (vs line mode) |
| `UI_EDIT_FLAG_MASK` | `0x0004` | Mask input with `fill_ch` (for passwords) |

### Bracket Styles

For lightbar and select prompt `show_brackets`:

| Constant | Value | Renders as |
|----------|-------|-----------|
| `UI_BRACKET_NONE` | 0 | `Item` |
| `UI_BRACKET_SQUARE` | 1 | `[Item]` |
| `UI_BRACKET_ROUNDED` | 2 | `(Item)` |
| `UI_BRACKET_CURLY` | 3 | `{Item}` |

### Prompt Start Modes

For `ui_prompt_field`:

| Constant | Value | Behavior |
|----------|-------|---------|
| `UI_PROMPT_START_HERE` | 0 | Start at current cursor position |
| `UI_PROMPT_START_CR` | 1 | Start at beginning of line |
| `UI_PROMPT_START_CLBOL` | 2 | Clear to BOL, then start |

---

## Complete Examples {#examples}

### Simple Login Form

```mex
#include <max.mh>
#include <maxui.mh>

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
  es.focus_attr  := UI_YELLOWONBLUE;
  es.fill_ch     := ' ';
  username := ui_edit_field(5, 11, 20, 30, "", es);

  // Password field (masked)
  ui_goto(7, 1);
  ui_set_attr(UI_YELLOW);
  print("Password: ");
  ui_edit_field_style_default(es);
  es.normal_attr := UI_GRAY;
  es.focus_attr  := UI_YELLOWONBLUE;
  es.fill_ch     := '*';
  es.flags       := UI_EDIT_FLAG_MASK;
  password := ui_edit_field(7, 11, 20, 20, "", es);

  // Result
  ui_goto(10, 1);
  if (strlen(username) > 0 && strlen(password) > 0)
    print("Login successful!\n");
  else
    print("Login cancelled.\n");
}
```

### Registration Form with Prompt Fields

```mex
#include <max.mh>
#include <maxui.mh>

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

  ui_prompt_field_style_default(ps);
  ps.prompt_attr := UI_YELLOW;
  ps.field_attr  := UI_YELLOWONBLUE;
  ps.fill_ch     := ' ';
  ps.start_mode  := UI_PROMPT_START_HERE;

  name  := ui_prompt_field("Full Name: ", 30, 50, "", ps);
  email := ui_prompt_field("Email: ", 40, 60, "", ps);

  ps.fill_ch := '_';
  city := ui_prompt_field("City: ", 25, 30, "", ps);

  print("\n\nThank you, " + name + "!\n");
}
```

---

## See Also

- [Standard Intrinsics]({% link mex-standard-intrinsics.md %}) — the core
  MEX functions for I/O, user data, and file access
- [Display Codes]({% link display-codes.md %}) — pipe color codes and MCI
  formatting operators
- [MEX Getting Started]({% link mex-getting-started.md %}) — introduction
  to MEX scripting and the compiler
