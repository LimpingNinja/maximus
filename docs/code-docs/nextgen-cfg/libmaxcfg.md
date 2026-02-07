# libmaxcfg (Next-Gen Config) - Layout + Version Policy

This document defines the **next-gen configuration layout** and **versioning policy** for the future `libmaxcfg`-powered runtime configuration system.

This is Phase **0.1** planning: it exists so MAXCFG export and the runtime loader can converge on a single, stable representation.

---

## Goals

- Replace compiled `max.prm` + heap configuration with **runtime-loaded config**.
- Replace “compilation tools” (SILT/MAID) with **parse + validate + run**.
- Make configuration:
  - human-editable
  - tool-editable
  - git-diffable
  - versioned and validated

**Backwards compatibility is not a goal.**

---

## Config root directory

### Canonical name

The next-gen configuration root directory is named:

- `config/`

### Where MAXCFG exports today (temporary)

MAXCFG currently has no explicit “config path” setting. Until we add one, export target is:

- `<systempath>/config/`

Where:

- `<systempath>` is the existing Maximus system path (currently sourced from PRM/max.ctl in MAXCFG).

**Important:** This is an implementation convenience only.

Today, MAXCFG export records the chosen config directory in `config/maximus.toml` as `config_path`.

At runtime, Maximus discovers a config root directory and then prefers `maximus.config_path` (TOML-first with PRM fallback) as the authoritative config root.

Long-term, Maximus should accept an explicit `--config <dir>` (or equivalent) and MAXCFG should edit/export to that location.

---

## On-disk layout (v1)

The layout is a directory tree split by subsystem.

The exact file formats are intentionally not locked in here (TOML/JSON/etc.), but the *shape* and naming are.

```
config/
  maximus.*                # system identity, paths, logging, toggles
  session.*                # login settings, defaults, time limits
  doors.*                  # dropfile policy, door execution preferences

  menus/
    <menu_name>.*          # one file per menu (e.g. main.*, sysop.*)

  areas/
    msg/
      <area_or_group>.*
    file/
      <area_or_group>.*

  access/
    levels.*               # access level definitions and flags

  protocols/
    protocols.*            # transfer protocol definitions

  language/
    language.*             # language index / default language
    <lang_id>.*            # string tables (e.g. en.*)

  events/
    events.*               # schedule/events definitions

  users/
    bad_users.*            # previously baduser.bbs
    reserved_names.*       # previously names.max
    users.*                # user DB plan TBD (may become separate datastore)
```

Notes:

- Filenames and directories should be lowercase.
- Where legacy uses case-insensitive names (menu names), the export format should preserve an original name field but normalize filenames.

---

## Versioning policy

### `config_version`

A single integer `config_version` MUST exist in `config/maximus.*`.

- Example: `config_version = 1`

### Compatibility rules

- `libmaxcfg` MUST reject config if `config_version` is unknown.
- Version changes follow “major-only” semantics for now:
  - Any incompatible schema change increments `config_version`.
- Minor/optional additions within a version are allowed:
  - Readers must ignore unknown keys (but can warn).

### Validation expectation

`libmaxcfg` is responsible for validation (not a compiler):

- Required keys
- Type correctness
- Reference validation:
  - menus reference valid commands
  - areas reference existing paths
  - protocols reference valid executables/arguments

---

## Registry data (shared metadata)

MAXCFG currently embeds important domain knowledge in picker lists (example: `src/apps/maxcfg/src/ui/command_picker.c`).

For next-gen config:

- This data should become **shared registry metadata** used by:
  - MAXCFG UI pickers
  - MAXCFG export validation
  - libmaxcfg validation
  - runtime error reporting

The registry should be authoritative for:

- menu command vocabulary (including `Xtern_Run`, `Door32_Run`, etc.)
- flags enumerations
- privilege level labels

---

## Open decisions (intentionally deferred)

- File format choice for v1 (`toml` vs `json` vs others)
- User database storage (file vs embedded DB)

These are deferred so we can complete ingestion/export structure first.

---

## Implementation notes (current state)

This section captures what has already been implemented in-tree so it can be promoted into a future `libmaxcfg/README.md`.

### Placement and ownership

`libmaxcfg` lives at repo root as its own module:

- `libmaxcfg/include/`
- `libmaxcfg/src/`
- `libmaxcfg/Makefile`

Rationale:

- `libmaxcfg` is intended to be shared by the runtime (`max/`) and tooling (`maxcfg/`).
- Keeping it out of `max/` and out of `maxcfg/` avoids coupling the library to either consumer.

### API direction (base-dir oriented)

The library is intentionally unaware of “export targets” or any global install paths.

- Callers provide a base directory (the `config/` root) when opening the config.
- This keeps `libmaxcfg` reusable in:
  - MAXCFG export validation
  - runtime config loading
  - tests / tooling

Current public API (initial skeleton):

- `maxcfg_open(MaxCfg **out_cfg, const char *base_dir)`
- `maxcfg_close(MaxCfg *cfg)`
- `maxcfg_base_dir(const MaxCfg *cfg)`
- `maxcfg_join_path(...)`
- `maxcfg_status_string(MaxCfgStatus st)`

### Build integration

`libmaxcfg` is built as a shared library:

- output: `libmaxcfg.so`

On macOS, the build sets:

- `-install_name @rpath/libmaxcfg.so`

### Current runtime consumers (selected)

- `config/general/colors.toml` (`general.colors`)
  - Exported from `lang/colors.lh` (`COL_*` prompt color defines)
  - Consumed by the runtime to override menu/file/message/FSR language color strings (the `*_col` AVATAR sequences)
  - Status/popup/WFC palette colors (`_maxcol col` / `colours.dat`) are a separate legacy domain

Integration points:

- Master `Makefile`: `libmaxcfg` is included in `MAX_LIB_DIRS` so `make build` and `make install_libs` build and stage it.
- `vars.mk.configure` and `vars.mk`: include and library search paths include `libmaxcfg/include` and `libmaxcfg`.
- `maxcfg/Makefile`: links with `-lmaxcfg`.
