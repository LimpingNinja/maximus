# Notorious DoorKit Manual

Notorious DoorKit (`notorious_doorkit`) is a Python library for writing interactive BBS door programs with a classic “door feel”: CP437 output, ANSI cursor control, raw key input, lightbar menus, file display, and full-screen BBS “EDIT SCREEN” forms.

This kit was originally built to power doors inside NotoriousPTY, but it’s intentionally *not* “locked” to one system. At its core it’s a general-purpose Door32-friendly toolkit that can run under a plain BBS launcher just fine. If you are running under NotoriousPTY (or another launcher that speaks LNWP), you can opt into richer features (activity strings, node messaging, input-mode hints) — but the important part is: **LNWP is optional**.

If you’ve ever written doors (or BBS scripts) and wished you could do the same thing in Python without re-inventing keystroke parsing and terminal drawing, that’s the point of this kit. DoorKit handles the annoying, error-prone parts, and you keep the creative control: you decide the UX, the layout, and the style.

## Table of contents

- [Welcome to Notorious DoorKit!](#welcome-to-notorious-doorkit)
- [Overview](#overview)
- [Compiling/Linking (Python edition)](#compilinglinking-python-edition)
- [Running a Door Program](#running-a-door-program)
- [Basics of Door Programming](#basics-of-door-programming)
- [Theming your door](#theming-your-door)
- [Tour of a Sample Door](#tour-of-a-sample-door)
- [Module reference](#module-reference)
  - [ANSI (`ansi.py`)](#ansi-ansipy)
  - [Screen (`screen.py`)](#screen-screenpy)
  - [Text (`text.py`)](#text-textpy)
  - [Input (`input.py`)](#input-inputpy)
  - [Dropfiles (`dropfiles.py`)](#dropfiles-dropfilespy)
  - [Display (`display.py`)](#display-displaypy)
  - [Lightbars (`lightbar.py`)](#lightbars-lightbarpy)
  - [Forms (`forms.py`)](#forms-formspy)
  - [Boxes (`boxes.py`)](#boxes-boxespy)
  - [LNWP (`lnwp.py`, `lnwp_door.py`)](#lnwp-lnwppy-lnwp_doorpy)
- [Common recipes](#common-recipes)
- [Do’s and don’ts](#dos-and-donts)
- [Client compatibility notes](#client-compatibility-notes)

## Welcome to Notorious DoorKit!

Welcome! Notorious DoorKit is a toolkit for writing “real” interactive door programs in Python — the kind that feel like doors: fast menus, crisp screen layouts, and raw keystroke control. The goal is to make door programming feel like programming again, instead of constantly fighting terminal edge cases.

So what does it actually do for you?

DoorKit provides a small set of primitives that cover the boring-but-critical pieces: sending CP437 output (and doing it consistently), moving the cursor, clearing regions, reading keys without line buffering, and loading dropfile metadata so your door behaves correctly under whatever BBS launched it. On top of that, it gives you higher-level building blocks (lightbars, forms, paged file viewing) that you can compose into a complete UI.

Most importantly: it stays out of your way. This is not a GUI toolkit. It’s closer to “structured terminal painting”: you decide what gets drawn, when it gets redrawn, and what the user experience feels like.

## Overview

At the highest level, a door is just a tight loop:

1. Draw a screen (or a portion of one)
2. Read a key (raw mode)
3. Update state
4. Repeat

DoorKit gives you solid building blocks for each step, and tries hard to preserve the classic “BBS door” vibe. You can write a door as a single tight loop, or you can split it into screens/handlers; both styles work.

One OpenDoors-ish idea that DoorKit leans on heavily is that the **dropfile is the first source of truth**. If a BBS (or supervisor) tells you the user is ANSI, 80x25, and has 12 minutes left, take that and run with it. Probing exists, but it’s best-effort spice — not something you should bet your UI on.

DoorKit also has a clean separation between:

- The **terminal stream** (the bytes you draw to the caller, and the keys you read back)
- Optional **control-plane features** (like LNWP messaging and activity)

If you’re on a plain BBS launcher, you just use the terminal stream and ignore everything else. If you’re under NotoriousPTY, you can opt in to LNWP features when you want them.

## Compiling/Linking (Python edition)

In OpenDoors, “compiling and linking” is where you wire your program up to the library. In DoorKit, the same idea exists — it just looks like Python.

This repo doesn’t currently ship DoorKit as an installable PyPI package, so the normal “door author” workflow is:

- Vendor `notorious_doorkit/` next to your door script, or
- Add the repo root to `PYTHONPATH`, or
- Adjust `sys.path` from your door entrypoint.

Minimal “make DoorKit importable” pattern:

```python
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from notorious_doorkit import Door, clear_screen, writeln
```

DoorKit also supports a simple TOML configuration file (plus environment variable overrides). You can copy `notorious_doorkit/config.sample.toml` as a starting point and keep a `config.toml` next to your door.

If you like using a `.env` file for local development (so you don’t keep exporting variables by hand), that works fine too. The kit itself doesn’t require `python-dotenv`, but it’s a nice optional convenience when you’re running your door from a shell or an IDE.

## Running a Door Program

Most doors run in one of three ways:

1. **Launched by a BBS/launcher** with a node number and a dropfile path.
2. **Launched by a Door32-style supervisor** (same idea, but the terminal stream is a raw byte stream).
3. **Run locally for development**, without any BBS involved.

The classic launch convention (used by NotoriousPTY and many wrappers) is:

- `argv[1]` = node number
- `argv[2]` = dropfile path

Here’s a “real door entrypoint” that expects that convention:

```python
from notorious_doorkit import Door, clear_screen, writeln, get_runtime

door = Door()
door.start()

rt = get_runtime()

clear_screen()
writeln(f"Hello {rt.username}")
```

For local development, you can run the same door without a dropfile using `start_local()`. This is a great way to iterate on UI without having to launch the whole BBS:

```python
from notorious_doorkit import Door

door = Door()
door.start_local(username="Sysop", node=1)
```

After `Door.start(...)` or `Door.start_local(...)`, DoorKit exposes a single global runtime context (similar to OpenDoors' `od_control`). Most public APIs default to this runtime, so you typically do not pass `dropfile`, `terminal`, or screen size around.

Helpful hints: `Door.start()` changes the working directory to your door script directory (OpenDoors-style). That means relative paths like `"./screens/main.ans"` usually behave how you expect.

### Terminal I/O stream selection

DoorKit always parses the dropfile you pass to `Door.start()` (e.g. `DOOR.SYS`, `CHAIN.TXT`, `DORINFO1.DEF`, `DOOR32.SYS`) so the runtime can populate session metadata (username, time left, ANSI hints, etc.).

Separately from *parsing*, DoorKit chooses which file descriptor/stream to use for the terminal byte stream (output + key input):

- If the dropfile filename is `DOOR32.SYS` / `door32.sys`:
  - Read comm type from line 1 (`0=local`, `1=serial`, `2=telnet`).
  - Read comm/socket handle from line 2.
  - If comm type is telnet (`2`) and line 2 contains a non-zero handle, DoorKit uses that handle for all terminal reads/writes.
  - Otherwise, DoorKit falls back to stdin/stdout.
- For any other dropfile filename, DoorKit uses stdin/stdout.

## Basics of Door Programming

DoorKit works best when you embrace the “door mindset”: small, intentional redraws and a tight input loop. The fastest doors aren’t the ones that clear-screen on every key; they’re the ones that paint a stable background once and then update only what changed.

One practical tip: after you start the door, grab the runtime object once and use it as your “control object” (OpenDoors-style). It’s the most discoverable way to access door state in Python:

```python
from notorious_doorkit import get_runtime

rt = get_runtime()
rows, cols = rt.screen_size
username = rt.username
```

Some rules of thumb that will keep you out of trouble:

1. **Door author controls the UX**
   - Required fields are enforced by logic, but you decide how they look (asterisks, colors, legends).
2. **Keep redraws small and intentional**
   - Don’t clear-screen on every key. Repaint the part that changed.
3. **Prefer primitives over magic**
   - A form is a list of positioned fields.
   - A menu is a list of items.

Example: the “classic door shape” — loop a menu until the user exits:

```python
from notorious_doorkit import clear_screen
from notorious_doorkit import LightbarMenu

while True:
    clear_screen()
    choice = LightbarMenu(["Play", "Scores", "Exit"], x=5, y=5).run()
    if choice is None or choice == 2:
        break
```

## Theming your door

DoorKit ships with a global, config-driven UI theme that covers the common “door UI roles” (text, label, input, selected item, warnings, etc). If you do nothing, DoorKit will use the theme from `config.toml`.

Where this gets fun is that you can also build a theme in Python, tweak it, and set it as the *current theme* for the whole door. That way you don’t have to pass a pile of `*_color=` arguments everywhere.

### The simplest pattern: build once, set globally

```python
from notorious_doorkit import Door, build_theme
from notorious_doorkit import LightbarMenu

door = Door()
door.start()

theme = build_theme()  # starts from config.toml defaults

# Door-local tweaks (mutable)
theme.colors.selected = "{RED}{BOLD}"
theme.colors.key = "{YELLOW}{BOLD}"
theme.format.hotkey = "[X]"
theme.format.press_any_key = "{YELLOW}Press any key...{RESET}"

door.set_current_theme(theme)

# After this, helpers that have theme defaults will pick this up automatically.
choice = LightbarMenu(["Play", "Scores", "Exit"], x=5, y=5).run()
```

### One-shot override: pass a theme to just one call

Sometimes you want a special “mode” screen (danger mode, admin mode, etc) without changing your global defaults.

```python
from notorious_doorkit import build_theme
from notorious_doorkit import LightbarMenu

base = build_theme()
danger = build_theme(source=base)
danger.colors.selected = "{RED}{BOLD}"

choice = LightbarMenu(["Nuclear Option", "Back"], x=10, y=10, theme=danger).run()
```

That pattern keeps your door’s normal theme stable, but lets you theme specific UI moments.

### What the theme contains (full surface)

Think of this as “semantic colors + a few shared formatting strings”. Today the theme is structured like:

```python
from notorious_doorkit import Theme, ThemeColors, ThemeFormat

theme = Theme(
    colors=ThemeColors(
        text="",
        text_muted="",
        label="",
        key="",
        input="",
        input_focused="",
        selected="",
        warning="",
        frame_border="",
        frame_fill="",
    ),
    format=ThemeFormat(
        hotkey="[X]",
        press_enter="",
        press_any_key="",
    ),
)
```

The strings inside `ThemeColors` and `ThemeFormat` can be:

- Raw ANSI escape sequences
- DoorKit token markup like `{YELLOW}{BOLD}{RESET}`
- OpenDoors-style backtick color descriptions (depending on your `[text]` config)

### A quick note on overrides (so it stays predictable)

Most UI helpers follow a consistent rule when you pass an explicit color string:

- `None` means “use the theme default”.
- Any explicit string (including `""`) means “use exactly what I passed”.

That gives you a clean split: the theme covers your defaults, and call sites can still force one-off behavior when you need it.

## Door Runtime - Tech Overview

DoorKit is a *door runtime*, not just a grab-bag of terminal utilities. After you call `Door.start(...)` or `Door.start_local(...)`, DoorKit initializes a single global runtime context.

### Discoverable surface (`Runtime`)

The primary, IDE-friendly surface is the `Runtime` object:

```python
from notorious_doorkit import get_runtime

rt = get_runtime()

rt.config
rt.session
rt.dropfile
rt.terminal
rt.screen_size
rt.username
```

### How `username` is derived

`rt.username` is resolved in this order:

1) `rt.session.username` if set and non-empty
2) `rt.dropfile.user.alias` if present
3) `rt.dropfile.user.full_name` if present
4) fallback default ("User")

If you need a different fallback for your specific door UI, handle it at the call site.

### Function helper synonyms

DoorKit also provides module-level helper functions which are synonyms backed by the same runtime:

- `get_runtime()` returns the `Runtime` object
- `get_session()` is the same as `get_runtime().session`
- `get_terminal()` is the same as `get_runtime().terminal`
- `get_screen_size()` is the same as `get_runtime().screen_size`
- `get_username()` is the same as `get_runtime().username`

These exist to keep ports readable and to make call sites concise, but the recommended discoverable surface is the `Runtime` object.

## Tour of a Sample Door

One of the best ways to learn DoorKit is to look at a door that does a few real things: starts a session, paints a screen, collects some input, and then exits cleanly.

This sample is intentionally small, but it demonstrates the “three big blocks” you’ll use in almost every door:

- session start (`Door.start()`)
- UI composition (draw background/panels, then run a menu or form)
- a loop that returns to a stable screen

```python
import sys

from notorious_doorkit import Door, clear_screen, writeln
from notorious_doorkit import LightbarMenu, AdvancedForm, FormField


def run_profile_form() -> None:
    fields = [
        FormField("handle", x=20, y=8, width=20, label="Handle", required=True),
        FormField("state", x=20, y=9, width=2, label="State", format_mask="AA", max_length=2, normalize=lambda s: s.upper()),
    ]
    result = AdvancedForm(fields, save_mode="esc_prompt").run()
    if result is None:
        return
    writeln(f"Saved: {result}")


def main() -> None:
    door = Door()
    door.start()

    while True:
        clear_screen()
        writeln("NOTORIOUS DOORKIT SAMPLE")

        choice = LightbarMenu(["Edit Profile", "Quit"], x=5, y=5, width="auto").run()
        if choice is None or choice == 1:
            break
        if choice == 0:
            run_profile_form()


if __name__ == "__main__":
    main()
```

Once you’re comfortable with this skeleton, the rest of the kit becomes “just swap in a different input primitive”: `RawInput` loops for games, `display_file_paged()` for help screens, `AdvancedLightbarMenu` for multi-column layouts, and `AdvancedForm` when you want an edit screen.

## Module reference

### ANSI (`ansi.py`)

This is the “paint” layer. It contains:

- color constants (`RED`, `BG_BLUE`, `BOLD`, etc.)
- cursor functions (`goto_xy`, move helpers)
- screen functions (`clear_screen`, `clear_to_eol`, etc.)
- output functions (`write`, `writeln`, `write_at`)

The important thing to know: `write()` and friends run through CP437 encoding (`encode_cp437(..., errors='replace')`). That’s the right default for BBS-style output.

#### Common primitives

These are the core “draw stuff” tools you’ll reach for constantly. They’re intentionally low-level: you decide when to move the cursor, when to clear regions, and what to redraw.

#### `write(text) -> None`

`write()` is the simplest “send bytes to the caller’s terminal” primitive. Use it when you want full control over line endings, cursor placement, and incremental drawing.

Output is encoded as CP437 (with replacement for unsupported characters), which is usually what you want for classic BBS art and box-drawing characters. `write()` does not add any newline characters for you, so it’s safe for prompts and for building up a line in parts.

Example: the classic “prompt without newline” pattern. Notice the trailing space — it reads nicer on slow links and makes backspacing feel natural:

```python
from notorious_doorkit import write

write("Name: ")
```

Helpful hints: if you’re emitting your own newlines manually, prefer `\r\n` (CRLF) for maximum terminal compatibility. If you’re doing a lot of repainting, consider switching to the Screen module (`puttext`, `window_create`) so you can redraw regions without flicker.

#### `writeln(text="") -> None`

`writeln()` is `write()` plus a line ending. It’s your go-to for “print a line and move on”, and it’s intentionally boring.

Unlike plain `print()`, this always uses `\r\n` (CRLF), which is the classic convention terminals expect. This makes it safer for remote callers and keeps output consistent between ANSI and non-ANSI clients.

Example: a simple banner line at the top of your door:

```python
from notorious_doorkit import writeln

writeln("Welcome to the door")
```

Helpful hints: you can call `writeln()` with no arguments (`writeln()`) to output a blank line. If you’re writing a tight input loop, avoid spamming newlines as “screen clears”; instead repaint the parts that changed (or use `clear_screen()` deliberately when changing screens).

#### `write_at(x, y, text) -> None`

`write_at()` is a convenience helper for “move, then write”. It’s great for small HUD-style updates (time left, current room, selection counts) where you don’t want to re-render the whole screen.

Coordinates are **1-indexed**, with `(1, 1)` at the top-left. Under the hood this uses `goto_xy()` and then `write()`, so it inherits the same CP437 encoding and cursor tracking behavior.

Example: update a status field without redrawing the whole screen. This is the basic pattern for classic door UIs:

```python
from notorious_doorkit import write_at

write_at(2, 24, "Time left: 12:34")
```

Helpful hints: if the new value might be shorter than the old one, clear or pad the remainder (for example, write a few spaces after the value). For larger repaint regions, prefer the Screen module so you’re not manually managing clearing and cursor placement.

#### `play_ansi_music(sequence, *, reset=False) -> None`

`play_ansi_music()` emits an "ANSI music" sequence to the caller's terminal.

Some terminal programs support a long-running convention where certain `ESC[` sequences are interpreted as musical notes and timing controls (instead of cursor movement). DoorKit does not try to parse or validate the music syntax — it simply gives you a clean, obvious helper for sending the sequence without having to remember the prefix bytes.

This is a fun tool for tiny "retro feedback" moments:

- short jingles when a player levels up
- a little failure chirp when input is invalid
- a quick "you got mail" riff in a chat door

**Important:** ANSI music support is client-dependent. Some modern clients will ignore it entirely, some will play it, and some users will have sound disabled. Treat it as optional flavor, not a core mechanic.

**How it works**

DoorKit writes the music prefix (`ESC[`) and then writes your payload exactly as provided (ASCII-encoded with replacement). This mirrors the common door pattern from classic examples: you send the prefix, then the music payload, and optionally send a reset sequence when you're done.

If you pass `reset=True`, DoorKit also sends a reset (`ESC[00m`) after your payload. This is handy when you want to ensure you don't leave the remote client "in music mode".

**ANSI music syntax reference**

The sequence string can be 0-250 characters and supports the following commands:

| Command | Description |
| --- | --- |
| `A` - `G` | Musical notes |
| `#` or `+` | Sharp (follows a note, e.g., `C#`) |
| `-` | Flat (follows a note, e.g., `B-`) |
| `<` | Move down one octave |
| `>` | Move up one octave |
| `.` | Dotted note (extends duration by 3/2) |
| `MF` | Music Foreground (pause until finished playing) |
| `MB` | Music Background (continue while music plays) |
| `MN` | Music note duration Normal (7/8 of interval) |
| `MS` | Music note duration Staccato |
| `ML` | Music note duration Legato |
| `Ln` | Length of note (n=1-64; 1=whole, 4=quarter, etc.) |
| `Pn` | Pause length (same n values as Ln) |
| `Tn` | Tempo in notes/minute (n=32-255, default=120) |
| `On` | Octave number (n=0-6, default=4) |

**Example: play a short sequence, then reset**

```python
from notorious_doorkit import play_ansi_music

# A short "test riff" style sequence (payload only; ESC[ is added for you)
play_ansi_music("MBT120L4MFMNO4C8C8DC", reset=True)
```

#### `play_music(sequence, *, reset=False) -> None`

`play_music()` is a friendly alias for `play_ansi_music()`.

#### `goto_xy(x, y) -> None`

`goto_xy()` moves the live terminal cursor to an absolute position. This is the foundational "positioning" primitive that everything else builds on.

Positions are **1-indexed**, with `(1, 1)` as the top-left. It’s best used when you’re doing small targeted updates or drawing fixed-layout screens where you control every character.

Example: move to a spot and write a label:

```python
from notorious_doorkit import goto_xy, write

goto_xy(10, 5)
write("Hello")
```

Helpful hints: avoid mixing `goto_xy()` with “free-form” output unless you’re also controlling line endings — otherwise your cursor position can drift in ways that are hard to reason about. If you need an OpenDoors-like signature (`row, col`), use `set_cursor(row, col)`.

#### `clear_screen() -> None`

`clear_screen()` clears the caller’s screen and resets the cursor to home. Use it when you’re changing “pages” (main menu to sub-menu, list to detail view) and you want a clean slate.

In door UIs, clears are a tool — not a default. You’ll usually get a nicer feel if you clear only when changing screens, and otherwise update in place.

Example: clear before drawing your top-level menu:

```python
from notorious_doorkit import clear_screen

clear_screen()
```

Helpful hints: if you want to clear *only* a region, don’t spam `clear_screen()` — use `clear_line()`, `clear_to_eol()`, or the Screen module’s `puttext()`/window helpers. Also remember that heavy clearing can be visually jarring on slow or laggy connections.

#### `clear_line() -> None`

`clear_line()` clears the current line and returns the cursor to column 1. It’s most useful for prompt/status lines where you want to erase the previous content and redraw just that line.

This helper is intentionally simple: it clears the whole line and returns the cursor to column 1 (so you can immediately re-write the line). If you only want to clear from the cursor to the end of the line, use `clear_to_eol()`.

Example: clear a prompt line before repainting it. This avoids “prompt trails” when the new prompt is shorter:

```python
from notorious_doorkit import goto_xy, clear_line, write

goto_xy(1, 24)
clear_line()
write("Press any key...")
```

Helpful hints: if you’re clearing a line to repaint a fixed-width field, it’s sometimes nicer to *pad* with spaces instead of clearing (it keeps the cursor where you expect). If you’re repainting multiple lines, you’ll quickly be happier with a shadow buffer approach (`ShadowScreen`) rather than hand-clearing.

#### `strip_ansi(text: str) -> str`

`strip_ansi()` removes ANSI escape sequences from a string and returns only the visible text. It’s a practical tool when you need to measure “what the caller sees” rather than the raw bytes.

This comes up a lot in door code: centering headers, trimming status lines, or computing column widths for menus where the displayed strings contain color codes.

Example: stripping ANSI so you can safely measure the visible width:

```python
from notorious_doorkit import strip_ansi

visible = strip_ansi("\x1b[31mRED\x1b[0m")  # "RED"
```

Helpful hints: this is a best-effort stripper (it targets common CSI-style sequences). If you’re embedding unusual terminal controls, don’t assume `strip_ansi()` will remove everything — treat it as “good enough for UI width math”, not a security filter.

#### Cursor position tracking

#### `get_cursor_position() -> Tuple[int, int]`

`get_cursor_position()` returns the current estimated cursor position as `(row, col)` (both 1-indexed). This is a convenience alias for the underlying `ansi.get_cursor()` soft tracker.

The key word is *estimated*: the value is maintained by watching the ANSI helper calls you make (`goto_xy()`, `write()`, `writeln()`, clears, etc.). It does not ask the terminal where the cursor is, and it can drift if you output raw escape sequences manually.

Example: move the cursor, write some text, then ask where the tracker thinks you landed:

```python
from notorious_doorkit import goto_xy, write, get_cursor_position

goto_xy(10, 5)
write("Hello")
row, col = get_cursor_position()  # Returns (5, 15)
```

Helpful hints: use this for “where am I about to draw next?” logic, not for terminal interrogation. If you’re drawing through `ShadowScreen`, prefer `screen.get_cursor()` instead — it reflects the shadow buffer’s authoritative cursor.

### Screen (`screen.py`)

This module gives you a **software shadow buffer** for the terminal: a memory-backed model of a CP437 text screen (cells, attributes, and a cursor). You can use it when you want to build classic door UI tricks like save-under windows, popups, and partial redraws without having to “repaint the whole world” every time.

The key idea is that the shadow screen is authoritative for anything you draw *through it*. When you call `window_create()` or `puttext()`, Doorkit updates the shadow buffer and then paints only the affected region to the live terminal.

#### `ShadowScreen`

`ShadowScreen` is the underlying object model: a grid of cells (character + PC attribute byte) plus a cursor and a “current attribute”. The point is **predictable redraw** — you can update a region in memory and then paint exactly that region to the live terminal.

This is the same family of idea as OpenDoors’ `od_gettext()`/`od_puttext()` (save and restore rectangular areas), but expressed as a Python object instead of raw buffers. Most doors can stick to the global helpers below, but if you want multiple virtual screens or a custom size, you can instantiate one and `set_screen()`.

Helpful hints: treat the shadow buffer as the source of truth for anything you draw through it. If you mix direct ANSI output (`write_at`, `goto_xy`) with shadow-screen painting, it can work fine, but you’re responsible for keeping the on-screen result coherent.

#### Global helpers (default screen)

These functions operate on a global default screen instance.

#### `get_screen() -> ShadowScreen`

Return the global default `ShadowScreen`.

Think of this as “the software copy of the screen”. It is created lazily the first time you call `get_screen()` and defaults to an 80x25 buffer.

Most doors don’t need to instantiate `ShadowScreen` directly; you’ll typically use the global helper functions (`save_screen`, `window_create`, etc.) which all delegate to the default screen.

Example: grab the screen object to do an explicit repaint. This is useful when you’ve updated the shadow buffer directly and want to force a full redraw:

```python
from notorious_doorkit import get_screen

screen = get_screen()
screen.paint_all()
```

Helpful hints: you generally don’t need to call `paint_all()` after `window_create()`/`window_remove()` (they paint automatically). Reach for `paint_all()` when you want a “hard refresh” after complex state changes, or when you suspect the caller’s terminal got out of sync.

#### `set_screen(screen: ShadowScreen) -> None`

Replace the global default `ShadowScreen`.

This is useful when you know the caller’s screen size and you want the shadow buffer to match it (for example, 24-row vs 25-row). It’s also useful if you want multiple “virtual screens” and you want to swap which one is considered the default.

Example: set a 24-row shadow screen at startup:

```python
from notorious_doorkit import ShadowScreen, set_screen

set_screen(ShadowScreen(width=80, height=24))
```

#### `get_cursor() -> Tuple[int, int]`

Return the shadow screen cursor as `(row, col)`.

This cursor is the *shadow buffer’s* cursor, not the real terminal cursor. It’s most useful when you’re treating the shadow screen as the authoritative model for “where you last drew”.

If you want the *ANSI soft-tracked* cursor from direct `goto_xy()`/`write()` calls, use `get_cursor_position()` instead.

Helpful hints: `ShadowScreen.paint_all()` will move the real terminal cursor to match the shadow cursor at the end of the paint. If your UI depends on “cursor ends up here”, use the shadow cursor for that logic and repaint through the screen.

#### `save_screen() -> ScreenSnapshot`

Capture the entire shadow screen (cells + cursor + current attribute) into a `ScreenSnapshot`.

This is the “big hammer” save-under. Use it when you’re about to draw something disruptive (a modal dialog, help overlay, sysop chat page) and you want to restore the screen exactly as it was.

This mirrors the spirit of OpenDoors’ `od_save_screen()`: it captures *everything* you need to put the UI back (including cursor position and current attribute), and it’s display-mode agnostic because it operates on the shadow model.

Example: full-screen save/restore around a modal overlay:

```python
from notorious_doorkit import save_screen, restore_screen, clear_screen, writeln

snap = save_screen()

clear_screen()
writeln("This is a modal screen.")

# ... wait for a key ...

restore_screen(snap)
```

Helpful hints: don’t confuse `ScreenSnapshot` with `ScreenBlock`. Snapshots are for “entire screen, cursor, and current attribute”, while blocks are for “a rectangle of cells”. If you only need a small save-under region, prefer `gettext()`/`puttext()` to avoid unnecessary full-screen repaint work.

#### `restore_screen(snap: ScreenSnapshot) -> None`

`restore_screen()` puts the shadow screen back to a previously captured `ScreenSnapshot` and then repaints the full screen. In other words, it’s both a state restore and a visual restore.

Like OpenDoors’ `od_restore_screen()`, this is meant for “return to exactly how it was” moments: exiting a popup, leaving chat mode, or undoing a temporary full-screen overlay.

Helpful hints: because restore repaints the entire screen, it’s intentionally heavyweight. If you’re doing frequent small overlays, you’ll get a smoother feel using `gettext()`/`puttext()` around just the affected region.

#### `gettext(left: int, top: int, right: int, bottom: int) -> ScreenBlock`

Capture a rectangular region from the shadow screen.

This is the OpenDoors-style “grab a rectangle” primitive (think `od_gettext()`), except it returns a structured `ScreenBlock` instead of writing into a raw buffer. It’s what `window_create()` uses internally, and it’s perfect when you want to save just a portion of the screen (for example, a menu panel or status box) instead of saving/restoring the whole screen.

Coordinates are 1-indexed and inclusive. If the rectangle extends out of bounds, it will be clipped to the screen.

Helpful hints: `gettext()` does not change the cursor, attribute, or terminal output — it’s purely a read from the shadow buffer. If you need “save-under + paint”, pair it with `puttext()` and a `paint_region()` call.

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `left`, `top`, `right`, `bottom` | `int` | Yes | | 1-indexed, inclusive rectangle |

#### `puttext(left: int, top: int, block: ScreenBlock) -> None`

Paste a captured `ScreenBlock` back into the shadow screen.

This is the companion to `gettext()` (think `od_puttext()`): it pastes the saved characters and attributes back into the shadow buffer at a destination top-left.

**Important:** `puttext()` updates the *shadow buffer*. It does not repaint the live terminal by itself. That’s intentional: sometimes you want to update multiple blocks and repaint once.

If you need to repaint immediately, call `get_screen().paint_region(...)` (or `paint_all()`).

Example: save a small region, overwrite it, then restore it and repaint:

```python
from notorious_doorkit import get_screen

screen = get_screen()
under = screen.gettext(10, 5, 30, 7)

screen.window_create(10, 5, 30, 7, title="Temp")

screen.puttext(10, 5, under)
screen.paint_region(10, 5, 30, 7)
```

Helpful hints: the pasted block dimensions are fixed by what you captured — if you captured a 21x3 region, you must paste it as a 21x3 region. If you need to restore a different size, recapture with `gettext()` instead of trying to stretch the block.

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `left`, `top` | `int` | Yes | | Destination top-left (1-indexed) |
| `block` | `ScreenBlock` | Yes | | Captured region from `gettext()` |

#### `scroll_region(left: int, top: int, right: int, bottom: int, distance: int, flags=SCROLL_NORMAL) -> None`

Scroll a rectangular area of the screen by a number of lines.

This is a simple “scroll a window without touching the rest of the screen” primitive.

In practical terms, it lets you treat part of the screen like a little terminal of its own. Imagine you have a boxed panel that shows the last 10 chat lines: when a new line arrives, you don’t want to clear the whole screen and redraw everything. You just want the contents of that panel to move up one row, leaving a blank row at the bottom where you can print the new line.

That’s exactly what `scroll_region()` does: it shifts the *existing characters* inside the rectangle up or down by a given number of rows, without changing anything outside the rectangle. It’s a great fit for:

- chat panes
- live log/status panes
- “ticker” style message areas
- any UI where the rest of the screen should stay stable while one panel updates

Under the hood, DoorKit performs the scroll using the shadow screen model: it moves the stored cells (characters + attributes) in memory and repaints only the affected region. That gives you the classic “door UI” feel where the rest of the screen stays steady.

If you’re deciding between `scroll_region()` and `ScrollingRegion`, here’s a simple rule of thumb:

- `scroll_region()` is a low-level “move the existing screen contents” tool. Use it when you’re hand-building a UI panel and you want to physically shift what’s already on the screen.
- `ScrollingRegion` is a higher-level widget with its own text buffer and viewport. Use it when you have a stream of lines (chat/log/feed) and you want to append to a buffer and let it handle rendering and “stick to bottom unless scrolled up” behavior.

Behavior notes:

- `distance > 0` scrolls *up* (blank space appears at the bottom).
- `distance < 0` scrolls *down* (blank space appears at the top).
- The cursor position is restored to where it was before the scroll.
- The current attribute is restored to where it was before the scroll.

By default, the newly exposed lines are cleared using the current attribute. If you pass `SCROLL_NO_CLEAR`, the region is scrolled but the newly exposed area is left unchanged (useful when you know you’re about to repaint those lines anyway).

Example: scroll a “chat window” region up by one line before writing a new message on the last line:

```python
from notorious_doorkit import scroll_region, write_at

top = 5
bottom = 20

scroll_region(1, top, 80, bottom, 1)
write_at(2, bottom, "[Alice] hello!")
```

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `left`, `top`, `right`, `bottom` | `int` | Yes | | 1-indexed, inclusive rectangle |
| `distance` | `int` | Yes | | Positive=up, negative=down |
| `flags` | `int` | No | `SCROLL_NORMAL` | Bitmask of `SCROLL_*` flags |

#### Scroll flags

These are the OpenDoors-style flags for controlling clearing behavior:

- `SCROLL_NORMAL` (default)
- `SCROLL_NO_CLEAR` (do not clear newly exposed area)

#### `window_create(left: int, top: int, right: int, bottom: int, *, fill_attr=None, border_attr=None, title=None) -> WindowHandle`

Create a simple save-under window. This captures the region under the window, draws a border and fills the inside, and returns a `WindowHandle` that you can later pass to `window_remove()`.

This is the classic “popup window” primitive. It’s ideal for overlays like help, confirmation dialogs, or status panels.

Under the hood, this is essentially `gettext()` + draw border/fill + paint region, packaged as one operation so it’s hard to mess up. It’s intentionally conservative: borders are ASCII-safe, and the behavior is predictable even on basic terminals.

Behavior notes:

- The border is intentionally simple (`+`, `-`, `|`) so it’s safe everywhere.
- The window is painted immediately.
- Windows are treated as a stack; `window_remove()` restores in LIFO order.

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `left`, `top`, `right`, `bottom` | `int` | Yes | | 1-indexed, inclusive rectangle |
| `fill_attr` | `Optional[int]` | No | `None` | Fill attribute (defaults to current) |
| `border_attr` | `Optional[int]` | No | `None` | Border attribute (defaults to fill) |
| `title` | `Optional[str]` | No | `None` | Optional title on the top border |

Helpful hints: if you plan to draw inside the window, the simplest approach is often: `window_create()` (paints), then `write_at()` your labels inside, then `window_remove()` when done. If you need to update the window interior repeatedly, consider updating the shadow buffer and repainting just the interior region.

#### `window_remove(handle: WindowHandle) -> None`

Remove a window and restore the saved-under region.

Windows are managed as a stack. When you remove a handle, Doorkit will restore windows in LIFO order until it reaches the handle you asked to remove (so nested popups behave the way you expect).

This stack behavior is deliberate: it matches the classic door expectation where popups can be layered (help over menu over status) and removed cleanly without having to redraw everything manually.

**Example: popup window**

This pattern is great for “press F1 for help” style overlays: create a window, draw inside it, wait for a key, then remove it.

```python
from notorious_doorkit import window_create, window_remove, write_at, Attr

normal = int(Attr.from_color_desc("WHITE ON BLUE"))
border = int(Attr.from_color_desc("BRIGHT WHITE ON BLUE"))

handle = window_create(10, 5, 70, 18, fill_attr=normal, border_attr=border, title="Help")
write_at(12, 7, "This is a popup window.")

# ... wait for a key ...

### Text (`text.py`)

This module is where the “classic door text” helpers live: OpenDoors-style `printf`, color parsing, RA/QBBS expansion (optional), and PC attribute byte helpers.

#### `Attr(value: int)`

`Attr` is a small value object for working with IBM-PC style attribute bytes (0-255). If you’ve ever dealt with DOS-style `0x1F` color values, this is the same world: foreground/background/bright/blink packed into a single byte.

The main reason to use `Attr` is to make your door code more readable when you’re juggling colors: you can build an attribute from a descriptive string (like `"WHITE ON BLUE"`), convert it back to a raw byte, or render it as the ANSI SGR sequence that `set_attrib()` will write.

Example: parsing a classic color description and using it as an attribute byte:

```python
from notorious_doorkit import Attr, set_attrib, writeln

attr = Attr.from_color_desc("BRIGHT WHITE BLUE")
set_attrib(attr)
writeln("Hello in a classic door palette")
```

Helpful hints: if you want compatibility with OpenDoors-era config files, keep your “color desc” strings simple and human readable; unknown tokens are ignored rather than raising. If you’re already storing attributes as integers (dropfile themes, config tables), `Attr.from_pc_attr()` is a clean wrapper when you want `.to_sgr()` or consistent `int(Attr)` behavior.

#### `set_attrib(attr) -> int`

`set_attrib()` sets the current IBM-PC attribute byte (0-255) and writes the equivalent ANSI SGR sequence to the terminal. This is the “single value color setter” (OpenDoors `od_set_attrib()` spirit) and is handy when you want to store the user’s preferred color as one byte.

You can pass either an `int` (0-255) or an `Attr`. The function returns the normalized attribute value that was set.

Example: set a bright white-on-blue scheme for a header line:

```python
from notorious_doorkit import Attr, set_attrib, write_at

set_attrib(Attr.from_color_desc("BRIGHT WHITE BLUE"))
write_at(2, 1, "My Door")
```

Helpful hints: `set_attrib()` is purely an output-side setting — it doesn’t “retroactively recolor” existing text. Set it *before* drawing the region you care about. If you’re doing heavy screen painting via `ShadowScreen`, you’ll usually set attributes on the shadow cells instead of sending SGR directly.

#### `get_attrib() -> int`

`get_attrib()` returns the last attribute byte set via `set_attrib()`. Think of it as a tiny piece of “current output state” that you can capture and restore around temporary color changes.

Example: temporarily switch colors and then restore the previous attribute:

```python
from notorious_doorkit import get_attrib, set_attrib, Attr, writeln

prev = get_attrib()
set_attrib(Attr.from_color_desc("YELLOW ON BLACK"))
writeln("Warning: low time remaining")
set_attrib(prev)
```

Helpful hints: this is not a terminal query; it’s Doorkit’s remembered state. If you output raw ANSI color sequences manually, `get_attrib()` won’t know about it — use `set_attrib()` consistently if you want the value to stay meaningful.

#### Output/Text processing (OpenDoors-style)

When porting OpenDoors doors, a common pattern is using `od_printf()` for:

- `printf`-style formatting
- inline color markup (Doorkit `color_delimiter` / `color_char` behavior)
- (optionally) RA/QBBS control codes (best-effort)

In `notorious_doorkit`, the equivalent developer-facing function is:

- `printf(fmt, *args, **kwargs) -> None`

`printf()` will:

- format the string using Python `%` formatting
  - if args/kwargs are provided, they are applied
  - if no args/kwargs are provided, `printf()` still applies an empty-format pass so `"%%"` becomes `"%"` (like classic `printf` usage)
- process OpenDoors-style backtick color blocks (example: `` `BRIGHT WHITE BLUE` ``)
- optionally expand RA/QBBS control codes (`^F`/`^K`) if enabled
- write the resulting text using the normal CP437 output path

`printf()` also supports keyword parameters that control the OpenDoors-style processing:

- `delimiter`: backtick delimiter character (defaults to config `color_delimiter`)
- `color_char`: single-byte attribute marker character (defaults to config `color_char`)
- `expand_ra_qbbs`: `True`/`False` to force behavior, or `None` to use the configuration default

Formatting note: `printf()` supports either `*args` *or* `**kwargs` for `%` formatting, but not both at the same time.

RA/QBBS expansion is **disabled by default** (so that the lower-level `write()` path stays “raw”), and is controlled by configuration.

Doorkit configuration precedence for these defaults is:

- built-in defaults
- global Doorkit config file (`NOTORIOUS_DOORKIT_CONFIG`, or `~/.config/notorious_doorkit/config.toml`)
- door-specific config file (`NOTORIOUS_DOORKIT_DOOR_CONFIG`, or `configure(door_config_path=...)`)
- environment variables
- per-call arguments to `printf()`

The Output/Text settings can be configured either via TOML (`[text]` section) or via environment variables.

**TOML keys (recommended)**

- `[text].expand_ra_qbbs = true|false`
- `[text].color_delimiter = "`"` (set to empty string to disable)
- `[text].color_char = ""` (set to empty string to disable)

**Environment variable overrides**

- `NOTORIOUS_DOORKIT_EXPAND_RA_QBBS`
- `NOTORIOUS_DOORKIT_COLOR_DELIMITER`
- `NOTORIOUS_DOORKIT_COLOR_CHAR`

#### Terminal Configuration

The `[terminal]` section controls screen initialization and terminal capability detection.

**TOML keys (recommended)**

- `[terminal].clear_on_entry = true|false` — Clear screen when `Door.start()` or `Door.start_local()` is called (default: `true`)
- `[terminal].detect_capabilities = true|false` — Probe for ANSI/RIP support (default: `false`)
- `[terminal].probe_timeout_ms = 250` — Timeout for capability probe in milliseconds
- `[terminal].detect_window_size = true|false` — Probe for terminal window size (default: `false`)
- `[terminal].window_size_timeout_ms = 250` — Timeout for window size probe in milliseconds

**Environment variable overrides**

- `NOTORIOUS_DOORKIT_CLEAR_ON_ENTRY`
- `NOTORIOUS_DOORKIT_DETECT_CAPABILITIES`
- `NOTORIOUS_DOORKIT_CAPABILITY_TIMEOUT_MS`
- `NOTORIOUS_DOORKIT_DETECT_WINDOW_SIZE`
- `NOTORIOUS_DOORKIT_WINDOW_SIZE_TIMEOUT_MS`

**Screen clearing behavior:**

By default (`clear_on_entry = true`), DoorKit clears the screen when your door starts, providing a clean slate similar to how most BBS systems launch doors. This matches typical BBS behavior where the screen is cleared before the door takes over.

If you want to preserve whatever was on screen before your door launched (for example, to display a "launching..." message from the BBS), set `clear_on_entry = false` in your config or via the environment variable.

Note: OpenDoors itself does not clear the screen at initialization (it sets an `od_always_clear` flag but only clears for RIP mode). DoorKit's default behavior is more user-friendly for standalone door development.

`NOTORIOUS_DOORKIT_EXPAND_RA_QBBS` supports common boolean strings:
  - truthy values: `1`, `true`, `yes`, `on`, `enabled`
  - falsy values: `0`, `false`, `no`, `off`, `disabled`

`NOTORIOUS_DOORKIT_COLOR_DELIMITER` controls backtick-style color blocks:

- set to `` ` `` to match OpenDoors default behavior
- set to an empty string to disable delimiter parsing

`NOTORIOUS_DOORKIT_COLOR_CHAR` controls the single-character “attribute byte follows” marker:

- if unset or empty, this mode is disabled
- if set, it must be exactly one character

- **Per-call override**: pass `expand_ra_qbbs=True` or `expand_ra_qbbs=False` to `printf()`

A hosting environment (Door32 launcher, BBS wrapper, etc.) may set these env vars before launching the door, but they are **overrides**, not the primary configuration mechanism.

Example:

```python
from notorious_doorkit import printf

printf("`BRIGHT WHITE BLUE`Hello `GREEN`%s`WHITE`!", "Kevin")
```

#### OpenDoors compatibility shims (Output/Text)

If you’re porting OpenDoors code and want “same name without the `od_` prefix”, Doorkit provides thin wrappers.

These are intentionally small and boring: they’re meant to make ports readable first, and “perfect OpenDoors emulation” second. If you’re writing new code, you’ll usually prefer `write()`/`writeln()`, `printf()`, or the higher-level UI primitives.

#### `sprintf(fmt, *args, **kwargs) -> str`

`sprintf()` returns a formatted string using Python’s `%` formatting rules (OpenDoors-style `sprintf` spirit). It’s useful when you want to build a message first (for menus, status lines, logging, or later output) without writing it immediately.

Unlike `printf()`, `sprintf()` does not apply OpenDoors-style color parsing or RA/QBBS expansion — it’s purely “format and return a string”.

Examples:

Building a formatted string for later output:

```python
from notorious_doorkit import sprintf

s = sprintf("User %s has %d credits", "Kevin", 42)
```

Helpful hints: like `printf()`, `sprintf()` rejects mixing `*args` and `**kwargs` in a single call. If you’re writing new code, consider f-strings for simple interpolation; `sprintf()` is most valuable when you’re porting OpenDoors-ish code or intentionally using `%` format strings.

#### `disp_str(text) -> None`

`disp_str()` writes a string to the terminal (equivalent to `write(text)`). This is the “I already have a string, just send it” helper that shows up all over OpenDoors-era code.

It does not add a newline for you, so it’s safe for prompts and string fragments. Terminal line endings are up to you — for maximum compatibility, prefer `\r\n` when you intend a newline.

Examples:

Displaying three string constants on separate lines. Notice the explicit `\r\n` (CRLF):

```python
from notorious_doorkit import disp_str

disp_str("This is an example\r\n")
disp_str("of the OpenDoors\r\n")
disp_str("disp_str() helper\r\n")
```

Displaying string fragments on the same line:

```python
from notorious_doorkit import disp_str

disp_str("Another ")
disp_str("disp_str() ")
disp_str("example\r\n")
```

Helpful hints: for new code, you’ll usually prefer `write()`/`writeln()` so the intent is obvious, but `disp_str()` is great for ports. If you’re outputting strings that contain OpenDoors backtick color blocks, use `printf()` instead — `disp_str()` will not interpret them.

#### `putch(ch) -> None`

`putch()` writes a single character at the current cursor position. If you pass a longer string, only the first character is written.

This is mainly a compatibility shim for OpenDoors-style code and for tiny UI touches like drawing one border character, echoing a keystroke, or emitting a single control character.

Examples:

Displaying a single character:

```python
from notorious_doorkit import putch

putch("X")
```

Helpful hints: because Doorkit output is CP437-encoded at the lowest level, `putch()` is a safe way to emit box-drawing characters too (as long as your string contains the correct character). If you need to output a byte value directly, use `disp(b"...")` instead.

#### `repeat(ch, times) -> None`

`repeat()` writes a character repeated `times` times. Like `putch()`, only the first character of `ch` is used.

In OpenDoors, `od_repeat()` can use graphics-mode optimizations (like AVATAR repeat sequences). In Doorkit, this is a simple and predictable helper: it just writes the repeated characters.

Examples:

Drawing a simple divider line:

```python
from notorious_doorkit import repeat, writeln

repeat("-", 40)
writeln("")
```

Helpful hints: if you’re drawing boxes and borders, `repeat()` + `putch()` is often clearer than manual string multiplication in the middle of UI code. If you need higher-performance repaint behavior (large areas changing often), draw into `ShadowScreen` and paint regions instead of streaming large repeated runs.

#### `disp(buf, n=None, local_echo=False) -> None`

`disp()` is a compatibility wrapper for “write a buffer”. If you pass `bytes`/`bytearray`, it writes raw bytes; if you pass `str`, it writes text.

This is useful when porting code that sometimes sends terminal control sequences as bytes and sometimes sends printable strings. The optional `n` parameter lets you send only the first N bytes/characters.

`n` is an optional length limit (handy when you want to send only the first N bytes/chars). `local_echo` is accepted for OpenDoors parity, but does not model a separate behavior in Doorkit.

Examples:

Sending exactly five characters from a buffer:

```python
from notorious_doorkit import disp

disp("HELLO WORLD", n=5)
```

Writing a raw byte sequence:

```python
from notorious_doorkit import disp

disp(b"\x1b[2J\x1b[H")
```

Helpful hints: use `disp(b"...")` for raw escape sequences when you *really* need it, but prefer the higher-level ANSI helpers (`clear_screen`, `goto_xy`, `set_attrib`) when possible — they keep cursor/attribute tracking consistent. When passing `str`, output goes through the normal CP437 path like `write()`.

#### `disp_emu(text, remote_echo=True, *, mode="ansi", expand_ra_qbbs=None, ...) -> None`

`disp_emu()` is a best-effort “emulated display” shim for ports that expect OpenDoors-style `disp_emu`. If `remote_echo` is `False`, nothing is written.

When enabled, this function can do two helpful compatibility transforms: optional RA/QBBS expansion, and a **best-effort AVATAR-to-ANSI translation** (a practical subset, not a full terminal emulator). The goal is to make common door-era output streams render sensibly on modern ANSI terminals.

Helpful hints: `mode` is currently treated as a hint (Doorkit normalizes `ansi`/`tty`/`ascii` the same way). If you need strict terminal emulation with a full screen buffer model, use `ShadowScreen` for your own drawing instead of relying on `disp_emu()` to “do everything.”

**AVATAR support note**

Doorkit currently implements **Option A**: a practical subset of AVATAR used by common doors. It is not a full OpenDoors terminal emulator (no local screen buffer model).

Supported AVATAR constructs (best-effort):

- `0x19 <ch> <count>`: repeat character
- `0x0C`: clear screen
- `^V` (`0x16`) commands:
  - `0x01 <attr>`: set PC attribute (mapped to ANSI SGR)
  - `0x02`: blink
  - `0x03/0x04/0x05/0x06`: cursor up/down/left/right
  - `0x07`: clear to end-of-line
  - `0x08 <row> <col>`: cursor position
  - `0x09`: insert mode (accepted but treated as a no-op)

Not yet supported (examples):

- AVATAR insert/scroll region semantics that require a full screen buffer
- full ANSI/AVATAR state machine parity with OpenDoors

Example: enable RA/QBBS expansion for a single call:

```python
from notorious_doorkit import printf

# ^F + 'A' is represented in the stream as 0x06 followed by a letter.
printf("User: \x06A", expand_ra_qbbs=True, ra_qbbs_f_map={"A": "Kevin"})
```

### Input (`input.py`)

This is the “read a key” layer.

#### Terminal detection helpers

In most production doors, `Door.start(...)` will seed terminal information from the dropfile first (graphics mode and screen rows when present). Capability probes are still useful when:

- you’re developing locally (`start_local`) and want best-effort detection
- the caller’s environment is ambiguous and you want to offer a prompt

These helpers live in `input.py` because they’re fundamentally “interactive I/O” operations.

One useful mental model: treat these probes as **hints**, not truth. Bridges differ, timeouts happen, and some clients simply won’t answer. Always have a sensible fallback (usually ANSI or ASCII).

#### `detect_capabilities_probe(*, timeout_ms=250) -> TerminalInfo`

`detect_capabilities_probe()` actively probes the current terminal stream for capabilities (best-effort) and returns a `TerminalInfo`. This is mainly useful for local dev (`start_local`) or for environments where you don’t have a trustworthy dropfile to tell you what the caller supports.

The important nuance is that this is a *probe*, not a guarantee: bridges can time out, clients can lie, and some terminal paths simply don’t answer queries. Treat the result as “strong hints” and still keep a reasonable fallback.

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `timeout_ms` | `int` | No | `250` | Total time budget for probe reads |

Detects (best-effort):

- ANSI via `ESC[6n` (cursor position query)
- RIP via `ESC[!` (RIPterm responds with `RIPSCRIPxxxxxx`)

Example: best-effort capability detection for a local dev run:

```python
from notorious_doorkit import detect_capabilities_probe

term = detect_capabilities_probe(timeout_ms=250)
if "ansi" in term.capabilities:
    pass
```

Helpful hints: this helper sends small escape-sequence queries and reads back replies. If you call it after you’ve already started a key-reading loop, be mindful that any “leftover” bytes are pushed back into Doorkit’s input buffer. If you’re writing a door meant to run under a BBS, prefer the dropfile first and probe only when you *must*.

#### `detect_window_size_probe(*, timeout_ms=250) -> Optional[Tuple[int, int]]`

`detect_window_size_probe()` is best-effort window size detection for ANSI terminals. It’s useful when you want to center a UI or choose a layout dynamically during local development.

Like any terminal size query, it is opportunistic: some clients don’t support it, some proxies strip replies, and timing can be unreliable. When it returns `None`, fall back to classic door defaults (often `24x80`).

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `timeout_ms` | `int` | No | `250` | Time budget for each query attempt |

Detection order:

1. `CSI 18 t` (xterm-style report)
2. Save cursor + move to far bottom-right + `CSI 6 n` + restore cursor

Returns `None` if no reply is received.

Example: center a panel when size is known, otherwise fall back:

```python
from notorious_doorkit import detect_window_size_probe

sz = detect_window_size_probe(timeout_ms=250)
rows, cols = sz if sz is not None else (24, 80)
```

Helpful hints: this call may temporarily move the cursor during the fallback strategy (save cursor, jump to bottom-right, query position, restore). It tries to be polite, but if your door is mid-draw, you’ll usually want to run size detection before painting your UI.

#### `prompt_graphics_mode(*, inp=None, default=None) -> str`

`prompt_graphics_mode()` prompts the user to pick a graphics mode from detected capabilities. This is designed for “I’m not sure what the environment is, so I’ll ask” workflows, especially during local development.

It interrogates the initialized door runtime (`get_runtime()`) for terminal capabilities, then asks a small single-key question and returns one of `"rip"`, `"ansi"`, or `"ascii"`.

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `inp` | `Optional[RawInput]` | No | `None` | If `None`, reads keys via the default raw input path |
| `default` | `Optional[str]` | No | `None` | Currently best-effort (future hook for preselect) |

Returns one of `"rip"`, `"ansi"`, or `"ascii"`.

Example: offer a mode choice during local testing:

```python
from notorious_doorkit import Door, prompt_graphics_mode

door = Door()
door.start_local(username="Sysop", node=1)

mode = prompt_graphics_mode()
```

Helpful hints: this helper reads keys using `get_answer()` internally, so it works well with a shared `RawInput` if you pass one via `inp=`.

#### Key constants

You’ll commonly use:

- `KEY_UP`, `KEY_DOWN`, `KEY_LEFT`, `KEY_RIGHT`
- `KEY_ENTER`, `KEY_ESC`
- `KEY_BACKSPACE`, `KEY_DELETE`

**Delete/backspace note:** many clients send `0x7f` (DEL) for backspace, so Doorkit exposes that as `KEY_DELETE`. Forward-delete (the “Delete key”) is represented separately as `KEY_DEL` in the lower-level input layer.

If you need the additional extended key tokens (`KEY_HOME`, `KEY_END`, `KEY_PAGEUP`, `KEY_PAGEDOWN`, `KEY_INSERT`, `KEY_DEL`, `KEY_SHIFTTAB`, etc.), import them from `notorious_doorkit.input`.

Example: simple “press a key” loop:

```python
from notorious_doorkit import RawInput, KEY_UP, KEY_DOWN, KEY_ESC

with RawInput() as inp:
    while True:
        k = inp.get_key()
        if k in (KEY_UP, KEY_DOWN):
            continue
        if k == KEY_ESC:
            break
```

#### `RawInput`

Use `RawInput` as a context manager for reading keys character-by-character and parsing common ANSI escape sequences into `KEY_*` tokens.

In door32 mode, `RawInput` reads from the terminal stream on FD3. In other environments, raw-mode behavior depends on the host/runtime and may be best-effort.

Helpful hints: `RawInput(extended_keys=True)` is the “UI-friendly” setting — it recognizes PgUp/PgDn/Ins/Del and Shift+Tab, which you’ll want for anything editor-like. If you only need arrows and Enter/Esc, the default is fine and keeps parsing simpler.

**Constructor**

- `RawInput(extended_keys=False)`
  - `extended_keys`: If `True`, recognizes PgUp/PgDn/Ins/Del (tilde-terminated CSI sequences)
  - Default `False` uses simple 3-byte arrow/home/end parsing only
  - **Use `extended_keys=True` for `RegionEditor` or other editors needing extended keys**

**Methods**

| Method | Purpose | Parameters |
| --- | --- | --- |
| `get_key(timeout=None)` | Read one key (blocking unless timeout provided) | `timeout: Optional[float]` |
| `read_line(prompt="", max_length=255, echo=True)` | Line editor (supports arrows/backspace/home/end) | see table below |
| `key_pressed()` | Non-blocking “is input ready?” | none |

**read_line(...) parameters**

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `prompt` | `str` | No | `""` | Written before input |
| `max_length` | `int` | No | `255` | Hard cap |
| `echo` | `bool` | No | `True` | Set `False` to suppress local echo |

**Example: wait for a single key**

```python
from notorious_doorkit import RawInput, KEY_ESC

with RawInput() as inp:
    key = inp.get_key()
    if key == KEY_ESC:
        pass
```

**Example: line input**

This is the classic “prompt + edit a line” flow: it supports arrow keys, backspace, Home/End.

```python
from notorious_doorkit import RawInput

with RawInput() as inp:
    name = inp.read_line(prompt="Name: ", max_length=25)
```

#### OpenDoors compatibility shims (Input 2/12)

For porting OpenDoors code, Doorkit provides thin wrappers matching OpenDoors input API semantics.

These are intentionally small and boring: they’re meant to make ports readable first, and “perfect OpenDoors emulation” second. If you’re writing new code, you’ll usually prefer `RawInput`, `hotkey_menu`, `AdvancedForm`, or `RegionEditor`.

#### `get_answer(options: str, inp=None) -> str`

Get a single key response from a list of valid options.

This is the classic “press Y/N” helper. It waits until the user hits a key that appears in `options`, then returns the option character using the same case you provided (handy if you want to preserve `"YN"` vs `"yn"` behavior).

This tiny detail (returning your case) is surprisingly useful when you’re porting older door code that uses the return value directly in comparisons or output.

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `options` | `str` | Yes | | Valid keys (case-insensitive match) |
| `inp` | `Optional[RawInput]` | No | `None` | If `None`, creates a temporary `RawInput(extended_keys=True)` |

Example: simple confirmation prompt:

```python
from notorious_doorkit import get_answer

# Returns 'Y' if user presses 'y' or 'Y'
answer = get_answer("YN")

# Returns 'y' if user presses 'y' or 'Y'
answer = get_answer("yn")
```

Helpful hints: if you want Enter to be accepted, include `"\r"` or `"\n"` in your `options` string. For “menu hotkeys” that include arrows or other `KEY_*` tokens, prefer `hotkey_menu()` instead of forcing everything through `get_answer()`.

#### `key_pending(inp=None) -> bool`

Check if a key is waiting in the input buffer without blocking.

This is mainly useful for “background loops” (chat, animations, status updates) where you want to keep rendering until the user presses something.

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `inp` | `Optional[RawInput]` | No | `None` | If `None`, creates a temporary `RawInput()` |

Example: simple “animate until keypress” loop:

```python
from notorious_doorkit import RawInput, key_pending

with RawInput() as inp:
    while not key_pending(inp):
        # ... update something on screen ...
        pass
```

Helpful hints: `key_pending()` doesn’t consume the key — it only checks readiness. You’ll still call `inp.get_key()` afterward to actually read it. If you’re already inside a `RawInput()` context, pass it in so you don’t create nested raw-mode contexts.

#### `clear_keybuffer(inp=None) -> None`

Clear the input keyboard buffer (flush pending keystrokes).

This is a pragmatic reset button: if you’re about to show a menu and you don’t want a buffered keypress to immediately select something, flush first.

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `inp` | `Optional[RawInput]` | No | `None` | If `None`, creates a temporary `RawInput()` |

Example: clear any buffered keys before a menu:

```python
from notorious_doorkit import RawInput, clear_keybuffer

with RawInput() as inp:
    clear_keybuffer(inp)
    # ... now show a menu or prompt ...
```

Helpful hints: flushing is especially helpful after you display a file (ANSI draw can be slow over some links, and users hammer keys). If you flush too aggressively you can make input feel “unresponsive”, so reserve this for real boundary points (entering a menu, switching screens, etc.).

#### `hotkey_menu(ansi_file: str, hotkeys: Dict[str, Callable[[], Optional[str]]], *, inp=None, time_to_wait_ms=NO_TIMEOUT, clear=True, case_sensitive=False, redraw=True, overlay_mode=0, overlay_prompt=None, overlay_pos=(1, 1), overlay_items=None, overlay_hotkey_format='[X]', overlay_hotkey_color='', overlay_text_color='', unknown_key=None) -> Optional[str]`

Display an ANSI/ASCII file and wait for a single key press, dispatching to a callback based on a hotkey map.

If you’ve ever done the classic “show `MAINMENU.ANS` and wait for a key”, this is that pattern in one place. You give it a menu file plus a dict of hotkeys -> callbacks; it handles file selection (RIP/AVATAR/ANSI/ASCII), draws the file, listens for keys, and calls your function. The callback contract is intentionally simple: return `None` to keep the menu running (and optionally redraw), or return a string to exit the menu and bubble that value up to your caller (often used as a result token like `"quit"` or `"scores"`).

**Parameters**

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `ansi_file` | `str` | Yes | | Full filename (`"MAINMENU.ANS"`) or basename (`"MAINMENU"`) |
| `hotkeys` | `Dict[str, Callable[[], Optional[str]]]` | Yes | | Map hotkey spec -> callback |
| `inp` | `Optional[RawInput]` | No | `None` | If `None`, creates a temporary `RawInput(extended_keys=True)` |
| `time_to_wait_ms` | `int` | No | `NO_TIMEOUT` | Timeout waiting for a key (ms). Returns `None` on timeout |
| `clear` | `bool` | No | `True` | If true, clears screen before displaying the file |
| `case_sensitive` | `bool` | No | `False` | If false, single-character hotkeys match case-insensitively |
| `redraw` | `bool` | No | `True` | If a callback returns `None`, redraw the menu file before continuing |
| `overlay_mode` | `int` | No | `0` | 0 = none, 1 = prompt line, 2 = vertical list overlay |
| `overlay_prompt` | `Optional[str]` | No | `None` | Used when `overlay_mode=1` |
| `overlay_pos` | `Tuple[int,int]` | No | `(1, 1)` | (row, col) for `overlay_mode=2` |
| `overlay_items` | `Optional[List[Tuple[str,str]]]` | No | `None` | Override overlay items for `overlay_mode=2` as `(key, label)` |
| `overlay_hotkey_format` | `str` | No | `"[X]"` | How to render the key in overlay (`X` is replaced) |
| `overlay_hotkey_color` | `str` | No | `""` | Separate color for hotkey char |
| `overlay_text_color` | `str` | No | `""` | Optional ANSI SGR string for overlay label |
| `unknown_key` | `Optional[Callable[[str], None]]` | No | `None` | Called when user presses a key that is not mapped |

**Menu file selection**

`ansi_file` may be a full filename, or a basename without an extension. If you pass a basename, Doorkit tries to pick an extension that matches the caller’s graphics mode:

RIP users prefer `.rip` (then `.avt`, then `.ans/.asc/.txt`), AVATAR users prefer `.avt` (then `.ans/.asc/.txt`), ANSI users prefer `.ans/.asc/.txt`, and ASCII users prefer `.asc/.txt` (falling back to `.ans`).

When you’re testing locally (not on the Door32 FD3 stream), there’s a small compatibility fallback: if the “best” match would be `.rip`/`.avt`, Doorkit prefers showing an ANSI/ASCII equivalent if one exists.

**Hotkey specs**

Key specs in `hotkeys` may be a single character (like `"T"`), a named alias (like `"ESC"` or `"ENTER"`), or one of the Doorkit `KEY_*` tokens (passed as strings).

One nice quality-of-life feature: hotkeys are checked both while the file is being displayed (so a key can interrupt a slow file draw) and after the file finishes displaying.

**Return value**

If the user presses ESC (and you didn’t explicitly map it), or a timeout occurs, `hotkey_menu()` returns `None`. Otherwise:

- If your callback returns `None`, the menu continues.
- If your callback returns a string, `hotkey_menu()` returns that string.

**Overlay modes**

Overlay is optional. It’s there for doors that want the “hotkeys legend” without baking the legend into the ANSI file itself:

- `0`: No overlay (default)
- `1`: Prompt line (uses `overlay_prompt` or auto-generates from keys)
- `2`: Vertical list at `overlay_pos` using `goto_xy()` (useful for ANSI frames)

Here’s a typical pattern: show a menu, let some hotkeys “do something and come back”, and have one hotkey exit the menu with a return token.

```python
from notorious_doorkit import hotkey_menu

def show_scores() -> None:
    # ... draw scores ...
    return None

def quit_menu() -> str:
    return "quit"

result = hotkey_menu(
    "mainmenu.ans",
    {
        "S": show_scores,
        "Q": quit_menu,
    },
)
```

#### `get_input(time_to_wait_ms=NO_TIMEOUT, flags=GETIN_NORMAL, inp=None) -> Optional[InputEvent]`

Get next input event with optional extended key translation.

Think of this as the OpenDoors-style “input event” API. It’s a little more structured than `RawInput.get_key()` and can be handy if you’re porting code that expects “character vs extended key” as distinct event types.

Returns an `InputEvent` with:
- `event_type`: `EVENT_CHARACTER` or `EVENT_EXTENDED_KEY`
- `from_remote`: Always `True` in current implementation
- `key_press`: Character code or `KEYCODE_*` constant

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `time_to_wait_ms` | `int` | No | `NO_TIMEOUT` | Wait time in ms (`NO_TIMEOUT` blocks) |
| `flags` | `int` | No | `GETIN_NORMAL` | Input translation flags |
| `inp` | `Optional[RawInput]` | No | `None` | If `None`, creates a temporary `RawInput(extended_keys=True)` |

In practice, you’ll almost always use `GETIN_NORMAL`. The others exist for parity and for doors that want to do their own translation:

- `GETIN_NORMAL`: Translate extended keys and control-key alternatives
- `GETIN_RAW`: Return raw bytes without translation
- `GETIN_RAWCTRL`: Disable control-key alternatives (Ctrl+E for up arrow, etc.)

Example: handle characters and extended keys differently:

```python
from notorious_doorkit import get_input, EVENT_CHARACTER, EVENT_EXTENDED_KEY

ev = get_input()
if ev is None:
    pass
elif ev.event_type == EVENT_CHARACTER:
    ch = chr(ev.key_press)
elif ev.event_type == EVENT_EXTENDED_KEY:
    keycode = ev.key_press
```

Helpful hints: `get_input()` returns OpenDoors-style `KEYCODE_*` integers for extended keys. If you prefer higher-level `KEY_UP`/`KEY_DOWN` strings, use `RawInput.get_key()` instead. Also note that `GETIN_RAW` returns raw bytes as character events — it’s a sharp tool, mostly for compatibility work.

#### `input_str(max_length: int, min_char: int, max_char: int, *, initial="", allow_cancel=False, inp=None) -> Optional[str]`

Simple line input with character range filtering.

This is the friendly, old-school input helper: give it a max length and an allowed character range and it’ll keep reading until it has a valid line. It’s great for prompts like “enter a 2-digit number” or “type a name” where you don’t want to write your own key filtering.

If `allow_cancel=True`, ESC becomes a clean escape hatch and you’ll get `None` back.

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `max_length` | `int` | Yes | | Hard cap |
| `min_char` | `int` | Yes | | Minimum allowed character (ord) |
| `max_char` | `int` | Yes | | Maximum allowed character (ord) |
| `initial` | `str` | No | `""` | Prefill buffer |
| `allow_cancel` | `bool` | No | `False` | If True, ESC returns `None` |
| `inp` | `Optional[RawInput]` | No | `None` | If `None`, creates a temporary `RawInput(extended_keys=True)` |

Example: numeric input and a simple name prompt:

```python
from notorious_doorkit import input_str

# Input 2-digit number (0-9 only)
num = input_str(2, ord('0'), ord('9'))

# Input 35-character name (space to ASCII 127)
name = input_str(35, 32, 127)
```

Helpful hints: `min_char`/`max_char` are numeric character codes (ord values), not literal characters. If you need richer validation (like “allow digits but also hyphen”), read a line with `RawInput.read_line()` and validate in Python.

#### `edit_str(value, fmt, row, col, normal_attr, highlight_attr, blank, flags=EDIT_FLAG_NORMAL, inp=None) -> Tuple[int, str]`

Formatted string input with full line editing (simplified implementation). Returns `(return_code, final_value)` tuple.

This is here mainly for OpenDoors parity. It’s good for fixed-width “fields on a screen” where you want basic editing, but if you’re building anything larger than a couple fields, `AdvancedForm` will be much nicer to live in.

**Parameters**

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `value` | `str` | Yes | | Original value (also used as the cancel return value) |
| `fmt` | `str` | Yes | | Only the length is used; max input length = `len(fmt)` |
| `row`, `col` | `int` | Yes | | Screen position (1-indexed) |
| `normal_attr` | `int` | Yes | | PC text attribute (0x00-0xFF) used when not focused |
| `highlight_attr` | `int` | Yes | | PC text attribute (0x00-0xFF) used when focused |
| `blank` | `str` | Yes | | Fill character; only `blank[0]` is used |
| `flags` | `int` | No | `EDIT_FLAG_NORMAL` | Bitmask; only a subset is implemented (see below) |
| `inp` | `Optional[RawInput]` | No | `None` | If `None`, creates a temporary `RawInput(extended_keys=True)` |

Return codes:
- `EDIT_RETURN_ACCEPT`: User pressed Enter
- `EDIT_RETURN_CANCEL`: User pressed ESC (if `EDIT_FLAG_ALLOW_CANCEL` set)
- `EDIT_RETURN_PREVIOUS`: User pressed Up/Shift+Tab (if `EDIT_FLAG_FIELD_MODE` set)
- `EDIT_RETURN_NEXT`: User pressed Down/Tab (if `EDIT_FLAG_FIELD_MODE` set)
- `EDIT_RETURN_ERROR`: Invalid parameters

Supported flags:

- `EDIT_FLAG_ALLOW_CANCEL`: allow ESC to cancel
- `EDIT_FLAG_FIELD_MODE`: return Previous/Next on Up/Shift+Tab and Down/Tab
- `EDIT_FLAG_EDIT_STRING`: prefill the buffer from `value` (otherwise starts empty)

**Note**: For full OpenDoors format string support and advanced editing, use `InputField` or `AdvancedForm` instead.

**Controls**

- **[accept]** Enter
- **[cancel]** ESC (only if `EDIT_FLAG_ALLOW_CANCEL` is set)
- **[field navigation]** Up/Shift+Tab (Previous), Down/Tab (Next) when `EDIT_FLAG_FIELD_MODE` is set
- **[cursor]** Left/Right, Home/End
- **[edit]** Backspace and Delete remove the character to the left of the cursor

**Example: edit a fixed-width field**

```python
from notorious_doorkit import edit_str
from notorious_doorkit import EDIT_FLAG_ALLOW_CANCEL, EDIT_FLAG_EDIT_STRING
from notorious_doorkit import EDIT_RETURN_ACCEPT
from notorious_doorkit import Attr

normal = int(Attr.from_color_desc("WHITE ON BLUE"))
hi = int(Attr.from_color_desc("BLACK ON CYAN"))

code, out = edit_str(
    value="KEVIN",
    fmt="XXXXXXXXXX",  # 10-char field
    row=10,
    col=20,
    normal_attr=normal,
    highlight_attr=hi,
    blank=" ",
    flags=EDIT_FLAG_ALLOW_CANCEL | EDIT_FLAG_EDIT_STRING,
)

if code == EDIT_RETURN_ACCEPT:
    print(f"New value: {out}")
```

#### `ScrollingRegion`

A `ScrollingRegion` is your “chat/log window” building block: you append lines to it over time, and it will render a viewport into that growing buffer. The nice part is the auto-scroll behavior is what callers expect from a BBS: when you’re already at the bottom, new lines follow along; when you scroll up to read history, new lines keep coming in *without yanking you back down*.

**Constructor parameters**

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `x`, `y` | `int` | Yes | | Top-left corner (1-indexed) |
| `width`, `height` | `int` | Yes | | Text viewport dimensions |
| `max_lines` | `int` | No | `1000` | Buffer limit (oldest lines dropped) |
| `show_scrollbar` | `bool` | No | `False` | If True, scrollbar draws at x+width (outside text area) |

**Methods**

- `append(text: str)` - Add text to buffer (can contain newlines and ANSI codes)
- `scroll_up(lines=1)` / `scroll_down(lines=1)` - Manual scrolling
- `page_up()` / `page_down()` - Scroll by viewport height
- `scroll_to_top()` / `scroll_to_bottom()` - Jump to oldest/newest
- `render()` - Draw current viewport to screen
- `clear()` - Clear buffer and reset

In a typical door, you’ll call `append()` whenever you receive a message/event, and call `render()` whenever you want to repaint the window (either every loop tick, or only when something changed).

**Auto-scroll behavior**: When at bottom, new messages auto-scroll. When scrolled up, new messages don't interrupt reading.

**Example: chat window**

This example shows the basic lifecycle: create the region, append a few lines, draw it, then append more later and draw again.

```python
from notorious_doorkit import ScrollingRegion, CYAN, GREEN, RESET

region = ScrollingRegion(x=5, y=10, width=60, height=12, show_scrollbar=True)

region.append(f"{CYAN}[System]{RESET} Welcome to chat!")
region.append(f"{GREEN}[Alice]{RESET} Hello everyone!")
region.render()

# User can scroll with arrows/PgUp/PgDn
# New messages continue to append
region.append(f"{GREEN}[Alice]{RESET} New message appears")
region.render()
```

Helpful hints: `append()` can include ANSI SGR sequences, but it won’t interpret OpenDoors backtick color markup — use `printf()`/`render()` yourself if you’re generating colored lines. If you’re rendering frequently, it’s usually worth tracking a “dirty” flag so you only call `render()` when something changed.

#### `TextBufferViewer`

`TextBufferViewer` is a “read-only viewer” for large blobs of text, similar to `less`. It’s great for viewing help files without paginating, browsing logs, or showing a scrollable message-of-the-day. It understands ANSI in the buffer, and it gives the user full control to scroll around without you writing your own key handling loop.

Helpful hints: this viewer is meant for “static buffers” (a snapshot of text). If you need a live-updating feed (chat/log tail), use `ScrollingRegion` instead. When you size the viewer, remember the optional status line — if you want a full `24` rows of content *plus* status, give it a taller region.

**Constructor parameters**

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `x`, `y` | `int` | Yes | | Top-left corner (1-indexed) |
| `width`, `height` | `int` | Yes | | Text viewport dimensions (height includes status line if enabled) |
| `show_status` | `bool` | No | `True` | Show line number/percentage at bottom |
| `show_scrollbar` | `bool` | No | `True` | If True, scrollbar draws at x+width (outside text area) |

**Methods**

- `set_text(text: str)` - Load text buffer (can contain ANSI codes)
- `view()` - Enter interactive mode (blocks until ESC/q)
- `scroll_up(lines=1)` / `scroll_down(lines=1)` - Manual scrolling
- `scroll_left(cols=1)` / `scroll_right(cols=1)` - Horizontal scrolling
- `page_up()` / `page_down()` - Scroll by page
- `scroll_to_top()` / `scroll_to_bottom()` - Jump to top/bottom
- `render()` - Draw current viewport

**Controls** (in `view()` mode):
These controls are meant to feel like a classic door text viewer: arrows for movement, PgUp/PgDn for paging, and a clean escape to exit.

- arrows: line/char navigation
- PgUp/PgDn or Ctrl+U/Ctrl+D: page up/down
- Home/End or Ctrl+H/Ctrl+E: jump to top/bottom
- ESC or 'q': exit viewer

**Example: view log file**

This loads the whole file into the viewer and then hands over control until the user exits.

```python
from notorious_doorkit import TextBufferViewer

viewer = TextBufferViewer(x=1, y=1, width=80, height=24)

with open('system.log') as f:
    viewer.set_text(f.read())

viewer.view()  # Blocks until user presses ESC
```

### Dropfiles (`dropfiles.py`)

Dropfiles are the “classic BBS handshake” files: DOOR.SYS, DORINFOx.DEF, CHAIN.TXT, etc.

This module takes a practical approach: parse what we can, normalize the useful bits into a friendly object model, but always keep the original `raw_lines` around so you can grab edge-case fields when you run into a weird BBS setup.

Dropfiles are not a single standard — there are “families” of formats, and even within the same filename there are variants (especially DOOR.SYS). The goal here is to give you a consistent place to ask the common questions (who is the caller, how much time is left, and what kind of screen they have) without pretending we can perfectly decode every BBS’ quirks.

If you need to write changes back (like updating an alias), you *can*, but Doorkit is intentionally conservative and only writes formats where it’s safe and well-defined.

#### `load_dropfile(path, drop_type=None) -> DropfileInfo`

Load and parse a dropfile, returning a `DropfileInfo` object with normalized fields.

This is the “front door” you’ll use most of the time: point it at whatever the BBS provided and it will best-effort detect the format from the filename, then populate fields like `df.user.alias`, `df.time_left_secs`, and `df.screen_rows` when available.

When the format is unknown (or only partially recognized), you still get a valid object back with `raw_lines` intact. That makes it easy to support one-off systems without forking the parser.

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `path` | `str | Path` | Yes | | Dropfile path |
| `drop_type` | `Optional[str]` | No | `None` | Override detection (`"door.sys"`, `"dorinfo1.def"`, etc.) |

Supported types (best-effort):

- `door.sys`
- `doorfile.sr`
- `dorinfo1.def`
- `dorinfo2.def`
- `chain.txt`

Additionally, `DOOR32.SYS` is parsed when the filename matches exactly (best-effort fields are extracted), even though it’s not a “classic” dropfile family.

If you know exactly which format you’re dealing with, passing `drop_type` can avoid mis-detection when a BBS uses nonstandard naming or wrapper behavior.

Examples:

Auto-detect and grab the fields most doors care about:

```python
from notorious_doorkit import load_dropfile

df = load_dropfile("/path/to/DOOR.SYS")
user = df.user.alias or df.user.full_name
rows = df.screen_rows or 24
```

Detect and use remaining time when available:

```python
from notorious_doorkit import load_dropfile

df = load_dropfile("/path/to/DOOR.SYS")
time_left = df.time_left_secs
if time_left is None:
    time_left = 15 * 60  # fallback (seconds)
```

Force a specific parser when you know the format:

```python
from notorious_doorkit import load_dropfile

df = load_dropfile("/path/to/CHAIN.TXT", drop_type="chain.txt")
```

Helpful hints: Doorkit reads dropfiles as `cp437` with replacement for unknown bytes. That’s the right default for BBS ecosystems, but if you’re parsing a nonstandard file created by a wrapper, expect occasional “best-effort” decoding artifacts. Also, avoid relying on any single line number in DOOR.SYS unless you’ve tested with the target BBS — variants are real.

#### `DropfileInfo`

`DropfileInfo` is the normalized view of the dropfile. Most doors only care about a few fields (who is the caller, how much time is left, and what kind of screen they have), so those are the ones you’ll reach for first.

The philosophy is “typed fields where we’re confident” and “raw escape hatch where we’re not.” If you ever run into a dropfile format we don’t recognize, you can still ship your door by reading `raw_lines` directly and stashing anything format-specific in `extra`.

| Field | Type | Notes |
| --- | --- | --- |
| `drop_type` | `DropfileType` | Parsed type or `UNKNOWN` |
| `path` | `Path` | Original path |
| `raw_lines` | `List[str]` | File content without trailing CR/LF |
| `user.full_name` | `Optional[str]` | Best-effort |
| `user.alias` | `Optional[str]` | Best-effort |
| `node` | `Optional[int]` | Node number when present (best-effort) |
| `time_left_secs` | `Optional[int]` | For DOOR.SYS: uses seconds/minutes lines when available |
| `graphics_mode` | `Optional[str]` | Best-effort emulation hint (format-dependent) |
| `screen_rows` | `Optional[int]` | For DOOR.SYS: usually line 21 |
| `temp_path` | `Optional[str]` | For DOOR.SYS: usually line 33 |
| `bbs_root` | `Optional[str]` | For DOOR.SYS: usually line 34 |
| `extra` | `Dict[str, str]` | Format-specific extras (best-effort) |

Example: “good enough defaults” pattern:

```python
username = df.user.alias or df.user.full_name or "Caller"
time_left = df.time_left_secs  # may be None
rows = df.screen_rows or 24
```

Example: pull a one-off field from an unknown dropfile:

```python
df = load_dropfile("/path/to/SOMETHING.DAT")

# Keep it safe: check bounds and treat it as best-effort text.
line_5 = df.raw_lines[4] if len(df.raw_lines) >= 5 else ""
```

Helpful hints: for DOOR.SYS specifically, Doorkit intentionally avoids treating the “password line” as an alias fallback. That keeps you from accidentally leaking credentials if a system stores sensitive data in one of the classic slots.

#### `DropfileSession(path, drop_type=None, write_back=False)`

A context manager for “load, then optionally write back on exit”.

Right now, write-back is only supported for **DOOR.SYS** (because it has a well-known line-based layout).

**Example: load DOOR.SYS and update alias on exit**

This is a common pattern when you want to normalize or tweak something for the duration of a door run and have it persist for the BBS environment.

```python
from notorious_doorkit import DropfileSession

with DropfileSession("/path/to/DOOR.SYS", write_back=True) as df:
    if df.user.alias:
        df.user.alias = df.user.alias.upper()
```

Example: update screen rows (only if you’re sure your BBS expects it):

```python
from notorious_doorkit import DropfileSession

with DropfileSession("/path/to/DOOR.SYS", write_back=True) as df:
    if df.screen_rows is not None and df.screen_rows < 24:
        df.screen_rows = 24
```

Helpful hints: treat write-back as an “opt-in power tool.” In real deployments, many sysops expect dropfiles to be ephemeral and regenerated each run; writing back is mainly useful when a wrapper uses the file as a shared state handshake. Keep your write-back edits minimal and only touch fields you understand.

### Display (`display.py`)

This module is the “show a file” side of door building. Use it for ANSI/ASCII art, help screens, credits, news files, and anything else where you want to treat a file as a screen template.

The overall shape is intentionally simple: you can display a file as-is (`display_file`), pick the “best match” file for the caller’s emulation (`display_file_best_match`), page through a long file without turning your whole UI into a pager (`display_file_paged`), or treat a large ANSI as a source and copy just a rectangular window out of it (`display_block`).

These helpers are meant for classic door workflows: pre-rendered ANSI screens, simple paged help, and “background template + live overlay” layouts.

#### `display_file(path, encoding="cp437", pause=False, clear=False)`

This is the simplest “show this file as-is” helper. It reads the file as bytes and writes it directly to the terminal stream.

This is intentionally low-magic: it does not try to interpret or transform the content, which makes it perfect for ANSI art and pre-authored templates. If the file doesn’t exist (or can’t be read), it prints a short error line to the terminal and returns.

Examples:

Display a splash screen:

```python
from notorious_doorkit import display_file

display_file("welcome.ans", clear=True)
```

Display a credits file and then pause:

```python
from notorious_doorkit import display_file

display_file("credits.asc", pause=True)
```

Example: display a menu by basename (paired with `display_file_best_match`):

```python
from notorious_doorkit import display_file_best_match

display_file_best_match("MAINMENU", clear=True)
```

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `path` | `str` | Yes | | File path |
| `encoding` | `str` | No | `"cp437"` | Kept for future use; file is read as bytes and written directly |
| `pause` | `bool` | No | `False` | Calls `more_prompt()` afterwards |
| `clear` | `bool` | No | `False` | Clear screen before display |

Helpful hints: the `encoding` parameter is kept for future flexibility, but the current implementation writes raw bytes. That’s great for `.ans`/`.asc`/`.txt` assets, but if you’re generating output dynamically, you’ll usually prefer `write()` / `printf()` so you can compose strings safely.

#### `display_file_best_match(path, *, pause=False, clear=False) -> None`

Displays a CP437/ANSI file, resolving `path` like a classic door “basename chooser.” If `path` already exists as a file, it uses it as-is. If you pass a basename (no extension), it tries a small ordered set of extensions based on the caller’s emulation mode.

The emulation hint comes from the initialized door runtime (`get_runtime()`), using `rt.dropfile` first and falling back to `rt.terminal` capabilities.

Example: pass a basename and let Doorkit pick `.rip`/`.ans`/`.asc`:

```python
from notorious_doorkit import display_file_best_match

display_file_best_match("MAINMENU", clear=True)
```

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `path` | `str` | Yes | | Filename or basename |
| `pause` | `bool` | No | `False` | Calls `more_prompt()` after display |
| `clear` | `bool` | No | `False` | Clear screen before display |

Helpful hints: best-match selection is intentionally conservative. Today it prefers:

- RIP callers: `.rip`, then `.ans`, `.asc`, `.txt`
- ANSI callers: `.ans`, `.asc`, `.txt`
- ASCII callers: `.asc`, `.txt`, then `.ans`

It also tries uppercase/lowercase extension variants. When you’re running locally (not on the door32 FD3 stream), there’s a small usability fallback: if the best match is a `.rip` file, Doorkit will prefer an ANSI/ASCII equivalent if one exists.

#### `display_file_paged(path, pause_prompt="Press any key to continue...", clear=False)`

Non-destructive paged ANSI/ASCII file viewer.

This is the “help file / news file” workhorse: it displays the content in pages, pauses at `(screen_rows - 1)` lines with a configurable prompt, then clears just the prompt line and continues. The important part is that it’s polite to your UI: it doesn’t clear the whole screen at page boundaries, and it doesn’t require you to re-implement paging in every door.

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `path` | `str` | Yes | | File path |
| `pause_prompt` | `str` | No | `"Press any key to continue..."` | Message shown at page breaks |
| `clear` | `bool` | No | `False` | Clear screen before display |

Screen height is determined from the initialized door runtime (`get_runtime()`), preferring `rt.screen_rows`.

**Example:** This is the usual “help file” flow: auto-detect where possible, or force a known row count if you’re drawing inside a fixed layout.
```python
from notorious_doorkit import display_file_paged

# Auto-detect screen size and page through a help file
display_file_paged("help.ans")

# Custom prompt
display_file_paged("news.ans", pause_prompt="-- More --")
```

Helpful hints: the pager uses a `RawInput()` loop internally and blocks until a key is pressed at each page break. For ANSI art that includes SAUCE metadata, Doorkit uses the SAUCE width as the “line wrap” width; otherwise it defaults to 80.

#### `display_block(path, x, y, width, height, offset_x=0, offset_y=0)`

Useful when you want to “window” a portion of an ANSI screen. This is the primitive you reach for when you have a large, pre-rendered ANSI background, and you want to copy only a panel-sized region of it onto the live screen.

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `path` | `str` | Yes | | File path |
| `x`, `y` | `int` | Yes | | Destination screen position (1-indexed) |
| `width`, `height` | `int` | Yes | | Block size |
| `offset_x` | `int` | No | `0` | Source column offset (in visible chars) |
| `offset_y` | `int` | No | `0` | Source row offset (0-indexed line index) |

**Behavior notes**

The file is decoded as CP437, `offset_x` is applied against the *visible* text after stripping ANSI escape sequences, and any ANSI sequences whose “visible position” falls inside the window are re-applied so colors/styles generally stay correct.

**Example: show a viewport of a large ANSI**

The first call copies the top-left of the source file; the second call pans inside the source by shifting the offsets.

```python
from notorious_doorkit import display_block

# Display a 40x10 window from the top-left of the source file
display_block("big_screen.ans", x=5, y=5, width=40, height=10)

# Pan right/down inside the source file
display_block("big_screen.ans", x=5, y=5, width=40, height=10, offset_x=20, offset_y=5)
```

Example: overlay a “panel” onto a static background:

```python
from notorious_doorkit import display_file, display_block

display_file("background.ans", clear=True)
display_block("background.ans", x=10, y=6, width=30, height=8, offset_x=50, offset_y=12)
```

Helpful hints: `display_block()` is best-effort with ANSI inside the window. It tries to keep colors/styles generally correct, but it’s not a full ANSI renderer, and complex files with lots of mid-line style changes may not reproduce perfectly when cropped. For “pixel-perfect” UI composition, prefer drawing the background once and then render live content with normal ANSI output (`write_at()`, `printf()`, etc.).

### Lightbars (`lightbar.py`)

Lightbars are the classic “highlight bar” menus: you draw a list of choices, the user moves the selection with the arrow keys, and the currently selected row is rendered in a different color. They’re a great fit when you want a menu that feels “door-native” without having to build a whole input loop yourself.

Doorkit’s lightbars are intentionally simple: they draw text, not widgets. That makes them easy to drop on top of an ANSI background, inside a box panel, or as a quick “pick one of these” prompt.

If you just want a vertical list, `LightbarMenu` is the quick win. If you want to place each item precisely (two-column layouts, “menu inside a screen”, etc.), `AdvancedLightbarMenu` is the flexible version.

#### `LightbarMenu(items, ...)`

`LightbarMenu` renders a uniform vertical menu: each item is one row, and the cursor moves up/down through the list. It hides the terminal cursor while active, uses `RawInput` under the hood, and returns the selected index when the user presses Enter.

Hotkeys are supported in two ways:

- If an item includes an explicit `{{X}}` marker, that letter becomes the hotkey.
- Otherwise, when `hotkeys=True`, the menu will auto-assign unique hotkeys by scanning letters in each item.

**Constructor parameters**

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `items` | `List[str]` | Yes | | Menu text (supports `{{X}}` marker for explicit hotkey) |
| `x`, `y` | `int` | No | `1` | Top-left |
| `width` | `Union[int,str,None]` | No | `None` | `None`/`"auto"`, `"exact"`, or fixed int |
| `justify` | `str` | No | `"left"` | `left`, `center`, `right` |
| `selected_color` | `str` | No | bright cyan | ANSI SGR string |
| `normal_color` | `str` | No | white | ANSI SGR string |
| `hotkeys` | `bool` | No | `True` | Enables hotkey selection |
| `hotkey_format` | `str` | No | `"none"` | `none`, `(X)`, `[X]`, `underline` |
| `hotkey_color` | `str` | No | `""` | Separate color for hotkey char |
| `margin_inner` | `int` | No | `0` | Inner padding inside each bar |
| `wrap` | `bool` | No | `True` | Wrap at top/bottom |

**Return value**

`run() -> Optional[int]` returns the selected index, or `None` if the user cancels with ESC.

Example: simplest possible vertical menu:

```python
from notorious_doorkit import LightbarMenu

menu = LightbarMenu(["Play", "Scores", "Quit"], x=5, y=5)
choice = menu.run()
```

Example: centered menu in a box panel, with explicit hotkeys:

```python
from notorious_doorkit import ansi_box, LightbarMenu
from notorious_doorkit import BLACK, WHITE, BG_BLUE, BG_BRIGHT_CYAN, BOLD, RESET

ansi_box(
    x=8, y=6, width=30, height=8,
    style="double",
    border_color=f"{WHITE}{BG_BLUE}{BOLD}",
    fill_color=f"{WHITE}{BG_BLUE}",
)

menu = LightbarMenu(
    ["{{P}}lay", "{{S}}cores", "{{Q}}uit"],
    x=10, y=8,
    width=26,
    justify="center",
    normal_color=f"{WHITE}{BG_BLUE}",
    selected_color=f"{BLACK}{BG_BRIGHT_CYAN}{BOLD}",
    hotkeys=True,
    hotkey_format="underline",
)

choice = menu.run()
print(RESET)
```

Example: treat ESC as “go back” and loop until the user picks something:

```python
from notorious_doorkit import LightbarMenu

while True:
    menu = LightbarMenu(["Start", "Help", "Back"], x=5, y=5)
    choice = menu.run()
    if choice is None:
        break
    if choice == 0:
        break
    if choice == 1:
        pass
    if choice == 2:
        break
```

Helpful hints: `width="auto"` makes every row the same width based on the longest item (plus `margin_inner`). If you want each row to “hug” its own text, use `width="exact"`. If you’re drawing a menu inside a UI panel, you’ll usually want a fixed integer width so the highlight bar always fills the same space.

Helpful hints: avoid embedding your own ANSI codes inside `items`. Lightbars measure text width and hotkey positions on the raw string, so inline ANSI can make alignment and underlining weird. Prefer controlling color via `normal_color`, `selected_color`, and (optionally) `hotkey_color`.

#### `AdvancedLightbarMenu(items, ...)`

`AdvancedLightbarMenu` is for “screen layout” menus: each item has its own `(x, y)` position, width, and justification. It still behaves like a lightbar (highlight + Enter to select), but you can build two-column menus, menus inside a framed area, or even scattered “button rows” on top of a static ANSI screen.

In addition to up/down selection, the advanced menu supports left/right navigation. It tries to find the nearest item horizontally by comparing the *center point* of each item (so it works well for column layouts). With `wrap=True`, left/right can “wrap” to the far side if there isn’t a neighbor in that direction.

Here, `items` is a list of `LightbarItem(text, x, y, width="auto", justify="left", hotkey=None)`.

#### `LightbarItem`

`LightbarItem` is one positioned menu entry. Think of it as “a clickable label at coordinates,” plus metadata about how wide the highlight bar should be and how the text should align within it.

Hotkeys for `AdvancedLightbarMenu` are resolved in a predictable order:

- If the text contains a `{{X}}` marker, that letter is used as the hotkey.
- Otherwise, if `hotkey="X"` is set and that letter appears in the text, it’s used.
- Otherwise, when `hotkeys=True`, the menu auto-assigns a unique hotkey by scanning letters in the text.

| Field | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `text` | `str` | Yes | | Display text (supports `{{X}}` marker for explicit hotkey) |
| `x`, `y` | `int` | Yes | | Screen position (1-indexed) |
| `width` | `Union[int,str]` | No | `"auto"` | `"auto"`/`"exact"` or fixed int |
| `justify` | `str` | No | `"left"` | `left`, `center`, `right` |
| `hotkey` | `Optional[str]` | No | `None` | Optional explicit hotkey preference |

**Constructor parameters** (`AdvancedLightbarMenu`)

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `items` | `List[LightbarItem]` | Yes | | Positioned menu items |
| `selected_color` | `str` | No | bright cyan | ANSI SGR string |
| `normal_color` | `str` | No | white | ANSI SGR string |
| `hotkeys` | `bool` | No | `True` | Enables hotkey selection |
| `hotkey_format` | `str` | No | `"none"` | `none`, `(X)`, `[X]`, `underline` |
| `hotkey_color` | `str` | No | `""` | Separate color for hotkey char |
| `margin_inner` | `int` | No | `0` | Inner padding inside each bar |
| `wrap` | `bool` | No | `True` | Wrap navigation |

**Return value**

`run() -> Optional[int]` returns the selected index, or `None` if the user cancels with ESC.

**Controls**

- arrows: move selection (left/right works for non-column layouts)
- Enter: accept
- ESC: cancel

**Example: “two-column” layout**

```python
from notorious_doorkit import AdvancedLightbarMenu, LightbarItem
from notorious_doorkit import BLACK, WHITE, BG_BLUE, BG_BRIGHT_CYAN, BOLD

items = [
    LightbarItem("{{P}}lay", x=10, y=8, width=18, justify="center"),
    LightbarItem("{{S}}cores", x=10, y=10, width=18, justify="center"),
    LightbarItem("{{O}}ptions", x=35, y=8, width=18, justify="center"),
    LightbarItem("{{Q}}uit", x=35, y=10, width=18, justify="center"),
]

menu = AdvancedLightbarMenu(
    items,
    selected_color=f"{BLACK}{BG_BRIGHT_CYAN}{BOLD}",
    normal_color=f"{WHITE}{BG_BLUE}",
    hotkeys=True,
    hotkey_format="[X]",
    margin_inner=1,
    wrap=True,
)

choice = menu.run()
```

Example: map menu indexes to actions (works for both menu types):

```python
from notorious_doorkit import AdvancedLightbarMenu, LightbarItem

items = [
    LightbarItem("Files", x=10, y=8, width=12, justify="left"),
    LightbarItem("Chat", x=10, y=10, width=12, justify="left"),
    LightbarItem("Logout", x=10, y=12, width=12, justify="left"),
]

actions = {
    0: "files",
    1: "chat",
    2: "logout",
}

choice = AdvancedLightbarMenu(items).run()
if choice is not None:
    action = actions[choice]
```

Helpful hints: `AdvancedLightbarMenu` redraws only the previously selected item and the newly selected item as you move, so it’s friendly to “background” screens. That said, it doesn’t clear the whole item area for you — it only draws within each item’s `width`. If you’re seeing visual leftovers, increase the item width (or use `margin_inner`) so the bar overwrites the full region you intend.

### Forms (`forms.py`)

Forms are how you get the classic BBS “EDIT SCREEN” experience: a full-screen (or panel-sized) set of fields the user can move through, edit, and save as a single unit. If lightbars are for choosing an action, forms are for collecting structured data — profile screens, configuration panels, or “fill in the blanks” questionnaires.

The model is deliberately old-school: you lay out fields at fixed coordinates, optionally draw a UI frame/background, and then let the user navigate with arrow keys and edit with Enter. It’s fast, predictable, and feels right over a terminal.

#### `FormField`

`FormField` is a positioned input slot. Think of it as “the text area where a value lives,” plus the rules for how the user can edit it. The `x`, `y`, and `width` coordinates describe the *input region* (not the label), and the form will draw the current value padded out to `width` so the field area stays visually stable.

There are three common editing modes:

- Plain text (optionally `mask=True` for password-style stars)
- Structured `format_mask` input (date/phone/zip patterns)
- Toggle fields via `options` (cycle a fixed set of choices)

| Field | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `name` | `str` | Yes | | Key in returned dict |
| `x`, `y`, `width` | `int` | Yes | | Screen placement |
| `label` | `str` | No | `""` | Rendered as `label + ": "` |
| `value` | `str` | No | `""` | Stored value |
| `max_length` | `int` | No | `255` | Input cap |
| `mask` | `bool` | No | `False` | Password rendering |
| `required` | `bool` | No | `False` | Enforced on save |
| `format_mask` | `str` | No | `""` | `0` digit, `A` alpha, `X` alnum |
| `options` | `List[Tuple[str,str]]` | No | `[]` | Toggle field: store value, display label |
| `hotkey` | `str` | No | `""` | Jump-to-field hotkey |
| `label_color/input_color/focus_color` | `str` | No | `""` | Per-field color overrides |
| `normalize` | `Callable[[str],str]` | No | `None` | Apply on commit |
| `validate` | `Callable[[str],bool]` | No | `None` | Validate on commit |

**Masked input**

`mask=True` renders asterisks while the user types, but stores the real value in `value`.

Example: password field:

```python
from notorious_doorkit import AdvancedForm, FormField

fields = [
    FormField("pw", x=20, y=10, width=16, label="Password", mask=True, required=True),
]

result = AdvancedForm(fields).run()
```

**Structured input (`format_mask`)**

`format_mask` is for “structured” inputs: `0` means digit, `A` means letter, and `X` means alphanumeric. Common masks look like a DOB (`"00/00/00"`), a phone (`"(000) 000-0000"`), a two-letter state (`"AA"`), or a ZIP (`"00000"`). Internally, the form stores only the raw typed characters (not the punctuation), and renders the mask as a template while editing.

Example: normalize a two-letter state code:

```python
from notorious_doorkit import AdvancedForm, FormField

fields = [
    FormField(
        "state",
        x=20, y=6, width=2,
        label="State",
        format_mask="AA",
        max_length=2,
        normalize=lambda s: s.upper(),
        required=True,
    ),
]

result = AdvancedForm(fields).run()
```

**Toggle fields (`options`)**

If `options` is set, the field edits by cycling choices: Left/Up selects the previous option, Right/Down/Space selects the next option, Enter accepts, and ESC cancels.

Example: a “toggle field” that cycles with arrows (and lets the user press the first letter to jump):

```python
from notorious_doorkit import AdvancedForm, FormField

fields = [
    FormField(
        "theme",
        x=20, y=8, width=12,
        label="Theme",
        value="D",
        options=[
            ("D", "Dark"),
            ("L", "Light"),
            ("H", "High-Contrast"),
        ],
    ),
]

result = AdvancedForm(fields).run()
```

Example: define a couple fields (the full “screen form” pattern is shown in Common recipes):

```python
from notorious_doorkit import AdvancedForm, FormField

fields = [
    FormField("name", x=10, y=5, width=25, label="Name", required=True),
    FormField("age", x=10, y=6, width=3, label="Age", required=False, max_length=3),
]

result = AdvancedForm(fields).run()
```

Helpful hints: for `options`, `value` should be the *stored option value* (the first element of each tuple). The form displays the option label (the second element). If `value` is empty, the first option’s label is shown.

Helpful hints: `label` is drawn to the left as `"{label}: "`. If your `x` is too small, labels can run into the left edge. A good mental model is: `x` is the start of the editable area, so give yourself room for the label.

#### `AdvancedForm(fields, ...)`

`AdvancedForm` draws all fields and runs the edit loop. It hides the cursor while you’re navigating, shows it while you’re actively editing a field, and returns a dict of `name -> value` when the user saves.

Navigation is “spatial” rather than strictly linear: arrow keys try to find the nearest neighbor in the direction you pressed based on the *center point* of each field’s input region. That means you can do column layouts naturally (e.g., “left column / right column” forms), and the cursor still moves in a way that feels right.

Saving has two flavors:

- `save_mode="ctrl_s"`: ESC exits immediately; Ctrl+S saves.
- `save_mode="esc_prompt"`: ESC shows a prompt to keep editing, save, or exit.

Required fields are enforced on save (Ctrl+S or “save” from the ESC prompt). If you set `required_splash_text`, the form will show a message and jump focus to the first missing required field.

**Constructor parameters**

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `fields` | `List[FormField]` | Yes | | |
| `wrap` | `bool` | No | `True` | Wrap navigation |
| `save_mode` | `str` | No | `"ctrl_s"` | `ctrl_s` or `esc_prompt` |
| `label_color/input_color/focus_color` | `str` | No | `""` | Default colors |
| `required_splash_text` | `str` | No | `""` | Optional message on failed save |
| `required_splash_x/y` | `int` | No | `1` / `24` | Where to show the message |
| `required_splash_color` | `str` | No | `""` | Color for message |

**Return value**

`run() -> Optional[Dict[str,str]]` returns a dict of `name -> value` when saved, or `None` if the user exits.

Example: a compact “profile edit” screen with per-field hotkeys and validation:

```python
from notorious_doorkit import AdvancedForm, FormField
from notorious_doorkit import WHITE, BG_BLUE, BG_BLACK, BG_RED, BOLD, YELLOW

def validate_age(s: str) -> bool:
    if not s.strip():
        return True
    if not s.isdigit():
        return False
    return 0 <= int(s) <= 120

fields = [
    FormField("handle", x=18, y=6, width=20, label="Handle", required=True, hotkey="h"),
    FormField("age", x=18, y=7, width=3, label="Age", max_length=3, hotkey="a", validate=validate_age),
    FormField("state", x=18, y=8, width=2, label="State", format_mask="AA", max_length=2, hotkey="s", normalize=lambda v: v.upper()),
]

form = AdvancedForm(
    fields,
    save_mode="esc_prompt",
    label_color=f"{WHITE}{BG_BLUE}",
    input_color=f"{WHITE}{BG_BLACK}",
    focus_color=f"{WHITE}{BG_RED}{BOLD}",
    required_splash_text="Please fill required fields",
    required_splash_x=5,
    required_splash_y=22,
    required_splash_color=f"{YELLOW}{BG_BLUE}{BOLD}",
)

result = form.run()
```

Helpful hints: `hotkey` is only for jumping focus while you’re on the form (not while actively editing inside a field). Choose letters that won’t conflict too badly with normal typing for your audience, or leave it blank and rely on arrow navigation.

Helpful hints: `validate` and `normalize` run when the user commits a field (Enter). If validation fails, the editor stays in the field and re-renders, so you can enforce constraints without extra UI.

### Boxes (`boxes.py`)

Boxes are here because every sysop ends up drawing panels. They’re a tiny convenience layer, but they unlock a ton of “door UI” patterns: framed menus, status panes, bordered forms, and little pop-up dialogs.

Use `ascii_box()` when you want a *string* (for logs, static output, or building a larger text block in memory). Use `ansi_box()` when you want to draw directly onto the screen at a coordinate — especially when you’re composing a UI (background + panels + menus).

#### `ascii_box(width, height, style="double") -> str`

Returns a CRLF-delimited box string. The returned string includes the border characters and spaces for the interior. If `width < 2` or `height < 2`, it returns an empty string.

Example: generate a box and print it:

```python
from notorious_doorkit import ascii_box, writeln

writeln(ascii_box(20, 5, style="single"))
```

Example: build a boxed “message of the day” block as a single string:

```python
from notorious_doorkit import ascii_box

box = ascii_box(32, 5, style="double").split("\r\n")
box[2] = box[2][:2] + " Welcome back! ".ljust(28) + box[2][-2:]
motd = "\r\n".join(box)
```

Helpful hints: the `single`/`double` styles use classic box-drawing glyphs. If you’re dealing with a client that renders those inconsistently, use `style="ascii"` for the “always safe” fallback.

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `width` | `int` | Yes | | must be `>= 2` |
| `height` | `int` | Yes | | must be `>= 2` |
| `style` | `str` | No | `"double"` | `ascii`, `single`, `double` |

#### `ansi_box(x, y, width, height, style="double", border_color="", fill_color="", fill_char=" ")`

Draws the box directly at screen coordinates (1-indexed). This is the one you usually want for UI work: draw a panel, then overlay a menu, a form, or your own `write_at()` labels inside it.

Example: draw a panel for a menu:

```python
from notorious_doorkit import ansi_box, WHITE, BG_BLUE, BOLD

ansi_box(
    x=3, y=3, width=40, height=10,
    style="double",
    border_color=f"{WHITE}{BG_BLUE}{BOLD}",
)
```

Example: filled panel + lightbar menu inside:

```python
from notorious_doorkit import ansi_box, LightbarMenu
from notorious_doorkit import BLACK, WHITE, BG_BLUE, BG_BRIGHT_CYAN, BOLD

ansi_box(
    x=8, y=6, width=30, height=9,
    style="double",
    border_color=f"{WHITE}{BG_BLUE}{BOLD}",
    fill_color=f"{WHITE}{BG_BLUE}",
    fill_char=" ",
)

menu = LightbarMenu(
    ["Play", "Scores", "Quit"],
    x=10, y=8,
    width=26,
    justify="center",
    normal_color=f"{WHITE}{BG_BLUE}",
    selected_color=f"{BLACK}{BG_BRIGHT_CYAN}{BOLD}",
    margin_inner=1,
)

choice = menu.run()
```

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `x`, `y` | `int` | Yes | | Top-left corner (1-indexed) |
| `width`, `height` | `int` | Yes | | Must be `>= 2` |
| `style` | `str` | No | `"double"` | `ascii`, `single`, `double` |
| `border_color` | `str` | No | `""` | ANSI SGR prefix for border |
| `fill_color` | `str` | No | `""` | ANSI SGR prefix for the interior |
| `fill_char` | `str` | No | `" "` | Intended as a single character; if you pass a longer string it will be repeated as-is |

Helpful hints: `ansi_box()` does nothing when `width < 2` or `height < 2`. For small “labels with borders” you usually want at least `width=3` / `height=3` so there’s interior space.

Helpful hints: `fill_color` is applied only to the interior, not the border. A common pattern is “colored border + colored fill”, then draw text inside with `write_at()` or a menu/form using matching colors.

### LNWP (`lnwp.py`, `lnwp_door.py`)

LNWP is the NotoriousPTY “control plane” that sits alongside the classic terminal stream. The goal is to keep normal door I/O compatible with the legacy world (a byte stream that looks like a real terminal, so doors still work over telnet/ssh/door32), while also giving modern supervisors a structured way to do things like node chat, activity strings, mailbox delivery, and richer session introspection.

If your door only needs to draw screens and read keys, you can ignore LNWP entirely and stick with `Door`. LNWP becomes useful when you want “modern” features that aren’t naturally expressed as terminal bytes (messages, node info, rich status).

There are two layers:

- `lnwp.py`: protocol helpers (frame parsing and payload key/value handling)
- `lnwp_door.py`: `LnwpDoor`, a door runtime that can send/receive LNWP messages

#### How it is wired (door32 vs stdio)

LNWP can be delivered in one of two ways:

- A dedicated control stream (LNWP frames are separate from the terminal byte stream).
- A multiplexed stdio stream (LNWP frames appear inline and must be framed out of the byte stream).

This matters because in a multiplexed stream you can end up with mixed data (LNWP frames + plain terminal bytes). `LnwpDoor.poll()` returns both the parsed frames and the “passthrough” bytes that were not part of LNWP, so you can keep your terminal output sane.

#### `LnwpDoor`

Use `LnwpDoor` when you want “door runtime + LNWP”. It inherits the normal session behavior from `Door` (dropfile parsing, terminal info, etc.) and adds LNWP send/receive and convenience helpers.

Practically: you still write to the terminal the normal way (same screen helpers, same input helpers), but you also get a way to exchange structured messages with the supervisor. If you’re building anything that wants node chat, status strings, or message delivery, `LnwpDoor` is the “right shaped” starting point.

**Constructor**

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `door_key` | `Optional[str]` | No | env `NOTORIOUS_DOOR_KEY` or `"UNKNOWN"` | Identifies the door/session key |
| `input_fd` | `int` | No | `0` | In door32 mode, `0` maps to LNWP FD4; otherwise treated as a normal fd |

**Common methods**

| Method | Purpose | Returns |
| --- | --- | --- |
| `enable_lnwp()` / `disable_lnwp()` | Arm/disarm LNWP for this session | `None` |
| `set_activity(activity)` | Set the user activity string shown in node listings | `None` |
| `set_input_mode(mode)` | Switch supervisor input mode (`"raw"`/`"cooked"` plus aliases like `menu/form/editor/chat`) | `None` |
| `set_push_messages(enabled, box="TEMP")` | Enable/disable push nudges (`NEW_MSG`) | `None` |
| `send_message(node_id, text, box="TEMP", tag="CHAT")` | Send message to node (0 broadcasts) | `None` |
| `poll(timeout=0.0)` | Parse LNWP frames and return `(frames, passthrough)` | `Tuple[List[Tuple[str, Dict[str,str]]], bytes]` |
| `poll_events(timeout=0.0)` | Higher-level event parsing (`NEW_MSG`, `MSG`, `INFO`, etc.) | `Tuple[List[LnwpEvent], bytes]` |
| `poll_messages(timeout=0.0)` / `get_messages(timeout=0.0)` | Convenience: return delivered message payload dicts | `Tuple[List[Dict[str,str]], bytes]` / `List[Dict[str,str]]` |
| `request_nodes_info(keys=None)` / `get_nodes_info()` | Query and/or retrieve nodes snapshot | `None` / `Dict[int, Dict[str,str]]` |
| `request_node_info(keys=None)` | Query your own node info snapshot | `None` |
| `get_nodes_info_cached()` / `get_node_info_cached()` | Return last cached snapshot | `Dict[...]` |

**Example: activity + message broadcast**

This is the “modern supervisor” pattern: you still draw to the terminal like a normal door, but you can also publish activity and send messages through LNWP.

```python
import sys

from notorious_doorkit import writeln
from notorious_doorkit.lnwp_door import LnwpDoor

door = LnwpDoor()
door.start()

door.enable_lnwp()
door.set_activity("Chatting")

door.send_message(node_id=0, text="Hello everyone!", tag="CHAT")
writeln("Broadcast sent.")
```

Example: toggle supervisor input mode around a raw-ish UI (forms/editors/chat):

```python
from notorious_doorkit.lnwp_door import LnwpDoor
from notorious_doorkit import AdvancedForm, FormField

door = LnwpDoor()

# Tell the supervisor we’re entering a raw-mode screen.
door.set_input_mode("form")

fields = [FormField("handle", x=10, y=5, width=20, label="Handle")]
result = AdvancedForm(fields).run()

# Back to menu/line mode.
door.set_input_mode("menu")
```

Example: enable “new message nudges” and drain delivered messages:

```python
from notorious_doorkit.lnwp_door import LnwpDoor

door = LnwpDoor()
door.enable_lnwp()
door.set_push_messages(True, box="TEMP")

while True:
    events, passthrough = door.poll_events(timeout=0.1)
    if passthrough:
        # In stdio mode this can contain terminal bytes that weren’t LNWP.
        pass

    msgs = door.drain_messages()
    for m in msgs:
        # Typical fields include: PRM=MSG, VAL=..., TAG=..., FROM_NODE=..., BOX=...
        pass
```

Example: request a nodes snapshot (best-effort helper):

```python
from notorious_doorkit.lnwp_door import LnwpDoor

door = LnwpDoor()
door.enable_lnwp()

nodes = door.get_nodes_info()
for node_id, fields in nodes.items():
    activity = fields.get("ACTIVITY", "")
    username = fields.get("USERNAME", "")
```

Helpful hints: `poll()` and friends default to `max_bytes=4096`. If you expect larger bursts (or you’re doing unusually chatty control traffic), you can raise this limit, but you should still assume supervisors may impose their own caps.

Helpful hints: `send_lnwp()` encodes payloads as ASCII (non-ASCII becomes `?`-style replacement). Keep LNWP payload values simple and treat it like a control channel, not a rich text transport.

**Note:** `LnwpDoor` is not currently re-exported at the top level of `notorious_doorkit`, so the import shown above is the reliable one.

#### `lnwp_payload(prm: str, val: Optional[str] = None, **kv: str) -> str`

Build an LNWP payload string (the `LNWP V1 PRM=...` format). This is mainly useful if you’re doing low-level custom commands via `send_lnwp()` or you want to log/debug what’s going over the control plane.

Values are quoted and sanitized: embedded double-quotes are replaced with single-quotes, and newlines are converted to spaces. This keeps payloads predictable even if you pass “user text” through LNWP.

| Parameter | Type | Required | Default | Notes |
| --- | --- | --- | --- | --- |
| `prm` | `str` | Yes | | PRM value |
| `val` | `Optional[str]` | No | `None` | Optional VAL |
| `**kv` | `str` values | No | | Extra key/value pairs |

Example: build the payload for a targeted message:

```python
from notorious_doorkit.lnwp import lnwp_payload

payload = lnwp_payload("SEND_MSG", "Hello", NODE_ID="2", BOX="TEMP", TAG="CHAT")
```

Helpful hints: `lnwp_payload()` doesn’t validate command names or keys. Treat it as a string builder and keep your `PRM` names / key names consistent with what your supervisor expects.

#### `parse_lnwp_payload(payload: str) -> Dict[str, str]`

Parse an LNWP payload string into a dictionary of fields.

This is intentionally forgiving: it tokenizes on whitespace while respecting quoted values, then collects `KEY=VALUE` pairs. Keys are uppercased, and surrounding quotes are stripped from values. It does not require the payload to start with `LNWP V1` — it’s happy to parse any “space-separated key/value” string.

Example: parse an incoming payload:

```python
from notorious_doorkit.lnwp import parse_lnwp_payload

fields = parse_lnwp_payload('LNWP V1 PRM=NEW_MSG VAL="Hello" FROM_NODE="2"')
prm = fields.get("PRM")
```

Helpful hints: if you care about the raw framing kind (`N`, `C`, `Q`, etc.), you’ll get that from `LnwpDoor.poll()` / `LnwpDoor.poll_events()`, not from `parse_lnwp_payload()`.

#### `LnwpFramer`

`LnwpFramer` is the byte-level parser that extracts LNWP frames from a mixed stream.

Frames use a classic DLE/STX … DLE/ETX wrapper. Each frame has a one-byte “kind” prefix (returned as a single-character string) followed by the payload bytes. Inside a frame, a literal DLE byte is escaped by doubling it.

You typically won’t use `LnwpFramer` directly unless you’re building your own supervisor, proxying streams, or experimenting with stdio mode. `LnwpDoor` wraps it in higher-level helpers.

#### `LnwpFramer.feed(data: bytes) -> Tuple[List[LnwpFrame], bytes]`

Feed bytes to the framer and get back `(frames, passthrough)`.

In stdio mode, `passthrough` contains bytes that were not part of LNWP frames (typically terminal bytes). In door32 mode, passthrough is usually irrelevant because terminal bytes arrive on FD3 and LNWP arrives on FD4.

Example: feed a mixed stream in chunks:

```python
from notorious_doorkit.lnwp import LnwpFramer

fr = LnwpFramer()
frames, passthrough = fr.feed(b"hello")
frames2, passthrough2 = fr.feed(b" world")
```

Helpful hints: LNWP framing is designed so you can safely scan a stream and strip frames out without losing non-LNWP bytes. That’s why the framer returns both parsed frames and passthrough.

## Common recipes

### Recipe: “Main Menu” loop

This is the classic door loop: draw a screen, run a menu, act on the selection, then redraw. It’s intentionally simple, because once you have this skeleton, everything else is just “swap in a different screen and a different input primitive”.

```python
from notorious_doorkit import Door, clear_screen, writeln
from notorious_doorkit import LightbarMenu, BLACK, WHITE, BG_BLUE, BG_BRIGHT_CYAN, BOLD

door = Door()

while True:
    clear_screen()
    door.set_activity("Main Menu")
    writeln("MAIN MENU")

    menu = LightbarMenu(
        items=["Play", "Scores", "Exit"],
        x=5, y=5,
        width="auto",
        selected_color=f"{BLACK}{BG_BRIGHT_CYAN}{BOLD}",
        normal_color=f"{WHITE}{BG_BLUE}",
        hotkeys=True,
        hotkey_format="[X]",
        margin_inner=1,
        wrap=True,
    )

    choice = menu.run()
    if choice is None or choice == 2:
        break
```

### Recipe: compact questionnaire form

This shows the “EDITSCREEN” style flow: define a few fields with labels and validation rules, then call `run()` and get back a dictionary when the user saves.

```python
from notorious_doorkit import AdvancedForm, FormField
from notorious_doorkit import WHITE, BG_BLUE, BG_BLACK, BG_RED, BOLD, YELLOW

fields = [
    FormField("name", x=20, y=5, width=34, label="Name", required=True),
    FormField("dob", x=20, y=6, width=8, label="DOB", required=True, format_mask="00/00/00", max_length=6),
    FormField(
        "sex",
        x=20, y=7, width=14, label="Sex", required=True,
        value="N",
        options=[("M", "Male"), ("F", "Female"), ("N", "Nondisclosed")],
    ),
]

form = AdvancedForm(
    fields,
    save_mode="esc_prompt",
    label_color=f"{WHITE}{BG_BLUE}",
    input_color=f"{WHITE}{BG_BLACK}",
    focus_color=f"{WHITE}{BG_RED}{BOLD}",
    required_splash_text="You must fill out required fields",
    required_splash_x=5,
    required_splash_y=22,
    required_splash_color=f"{YELLOW}{BG_BLUE}{BOLD}",
)

result = form.run()
```

## Do’s and don’ts

### Do

- Prefer `goto_xy` + redraw small regions instead of clearing the whole screen repeatedly.
- Hide the cursor when showing menus (the kit already does this for lightbars/forms).
- Use `format_mask` for structured inputs (dates/phones/state/zip) so users can’t type garbage.
- Use `options` for classic “toggle field” behavior instead of forcing typed input.

### Don’t

- Don’t assume every client will render Unicode box drawing the same way. If a client looks weird, switch box style to `"ascii"`.
- Don’t nest multiple raw-mode contexts unnecessarily. Prefer one `RawInput()` loop for a screen.

## Client compatibility notes

Modern clients (SyncTERM, MuffinTerm, NetTerm, Mystic, etc.) generally render the default box styles (`single`, `double`) correctly. If a particular setup shows odd glyphs, use `style="ascii"` as the “always safe” fallback.

## Appendix: Environment Variables

This appendix consolidates environment variables used by DoorKit and NotoriousPTY integration.

### Startup (`Door.start()`)

`Door.start()` resolves the node and dropfile path using:

1. Environment variables (authoritative when present)
2. `argv[1]` and `argv[2]` (classic door convention)

| Variable | Purpose | Notes |
| --- | --- | --- |
| `NOTORIOUS_DOOR_NODE` | Node number | If set, overrides `argv[1]` |
| `NOTORIOUS_DOOR_DROPFILE_PATH` | Full path to the dropfile (e.g., `DOOR.SYS`, `CHAIN.TXT`) | If set, overrides `argv[2]` |

Optional dropfile type override:

| Variable | Purpose | Notes |
| --- | --- | --- |
| `NOTORIOUS_DOOR_DROPFILE_TYPE` | Explicit dropfile parser hint | If set, passed to `load_dropfile(..., drop_type=...)` |
| `NOTORIOUS_DROPFILE_TYPE` | Same as above | Accepted alias |
| `DOOR_DROPFILE_TYPE` | Same as above | Accepted alias |

### Door32 / LNWP integration

These variables are used when running under NotoriousPTY’s Door32 and LNWP integrations.

| Variable | Purpose | Notes |
| --- | --- | --- |
| `NOTORIOUS_DOOR_LNWP_FD` | LNWP control file descriptor | When set to a positive integer, DoorKit enables LNWP helpers and sends/receives LNWP frames on that fd. If unset/0, LNWP helpers are disabled (no-op). Door32 detection is dropfile-driven (`DOOR32.SYS`), independent of LNWP. |
| `NOTORIOUS_DOOR_KEY` | Identifies the current door/session key | Used by `LnwpDoor` (defaults to `"UNKNOWN"`) |
| `NOTORIOUS_DOOR_IO_TRACE` | Enables extra door I/O tracing in the NotoriousPTY supervisor | Primarily used by the Rust runtime; useful for debugging |

### DoorKit config env vars (`NOTORIOUS_DOORKIT_*`)

DoorKit config lookup and precedence:

1. Global config file (if it exists)
2. Door-local config file (if it exists)
3. Environment variable overrides (highest precedence)

Config path variables:

| Variable | Purpose | Notes |
| --- | --- | --- |
| `NOTORIOUS_DOORKIT_CONFIG` | Override global config path | Points to a TOML file |
| `XDG_CONFIG_HOME` | Changes default global config location | Used to derive `$XDG_CONFIG_HOME/notorious_doorkit/config.toml` |
| `NOTORIOUS_DOORKIT_DOOR_CONFIG` | Override door-local config path | If not set, DoorKit looks for `./config.toml` |

Behavior overrides:

| Variable | Purpose | Values |
| --- | --- | --- |
| `NOTORIOUS_DOORKIT_COLOR_DELIMITER` | Color delimiter used by DoorKit text helpers | string (empty disables delimiter sequences) |
| `NOTORIOUS_DOORKIT_COLOR_CHAR` | Optional single-character color prefix | string (empty disables) |
| `NOTORIOUS_DOORKIT_EXPAND_RA_QBBS` | Expand RA/QBBS control codes when rendering text | `1/0`, `true/false`, `yes/no`, `on/off` |
| `NOTORIOUS_DOORKIT_DETECT_CAPABILITIES` | Enable terminal capability probing | `1/0`, `true/false`, `yes/no`, `on/off` |
| `NOTORIOUS_DOORKIT_CAPABILITY_TIMEOUT_MS` | Capability probe timeout | integer ms |
| `NOTORIOUS_DOORKIT_DETECT_WINDOW_SIZE` | Enable window size probing | `1/0`, `true/false`, `yes/no`, `on/off` |
| `NOTORIOUS_DOORKIT_WINDOW_SIZE_TIMEOUT_MS` | Window size probe timeout | integer ms |
| `NOTORIOUS_DOORKIT_CLEAR_ON_ENTRY` | Clear screen on `Door.start()` / `Door.start_local()` | `1/0`, `true/false`, `yes/no`, `on/off` |
