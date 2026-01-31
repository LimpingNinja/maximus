# Notorious DoorKit (`notorious_doorkit`)

DoorKit is a small, pragmatic Python library for writing classic BBS doors with a modern dev loop.
It’s designed to feel like “OpenDoors-style terminal programming”: draw a screen, read a key, update state, repeat.

If you’re running under NotoriousPTY, you can optionally use LNWP (the control plane) for things like activity strings and inter-node messages.
If you’re not, DoorKit still gives you solid primitives (ANSI/CP437 output, raw-ish input parsing, menus, forms).

## What you get

- **Terminal output primitives**
  - CP437-friendly `write()` / `writeln()`
  - Colors + cursor control
- **Input primitives**
  - `RawInput` for key-by-key reads (including arrow keys)
- **Classic UI building blocks**
  - Lightbars, forms/editors, file display helpers
- **Session + dropfile parsing**
  - `Door.start()` loads a dropfile and populates `door.session`
- **Optional LNWP integration**
  - LNWP helpers are available on `Door` but are only active when LNWP is explicitly enabled (see below)

## Getting started

### 1) Vendor DoorKit as a local package

This repo doesn’t currently ship DoorKit as an installable PyPI package.
The normal “door author” flow is to copy the `notorious_doorkit/` folder into your door project so it’s importable as a local package.

Example layout:

```text
mydoor/
  mydoor.py
  notorious_doorkit/
    __init__.py
    door.py
    ...
```

Minimal import:

```python
from notorious_doorkit import Door, clear_screen, writeln
```

### 2) Start a session (dropfile-driven)

Use `Door` when you only need session state + terminal probing (no LNWP).

```python
from notorious_doorkit import Door, clear_screen, writeln, get_username

door = Door()
session = door.start()

clear_screen()
writeln(f"Hello, {get_username()}")
```

### 3) If you want NotoriousPTY features, enable LNWP

DoorKit’s LNWP helpers are additive and gated. To enable LNWP, provide a positive fd via `NOTORIOUS_DOOR_LNWP_FD` or `--lnwp <fd>`.

```python
from notorious_doorkit import clear_screen, writeln

door = Door()
door.door_key = "MYDOOR"
session = door.start()

door.arm()
door.set_activity("In My Door")

clear_screen()
writeln("Welcome!")
```

## Configuration (TOML)

DoorKit reads config from TOML and from environment variable overrides.
You can copy `notorious_doorkit/config.sample.toml` as a starting point.

### Lookup & precedence

DoorKit loads config in this order:

1) **Global config** (if it exists)
   - `NOTORIOUS_DOORKIT_CONFIG=/path/to/config.toml`, else
   - `$XDG_CONFIG_HOME/notorious_doorkit/config.toml`, else
   - `~/.config/notorious_doorkit/config.toml`

2) **Door-local config** (if it exists)
   - `NOTORIOUS_DOORKIT_DOOR_CONFIG=/path/to/config.toml`, else
   - `./config.toml` (current working directory)

Note: `Door.start()` changes the working directory to your door script directory (OpenDoors-style). In practice that means `./config.toml` is usually “next to your door script”.

3) **Env var overrides** (highest precedence)
   - See `notorious_doorkit/config.sample.toml` for the full list.

### The “good defaults” approach

By default, terminal probing is **off** (`detect_capabilities=false`, `detect_window_size=false`).
This keeps I/O conservative and lets you lean on dropfile/BBS-provided values unless you explicitly opt in.

## A tiny interactive loop (classic door shape)

```python
from notorious_doorkit import clear_screen, writeln, KEY_ESC, get_input

clear_screen()
writeln("Press keys (ESC to exit)")

while True:
    ev = get_input()
    if ev is None:
        continue
    if ev.event_type == "EVENT_CHARACTER" and chr(int(ev.key_press) & 0xFF) == KEY_ESC:
        break
    writeln(f"EVENT={ev.event_type} KEY={ev.key_press!r}")
```

## Where to go next

- **Manual**: `notorious_doorkit/notorious-doorkit-manual.md`
- **LNWP protocol reference**: `docs/LNWP_v1_Spec.md`
- **NotoriousPTY door configuration** (runtime/supervisor side): `docs/door_configuration.md`
- **Example door(s)**: see `doors/` (notably `doors/testdoor/testdoor.py`)

## License

Part of the NotoriousPTY project.
