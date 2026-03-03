---
layout: default
title: "Screen Types"
section: "display"
description: "TTY, ANSI, and AVATAR terminal modes — what each supports, how Maximus selects them, and why ANSI is king"
---

Every caller who connects to your board has a terminal type — a set of
capabilities that determines what Maximus can put on their screen. Colors?
Cursor positioning? Box-drawing characters? Blinking? It all depends on
what the caller's terminal supports and what they've told Maximus they can
handle.

Maximus supports three screen types: **TTY**, **ANSI**, and **AVATAR**. In
practice, ANSI dominates the modern BBS world so thoroughly that the other
two exist mainly for completeness and backward compatibility. If you're
setting up a new board today, ANSI is the only mode that matters — but
understanding all three helps when you encounter legacy callers or configure
fallback behavior.

---

## On This Page

- [The Three Screen Types](#screen-types)
- [How Maximus Selects a Screen Type](#selection)
- [ANSI: The Modern Standard](#ansi)
- [TTY: The Fallback](#tty)
- [AVATAR: The Historical Middle Ground](#avatar)
- [Display File Selection](#file-selection)
- [Configuring Terminal Defaults](#configuration)

---

## The Three Screen Types {#screen-types}

| Mode | Colors | Cursor Control | Box Drawing | Blinking | Status |
|------|--------|---------------|-------------|----------|--------|
| **ANSI** | 16 foreground, 8 background | Full positioning | Yes (CP437) | Yes | The standard — use this |
| **AVATAR** | 16 foreground, 8 background | Full positioning | Yes (CP437) | Yes | Legacy; functionally equivalent to ANSI |
| **TTY** | None | None | None | No | Plain text fallback |

Every caller's terminal setting is stored in their user record (`usr.video`)
and can be changed through the `Chg_Video` menu command. New users are
typically prompted for their terminal type during registration.

---

## How Maximus Selects a Screen Type {#selection}

When a caller connects, Maximus checks their stored terminal preference.
This determines how every display file, prompt, and system screen is
rendered for that session:

- **ANSI or AVATAR callers** get the full experience — colors, cursor
  positioning, clear-screen, box-drawing characters, and blinking text.
  MECCA tokens like `[yellow on blue]`, `[locate 5 20]`, and `[cls]` all
  produce the appropriate escape sequences.

- **TTY callers** get a stripped-down version. Maximus automatically removes
  all color codes, cursor positioning, and graphic characters from the
  output. Your display files don't need separate ASCII versions — the same
  compiled `.bbs` file serves everyone. TTY callers see the text content;
  ANSI callers see the full colorized layout.

This automatic stripping is one of MECCA's best features. You design your
screens once with full color and positioning, and Maximus handles the
graceful degradation for callers who can't display it.

---

## ANSI: The Modern Standard {#ansi}

ANSI terminal emulation — specifically, the subset defined by ANSI X3.64 and
popularized by `ANSI.SYS` on DOS — is the lingua franca of the BBS world.
Every modern terminal emulator supports it: SyncTERM, NetRunner, mTelnet,
PuTTY, and even plain `xterm` and macOS Terminal.

When a caller is in ANSI mode, Maximus generates standard ANSI escape
sequences:

- **Colors** — SGR (Select Graphic Rendition) sequences for all 16
  foreground and 8 background colors, plus bold/blink attributes
- **Cursor movement** — CUP (Cursor Position), CUU/CUD/CUF/CUB for
  relative movement, ED (Erase Display), EL (Erase Line)
- **Character set** — CP437 (Code Page 437) box-drawing and graphic
  characters are sent as raw bytes, which ANSI-capable terminals render
  correctly

ANSI is what you should assume your callers are using. Design your display
files for ANSI, test them in an ANSI-capable terminal, and you're set.

### Why ANSI Won

The BBS world standardized on ANSI decades ago, and the modern retro-BBS
community has doubled down on it. SyncTERM — the terminal of choice for most
BBS callers today — is built around ANSI and CP437. ANSI art is a living
creative medium with active communities producing new work. Every tool in
the chain — from TheDraw and PabloDraw for creating art, to Maximus's MECCA
compiler for embedding it in display files — speaks ANSI natively.

There's no reason to use anything else for a new board. ANSI gives you
everything you need: 16 colors, full cursor control, box drawing, and
universal terminal support.

---

## TTY: The Fallback {#tty}

TTY mode is the absolute baseline — plain text with no escape sequences of
any kind. No colors, no cursor positioning, no screen clearing, no
box-drawing characters. Text flows top-to-bottom, left-to-right, and that's
it.

TTY mode exists for callers who genuinely have no terminal emulation — rare
today, but not impossible. Someone connecting through a minimal serial
terminal, an old teletype-style device, or a very restricted SSH client might
need TTY mode.

When Maximus displays a screen to a TTY caller, it strips out:

- All color tokens (`[yellow]`, `[lightcyan on blue]`, pipe color codes)
- Cursor positioning (`[locate]`, `[up]`/`[down]`/`[left]`/`[right]`)
- Screen clearing (`[cls]`, `[cleol]`, `[cleos]`)
- Graphic characters (high-bit CP437 box-drawing characters, if the caller
  also has IBM characters disabled)

What remains is the raw text content of your display files. If you've
designed your screens well, they'll still be readable — just not pretty.

### Designing for TTY Compatibility

You don't need to create separate TTY-specific display files. MECCA's
automatic stripping handles it. But if you want to show something extra to
TTY callers — or hide something that makes no sense without graphics — use
the `[color]`/`[nocolor]` block tokens:

```
[color]
[lightcyan]╔══════════════════════════════╗
[lightcyan]║  [white]Welcome to the Board!      [lightcyan]║
[lightcyan]╚══════════════════════════════╝
[endcolor]
[nocolor]
=== Welcome to the Board! ===
[endcolor]
```

ANSI callers see the box. TTY callers see the simple text header. Same
file, two presentations.

---

## AVATAR: The Historical Middle Ground {#avatar}

AVATAR (Advanced Video Attribute Terminal Assembler and Recreator) was an
alternative to ANSI developed in the late 1980s. It used shorter escape
sequences than ANSI, which mattered when every byte counted over a 2400 bps
modem connection — a two-byte AVATAR color change vs. a five-to-seven-byte
ANSI SGR sequence could add up across a full screen repaint.

Maximus has deep historical roots with AVATAR. The original Maximus was one
of the few BBS packages that supported AVATAR natively, and the internal
bytecode format used by compiled `.bbs` files is actually based on AVATAR
opcodes. When displaying to an ANSI caller, Maximus translates these
internal codes into ANSI sequences on the fly.

### AVATAR in Practice Today

AVATAR is functionally equivalent to ANSI for everything Maximus does — same
16 colors, same cursor control, same box drawing. The only difference is the
wire format of the escape sequences sent to the caller's terminal.

The problem: almost no modern terminal emulator supports AVATAR. SyncTERM
does (it's one of the few), but most other terminals don't recognize AVATAR
sequences and would display garbage. Unless you have a specific reason to
support AVATAR callers — and you almost certainly don't — treat it as a
historical curiosity.

If a caller does select AVATAR mode and their terminal supports it, they'll
see exactly the same screens as ANSI callers. The visual result is identical;
only the underlying protocol differs.

---

## Display File Selection {#file-selection}

When Maximus needs to display a screen, it looks for files in a specific
order based on the caller's terminal type:

1. **RIP callers** — Maximus first checks for a `.rbs` file with the same
   base name. If found, it's displayed instead of the `.bbs` file. (See
   [ANSI Art & RIP Graphics]({% link display-ansi-rip.md %}) for the state
   of RIP support.)

2. **All callers** — The `.bbs` file is displayed. Maximus handles the
   translation internally:
   - ANSI callers receive ANSI escape sequences
   - AVATAR callers receive AVATAR escape sequences
   - TTY callers receive stripped plain text

You don't need separate `.bbs` files for each terminal type. One compiled
file serves everyone. This is by design — MECCA's internal bytecode format
is terminal-agnostic, and the translation to the caller's specific protocol
happens at display time.

---

## Configuring Terminal Defaults {#configuration}

### New User Default

The default terminal type for new users is configured in `maximus.toml`.
Set it to ANSI — that's the right default for a modern board:

```toml
[terminal]
default_video = "ANSI"
```

### Changing Terminal Type

Callers can change their terminal type at any time using the `Chg_Video`
menu command (usually found on the Change Setup menu). This updates their
user record and takes effect immediately.

### Screen Width and Height

Terminal dimensions are stored per-user as well (`usr.width` and `usr.len`).
The defaults are 80×24, which is standard for BBS use. Callers with wider
or taller terminals can adjust these through their user settings.

The `[col80]` MECCA token tests whether the caller's screen is at least 79
columns wide — use it to conditionally show wider content.

---

## See Also

- [Display Files]({% link display-files.md %}) — how display files work and
  where they live
- [MECCA Language]({% link mecca-language.md %}) — the complete token
  reference, including `[color]`/`[nocolor]` blocks
- [ANSI Art & RIP Graphics]({% link display-ansi-rip.md %}) — art creation
  workflows and RIP status
- [Display Codes]({% link display-codes.md %}) — pipe codes for inline color
  and data substitution
