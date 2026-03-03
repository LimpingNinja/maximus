---
layout: default
title: "Building Menus: Press Any Key to Be Amazing"
section: "mex"
description: "Lesson 7 — Lightbar menus, selection prompts, and making your board feel like a place"
---

*Lesson 7 of Learning MEX*

---

> **What you'll build:** A custom lightbar menu — arrow keys move, Enter
> selects, and your board finally feels like *software* instead of
> scrolling text.

## 3:42 AM

You've built scripts that greet, interview, quiz, guess, and sign
guestbooks. They all work. They also all look the same — text flowing down
the terminal line by line, prompts waiting for typed input, the whole
experience feeling like a very polite interrogation.

Real BBS interfaces don't work that way. Real BBS interfaces have *menus*.
Highlight bars that slide up and down. Arrow keys that move. Enter that
selects. The kind of UI where the caller doesn't need instructions because
the interface explains itself.

This lesson introduces `maxui.mh` — MEX's UI library. It handles cursor
positioning, color attributes, highlight rendering, and keyboard input so
you don't have to do the ANSI math yourself. By the end you'll have a
lightbar menu that ties together everything you've built so far.

## Including the UI Library

```c
#include <max.mh>
#include <maxui.mh>
```

`maxui.mh` gives you the lightbar, selection prompt, edit fields, and
low-level screen primitives. It defines its own color constants (`UI_CYAN`,
`UI_WHITE`, etc.) and style structs that control how everything looks.

## Your First Lightbar

The `ui_lightbar()` function does the heavy lifting. You give it an array
of strings (the menu items), a position, a width, and a style struct. It
draws the menu, handles arrow keys, and returns the index of whatever the
caller picked.

Here's the simplest possible version:

```c
#include <max.mh>
#include <maxui.mh>

int main()
{
  array [1..3] of string: items;
  struct ui_lightbar_style: style;
  int: choice;

  items[1] := "Profile";
  items[2] := "Guestbook";
  items[3] := "Quit";

  ui_lightbar_style_default(style);
  choice := ui_lightbar(items, 3, 5, 3, 20, style);

  if (choice = 1)
    print("\nYou picked Profile.\n");
  else if (choice = 2)
    print("\nYou picked Guestbook.\n");
  else
    print("\nGoodbye.\n");

  return 0;
}
```

That's it. Three items. One function call. Arrow keys move the highlight,
Enter selects, and `choice` holds the result. The caller never types a
letter — they just *navigate*.

### The Parameters

```c
int ui_lightbar(ref array [1..] of string: items, int: count,
                int: x, int: y, int: width,
                ref struct ui_lightbar_style: style);
```

| Parameter | What It Does |
|-----------|-------------|
| `items` | Array of strings — the menu labels |
| `count` | How many items in the array |
| `x` | Column position (1 = left edge) |
| `y` | Row position (1 = top edge) |
| `width` | Width of each menu item in characters |
| `style` | A style struct controlling colors, wrapping, etc. |

Returns the 1-based index of the selected item, or `0` if the caller
pressed Escape.

## Styling the Lightbar

The `ui_lightbar_style` struct controls everything about how the lightbar
looks and behaves:

```c
struct ui_lightbar_style: style;
ui_lightbar_style_default(style);

// Customize
style.normal_attr := ui_make_attr(UI_CYAN, UI_BLACK);
style.selected_attr := ui_make_attr(UI_WHITE, UI_BLUE);
style.wrap := 1;
style.enable_hotkeys := 1;
style.show_brackets := UI_BRACKET_SQUARE;
```

### Key Style Fields

| Field | What It Does |
|-------|-------------|
| `normal_attr` | Color for unselected items |
| `selected_attr` | Color for the highlighted item |
| `wrap` | `1` = wrap from bottom to top (and vice versa) |
| `enable_hotkeys` | `1` = first unique letter of each item acts as a hotkey |
| `show_brackets` | `UI_BRACKET_NONE`, `UI_BRACKET_SQUARE` (`[item]`), `UI_BRACKET_ROUNDED`, `UI_BRACKET_CURLY` |
| `hotkey_attr` | Color for the hotkey character |

### Making Color Attributes

`ui_make_attr()` combines a foreground and background color into a single
attribute integer:

```c
int: attr;
attr := ui_make_attr(UI_WHITE, UI_BLUE);  // White text on blue background
```

The UI color constants are in `maxui.mh` — `UI_BLACK`, `UI_BLUE`,
`UI_GREEN`, `UI_CYAN`, `UI_RED`, `UI_MAGENTA`, `UI_BROWN`, `UI_GRAY`,
`UI_DKGRAY`, `UI_LBLUE`, `UI_LGREEN`, `UI_LCYAN`, `UI_LRED`,
`UI_LMAGENTA`, `UI_YELLOW`, `UI_WHITE`.

## The Inline Selection Prompt

Sometimes you don't need a full vertical lightbar — you just need a
horizontal "Pick one" prompt on a single line. That's `ui_select_prompt()`:

```c
array [1..3] of string: opts;
struct ui_select_prompt_style: pstyle;
int: pick;

opts[1] := "Yes";
opts[2] := "No";
opts[3] := "Maybe";

ui_select_prompt_style_default(pstyle);
pick := ui_select_prompt("Continue? ", opts, 3, pstyle);
```

This renders something like:

```
Continue? [Yes] No  Maybe
```

Arrow keys slide the highlight left and right. Enter picks. It's perfect
for confirmation dialogs and quick choices that don't deserve a whole
vertical menu.

## The Sysop Toolbox

Let's put it all together. A menu that launches the scripts you've built
in previous lessons — wrapped in a lightbar, running in a loop until the
caller quits. Call it `toolbox.mex`:

```c
#include <max.mh>
#include <maxui.mh>

#define NUM_ITEMS  5

void draw_header()
{
  // 0xCD = CP437 code for "═" (double horizontal line)
  // You could use '═' directly — this just shows that hex codes work.
  ui_fill_rect(1, 1, 40, 1, 0xCD, ui_make_attr(UI_LCYAN, UI_BLACK));
  ui_write_padded(2, 1, 40, "  Sysop Toolbox", ui_make_attr(UI_YELLOW, UI_BLACK));
  ui_fill_rect(3, 1, 40, 1, 0xCD, ui_make_attr(UI_LCYAN, UI_BLACK));
}

void draw_footer(int: row)
{
  ui_goto(row, 1);
  ui_set_attr(ui_make_attr(UI_DKGRAY, UI_BLACK));
  print("Arrow keys move, Enter selects, Esc quits");
  ui_set_attr(ui_make_attr(UI_GRAY, UI_BLACK));
}

int main()
{
  array [1..NUM_ITEMS] of string: items;
  struct ui_lightbar_style: style;
  int: choice;

  items[1] := " View Profile    ";
  items[2] := " Sign Guestbook  ";
  items[3] := " BBS Trivia      ";
  items[4] := " Number Game     ";
  items[5] := " Exit            ";

  ui_lightbar_style_default(style);
  style.normal_attr   := ui_make_attr(UI_CYAN, UI_BLACK);
  style.selected_attr := ui_make_attr(UI_WHITE, UI_BLUE);
  style.wrap := 1;
  style.enable_hotkeys := 1;
  style.show_brackets := UI_BRACKET_NONE;

  choice := 1;

  while (choice <> 0)
  {
    // Clear and draw the chrome
    print("\f");
    draw_header();
    draw_footer(5 + NUM_ITEMS + 2);

    // Show the lightbar — starts at row 5, column 5
    choice := ui_lightbar(items, NUM_ITEMS, 5, 5, 20, style);

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
  }

  print("\f|07");
  return 0;
}
```

### What's New Here

**Arrays.** `array [1..5] of string: items` declares a fixed-size array
of five strings. Arrays in MEX are 1-indexed — the first element is
`items[1]`, not `items[0]`. You fill them by assigning each element
individually.

**`ui_fill_rect()`** fills a rectangular region of the screen with a
character and color attribute. We use it to draw the top and bottom
border lines with the double-line box character (`═`).

**`ui_write_padded()`** writes a string at a specific position, padded to
a given width. Good for centered headers and labels.

**`ui_goto()` and `ui_set_attr()`** are the low-level primitives — move
the cursor, change the color. You won't need them often (the lightbar
handles its own rendering), but they're there for footer text and custom
chrome.

**`\f` (formfeed)** clears the screen. We do this at the top of each loop
iteration so the menu redraws cleanly after returning from a sub-script.

**`shell(0, ":profile")`** runs another MEX script from within your
script. The `:` prefix tells Maximus to treat the argument as a MEX
script name (without the `.vm` extension). When the sub-script finishes,
control returns to your menu loop.

**The menu loop.** The `while (choice <> 0)` loop keeps the menu alive.
Each time the caller picks something, we run it, then loop back and
redraw. Pressing Escape (which makes `ui_lightbar()` return `0`) or
picking "Exit" breaks the loop.

## Screen Primitives Reference

| Function | What It Does |
|----------|-------------|
| `ui_goto(row, col)` | Move cursor to position |
| `ui_set_attr(attr)` | Set current color attribute |
| `ui_make_attr(fg, bg)` | Combine foreground/background into one int |
| `ui_fill_rect(r, c, w, h, ch, attr)` | Fill a rectangle with a character |
| `ui_write_padded(r, c, w, str, attr)` | Write a string, padded to width |
| `ui_read_key()` | Read a single decoded keypress |

These are the building blocks. The lightbar and select-prompt are built on
top of them. For most scripts, you won't need the primitives directly —
but when you want to draw a box, paint a header, or position text
precisely, they're there.

## Compile and Run

```
mex toolbox.mex
```

Wire it to a menu and log in. You'll see a bordered menu with five items.
Arrow keys slide the highlight. Enter runs a script. Esc exits. It looks
like a real application.

Because it is one.

## What You Learned

- **`ui_lightbar()`** — a vertical lightbar menu. Pass an array of
  strings, get back the selected index. Arrow keys, Enter, Esc — all
  handled for you.
- **`ui_select_prompt()`** — an inline horizontal selector for quick
  choices.
- **Style structs** — `ui_lightbar_style`, `ui_select_prompt_style`. Call
  the `*_default()` initializer, then override what you want.
- **`ui_make_attr()`** — combine foreground and background colors.
- **Arrays** — `array [1..N] of type`. 1-indexed. Declared at the top of
  the function like all variables.
- **`shell(0, ":scriptname")`** — run another MEX script and return.
- **Menu loops** — draw, select, act, repeat. The fundamental UI pattern.

## Next

Your board has a menu now. But it only knows about itself — its own files,
its own users, its own little world. What if your scripts could read the
*message base*? Display recent posts, check for mail, search for topics?

That's where MEX stops being a toy and becomes infrastructure.

**[Lesson 8: "You've Got Mail" →]({% link mex-learn-messages.md %})**
