# Maximus-CBCS Change Log

---

## Summary of Changes (Jan 30 – Feb 21, 2026)

A lot has happened since the 4.0 baseline landed. Here's the short version:

**The entire source tree got reorganized.** Code now lives in a clean `src/` hierarchy, runtime assets in `resources/`, and all the DOS/OS2-era cruft that nobody's touched in 20 years is gone. The build system was updated to match and this is the foundation everything else builds on.

**Maximus now speaks TOML for everything.** The old binary language files (`.mad`/`.ltf`/`.lth`) are retired. All 1,334 display strings live in `english.toml`, editable with any text editor. There's a full converter for migrating legacy language packs, a delta overlay system for theme colors, and positional parameters (`|!1` through `|!F`) replace the old `printf`-style `%s`/`%d` codes. MEX scripts can query language strings at runtime via new `lang_get()` intrinsics.

**Lightbar everything.** Menus got a proper lightbar renderer. A classic single-column, positioned multi-column, and a new `ui_select_prompt` for inline selections. On top of that, the traditional canned menu system now supports bounded rendering: you define a rectangle in your menu config and Maximus lays out options inside it with configurable justification and spacing. File and message areas got their own dedicated lightbar mode with division drill-in, hierarchy navigation, configurable highlight styles, and optional custom screen support.

**MaxCFG keeps getting better.** The config editor gained a full language string browser/editor with TOML write-back, a shared MCI preview interpreter for live-previewing display strings, and menu editing improvements for the new lightbar and bounded layout features.

**New UI primitives - and they're all scriptable.** The lightbar list engine (`ui_lightbar_list_run`) is a generic paged list with full keyboard nav (arrows, PgUp/PgDn, Home/End, Enter, ESC) and a key-passthrough mechanism so callers can layer domain-specific behavior on top without touching the primitive. Lightbar prompts (`ui_select_prompt`) give you inline single-key selection with highlight bar navigation that us used everywhere from Yes/No/Cancel to multi-option menus. Input fields (`ui_edit_field`) got centralized key decoding, forward-delete, format masks, and start-mode control (append vs. overwrite vs. cursor-at-end). On top of all that, there's a new struct-based form runner with 2D spatial navigation across fields, cursor show/hide tied to edit state, and required-field validation. Every one of these primitives is fully exposed to MEX via intrinsics, so sysops can build custom interactive screens entirely from script.

**MCI display codes expanded.** New terminal control codes, semantic theme color stubs (the lowercase pipe namespace — `|tx`, `|pr`, `|hi`, etc.), deferred parameter expansion (`|#N`) with format-op support, and a bunch of rendering/attribute fixes across the board.

**Runtime foundations for 4.0.** SQLite-backed user database, TOML-first configuration via `libmaxcfg`, and Door32 support with automatic dropfile generation. MAXTEL gained headless/daemon modes and interactive sysop features (snoop overlay, chat break-in, shell-out to maxcfg).

**Hardened input flows.** Remote login now enforces attempt limits on name, city, alias, and password entry — excess failures disconnect the caller. Legacy DOS dropfile generators (WWIV chain.txt, CallInfo, door.sys, dorinfo) removed in favor of native C dropfile support. All MEC display files cleaned up for Unix path compatibility.

Full details for each change are in the dated entries below.

---

## Fri Feb 21 2026 - Maximus/UNIX 3.04a-r2 [alpha]

*Preparation for MaximusNG 4.0*

**Deferred MCI Params, Input Hardening, MEC Path Cleanup**  
Maintainer: Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

### New: Deferred Parameter Expansion (|#N)

- New `|#N` MCI codes for deferred parameter expansion during `MciExpand`
- Format ops (`$L`, `$R`, `$T`, `$C`) now apply to resolved `|#N` values (e.g., `$L04|#2` pads to width 4)
- `|!N` (early, via `LangVsprintf`) and `|#N` (deferred, via `MciExpand`) can be mixed in a single `LangPrintf` call
- `LangPrintf` binds params via `va_copy` to `g_lang_params` for deferred resolution

### Improved: Who-Is-On Display

- WFC/idle nodes now shown with dim formatting via new `hu_is_on_4` lang string
- Active user and chat-available counts tracked and displayed
- `hu_is_on_3` uses deferred `|#N` for padded username and node number

### Improved: Input Hardening

- Name, city, alias, and password entry loops enforce a 3-attempt limit for remote callers
- Excess attempts trigger disconnect with log entry and `too_many_attempts` lang string

### Changed: Version Screen

- Stripped all DOS/OS2 hardware detection code (PC/XT/AT/PS2/Convertible)
- Clean MaximusNG about screen with MCI-formatted output

### Fixed: MEC Display File Paths

- All legacy `Misc\` backslash paths updated to `display/screens/` (bulletin, bul_hdr, bul_next, mailchek, rookie, welcome, newuser2)
- `Hlp\mb_help` updated to `display/help/mb_help` in mexbank.mec/.mer
- `[mex]m/oneliner` updated to `[mex]scripts/oneliner` in welcome.mec, newuser2.mec

### Removed: Deprecated MEC Dropfile Generators

- Removed `wwiv.mec`, `callinfo.mec`, `doorsys.mec`, `dorinfo.mec`, `outside.mec`
- All superseded by native C dropfile support in `dropfile.c` (door.sys, door32, chain.txt)

### Improved: Lightbar Area Selection

- `ListFileAreas`/`ListMsgAreas` return value now distinguishes selection vs. back-out (positive = selected, negative = escape)

### Changed: MAXTEL Popup Handling

- Popup overlay state managed by main loop with auto-dismiss timer instead of inline ncurses window creation

### Docs & Scripts

- New `using-lightbar-areas.md` - full lightbar configuration guide with recipes, troubleshooting, and format token reference
- Updated `mci-codes.md`, `mci-feature-spec.md`, `using-display-codes.md`
- New `propagate_lang.sh` script for lang file sync across config trees

---

## Tue Feb 18 2026 - Maximus/UNIX 3.04a-r2 [alpha]

*Preparation for MaximusNG 4.0*

**File-Area Lightbar, MaxCFG Hardening, Install Tree Cleanup**  
Maintainer: Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

### New: File-Area Lightbar Selection (Phase 1)

- Full interactive lightbar mode for file-area listing and selection
- `lb_file_area_interact()` with division drill-in, area selection, and hierarchy navigation
- Keyboard semantics: `/` (jump to root), `.` (go up one level), `Q` (quit), `ESC` (back/exit)
- Configurable highlight modes: `row`, `full`, `name` via `lightbar_what`
- Per-slot color overrides: `lightbar_fore`, `lightbar_back` (named colors or hex nibble)
- Boundary resolution with `top_boundary`/`bottom_boundary`, `reduce_area` fallback, safety clamping
- Optional `custom_screen` display file resolved against `display_path`; suppresses header/footer/help when shown
- Header/footer render at anchor locations (`header_location`/`footer_location`) when configured, inline otherwise
- `ListFileAreas()` now accepts `selected_out` for lightbar-driven area changes
- `FileAreaMenu()` wired to accept lightbar selections and sync division context
- Test mode (`LB_FAREA_TEST`) with synthetic division/area data for development

### Improved: Lightbar Primitive

- `ui_lb_draw_list_row()` helper: blanks full row width before redraw to eliminate stale right-side artifacts
- Key passthrough: `out_key` field and `LB_LIST_KEY_PASSTHROUGH` return code for caller-handled printable keys
- `selected_index_ptr` for external observation of current selection

### MaxCFG Hardening

- Menu parse/edit improvements: `footer_file` field support, improved type handling
- Language conversion (`lang_convert.c`): extended output handling and formatting
- TOML export fidelity improvements in `nextgen_export.c`

### Install Tree + Asset Cleanup

- Removed stale `max_main.ans`, `headfile.mex`, and obsolete Python scripts
- Updated `logo.ans`, menu configs (`file.toml`, `main.toml`), `display_files.toml`
- Added `resources/config/` as canonical base config mirror
- Added `footerfile.mex` and `compile-mex.sh`
- Synced `english.toml` and `delta_english.toml` across `resources/lang` and install tree

---

## Tue Feb 18 2026 (evening) - Maximus/UNIX 3.04a-r2 [alpha]

*Preparation for MaximusNG 4.0*

**Message-Area Lightbar (Phase 2), Rendering Fixes, and Config Sync**  
Maintainer: Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

### New: Message-Area Lightbar Selection (Phase 2)

- Full interactive lightbar mode for message areas, mirroring Phase 1 file-area lightbar
- Division drill-in, hierarchy navigation, tag state via `TagQueryTagList()`, and `LB_MAREA_TEST` synthetic data
- `ListMsgAreas()` returns int with `selected_out`; `MsgAreaMenu()` and `ChangeToArea()` wired for lightbar dispatch

### Fixed: Lightbar Rendering

- Footer and `achg_lb_help` were rendered at list-top row then overwritten by the lightbar — fixed by parking cursor at `ly + lh` before inline footer/help in both `f_area.c` and `m_area.c`
- Cursor now parks below list region on area selection (not just on cancel/ESC)
- Added missing `%b` breadcrumb handler to `ParseCustomMsgAreaList` in `max_cust.c`

### Improved: Config and Defaults

- `msg_footer` set to `"%x0a|prLocation: |hi%b|07%x0a"` matching `file_footer`
- `reduce_area` lowered from 8 to 5 for both `[file_areas]` and `[msg_areas]`
- Config synced across `build/`, `resources/config/`, and `resources/install_tree/`
- `lightbar_msg_areas.md` updated from proposal to implemented-behavior doc

---

## Sun Feb 16 2026 - Maximus/UNIX 3.04a-r2 [alpha]

*Preparation for MaximusNG 4.0*

**TOML Language Delta Architecture + MCI/Theme Polish**  
Maintainer: Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

### New: Delta Overlay Architecture

- `delta_english.toml` with tiered application:
  - Tier 1 (`@merge`): params metadata (name, type, desc, default) — always applied
  - Tier 2 (`[maximusng-*]`): semantic theme color replacements per heap — stock only
- `lang_apply_delta()` with `LangDeltaMode` enum (`FULL` / `MERGE_ONLY` / `NG_ONLY`)
- CLI flags: `--apply-delta`, `--delta`, `--full/--merge-only/--ng-only`
- Enriched params display in maxcfg language editor (name, type, desc from raw TOML)

### New: `|!N` Positional Parameter Standardization

- All 139 typed suffixes (`|!Nd`, `|!Nl`, `|!Nu`, `|!Nc`) stripped from `english.toml` — bare `|!N` only
- All callers converted to pass pre-formatted `const char *` strings
- `LangVsprintf` simplified: always `va_arg(va, const char *)` for bare codes

### Improved: MCI/Display

- New terminal control codes: `|CL`, `|BS`, `|CR`, `|CD`, `|SA`, `|RA`, `|SS`, `|RS`, `|LC`, `|LF`, `|&&`
- Semantic theme color stubs (lowercase pipe namespace)
- Improved control-code handling and preview behavior in maxcfg
- Color/theme defaults adjusted for cleaner resets and consistent attributes

### Improved: Menu/Runtime UX

- Lightbar redraw cleanup, prompt/header behavior tuning
- FILE menu header script positioning for better re-entry rendering
- Protocol/file/msg/core targeted formatting and stability fixes

### Documentation

- `using-display-codes.md`: public sysop wiki for color codes, info codes, formatting, terminal control, positional params
- Updated `mci-codes.md` and `mci-feature-spec.md`
- Footer-file feature spec draft for upcoming menu footer support

---

## Thu Feb 13 2026 - Maximus/UNIX 3.04a-r2 [alpha]

*Preparation for MaximusNG 4.0*

**TOML Language System — Full Migration from Legacy Binary**  
Maintainer: Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

### New: TOML Language System

- Complete migration from legacy `.mad`/`.ltf`/`.lth` binary pipeline to human-editable TOML
- `maxlang_*` API in libmaxcfg: `maxlang_open/close`, `maxlang_get` (dotted key), `maxlang_get_by_id` (legacy numeric), `maxlang_set_use_rip`
- `english.toml`: 1334 strings, 154 RIP alternates, 437 parameterized strings with enriched metadata
- `s_ret()`/`s_reth()` reimplemented to delegate through `maxlang_get_by_id()` with `.ltf` fallback
- `english.h` compatibility shim mapping ~1300 legacy `#define` macros to `maxlang_get()` calls
- Removed from build: MAID, `.mad`/`.ltf`/`.lth`, `rdlang.c`, `language.mh`, `colors.lh`, `rip.lh`, `user.lh`

### New: `lang_convert.c` (MaxCFG)

- Full `.mad` → TOML converter with MAID preprocessing, AVATAR→MCI color/cursor conversion
- `lc_printf_to_positional()`: `%s`/`%d`/`%c`/`%u`/`%lu`/`%ld` → `|!1`–`|!F` with MCI format ops
- Delta overlay support: delta keys replace existing TOML keys; new keys inserted before `[_legacy_map]`
- CLI: `--convert-lang`, `--convert-lang-all`; TUI: Tools → Convert Legacy Language

### New: `LangPrintf` / `LangSprintf`

- `|!N` positional parameter expansion in `MciExpand()`
- `LangPrintf()`: dual path — `|!N` substitution for TOML, `%`-coercion fallback for legacy
- 31+ `Printf` → `LangPrintf` and ~37 `sprintf` → `LangSprintf` call-site migrations

### New: Lightbar Prompt Support

- Lightbar path in `GetListAnswer` for graphical terminals with no stacked input
- Rich `[X]` bracket format and compact single-char format parsing into `ui_select_prompt`
- `default_index` support via `UI_SP_DEFAULT_SHIFT` flag packing
- TTY fallback: extract hotkey chars from rich format into compact string

### New: Shared MCI Preview Interpreter

- `mci_preview.c/h`: full MCI expansion into virtual screen buffers
- Supports pipe colors, `$D`/`$X`/`$L`/`$R`/`T`/`$C` format ops, `|!N` params, `|XY` info codes, cursor codes, AVATAR attributes
- Generic `MciVScreen` abstraction used by language editor and menu preview
- Refactored `le_preview` to delegate to shared module (~500 lines removed)

### New: MaxCFG Language Editor

- String browser with per-column coloring (heap=yellow, symbol=bold white, text=grey)
- String editor with TOML write-back (`lb_edit_entry`, `lb_write_back`)
- Selected row retains uniform highlight

### New: MEX Language Intrinsics

- `lang_get()`, `lang_get_rip()`, `lang_register()`, `lang_unregister()`, `lang_load_extension()`
- Full TOML language access from MEX scripts
- Migrated MEX scripts from `language.mh` → `lang_get()`: stats, chgdob, chgsex, forcepwd, headchat, headchg, headfile, headmsg, logo, mexbank, mexchat

### Bug Fixes

- `max_out.c` / `LangVsprintf`: fixed `|!N` type suffix bug — `va_arg` read 4 bytes of 8-byte `char*` pointers producing garbled output
- `mexlang.c` / `intrin_lang_get`: fixed by-ref/by-value mismatch causing MEX VM crashes

### Install Tree Cleanup

- Removed duplicated `m/` (MEX sources canonical in `resources/m/`)
- Removed legacy `etc/rip/`, `route.cfg`, `squish.cfg`, `tunes.bbs`
- Removed legacy `man/` docs (`max.doc`, `upgrade.txt`, `whatsnew.300`)
- New structure: `config/`, `data/`, `display/`, `scripts/`

---

## Sat Feb 8 2026 - Maximus/UNIX 3.04a-r2 [alpha]

*Preparation for MaximusNG 4.0*

**Bounded Canned Menus + Lightbar Layout Controls**  
Maintainer: Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

### New: Bounded Canned Menu Rendering

- `ShowMenuCannedBounded()` driven by `[custom_menu]` bounds
- Positions options inside a configured rectangle using `Goto(row,col)`
- `ShowMenuBody()` updated: display menu file first, respect `skip_canned_menu`, fall back to unbounded when bounds invalid
- Bounded positioning for NOVICE mode only (REGULAR/EXPERT remain sequential/unbounded)

### New: Layout Controls (`[custom_menu]`)

- `option_spacing` and `option_justify` (left/center/right)
- `boundary_justify`: 1 or 2 tokens (horizontal + vertical), e.g. `"center"` or `"left top"`
- `boundary_layout`: `grid`, `tight`, `spread` (full), `spread_width`, `spread_height`
- `title_location` and `prompt_location` for bounded menus

### Improved: MaxCFG Menu Editor

- Lightbar UI and customization support for menu editing
- Margin updates for lightbar rendering

### Bug Fixes

- Fixed bounded NOVICE prompt crash from incorrect `Printf()` argument counts
- Fixed option justification so hotkey + label justify as a single unit, preserving highlight colors
- Applied vertical boundary justification for non-spread-height layouts

---

## Fri Feb 7 2026 - Maximus/UNIX 3.04a-r2 [alpha]

*Preparation for MaximusNG 4.0*

**MaxUI Field/Input + Forms + Lightbar Menus**  
Maintainer: Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

### New: Lightbar Menus

- Classic lightbar menus with full keyboard navigation
- Positioned/multi-column lightbar layouts
- `ui_select_prompt` for inline lightbar selection prompts

### New: MaxUI Form Runner

- New struct-based form system with spatial (2D) navigation and field editing
- Cursor behavior aligned with other UI widgets:
  - Cursor hidden during form navigation
  - Cursor shown while actively editing a field
- Required-field validation with a simple splash message on save

### Improved: Field Editing + Key Handling

- Centralized UI key decoding via shared helper (`ui_read_key`) so ESC-sequences behave consistently across UI widgets
- Forward-delete support (Delete key / `ESC[3~`) in `ui_edit_field`:
  - Works for both plain fields and `format_mask` fields
- Documentation updated for these behaviors, including deferred "missing functionality" notes for more advanced ESC parsing

### MEX + Documentation

- All new/updated UI features are exposed to MEX (including `ui_form_run`)
- `uitest.mex` expanded with interactive coverage for prompt start modes and format-mask cancel/edit flows

---

## Thu Jan 30 2026 - MaximusNG 4.0 [baseline]

**4.0 Baseline: Source Reorganization + Runtime Foundations**  
Maintainer: Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

This is a consolidation checkpoint (layout + plumbing + docs) returning to 1-commit-per-feature going forward.

### Source Tree Reorganization (Phase 3)

- Code moved into structured `src/` hierarchy (`src/max`, `src/libs`, `src/apps`, `src/utils`)
- Runtime content moved into `resources/` (`resources/install_tree`, `resources/lang`, `resources/m`)
- Build/install/release scripts updated for new layout
- Large delete count is primarily file moves/renames, not actual removals

### Obsolete/Legacy Code Retirement

- Removed unmaintained DOS/OS2-era installer/UI, OS/2 session manager, swap systems
- Removed legacy mail processors (sqafix, squish), legacy bindings/test harness cruft

### New: Runtime Foundations

- **SQLite user database**: `libmaxdb` + `UserFile*` shim enabling SQLite-backed user database
- **TOML-first runtime config**: `libmaxcfg` + `ngcfg_get_*` accessors with legacy fallback
- **Door support**: automatic dropfile generation + `Door32_Run` menu token plumbing

### Improved: MAXTEL

- Headless (`-H`) and daemon (`-D`) modes confirmed
- Interactive sysop features: snoop overlay (`F1` exit, `F2` chat break-in)
- Shell/config workflow (`C` key to launch maxcfg and return)

### Documentation

- Updated `README.md`, `BUILD.md`, `STRUCTURE.md`, and MAXTEL docs for new layout
- Documented runtime config base (`config/maximus`) and behavioral caveats

---

## Sat Nov 30 2025 - Maximus/UNIX 3.04a-r2 [alpha]

**MAXTEL + Linux Support Release**  
Maintainer: Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

### New: Linux x86_64 Support

- **Full Linux build support** alongside macOS (arm64/x86_64)
- GCC compatibility fixes for stricter type checking
- POSIX `spawnlp()` implementation for process spawning
- Tested on Debian/Ubuntu x86_64 (OrbStack, native)

**Linux-Specific Changes:**
- Added missing function declarations for GCC strictness
- Explicit pointer casts for `byte*`/`char*` type mismatches
- Added `<ctype.h>` includes where needed
- Fixed linker dependencies for shared libraries

**Platform Support:**
- macOS arm64 (Apple Silicon) - native
- macOS x86_64 (Intel/Rosetta) - native
- Linux x86_64 - native
- Linux arm64 - untested but should work

### New: MAXTEL Telnet Supervisor

- **Multi-node telnet supervisor** with real-time ncurses UI
- Manages 1-32 simultaneous BBS nodes
- Automatic node assignment for incoming callers
- Telnet protocol negotiation (terminal type, ANSI detection)

**UI Features:**
- Responsive layouts: Compact (80x25), Medium (100x30), Full (132x60+)
- Real-time node status, user info, and caller history panels
- Keyboard controls for node management (kick, restart)
- Dynamic terminal resize handling (SIGWINCH)

**Operation Modes:**
- Interactive mode with full ncurses interface
- Headless mode (`-H`) for scripts and process supervisors
- Daemon mode (`-D`) for background/startup operation

**Documentation:**
- Complete user manual: `docs/maxtel.txt` and `docs/maxtel.md`

### Build System Updates

- MAXTEL included in release packages
- Linux release packaging support in make-release.sh
- Release README updated with MAXTEL quick-start instructions
- Documentation (.txt and .md) included in release archives
- See `code-docs/LINUX_PORT_CHANGES.md` for detailed porting notes

---

## Thu Nov 27 2025 - Maximus/UNIX 3.04a [alpha]

**Modern macOS/Darwin Revival Release**  
Maintainer: Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja  
Current Status: Operational on macOS arm64, builds on x86_64

### Major Changes

- Full macOS (Darwin) support for arm64 and x86_64 architectures
- MEX VM fixed for 64-bit systems (struct alignment, pointer handling)
- Removed vestigial anti-tampering checks (VER_CHECKSUM, KEY system)
- Added MAX_INSTALL_PATH environment variable for portable installs
- Version now includes VER_SUFFIX for consistent release tagging
- Platform identification (/OSX, /LINUX) in version strings

### Build System

- Cross-architecture build support (arm64/x86_64 on Apple Silicon)
- Release packaging scripts (make-release.sh, make-all-releases.sh)
- Version extracted from max/max_vr.h for releases

### MEX Compiler/VM Fixes

- Fixed 64-bit pointer alignment in _usrfunc struct
- Added NULL terminator to intrinsic function table
- Fixed IADDR struct packing for proper string handling
- Documented parameter scoping bug in MEX compiler

### Tested Functionality

- Local console mode (max -c) login and operation
- MEX scripts: card.mex, dorinfo.mex, stats.mex, userlist.mex
- Tools: maid, silt, mecca, mex compiler
- User data structures readable and writable

### Known Issues

- Stability not fully validated across all features
- Some MEX scripts may need parameter renaming (scoping bug)

---

## Wed Jun 11 2003 - Maximus/UNIX 3.03b [pre-release]

- More changes to allow support for Sparc and other BIG_ENDIAN platforms
- Data files are still not binary compatible with other systems
- Massive work done on configure script and Makefile rules for cleaner builds
- Now detect platform endianness rather than reading from C header files
- Changed struct _stamp to union _stampu for proper DOS date/time handling
- Fixed lack-of-responsiveness in keyboard/video driver in max -c and max -k modes
- Integrated patches from Andrew Clarke for FreeBSD support
- Many other changes; see CVS for details

---

## Fri Jun 6 2003 - Maximus/UNIX 3.03b [pre-release]

- Incorporated 3.03a-patch1: 7-bit NVT (telnet capability) with multi-user support
- Incorporated FreeBSD & Linux build fixes from Andrew Clarke and Bo Simonsen
- Incorporated msgapi changes from Bo Simonsen (Husky Project origins)
- Squish bases now binary-compatible with other software
- Modified comdll API to COMMAPI_VER=2 (Nagle algorithm support)
- Added BINARY-TRANSFER NVT option for 8-bit clear sessions
- Major work to configuration engine
- Support for GNU Make 3.76 deprecated; 3.79+ recommended
- Added -fpermissive to g++ CXXFLAGS
- Now builds under Solaris 8/SPARC (data structs still wrong endian)
- Many other changes; see CVS for complete details

---

## Sat May 24 2003 - Maximus/UNIX 3.03a

**Initial Public Release**  
Bumped version number to 3.03 to avoid confusion.

### Major Problem Areas

- No Installer
- Local console output buggy (always one keystroke behind)
- fdcomm.so comm driver buggy, does not speak telnet
- fdcomm.so only allows one concurrent session
- MexVM crashes regularly on 64-bit arch

### Added Features

- `max -p2000` makes Maximus listen on TCP port 2000
- Added more options to master makefile
- Added simple ./configure script

---

## Tue May 13 2003 - Maximus/UNIX 3.02

**Initial Release**  
Released so Tony Summerfelt can play with Squish.  
Current Status: Squish "works for me"
