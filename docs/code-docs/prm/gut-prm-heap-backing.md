# Gut PRM Heap Backing Plan

Goal: systematically remove PRM heap/offset backing (the `offsets` heap, `PRM(...)` string macro, and runtime `struct m_pointers prm` dependency) from **runtime-shipping code**, then remove PRM entirely.

Scope rules:

- Runtime-shipping scope includes `max/`, `slib/`, `prot/`, `mex/`, `comdll/`, `msgapi/`, etc.
- `maxcfg/` is the next target for full libmaxcfg/TOML-only operation.
- `util/` tools (SILT/MAID, etc.) are expected to be removed from the build pipeline and are not a long-term compatibility surface.

## Definitions

- **PRM heap backing**: the compiled `.prm` file’s string heap loaded into `offsets`, accessed via `PRM(field)` where `field` is an OFS (offset) member in `struct m_pointers`.
- **PRM struct backing**: the runtime global `struct m_pointers prm` and direct reads from it.
- **Fallback signature**: `ngcfg_get_*()` call sites passing legacy PRM-derived values as the fallback argument.

## Current runtime entry points (authoritative)

These are the places that must be dismantled/isolated:

- `max/max_init.c`:
  - Loads next-gen TOML (`maxcfg_toml_*`) into `ng_cfg`.
  - Opens the PRM file and reads `struct m_pointers prm`.
  - Validates `prm.id`, `prm.version`, `prm.heap_offset`.
  - Allocates and reads the PRM heap into global `offsets`.
- `max/max_fini.c`:
  - Frees `offsets` and `ng_cfg`.
- `src/max/max_v.h`:
  - Declares `extrn struct m_pointers prm;`
  - Declares `extrn char *offsets;`
- `src/max/prm.h`:
  - Defines `PRM(s) (offsets+(prm.s))`.

Additionally, runtime-shipping headers/macros:

- `slib/prog.h` currently references `prm.carrier_mask` via `carrier_flag`.

## Milestones

### Milestone 1: Remove PRM fallback signature from `ngcfg_get_*()` call sites (TOML-only runtime reads)

Outcome: runtime no longer passes legacy PRM-derived values (including `PRM(...)` heap strings) as fallbacks into `ngcfg_get_*()`.

- Change the `ngcfg_get_*()` API(s) used by runtime to no longer take PRM-derived fallback arguments.
- Update call sites across runtime to:
  - Use TOML keys as primary.
  - Use explicit constants/defaults when a fallback value is still needed.

Notes:

- This is expected to be mechanically large but straightforward.
- This should be done before disabling PRM loading, so you can keep builds green while changing call patterns.

Checklist:

- Decide the defaulting policy per access type:
  - `ngcfg_get_int`: choose explicit default constants (or treat missing as fatal if appropriate).
  - `ngcfg_get_bool`: explicit `true`/`false` default.
  - `ngcfg_get_string_raw` / `ngcfg_get_path`: explicit default string (`""`) or a build-time/system default.
- Change the runtime-facing `ngcfg_get_*()` APIs to remove the fallback parameter.
  - Prefer renaming the old fallback-taking APIs to something explicit (example: `ngcfg_get_int_fb`) and keeping the no-fallback versions as the canonical names.
  - Keep the fallback-taking APIs temporarily only if needed to stage the conversion.
- Convert call sites in runtime-shipping code in mechanical batches:
  - `max/`
  - `prot/`
  - `slib/` (notably `carrier_flag`)
  - `mex/`, `comdll/`, `msgapi/` (as applicable)
- After each batch:
  - Ensure `make build` still succeeds.
  - Ensure `make install` succeeds in the new default pipeline (without building `util/` or `maxcfg/`).
- When no call sites use the fallback-taking APIs:
  - Delete the fallback-taking APIs entirely.
  - Proceed to Milestone 2 (removing PRM heap load / `offsets`).

### Milestone 2: Remove PRM heap backing (`offsets`) and `PRM(...)` usage in runtime

Outcome: runtime no longer needs `offsets` nor the `PRM(...)` macro.

- Remove/disable PRM heap loading in startup.
- Ensure there are no remaining uses of `PRM(...)` in runtime-shipping code.

End state for this milestone:

- `offsets` can be removed from runtime (`max/max_v.h`).
- The `PRM(...)` macro is unused by runtime code.

### Milestone 3: Remove PRM struct dependence in runtime (`struct m_pointers prm`)

Outcome: runtime no longer needs the `prm` global or PRM struct layout.

- Identify remaining runtime references to `prm.<field>`:
  - Some may be genuine runtime-state (ex: historically `noise_ok`), but should now be TOML overrides or internal runtime variables.
  - Any remaining config semantics must be TOML-only.
- Remove `extrn struct m_pointers prm` from runtime builds.

Approach:

- Split PRM definitions so that `src/apps/maxcfg/` and `src/utils/util/` can keep including `src/max/prm.h`.
- For runtime builds, either:
  - Provide a minimal stub type that is no longer used (only for compatibility), or
  - Remove inclusion paths so runtime doesn’t include `prm.h` at all.

### Milestone 4: Remove the “fallback signature” from `ngcfg_get_*` at call sites

Outcome: runtime treats TOML/libmaxcfg as authoritative with no PRM fallback values.

- Replace call patterns like:
  - `ngcfg_get_int("...", prm.some_field)`
  - `ngcfg_get_string_raw("...", PRM(some_ofs))`

With TOML-only patterns:

- `ngcfg_get_int("...", <constant default>)`
- `ngcfg_get_string_raw("...", "")` or equivalent.

To preserve behavior during transition:

- If we need a brief transitional period, gate legacy fallback support behind a short-lived build option.
- The end goal is to delete that option.

## Version / compatibility checks (replace PRM version checks)

Problem:

- Runtime currently validates the PRM file using:
  - `prm.version == CTL_VER`
  - `prm.heap_offset == sizeof(struct m_pointers)`

When PRM is removed, we still want a meaningful “config ABI/version mismatch” check.

Plan:

- Add a libmaxcfg-exposed version constant/ABI value (example names):
  - `#define LIBMAXCFG_ABI_VERSION <int>` in `libmaxcfg/include/libmaxcfg.h`
  - `int maxcfg_abi_version(void)` (optional)
- In runtime code, define a matching required version in a Maximus header (example):
  - `#define MAXCFGVER <int>`
- At runtime startup (in the TOML init path), verify:
  - `LIBMAXCFG_ABI_VERSION == MAXCFGVER` (or `>=`/range if we want)
  - If mismatch: print a clear error and abort startup.

This replaces PRM’s “is this the current version?” safety check.

## Guardrails / rules while executing

- Keep builds green by iterating in small batches:
  - `make build`
  - `make install`
- Do not refactor `util/` or `maxcfg/` as part of this plan.
- Prefer deleting dead fallback logic rather than introducing new long-lived compatibility layers.

## Tracking / reporting

- Maintain:
  - `code-docs/runtime-toml-wiring-report.md` (runtime TOML wiring state)
  - `code-docs/raw-prm-not-ngcfg-report.md` (raw `PRM(...)` callsites)

Key “done” signals:

- Runtime builds with PRM load disabled.
- Runtime has no `PRM(...)` usage.
- Runtime has no `prm.<field>` usage.
- Runtime no longer defines/links `offsets`.
- Runtime no longer validates PRM version; it validates libmaxcfg ABI/version.
