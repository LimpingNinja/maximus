---
layout: default
section: "reference"
title: "Updates & Changelog"
description: "Recent changes and version history"
---

This page tracks what's changed in Maximus NG — both the high-level "what's
different from legacy 3.0" summary and the detailed per-release changelog.

---

## What's Different from Legacy Maximus 3.0

If you're coming from an older Maximus 3.0 installation, here's the short
version of what changed.

### Configuration System

**Legacy (pre-2025):**
- Configuration stored in `.ctl` text files
- Compiled to binary `.prm` and `.dat` files via `silt` utility
- Required recompilation after every config change

**Modern (2025+):**
- Configuration stored in TOML files under `config/`
- No compilation step required
- Direct runtime parsing of TOML
- Legacy `.ctl` files can be converted via
  [maxcfg --export-nextgen]({{ site.baseurl }}{% link maxcfg-cli.md %})

### Removed Components

- **SILT** — Configuration compiler (replaced by TOML runtime parsing)
- **SM** — OS/2 session manager (obsolete)
- **PRM files** — Binary compiled config format (replaced by TOML)

### What Still Works

- All BBS functionality (telnet, local console)
- MEX scripting language (now with JSON and socket intrinsics)
- MECCA display file compiler
- FidoNet message processing (Squish)
- File transfers
- Multi-node operation via MAXTEL

---

## Summary of Changes (Jan 30 – Feb 21, 2026)

A lot has happened since the 4.0 baseline landed. Here's the short version:

**The entire source tree got reorganized.** Code now lives in a clean `src/`
hierarchy, runtime assets in `resources/`, and all the DOS/OS2-era cruft that
nobody's touched in 20 years is gone. The build system was updated to match and
this is the foundation everything else builds on.

**Maximus now speaks TOML for everything.** The old binary language files
(`.mad`/`.ltf`/`.lth`) are retired. All 1,334 display strings live in
`english.toml`, editable with any text editor. There's a full converter for
migrating legacy language packs, a delta overlay system for theme colors, and
positional parameters (`|!1` through `|!F`) replace the old `printf`-style
`%s`/`%d` codes. MEX scripts can query language strings at runtime via new
`lang_get()` intrinsics.

**Lightbar everything.** Menus got a proper lightbar renderer. A classic
single-column, positioned multi-column, and a new `ui_select_prompt` for inline
selections. On top of that, the traditional canned menu system now supports
bounded rendering: you define a rectangle in your menu config and Maximus lays
out options inside it with configurable justification and spacing. File and
message areas got their own dedicated lightbar mode with division drill-in,
hierarchy navigation, configurable highlight styles, and optional custom screen
support.

**MaxCFG keeps getting better.** The config editor gained a full language string
browser/editor with TOML write-back, a shared MCI preview interpreter for
live-previewing display strings, and menu editing improvements for the new
lightbar and bounded layout features.

**New UI primitives — and they're all scriptable.** The lightbar list engine
(`ui_lightbar_list_run`) is a generic paged list with full keyboard nav. Input
fields (`ui_edit_field`) got centralized key decoding, forward-delete, format
masks, and start-mode control. There's a new struct-based form runner with 2D
spatial navigation across fields and required-field validation. Every one of
these primitives is fully exposed to MEX.

**MCI display codes expanded.** New terminal control codes, semantic theme color
stubs (the lowercase pipe namespace — `|tx`, `|pr`, `|hi`, etc.), deferred
parameter expansion (`|#N`) with format-op support, and rendering/attribute
fixes across the board.

**Runtime foundations for 4.0.** SQLite-backed user database, TOML-first
configuration via `libmaxcfg`, and Door32 support with automatic dropfile
generation. MAXTEL gained headless/daemon modes and interactive sysop features.

**Hardened input flows.** Remote login now enforces attempt limits on name,
city, alias, and password entry. Legacy DOS dropfile generators removed in favor
of native C dropfile support.

---

## Fri Feb 21 2026 — Maximus/UNIX 3.04a-r2 [alpha]

*Preparation for MaximusNG 4.0*

**Deferred MCI Params, Input Hardening, MEC Path Cleanup**

### New: Deferred Parameter Expansion (|#N)

- New `|#N` MCI codes for deferred parameter expansion during `MciExpand`
- Format ops (`$L`, `$R`, `$T`, `$C`) now apply to resolved `|#N` values
- `|!N` (early, via `LangVsprintf`) and `|#N` (deferred, via `MciExpand`) can
  be mixed in a single `LangPrintf` call

### Improved: Who-Is-On Display

- WFC/idle nodes shown with dim formatting via new `hu_is_on_4` lang string
- Active user and chat-available counts tracked and displayed
- `hu_is_on_3` uses deferred `|#N` for padded username and node number

### Improved: Input Hardening

- Name, city, alias, and password entry loops enforce a 3-attempt limit for
  remote callers
- Excess attempts trigger disconnect with log entry and `too_many_attempts`
  lang string

### Changed: Version Screen

- Stripped all DOS/OS2 hardware detection code
- Clean MaximusNG about screen with MCI-formatted output

### Fixed: MEC Display File Paths

- All legacy `Misc\` backslash paths updated to `display/screens/`
- `Hlp\mb_help` updated to `display/help/mb_help`
- `[mex]m/oneliner` updated to `[mex]scripts/oneliner`

### Removed: Deprecated MEC Dropfile Generators

- Removed `wwiv.mec`, `callinfo.mec`, `doorsys.mec`, `dorinfo.mec`,
  `outside.mec`
- All superseded by native C dropfile support in `dropfile.c`

### Improved: Lightbar Area Selection

- `ListFileAreas`/`ListMsgAreas` return value now distinguishes selection vs.
  back-out

### Docs & Scripts

- New lightbar areas configuration guide
- Updated MCI codes and display codes documentation
- New `propagate_lang.sh` script for lang file sync across config trees

---

## Tue Feb 18 2026 — Maximus/UNIX 3.04a-r2 [alpha]

*Preparation for MaximusNG 4.0*

**File-Area Lightbar, MaxCFG Hardening, Install Tree Cleanup**

### New: File-Area Lightbar Selection (Phase 1)

- Full interactive lightbar mode for file-area listing and selection
- Division drill-in, area selection, and hierarchy navigation
- Configurable highlight modes: `row`, `full`, `name` via `lightbar_what`
- Per-slot color overrides: `lightbar_fore`, `lightbar_back`
- Optional `custom_screen` display file
- Header/footer rendering at anchor locations

### Improved: Lightbar Primitive

- `ui_lb_draw_list_row()` helper for artifact-free redraws
- Key passthrough mechanism for caller-handled printable keys
- `selected_index_ptr` for external observation of current selection

### MaxCFG Hardening

- Menu parse/edit improvements, footer_file field support
- Language conversion and TOML export fidelity improvements

### Install Tree + Asset Cleanup

- Removed stale assets and obsolete scripts
- Updated logo, menu configs, display_files.toml
- Synced english.toml and delta_english.toml across config trees

---

## Tue Feb 18 2026 (evening) — Message-Area Lightbar (Phase 2)

### New: Message-Area Lightbar Selection (Phase 2)

- Full interactive lightbar mode for message areas, mirroring Phase 1
- Division drill-in, hierarchy navigation, tag state support
- `ListMsgAreas()` returns int with `selected_out`

### Fixed: Lightbar Rendering

- Footer and help text no longer overwritten by lightbar redraws
- Cursor parks below list region on area selection
- Added missing `%b` breadcrumb handler

### Improved: Config and Defaults

- `msg_footer` set to match `file_footer` format
- `reduce_area` lowered from 8 to 5 for both area types
- Config synced across build, resources, and install tree

---

## Sun Feb 16 2026 — TOML Language Delta Architecture + MCI/Theme Polish

### New: Delta Overlay Architecture

- `delta_english.toml` with tiered application:
  - Tier 1 (`@merge`): params metadata — always applied
  - Tier 2 (`[maximusng-*]`): semantic theme color replacements — stock only
- `lang_apply_delta()` with `LangDeltaMode` enum
- CLI flags: `--apply-delta`, `--delta`, `--full/--merge-only/--ng-only`

### New: |!N Positional Parameter Standardization

- All 139 typed suffixes stripped from `english.toml` — bare `|!N` only
- All callers converted to pass pre-formatted strings

### Improved: MCI/Display

- New terminal control codes: `|CL`, `|BS`, `|CR`, `|CD`, `|SA`, `|RA`,
  `|SS`, `|RS`, `|LC`, `|LF`, `|&&`
- Semantic theme color stubs (lowercase pipe namespace)
- Improved control-code handling and preview behavior in maxcfg

---

## Thu Feb 13 2026 — TOML Language System — Full Migration

### New: TOML Language System

- Complete migration from legacy `.mad`/`.ltf`/`.lth` binary pipeline to
  human-editable TOML
- `maxlang_*` API in libmaxcfg for runtime language access
- `english.toml`: 1334 strings, 154 RIP alternates, 437 parameterized strings
- `english.h` compatibility shim mapping ~1300 legacy `#define` macros

### New: lang_convert.c (MaxCFG)

- Full `.mad` → TOML converter with MAID preprocessing
- AVATAR → MCI color/cursor conversion
- Delta overlay support
- CLI: `--convert-lang`, `--convert-lang-all`

### New: LangPrintf / LangSprintf

- `|!N` positional parameter expansion in `MciExpand()`
- 31+ `Printf` → `LangPrintf` and ~37 `sprintf` → `LangSprintf` migrations

### New: Lightbar Prompt Support

- Lightbar path in `GetListAnswer` for graphical terminals
- Rich bracket format and compact single-char format parsing

### New: Shared MCI Preview Interpreter

- `mci_preview.c/h`: full MCI expansion into virtual screen buffers
- Generic `MciVScreen` abstraction used by language editor and menu preview

### New: MaxCFG Language Editor

- String browser with per-column coloring
- String editor with TOML write-back

### New: MEX Language Intrinsics

- `lang_get()`, `lang_get_rip()`, `lang_register()`, `lang_unregister()`,
  `lang_load_extension()`
- Full TOML language access from MEX scripts

---

## Sat Feb 8 2026 — Bounded Canned Menus + Lightbar Layout Controls

### New: Bounded Canned Menu Rendering

- `ShowMenuCannedBounded()` driven by `[custom_menu]` bounds
- Positions options inside a configured rectangle

### New: Layout Controls ([custom_menu])

- `option_spacing` and `option_justify` (left/center/right)
- `boundary_justify`, `boundary_layout` (grid, tight, spread)
- `title_location` and `prompt_location` for bounded menus

---

## Fri Feb 7 2026 — MaxUI Field/Input + Forms + Lightbar Menus

### New: Lightbar Menus

- Classic lightbar menus with full keyboard navigation
- Positioned/multi-column lightbar layouts
- `ui_select_prompt` for inline lightbar selection prompts

### New: MaxUI Form Runner

- Struct-based form system with spatial (2D) navigation
- Required-field validation

### Improved: Field Editing + Key Handling

- Centralized UI key decoding
- Forward-delete support in `ui_edit_field`

---

## Thu Jan 30 2026 — MaximusNG 4.0 [baseline]

**4.0 Baseline: Source Reorganization + Runtime Foundations**

### Source Tree Reorganization (Phase 3)

- Code moved into structured `src/` hierarchy
- Runtime content moved into `resources/`
- Build/install/release scripts updated for new layout

### New: Runtime Foundations

- **SQLite user database** — `libmaxdb` + `UserFile*` shim
- **TOML-first runtime config** — `libmaxcfg` + `ngcfg_get_*` accessors
- **Door support** — automatic dropfile generation + `Door32_Run` plumbing

### Improved: MAXTEL

- Headless (`-H`) and daemon (`-D`) modes confirmed
- Interactive sysop features: snoop overlay, chat break-in
- Shell/config workflow (`C` key to launch maxcfg and return)

---

## Sat Nov 30 2025 — MAXTEL + Linux Support Release

### New: Linux x86_64 Support

- Full Linux build support alongside macOS (arm64/x86_64)
- GCC compatibility fixes for stricter type checking
- Tested on Debian/Ubuntu x86_64

### New: MAXTEL Telnet Supervisor

- Multi-node telnet supervisor with real-time ncurses UI
- Manages 1-32 simultaneous BBS nodes
- Responsive layouts: Compact (80x25), Medium (100x30), Full (132x60+)
- Headless mode (`-H`) and daemon mode (`-D`)

---

## Thu Nov 27 2025 — Modern macOS/Darwin Revival Release

### Major Changes

- Full macOS (Darwin) support for arm64 and x86_64 architectures
- MEX VM fixed for 64-bit systems (struct alignment, pointer handling)
- Removed vestigial anti-tampering checks
- Added `MAX_INSTALL_PATH` environment variable for portable installs
- Cross-architecture build support (arm64/x86_64 on Apple Silicon)
- Release packaging scripts

---

## Historical Releases

### Wed Jun 11 2003 — Maximus/UNIX 3.03b [pre-release]

- Big-endian platform support improvements (Sparc)
- Configure script and Makefile overhaul
- FreeBSD support (Andrew Clarke patches)
- Keyboard/video driver responsiveness fixes

### Fri Jun 6 2003 — Maximus/UNIX 3.03b [pre-release]

- 7-bit NVT (telnet) capability with multi-user support
- FreeBSD & Linux build fixes (Andrew Clarke, Bo Simonsen)
- Squish bases now binary-compatible with other software
- Nagle algorithm support in COMMAPI
- Solaris 8/SPARC build support

### Sat May 24 2003 — Maximus/UNIX 3.03a

**Initial Public Release.** Bumped version number to 3.03 to avoid confusion.

### Tue May 13 2003 — Maximus/UNIX 3.02

**Initial Release.** Released so Tony Summerfelt can play with Squish.

---

## See Also

- [Building Maximus]({{ site.baseurl }}{% link building.md %}) — compiling from source
- [Directory Structure]({{ site.baseurl }}{% link directory-structure.md %}) — runtime layout
- [Display Codes]({{ site.baseurl }}{% link display-codes.md %}) — the MCI display code system
- [Language Files (TOML)]({{ site.baseurl }}{% link lang-toml.md %}) — the TOML language system
