# Next-Gen Configuration (No PRM/Heap, No SILT/MAID) + MAXCFG Roadmap

## Goals

- Replace the legacy **compiled PRM + string heap** configuration model with a **runtime-loaded, human-editable** configuration.
- Eliminate compilation tools in the critical path:
  - No `silt` for CTL→PRM
  - No `maid` for language compilation
  - No “recompile scripts” as a required step after editing configuration
- Make configuration changes:
  - easy to reason about
  - easy to diff/review in git
  - safe (atomic writes)
  - programmatically editable
- Use `maxcfg` as the migration tool: **ingest legacy flat files** (as they exist today) and convert into the new format.

**Non-goals**

- Backwards compatibility with the legacy PRM/heap runtime.
- Preserving every historical keyword and odd corner case in legacy formats.

---

## Current state (what exists today)

### Runtime (Maximus)

Maximus today fundamentally assumes:

- A compiled `max.prm` which contains:
  - a large `struct m_pointers`
  - a string heap referenced by offsets
- Multiple “source” text files (`*.ctl`, `*.bbs`, `*.mec`, etc.) that are compiled into PRM or other generated artifacts.

This is why tooling exists:

- **SILT**: compiles `etc/max.ctl` + associated CTL files into `etc/max.prm`
- **MAID**: compiles language assets (`*.mad` etc.)

### `maxcfg` (what it currently ingests)

`maxcfg` is an ncurses config editor, but **today it is still PRM-centered**.

- **PRM loading**
  - Entry point: `src/apps/maxcfg/src/main.c`
  - Loads `<config_path>.prm` (default: `etc/max.prm` via `DEFAULT_CONFIG_PATH "etc/max"`)
  - Reads/writes the PRM heap via `prm_load()`, `prm_set_string()`, `prm_flag_set()`, `prm_save()`

- **Direct CTL editing (partial)**
  - `max.ctl`:
    - Reads keywords via `ctl_parse_keyword()` / `ctl_parse_boolean()`
    - Writes changes by patching `etc/max.ctl` in-place (`ctl_patch_max_ctl_from_prm()` / `ctl_sync_dirty_fields()`)
    - Sets `g_state.ctl_modified = true`

- **Parsers/editors that exist and look real**
  - `msgarea.ctl`:
    - `parse_msgarea_ctl()` / `save_msgarea_ctl()` in `src/apps/maxcfg/src/config/area_parse.c`
  - `filearea.ctl`:
    - `parse_filearea_ctl()` / `save_filearea_ctl()` in `src/apps/maxcfg/src/config/area_parse.c`
  - `menus.ctl`:
    - `parse_menus_ctl()` / `save_menus_ctl()` in `src/apps/maxcfg/src/config/menu_parse.c`

- **Colors**
  - Current “Default Colors” editor is implemented against `colors.lh` (a header-like define file), not `colors.ctl`.

- **Security levels / access.ctl**
  - UI entry exists, but the current implementation is populated with sample values (not a real `access.ctl` parser/writer yet).

- **Compilation integration (still present)**
  - `src/apps/maxcfg/src/config/compile.c` runs external commands using `popen()`:
    - `silt etc/max -x`
    - `maid english ...`

**Key takeaway:** `maxcfg` is already proving out a useful approach (parse/edit/save of CTL files), but it is not yet a full replacement for the old toolchain.

---

## What should replace the PRM/heap model?

### Requirements for the replacement

- **Runtime-loaded** at Maximus startup (and reloadable for some subsets later)
- **Strongly-typed** in memory (struct-backed)
- **Schema’d** on disk (validation, defaults, versioning)
- **Composable** configuration (split into logical files; no monolithic 2000-line config)
- **Atomic writes** and minimal corruption risk
- **Human editable** (primary) and tool editable (secondary)

### Recommended direction (practical + future-proof)

Use a **directory-based config** with **TOML** files as the primary representation.

Why TOML:

- Human-friendly like INI, but supports nested tables and arrays cleanly
- More rigid than YAML (fewer footguns)
- Great fit for “config as data”

Suggested layout:

```
config/
  maximus.toml                 # top-level system config (paths, logging, toggles)
  comms.toml                   # comm driver settings
  session.toml                 # login, time limits, defaults
  ui.toml                      # colors, menu rendering, etc.
  doors.toml                   # door/dropfile policy

  menus/
    main.toml
    sysop.toml
    ...

  areas/
    msg/
      users.toml
      general.toml
      ...
    file/
      uploads.toml
      ...

  access/
    levels.toml                # security levels and flags

  protocols/
    protocols.toml

  language/
    en.toml                    # string tables
    ...
```

### In-memory model (C)

Define a root struct representing the whole configuration:

- `MaxConfig` (top-level)
  - `SystemConfig` (paths, name, sysop, etc.)
  - `SessionConfig`
  - `CommsConfig`
  - `UiConfig`
  - `DoorConfig`
  - `MenuConfig` (list of menus)
  - `AreaConfig` (msg/file trees)
  - `AccessConfig` (levels)
  - `ProtocolConfig`
  - `LanguageConfig`

Implementation approach:

- Parse TOML → build structs
- Validate + apply defaults
- Provide a single `cfg_get()` API for runtime subsystems
- Provide `cfg_save_atomic()` for tooling

### Versioning

Add an explicit version field:

- `config/maximus.toml` contains `config_version = 1`
- On load:
  - reject unknown major versions
  - allow minor extensions

This avoids the PRM-era “implicit layout coupling” problem.

---

## How “easy” is it to replace the heap configuration?

### What makes it hard today

- The PRM heap model is used everywhere through macros like `PRM(x)` and `PRMSTR(x)`.
- Many code paths assume strings live in a single global heap and are referenced by offsets.
- Tools (SILT/MAID) are effectively “build steps” for runtime.

### What makes it easier with the new approach (since we’re dropping backwards compat)

- We can remove `PRM(...)` access patterns and replace with explicit config accessors.
- We don’t need to preserve binary layouts or the heap; we can refactor aggressively.
- We can redesign config organization around **subsystems** rather than “what SILT historically compiled”.

### Practical migration strategy (without backwards compatibility)

1. Create the new config loader library (TOML → structs) and load it very early.
2. Pick one subsystem at a time and migrate it from `PRM(...)` to `cfg->...`.
3. Once enough subsystems are migrated, delete PRM loading entirely.

Even without backwards compat, this is still a non-trivial refactor, so the order matters.

---

## MAXCFG as the migration tool

### Where MAXCFG is already useful

- It already has working parsers/savers for:
  - `menus.ctl`
  - `msgarea.ctl`
  - `filearea.ctl`
- It can patch and write `max.ctl`.

### What MAXCFG is missing to be a true “replacement pipeline”

- Real `access.ctl` parse/save
- Real `protocol.ctl` parse/save
- Real `language.ctl` + language assets ingestion
- Events ingestion
- Users/bad users/reserved names ingestion
- A single “export to next-gen config/” step

### Priority order (before changing runtime)

If the goal is to stop depending on SILT/MAID and stop thinking in terms of compiled heaps, prioritize ingestion that:

1. Blocks runtime startup/config correctness
2. Blocks menus/doors working
3. Blocks user login/session defaults

Recommended ingestion priorities:

1. **`max.ctl` → `config/maximus.toml`**
   - System name/sysop/paths/logging
   - Temp path + IPC path
   - First menu / defaults

2. **`access.ctl` → `config/access/levels.toml`**
   - This affects access control everywhere.

3. **`menus.ctl` → `config/menus/*.toml`**
   - Menus are the core UX.

4. **`msgarea.ctl` + `filearea.ctl` → `config/areas/...`**

5. **`protocol.ctl` → `config/protocols/protocols.toml`**

6. **Language system**
   - Replace MAID-style compilation with runtime string tables.

7. The long tail:
   - `reader.ctl`, `events00.bbs`, `names.max`, `baduser.bbs`, `tunes.bbs`, archivers, etc.

---

## Design recommendation: make config editable without “compilation”

### Key principle

**Runtime should read what tools write.**

No compiled “product” files should exist as a separate step.

### Atomic edits

All writers should:

- write `*.tmp`
- `fsync()`
- rename into place

(Equivalent to the pattern already used in `ctl_patch.c`.)

### Validation

Config loader should validate:

- required keys present
- paths exist (optional strict mode)
- references exist (e.g., menus referencing display files)

This replaces a lot of what “compilers” historically provided.

---

## Recommended next deliverables

### A. Create a new unified config library

- `libmaxcfg` (name TBD)
- Implements:
  - TOML parsing
  - struct model
  - schema validation
  - atomic save

### B. Extend MAXCFG to export next-gen config

- Add a “Tools → Export Next-Gen Config” action
- Reads legacy files (as they exist)
- Writes `config/` directory

### C. Begin runtime migration

- Add `--config <dir>` to Maximus
- Load `config/` at startup
- Migrate subsystems incrementally

---

## Notes / observations

- Current `maxcfg` relies on PRM mostly to:
  - locate `sys_path` (`PRMSTR(sys_path)`)
  - provide default values for UI when CTL keys are missing
  - decide where `etc/` lives

In the next-gen world, `sys_path` should effectively disappear as a “magic anchor” concept; everything should be relative to the config directory or explicit paths.

---

## Appendix: quick status table (maxcfg ingestion)

| File | Read | Write | Notes |
|------|------|-------|------|
| `max.prm` | yes | yes | PRM heap access in `src/apps/maxcfg/src/data/prm.c` |
| `max.ctl` | partial | patch-based | keyword patching + dirty-field sync |
| `menus.ctl` | yes | yes | `menu_parse.c` |
| `msgarea.ctl` | yes | yes | `area_parse.c` |
| `filearea.ctl` | yes | yes | `area_parse.c` |
| `access.ctl` | no (real) | no | UI exists, parser not implemented |
| `protocol.ctl` | no | no | placeholder |
| `language.ctl` | no | no | placeholder + MAID compile action |
| `colors.ctl` | no | no | colors editing is currently `colors.lh` |
| `events00.bbs` | no | no | placeholder |
