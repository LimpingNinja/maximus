---
layout: default
title: "ANSI Art & RIP Graphics"
section: "display"
description: "Creating ANSI art for your board, converting existing art, CP437 considerations, and the state of RIP graphics support"
---

ANSI art is the visual identity of your BBS. It's what makes a caller's
first connection feel like arriving somewhere — not just reading text, but
stepping into a place with walls and colors and personality. The welcome
screen, the menu frames, the file area headers, the goodbye screen — every
one of these is an opportunity to make your board feel like *yours*.

This page covers the practical workflow of creating and using ANSI art in
Maximus, the tools that make it possible, the CP437 character set that
underlies it all, and the honest state of RIP graphics support.

---

## On This Page

- [ANSI Art Basics](#ansi-basics)
- [How Maximus Finds Display Files](#file-lookup)
- [Creating ANSI Art](#creating)
- [Converting Existing Art](#converting)
- [CP437: The Character Set](#cp437)
- [Embedding Art in Display Files](#embedding)
- [The ANSI-to-MECCA Pipeline](#pipeline)
- [RIP Graphics: An Honest Assessment](#rip)

---

## ANSI Art Basics {#ansi-basics}

ANSI art is text-mode graphics built from colored characters on an 80-column
grid. The "pixels" are characters from the CP437 character set — the
box-drawing characters, block elements, shading characters, and the full
ASCII printable set. Color comes from ANSI escape sequences that set
foreground and background attributes.

The constraints are part of the charm: 16 foreground colors, 8 background
colors, 80 columns, and a fixed-width character grid. Within those
constraints, skilled ANSI artists create everything from simple borders and
headers to elaborate full-screen illustrations.

For BBS use, ANSI art typically serves as:

- **Menu frames** — the visual structure around your menu options
- **Welcome screens** — the first thing callers see when they connect
- **Section headers** — file areas, message areas, bulletins
- **Status panels** — sysop dashboards, node status displays
- **Goodbye screens** — the last impression before disconnect

The art lives in `.mec` source files (or standalone `.ans` files), is
optionally compiled by the MECCA compiler into `.bbs` display files, and is
rendered by Maximus at runtime. But compiled MECCA isn't the only option —
read on.

---

## How Maximus Finds Display Files {#file-lookup}

Before you decide *how* to build your screens, it helps to understand what
Maximus actually does when it needs to display one. When the BBS looks for a
screen — say, `display/screens/welcome` — it doesn't just open one file. It
walks through a search order, trying extensions based on the caller's
terminal type:

| Priority | Extension | Tried When | Format |
|----------|-----------|-----------|--------|
| 1 | `.rbs` | Caller has RIP | Compiled MECCA (RIP bytecodes) |
| 2 | `.gbs` | Caller is ANSI | Compiled MECCA (graphics-specific) |
| 3 | `.bbs` | Always | Compiled MECCA *or* plain text with MCI codes |
| 4 | `.ans` | Caller is ANSI | Raw ANSI — sent as-is |
| 4 | `.avt` | Caller is AVATAR | Raw AVATAR — sent as-is |
| 4 | `.txt` | Caller is TTY | Raw plain text — sent as-is |
| 5 | *(none)* | Always | Bare filename, last resort |

The first file that exists wins. Maximus stops searching as soon as it
finds a match.

A few things jump out from this list:

### `.bbs` Files Don't Have to Be Compiled MECCA

This is a common misconception. A `.bbs` file *can* be compiled MECCA output
(and usually is), but it can also be a plain text file containing
[MCI display codes]({{ site.baseurl }}{% link display-codes.md %}) — pipe color codes like
`|15` and info codes like `|FN` (caller's first name) or `|TL` (time left).
Maximus processes MCI codes in `.bbs` files regardless of whether the file
went through the MECCA compiler.

For simple screens — a short bulletin, a notice, a one-line header — you can
skip MECCA entirely and just write a `.bbs` file with MCI codes in any text
editor. No compilation step, no toolchain fuss.

### Raw `.ans` Files Work Directly

If a `.bbs` or `.gbs` isn't found for an ANSI caller, Maximus looks for a
raw `.ans` file and sends it straight through — escape sequences and all, no
bytecode interpretation. It even detects and strips
[SAUCE](https://www.acid.org/info/sauce/sauce.htm) metadata automatically,
so art downloaded from scene packs or exported from editors works without
post-processing.

This matters because **`ans2mec` doesn't always handle complex art
cleanly.** The converter works well for straightforward ANSI — simple color
changes, basic cursor positioning, standard box-drawing — but it can choke
on dense, optimized art that uses cursor save/restore sequences, partial SGR
resets, or other tricks that skilled ANSI artists rely on. When `ans2mec`
produces a garbled `.mec` file, the compiled `.bbs` looks wrong, and you end
up debugging a toolchain problem instead of running your board.

The pragmatic alternative: **skip the conversion entirely.** Drop the `.ans`
file in your display directory and let Maximus serve it raw. For ANSI callers
it looks pixel-perfect — exactly what the artist intended. Pair it with a
`.txt` file containing a stripped-down plain-text version for TTY callers,
and you've covered everyone without touching the MECCA compiler at all.

For example, to set up a welcome screen with raw art:

```
display/screens/welcome.ans    # Full ANSI art for ANSI callers
display/screens/welcome.txt    # Plain text fallback for TTY callers
```

No `.bbs`, no `.mec`, no compilation. Maximus finds the `.ans` for ANSI
callers and the `.txt` for TTY callers automatically.

The tradeoff: raw `.ans` files can't contain MECCA tokens or MCI codes.
They're static — no `[user]`, no `|TL`, no conditional logic. If you need
dynamic content in your screen, you'll need to go through MECCA (or use a
plain `.bbs` with MCI codes for simpler cases). But for pure art screens —
welcome splash, section dividers, goodbye — raw `.ans` is often the cleanest
path.

### The `.gbs` Override

The `.gbs` extension is tried *before* `.bbs` but *only* for ANSI callers.
This lets you maintain two compiled versions of the same screen: a
full-graphics `.gbs` for ANSI callers and a simpler `.bbs` for everyone
else. In practice, most sysops don't bother with `.gbs` — a single `.bbs`
with `[color]`/`[nocolor]` blocks handles the common case — but it's there
if you want completely different presentations per terminal type.

---

## Creating ANSI Art {#creating}

### Editors

ANSI art is best created in a dedicated ANSI editor — a tool that lets you
draw characters on a grid, set colors visually, and export the result in a
format that Maximus can use.

**Popular editors:**

- **PabloDraw** — Cross-platform (Windows, macOS, Linux via Mono). Modern UI,
  good CP437 support, exports to `.ans` and other formats. The go-to choice
  for most BBS artists today.
- **Moebius** — Cross-platform, web-based and Electron. Clean interface,
  built for the modern ANSI art community.
- **TheDraw** — The classic DOS-era ANSI editor. Runs in DOSBox. If you
  grew up with it, it still works. Not recommended for new users, but it has
  a certain nostalgic appeal.
- **ACiDDraw** — Another DOS classic from the ACiD art scene. Similar to
  TheDraw but with a different workflow.
- **SyncDraw** — A newer option with SyncTERM integration.

All of these export `.ans` files — standard ANSI art files containing raw
ANSI escape sequences and CP437 characters.

### Workflow

The typical flow for creating a display file with ANSI art:

1. **Design your art** in an ANSI editor. Work on an 80-column canvas.
   Keep the height reasonable — 24 lines for a full screen, less for
   headers and frames.

2. **Export as `.ans`** from your editor.

3. **Convert to `.mec`** using the `ans2mec` utility or by hand (see
   [Converting Existing Art](#converting)).

4. **Add MECCA tokens** for dynamic content — embed `[user]`, `[remain]`,
   `[msg_carea]` and other tokens where you want live data to appear.
   Add `[cls]` at the top if it's a full-screen display.

5. **Compile with MECCA** to produce the `.bbs` file:

   ```bash
   bin/mecca display/screens/main_menu.mec
   ```

6. **Test** by connecting to your board and viewing the screen. Check it
   in SyncTERM or another ANSI-capable terminal to see exactly what callers
   will see.

### Design Tips

- **Leave room for dynamic content.** If your menu art has a status bar that
  shows the caller's name and time remaining, leave blank areas in the art
  where MECCA tokens will insert that data.
- **Test at 80×24.** That's the standard BBS screen size. Art that looks
  great at 80×50 will scroll awkwardly on a standard terminal.
- **Use `[cleol]` after colored lines.** If your art has a colored
  background, `[cleol]` fills the rest of the line with that background
  color, preventing ugly gaps.
- **Keep file sizes reasonable.** A full-screen ANSI screen is typically
  2–8 KB as a `.mec` file. If your art is much larger, it may cause noticeable
  delays for callers on slower connections.

---

## Converting Existing Art {#converting}

If you have existing `.ans` files — downloaded from an art pack, exported
from an editor, or inherited from a legacy board — you can convert them to
MECCA source.

### ans2mec / ans2bbs

Maximus includes the `ans2mec` utility (historically called `ans2bbs`) that
reads an `.ans` file containing ANSI escape sequences and produces a `.mec`
source file with equivalent MECCA color and cursor tokens:

```bash
bin/ans2mec artwork/welcome.ans display/screens/welcome.mec
```

The converter translates ANSI SGR color sequences into MECCA color tokens
(`[yellow]`, `[lightcyan on blue]`, etc.) and ANSI cursor sequences into
MECCA cursor tokens (`[locate]`, `[cls]`, `[cleol]`). The resulting `.mec`
file is human-readable and editable — you can open it, add MECCA tokens for
dynamic content, and compile it normally.

> **A word of caution:** `ans2mec` handles straightforward art well, but
> complex or heavily optimized ANSI can trip it up — see
> [Raw `.ans` Files Work Directly](#file-lookup) above. If your converted
> `.mec` doesn't look right, consider serving the `.ans` directly instead of
> fighting the converter.

### Manual Conversion

For small pieces of art — a border, a header line, a divider — you might
find it easier to just embed the ANSI art directly in your `.mec` file. Use
MECCA color tokens instead of raw ANSI escapes:

```
[lightcyan]╔═══════════════════════════════════════╗
[lightcyan]║ [white]Main Menu                           [lightcyan]║
[lightcyan]╠═══════════════════════════════════════╣
```

This is often cleaner than running a conversion tool, especially when the art
is simple enough to type directly.

### Importing from Art Packs

The ANSI art community distributes art in `.ans` files, often bundled in
packs (`.zip` archives with a `FILE_ID.DIZ`). These files are ready to
convert. Download the pack, extract the `.ans` files, run them through
`ans2mec`, add your MECCA tokens, and compile.

Some art packs include SAUCE (Standard Architecture for Universal Comment
Extensions) metadata — title, artist, group, and character set information
embedded at the end of the file. `ans2mec` strips this automatically during
conversion.

---

## CP437: The Character Set {#cp437}

ANSI art is built on **Code Page 437** — the original IBM PC character set.
CP437 includes the standard ASCII printable characters (32–126) plus a rich
set of box-drawing, block, and graphic characters in the upper range
(128–255).

### The Characters That Matter

The most-used CP437 characters for BBS art:

| Range | Characters | Used For |
|-------|-----------|----------|
| 176–178 | `░▒▓` | Shading and gradients |
| 179–218 | `│┤╡╢╖╕╣║╗╝╜╛┐└┴┬├─┼╞╟╚╔╩╦╠═╬` | Box drawing (single and double lines) |
| 219–223 | `█▄▌▐▀` | Block elements (full, half, quarter blocks) |
| 254 | `■` | Small square |

These characters are what give ANSI art its distinctive look — the clean
lines of box-drawing characters for frames and borders, the half-blocks for
pseudo-pixel rendering, and the shading characters for depth and texture.

### Terminal Compatibility

Modern BBS terminals like SyncTERM handle CP437 natively — they render the
raw byte values as the correct glyphs. Standard terminal emulators (xterm,
macOS Terminal, PuTTY) typically default to UTF-8, which means raw CP437
bytes above 127 will display as mojibake unless the terminal is configured
for CP437 or the BBS translates to UTF-8.

Maximus NG includes a CP437-to-UTF-8 translation layer in its local terminal
backend, so the sysop console always renders correctly regardless of the
host system's locale. For remote callers, the translation depends on their
terminal's character set setting — SyncTERM and other BBS-focused terminals
handle this automatically.

### The `[ibmchars]` Token

The MECCA `[ibmchars]` token tests whether the caller has IBM character
support enabled. Use it to conditionally show CP437 art only to callers who
can render it:

```
[ibmchars]╔══════════════╗
[ibmchars]║  Status: OK  ║
[ibmchars]╚══════════════╝
```

Callers without IBM character support see nothing for those lines — which is
fine, because the box-drawing characters would display as garbage on their
terminal anyway.

---

## Embedding Art in Display Files {#embedding}

Once your art is in `.mec` format (either hand-written or converted from
`.ans`), you embed it in a display file alongside MECCA tokens for dynamic
content.

### The Pattern

A typical menu display file looks like this:

```
[cls moreoff]
[lightcyan]╔══════════════════════════════════════════════════════════════════════════════╗
[lightcyan]║ [white]Welcome to [yellow][sys_name][white]!                                                     [lightcyan]║
[lightcyan]╠══════════════════════════════════════════════════════════════════════════════╣
[lightcyan]║                                                                              ║
[lightcyan]║  [white]Hello, [yellow][first][white]! This is your [lightgreen][usercall][white] call.                           [lightcyan]║
[lightcyan]║  [gray]You have [white][remain][gray] minutes remaining.                                      [lightcyan]║
[lightcyan]║                                                                              ║
[lightcyan]╚══════════════════════════════════════════════════════════════════════════════╝
[gray]
```

The art (the box frame) is static. The MECCA tokens inside it (`[sys_name]`,
`[first]`, `[usercall]`, `[remain]`) are replaced with live data at
runtime. The `[moreoff]` at the top prevents Maximus from inserting "More?"
prompts in the middle of your carefully positioned screen.

### Mixing Art and Logic

MECCA tokens work anywhere in the file — you can mix art, conditionals, and
flow control freely:

```
[cls moreoff]
[comment Welcome screen with conditional bulletin display]
[include welcome_header.mec]

[notontoday][lightgreen]
  *** You have new bulletins! ***
[gray]
[filenew]display/misc/bulletin.bbs [link]display/misc/bulletin.bbs

[enter]
```

The `[include]` directive pulls in a shared header (maybe your board's logo).
The `[notontoday]` line only shows for first-time-today callers. The
`[filenew]` line links to the bulletin file only if it's been updated since
the caller's last visit.

---

## The ANSI-to-MECCA Pipeline {#pipeline}

For complex art, the workflow is:

```
ANSI Editor → .ans file → ans2mec → .mec file → add tokens → mecca → .bbs file
```

For simpler screens, you skip the editor and write `.mec` directly:

```
Text editor → .mec file → mecca → .bbs file
```

Either way, the final product is a `.bbs` file in Maximus's display
directory. The BBS loads it fresh each time the screen is displayed, so
changes take effect immediately after recompilation — no restart needed.

---

## RIP Graphics: An Honest Assessment {#rip}

RIP (Remote Imaging Protocol) was an ambitious attempt in the early 1990s to
bring graphical interfaces to BBS systems. Where ANSI art gives you colored
text on a grid, RIP aimed for actual vector graphics — buttons, icons,
mouse-driven menus, scene files. It was backed by TeleGrafix Communications
and supported by a few terminal programs, most notably RIPterm.

Maximus has RIP support baked into its codebase. The MECCA language includes
a full set of RIP-specific tokens: `[rip]`/`[endrip]` for conditional
display, `[ripsend]` and `[ripdisplay]` for scene file management,
`[riphasfile]` for querying the remote terminal, `[rippath]` for managing
RIP asset directories, and `[textsize]` for adjusting the text window in
graphical mode. The MECCA compiler can produce `.rbs` files from `.mer`
source. The runtime includes RIP detection, scene file caching, and
automatic fallback for non-RIP callers.

### The Reality

All of that machinery exists, and none of it has been actively maintained or
tested in Maximus NG. RIP as a protocol is frozen in time — it was a product
of a specific era when graphical BBS interfaces seemed like the future, and
then the actual future turned out to be the World Wide Web.

Here's what you should know:

- **The tokens compile.** MECCA will happily process `[rip]`, `[ripsend]`,
  and friends. The compiler side is working code.
- **The runtime hooks exist.** The display engine has code paths for RIP
  detection, `.rbs` file selection, and scene file transfer.
- **Nobody has tested it recently.** The RIP code paths haven't been
  exercised in any meaningful way during the Maximus NG development cycle.
  There are almost certainly bugs, missing features, and integration gaps.
- **Terminal support is nearly extinct.** RIPterm is abandonware.
  SyncTERM has partial RIP support. That's about it.
- **No modern tooling exists.** There are no actively maintained RIP
  editors or content creation tools.

### If You Want to Explore It

The hooks are there. If you're the kind of person who finds joy in
resurrecting forgotten protocols — and the BBS community has more than a few
of those people — you could potentially get basic RIP graphics working.
You'd need a RIP-capable terminal (SyncTERM in RIP mode), `.rbs` display
files (either hand-crafted or compiled from `.mer` source), and patience
for debugging code paths that haven't been exercised in years.

The MECCA tokens are documented in the
[MECCA Language Reference]({{ site.baseurl }}{% link mecca-language.md %}#rip-graphics) if you
want to dig in.

### For Everyone Else

Use ANSI. It's the standard, it's well-supported, the tooling is excellent,
the art community is active, and every terminal your callers use handles it
perfectly. ANSI art gives your board a visual identity that RIP was designed
to replace but never actually did.

---

## See Also

- [Display Files]({{ site.baseurl }}{% link display-files.md %}) — where display files live and
  how they're loaded
- [MECCA Language]({{ site.baseurl }}{% link mecca-language.md %}) — the complete token
  reference for display files
- [Screen Types]({{ site.baseurl }}{% link display-screen-types.md %}) — TTY, ANSI, and AVATAR
  terminal modes
- [Display Codes]({{ site.baseurl }}{% link display-codes.md %}) — pipe codes for inline color
  and data substitution
- [MECCA Compiler]({{ site.baseurl }}{% link mecca-compiler.md %}) — compiling `.mec` source to
  `.bbs`
