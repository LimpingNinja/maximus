# Source Layout Reorganization Plan

## Goals

- Make the codebase easier to browse and reason about.
- Keep the build working throughout migrations (incremental, validated steps).
- Maintain a clear separation between:
  - Source code (things that compile)
  - Runtime resources (things that ship)
  - Developer tooling (things that help build/test, but don’t ship)

## Non-Goals

- No functional refactors bundled into directory moves.
- No API redesign.
- No mass `#include` rewrites unless a move forces it.

## Mental Model (the structure we want the tree to communicate)

### Source code vs runtime resources

- **Source code** is everything that compiles into libraries and executables.
- **Runtime resources** are files the runtime expects at install/run time:
  - language data
  - menus/scripts
  - install tree defaults

This separation is the basis of the `src/` + `resources/` split.

### Libraries vs apps vs utils

- **Libraries**: compiled artifacts that are linked into one or more executables.
- **Apps**: end-user executables (runtime daemons, config UI/CLI, compilers, packers, etc.).
- **Utils**: supporting executables and helper code which are not part of the “main runtime surface area”, but are still shipped and/or used by the build.

### `mex` is an embeddable VM (not “a toolchain”)

`mex` should be modeled as:

- A **VM/runtime library** (embeddable)
- One or more **apps** built on top (compiler/driver tooling)

That means `mex` should be split across `src/libs/...` and `src/apps/...` in the target layout.

## Current State (completed work)

### Phase 1 (completed): `max/` internal browsing structure

The `max/` runtime sources were grouped into subsystem folders to improve browsing without changing include semantics.

### Phase 2 (completed): consolidate libraries under `src/libs/`

Libraries were moved into `src/libs/` and build wiring was updated to avoid fragile relative includes.

- `libmaxcfg` -> `src/libs/libmaxcfg`
- `unix` -> `src/libs/unix`
- `btree` -> `src/libs/btree`
- `slib` -> `src/libs/slib`
- `msgapi` -> `src/libs/msgapi`
- `prot` -> `src/libs/prot`
- `comdll` -> `src/libs/legacy/comdll`

### Phase 3 (completed): Introduce `src/` + `resources/` and move the tree

- Runtime: `max/` -> `src/max/`
- Libraries: `libs/` -> `src/libs/`
- Apps: `maxcfg/` -> `src/apps/maxcfg/`, `maxtel/` -> `src/apps/maxtel/`, `mcp/` -> `src/apps/mcp/`
- Utils: `squish/` -> `src/utils/squish/`, `sqafix/` -> `src/utils/sqafix/`, `util/` -> `src/utils/util/`, `maxblast/` -> `src/utils/maxblast/`
- Resources: `install_tree/` -> `resources/install_tree/`, `lang/` -> `resources/lang/`, `m/` -> `resources/m/`
- Shipped scripts: `runbbs.sh` -> `resources/scripts/runbbs.sh` (dev-only scripts remain in repo-root `scripts/`)

Build invariants established:

- Top-level Makefile drives builds with `$(MAKE) -C <dir> SRC=$(SRC)`.
- Subprojects include config with `include $(SRC)/vars.mk`.

## Target Repo Layout (next base-level re-org)

This is the “base-level” structure that makes the mental model obvious:

```
docs/

build/                     (build output prefix)

resources/                 (runtime assets shipped/installed)
  scripts/
  install_tree/
  lang/
  m/

src/                       (all code that compiles)
  max/                     (runtime)
  libs/                    (all shared libs)
  apps/                    (executables / user-facing programs)
  utils/                   (supporting tools; may be shipped)

scripts/                   (developer scripts, CI helpers)
```

Notes:

- Whether `scripts/` lives under `resources/` vs repo-root depends on whether those scripts are shipped vs dev-only.
- Current decision: keep dev tooling in repo-root `scripts/`, and put shipped/runtime scripts in `resources/scripts/`.
- `build/` remains the PREFIX output directory (not a source tree).

## Phase Plan (phases + steps)

Each phase is intentionally mechanical. After each step, run:

- `make build`
- `make install`

### Phase 3: Introduce `src/` and `resources/` and move the tree to match

**Outcome**: the repo-root becomes navigation-friendly and the build becomes insensitive to nesting.

#### Phase 3.0: Create the target directories (no moves)

- Create `src/` and `resources/` trees.
- No Makefile changes required yet.

#### Phase 3.1: Move source code into `src/`

Recommended order:

- Move `max/` -> `src/max/`
- Move `libs/` -> `src/libs/`
- Decide and move app directories into `src/apps/`:
  - `maxcfg/` -> `src/apps/maxcfg/`
  - `maxtel/` -> `src/apps/maxtel/`
  - `mcp/`, `util/`, `squish/`, `sqafix/`, `maxblast/` as `src/apps/` or `src/utils/` (decision by “shipped + user-facing” vs “supporting tool”)

Makefile rules during this phase:

- Replace hardcoded root-relative paths with `$(SRC)`-based paths.
- Replace any remaining `include ../vars.mk` with `include $(SRC)/vars.mk`.
- Prefer `$(MAKE) -C <dir> SRC=$(SRC)` for recursion.

#### Phase 3.2: Move runtime assets into `resources/`

Recommended order:

- `install_tree/` -> `resources/install_tree/`
- `lang/` -> `resources/lang/`
- `m/` -> `resources/m/`
- shipped scripts -> `resources/scripts/`

Then update:

- install scripts that reference these directories
- build rules that generate language assets

### Phase 4: Split `mex` cleanly into embeddable VM + apps

**Outcome**: `mex` becomes “a library + apps” instead of a mixed bucket.

Target end state:

```
src/
  libs/
    mexvm/                 (embeddable VM library)
      include/
      src/
  apps/
    mexc/                  (compiler)
    mex/                   (optional driver/runner if it exists as a separate executable)
```

Migration steps (safe, incremental):

- Move current `mex/` into `src/libs/mexvm/` as a temporary “all-in-one” location.
- Identify compiler/driver source files vs VM/runtime source files.
- Split build targets:
  - build `libmexvm.so` (or equivalent) from VM sources
  - build `mexc` executable from compiler sources, linking against the VM lib
- Update consumers to link against the VM library from `src/libs/mexvm/`.

## Legacy / Retire Candidates (flag list)

This list is intentionally conservative: it identifies candidates and suggests investigation paths.

### Likely legacy / platform baggage

- OS/2-specific code paths
- DOS-specific code paths
- `asyncnt.c` / NT-era async bits

### `comdll`

- Communications drivers / modem/serial abstraction.
- Still linked today, so treat as isolate-and-measure before any removal.

## Build invariants (must stay true)

- Recursive builds always pass `SRC=$(SRC)`.
- Subprojects include configuration with `include $(SRC)/vars.mk`.
- Prefer `$(MAKE) -C <dir>` over `cd ...; $(MAKE)`.
- Avoid new hardcoded relative paths (`../../..`).

## Appendix A: Proposed end-state repository tree (expanded)

This is the detailed end-state layout. The `Target Repo Layout` section above remains the base-level shape; this appendix expands the nodes so you can validate placement.

```
.
├── docs/
│   ├── code-docs/
│   └── ...
│
├── build/                              (build output prefix)
│   ├── bin/
│   ├── lib/
│   ├── etc/
│   └── ...
│
├── resources/                          (runtime assets shipped/installed)
│   ├── install_tree/                   (packaged runtime tree; source-of-truth copy)
│   │   ├── bin/
│   │   ├── docs/
│   │   ├── etc/
│   │   ├── help/
│   │   ├── ipc/
│   │   ├── lang/
│   │   ├── ansi/
│   │   └── ...
│   ├── lang/                           (source language definitions)
│   │   ├── english.mad
│   │   └── ...
│   ├── m/                              (MEX scripts / runtime content)
│   │   └── ...
│   └── scripts/                        (runtime/shipped scripts, if any)
│       └── ...
│
├── src/                                (all code that compiles)
│   ├── max/                            (Maximus runtime)
│   │   ├── core/
│   │   ├── display/
│   │   ├── file/
│   │   ├── msg/
│   │   ├── mex_runtime/
│   │   ├── netmail/
│   │   ├── platform_legacy/
│   │   ├── track/
│   │   ├── Makefile
│   │   └── ...
│   │
│   ├── libs/                           (shared libraries)
│   │   ├── btree/
│   │   ├── libmaxcfg/
│   │   ├── msgapi/
│   │   ├── prot/
│   │   ├── slib/
│   │   ├── unix/
│   │   ├── mexvm/                       (NEW: embeddable MEX VM library)
│   │   │   ├── include/
│   │   │   ├── src/
│   │   │   └── Makefile
│   │   └── legacy/
│   │       └── comdll/
│   │
│   ├── apps/                           (executables / user-facing programs)
│   │   ├── maxcfg/
│   │   ├── maxtel/
│   │   ├── mcp/
│   │   ├── mexc/                        (NEW: MEX compiler)
│   │   ├── mex/                         (optional: runner/driver if retained as a separate executable)
│   │   └── ...
│   │
│   └── utils/                          (support tools; shipped or build-time)
│       ├── util/                        (maid/mecca/etc; or split per-tool)
│       ├── sqafix/
│       ├── squish/
│       ├── maxblast/
│       ├── ui/
│       ├── swap/
│       ├── swap2/
│       └── ...
│
└── scripts/                            (developer scripts only)
    └── ...
```

## Appendix B: Inventory / mapping (current -> proposed)

This table is intended to be checked off during the migration.

| Current path | Proposed destination | Category | Notes |
|---|---|---|---|
| `max/` | `src/max/` | source | already internally grouped into subsystem dirs |
| `libs/` | `src/libs/` | source | includes `libs/legacy/comdll` -> `src/libs/legacy/comdll` |
| `mex/` | `src/libs/mexvm/` + `src/apps/mexc/` | source | split VM vs compiler/driver |
| `maxcfg/` | `src/apps/maxcfg/` | app | config editor |
| `maxtel/` | `src/apps/maxtel/` | app | terminal program |
| `mcp/` | `src/apps/mcp/` | app | MCP utilities |
| `util/` | `src/utils/util/` | utils | consider later splitting into per-tool dirs |
| `sqafix/` | `src/utils/sqafix/` | utils | supporting tool |
| `squish/` | `src/utils/squish/` | utils | supporting tool |
| `maxblast/` | `src/utils/maxblast/` | utils | supporting tool |
| `ui/` | `src/utils/ui/` | utils | supporting code |
| `swap/` | `src/utils/swap/` | utils | supporting code |
| `swap2/` | `src/utils/swap2/` | utils | supporting code |
| `install_tree/` | `resources/install_tree/` | resources | shipped runtime tree |
| `lang/` | `resources/lang/` | resources | language sources |
| `m/` | `resources/m/` | resources | scripts/content |
| `scripts/` | `scripts/` and/or `resources/scripts/` | dev/resources | split dev-only vs shipped |
| `docs/` | `docs/` | docs | unchanged |
| `notorious_doorkit/` | TBD | source | likely `src/apps/` or standalone package |
| `install/` | removed | source | deprecated and removed |
| `configuration-tests/` | TBD | source | likely `src/utils/` or keep as top-level test harness |
| `prot/` (historical) | already moved under `libs/prot` | source | already completed in Phase 2 |
| `msgapi/` (historical) | already moved under `libs/msgapi` | source | already completed in Phase 2 |
