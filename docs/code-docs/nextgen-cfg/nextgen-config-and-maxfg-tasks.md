# Next-Gen Config + MAXCFG Task Matrix (Ingestion → libmaxcfg → Unified Runtime)

This document is the step-by-step task inventory to reach the next major milestone:

- **Milestone:** Maximus can run using a **runtime-loaded next-gen config** with **no SILT/MAID/heap compilation step**.
- **Constraint:** We are **not** preserving backwards compatibility.
- **Approach:** Finish **ingestion/export** in `maxcfg` first, then build `libmaxcfg` and migrate the runtime to consume the new config directly.

---

## Guiding principles

- **Single source of truth:** runtime reads what tools write.
- **Config is data:** schema’d, versioned, validated.
- **Domain separation:** split files by subsystem (system/session/menus/areas/access/etc.).
- **Atomic writes:** always write temp + rename.
- **Central registries:** picker data (commands, flags, enums) should live in one shared place so we don’t hardcode lists multiple times.

---

## Current relevant notes (MAXCFG)

- `maxcfg/src/ui/command_picker.c` contains a large curated registry of menu commands (`Xtern_Run`, etc.) with help text and categories.
  - This data is highly valuable and should be treated as a **first-class registry** (shared by UI, config validation, and export).

---

## Planned phases

- **Phase 0:** Finish ingestion/export coverage in `maxcfg` (legacy → new config). No runtime changes yet.
- **Phase 1:** Build `libmaxcfg` (new config loader + schema + validation + save helpers).
- **Phase 2:** Migrate Maximus runtime off PRM/heap to `libmaxcfg`.
- **Phase 3:** Delete dead code / duplicate registries / toolchain dependencies.

---

## Task table

Legend:

- **Priority:** P0 blocks the milestone, P1 important, P2 later.
- **Done-when:** concrete success criteria.

| ID | Done | Phase | Priority | Area | Task | Inputs (legacy) | Outputs (next-gen) | Depends on | Done-when |
|----|------|-------|----------|------|------|------------------|--------------------|------------|----------|
| 0.1 | [x] | 0 | P0 | Planning | Define next-gen config directory layout + `config_version` + naming conventions | n/a | Documented layout + version policy | none | Layout agreed and stable enough for export |
| 0.2 | [x] | 0 | P0 | MAXCFG export | Add “Export Next-Gen Config” tool action (writes `config/` dir) | all ingested data | `config/` written atomically | 0.1 | Export creates full tree without crashing |
| 0.3 | [ ] | 0 | P0 | Registry | Create shared registry module for pickers (commands, flags, etc.) | command_picker + other pickers | `maxcfg_registry_*` API | none | UI uses registry, export uses registry |
| 0.4 | [ ] | 0 | P0 | MAXCFG ingestion | `access.ctl` parse + edit + save (real, not sample) | `etc/access.ctl` | `config/access/levels.*` | 0.1 | Round-trip: parse→edit→save preserves semantics |
| 0.5 | [ ] | 0 | P0 | MAXCFG ingestion | `protocol.ctl` parse + edit + save | `etc/protocol.ctl` | `config/protocols/protocols.*` | 0.1 | Round-trip works; UI edits are persisted |
| 0.6 | [ ] | 0 | P0 | MAXCFG ingestion | `language.ctl` parse + edit + save + ingest available languages | `etc/language.ctl` + lang assets | `config/language/*` | 0.1 | Language choices and defaults export correctly |
| 0.7 | [ ] | 0 | P1 | MAXCFG ingestion | `events??.bbs` parse + edit + save | `etc/events00.bbs` (and siblings) | `config/events.*` | 0.1 | Events load + save, no format loss for supported fields |
| 0.8 | [ ] | 0 | P1 | MAXCFG ingestion | `baduser.bbs` ingestion/editing | `etc/baduser.bbs` | `config/users/bad_users.*` | 0.1 | Can add/remove entries and export |
| 0.9 | [ ] | 0 | P1 | MAXCFG ingestion | `names.max` ingestion/editing | `etc/names.max` | `config/users/reserved_names.*` | 0.1 | Can add/remove entries and export |
| 0.10 | [ ] | 0 | P1 | MAXCFG ingestion | User database ingestion plan (read-only first) | `etc/user.bbs` | `config/users/*` OR separate DB plan | 0.1 | Documented decision + minimal ingest tool |
| 0.11 | [x] | 0 | P0 | MAXCFG ingestion | Finish gaps in menus/areas ingestion for export correctness | `menus.ctl`/`msgarea.ctl`/`filearea.ctl` | `config/menus/*`, `config/areas/*` | 0.2 | Export output is complete and references resolve |
| 0.12 | [ ] | 0 | P2 | MAXCFG ingestion | `reader.ctl` ingestion | `etc/reader.ctl` | `config/reader.*` | 0.1 | Parsed and exported |
| 0.13 | [ ] | 0 | P2 | MAXCFG ingestion | Archivers (`compress.cfg`) ingestion | `etc/compress.cfg` | `config/archivers.*` | 0.1 | Parsed and exported |
| 1.1 | [ ] | 1 | P0 | libmaxcfg | Define C structs for config domains | n/a | `MaxConfig` + sub-structs | 0.1 | Structures cover exported fields |
| 1.2 | [ ] | 1 | P0 | libmaxcfg | Implement loader (parse files → structs) | `config/` dir | `MaxConfig* cfg_load()` | 1.1 | Loads full config tree |
| 1.3 | [ ] | 1 | P0 | libmaxcfg | Schema + validation layer | `config/` dir | validation errors with paths | 1.2, 0.3 | Bad config reports actionable errors |
| 1.4 | [ ] | 1 | P1 | libmaxcfg | Save helpers (atomic writes) | `MaxConfig` | `cfg_save_atomic()` | 1.2 | No partial writes; crash-safe |
| 1.5 | [ ] | 1 | P1 | libmaxcfg | Registry sharing (pickers/commands/flags) moved out of maxcfg | existing picker lists | common registry module | 0.3 | Both maxcfg + runtime share one registry |
| 2.1 | [ ] | 2 | P0 | Runtime | Add `--config <dir>` boot path (load next-gen config early) | `config/` | runtime uses cfg | 1.2 | Maximus starts using cfg when passed |
| 2.2 | [ ] | 2 | P0 | Runtime | Migrate “system paths + logging + basics” off PRM | PRM usage | cfg usage | 2.1 | No PRM dependency in these paths |
| 2.3 | [ ] | 2 | P0 | Runtime | Migrate access control to next-gen access levels | `access.ctl`/PRM | cfg access model | 2.2, 0.4, 1.3 | Access checks work off cfg |
| 2.4 | [x] | 2 | P0 | Runtime | Migrate menus to cfg menu model | `menus.ctl` | cfg menus | 2.2, 0.11 | Menus render and dispatch |
| 2.5 | [x] | 2 | P0 | Runtime | Migrate areas (msg/file) to cfg model | `msgarea.ctl`/`filearea.ctl` | cfg areas | 2.2, 0.11 | Can enter areas; scanning works |
| 2.6 | [ ] | 2 | P1 | Runtime | Migrate protocols + events | `protocol.ctl`, `events??.bbs` | cfg equivalents | 2.2, 0.5, 0.7 | Transfers/events use cfg |
| 2.7 | [ ] | 2 | P1 | Runtime | Migrate language system away from MAID | MAID artifacts | runtime-loaded strings | 0.6, 1.2 | No MAID in runtime pipeline |
| 2.8 | [x] | 2 | P0 | Runtime | Add config-root discovery and stop relying on working directory for `config/` | env + filesystem | runtime-resolved config root + `maximus.config_path` | 2.2 | TOML loads/menus resolve without CWD assumptions |
| 2.9 | [x] | 2 | P1 | Runtime | Finish PRM hotwiring for attachment subsystem paths | PRM attach/inbound paths | TOML-first `ngcfg_get_path` with PRM fallback | 2.8 | `m_attach.c` no longer directly depends on PRM paths |
| 2.10 | [x] | 2 | P1 | Runtime | Finish PRM hotwiring for area-change keys | `achg_keys` | `general.session.area_change_keys` | 2.8 | Area change prompt keys come from TOML |
| 2.11 | [x] | 2 | P1 | Runtime | Wire `general.colors` (from `colors.lh`) into runtime prompt color strings | `lang/colors.lh` + MAID artifacts | `config/general/colors.toml` consumed at runtime | 2.8 | Menu/file/message/FSR `*_col` color strings can be overridden via TOML |
| 2.12 | [ ] | 2 | P1 | Runtime | PRM hotwiring sweep: logging + paths | PRM | TOML-first `ngcfg_get_*` with fallback | 2.8 | Logging/path hotspots no longer directly depend on PRM |
| 2.13 | [ ] | 2 | P1 | Runtime | PRM hotwiring sweep: protocols + transfers | PRM | TOML-first `ngcfg_get_*` with fallback | 2.8 | Transfers/protocol hotspots no longer directly depend on PRM |
| 3.1 | [ ] | 3 | P0 | Cleanup | Remove PRM/heap loader + related tooling hooks | PRM code | deleted paths | 2.x complete | Build works without PRM/heap |
| 3.2 | [ ] | 3 | P0 | Cleanup | Remove SILT/MAID invocation from `maxcfg` | compile.c | deleted compile UI | 2.x complete | maxcfg no longer offers compile |
| 3.3 | [ ] | 3 | P1 | Cleanup | Reduce duplicate “picker options” in codebase via registry | scattered lists | registry-only | 1.5 | Only one registry source remains |

---

## Notes on registries / pickers (important for reducing code)

Today, `maxcfg` embeds significant domain knowledge in:

- command pickers (menu command vocabulary + help text)
- privilege pickers
- checkbox pickers for flags

The next-gen system should treat this as **structured metadata**, not random UI lists:

- Share with:
  - maxcfg UI
  - maxcfg export validation
  - runtime validation errors (“unknown menu command X”)

This is one of the biggest opportunities to reduce duplicated logic and make the system more maintainable.

---

## Next action

If you confirm the table structure/wording is what you want, the next concrete coding step should be **Phase 0 / P0**:
 
 - Implement `access.ctl` ingestion (parser + writer + editor hookup)
 
 because it unblocks both export completeness and later runtime migration.

---

## Completed work (recorded here so the matrix stays truthful)

- Next-gen config layout + `config_version` policy documented in `code-docs/nextgen-cfg/libmaxcfg.md`.
- `libmaxcfg` scaffolding added as a shared library module:
  - `libmaxcfg/include/libmaxcfg.h`
  - `libmaxcfg/src/libmaxcfg.c`
  - `libmaxcfg/src/path.c`
  - `libmaxcfg/Makefile` (builds `libmaxcfg.so`)
- Build system integration:
  - Master `Makefile` builds/installs `libmaxcfg`
  - `vars.mk` and `vars.mk.configure` include `libmaxcfg` include/lib search paths
  - `maxcfg` links against `-lmaxcfg`
- Runtime wiring:
  - Maximus loads next-gen TOML files at startup (flag-day load)
  - Menus are TOML-first (menu engine consumes `menus/*.toml`)
  - Message/file areas are TOML-first (runtime generates temporary `marea.*` / `farea.*` from `areas/msg/areas.toml` + `areas/file/areas.toml`, with fallback)
- Config root discovery:
  - Maximus discovers a config root at startup and prefers `maximus.config_path` for future config loads
- MAXCFG export correctness:
  - Hardened CTL parsing so `msgarea.ctl` / `filearea.ctl` export reliably preserves required `path` / `download` fields
