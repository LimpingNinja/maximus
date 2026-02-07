# Raw PRM Call Sites (Not Inside `ngcfg*()`)

This file is generated from:

```
python3 scripts/find_raw_prm.py --root . --format text
```

It lists every `PRM(...)` call site found in the repository that is **not** lexically inside an `ngcfg*()` call (comments and string/char literals are ignored).

## Snapshot (verbatim)

```
src/max/oldprm.h:46:11: PRM(s)
src/max/prm.h:66:11: PRM(s)
TOTAL_RAW_PRM=2
```

## Sorted backlog (by type + importance)

### 1) TOML-backed PRM statements left (not wired to `ngcfg*()` as primary)

These are raw `PRM(...)` call sites outside any `ngcfg*()` call, but each has a known TOML key.

None.

### 2) CTL-based PRM statements left with no TOML backing

None found by the raw PRM scanner.

### 3) PRM statements left that are neither CTL-based nor TOML-backed

| PRM key | Count | Notes |
|---|---:|---|
| `s` | 2 | Header/macro mechanics (`src/max/prm.h`, `src/max/oldprm.h`). |

### 4) Meaningful subset of #3

None.

### 5) TOML-backed `prm.<field>` usages left (not wired to `ngcfg*()` as primary)

These are direct `prm.<field>` uses (not `PRM(...)`) that still exist in runtime code.

Classification rule:

- A field is treated as **TOML-backed** if it is exported by `src/apps/maxcfg/src/config/nextgen_export.c` and/or is explicitly modeled differently in next-gen TOML (e.g. `flags`, `flags2`).

None.

### 6) CTL-based `prm.<field>` usages left with no TOML backing

| PRM field | Count | Files | Notes |
|---|---:|---|---|
| `PRM_HEAP_START` | 1 | `src/max/mexint.c` | Meta/struct mechanics (not a config field) |
| `cls` | 1 | `src/max/max_main.c` | Obsolete compatibility (Max 2.x) |
| `h` | 2 | `src/max/max.h`, `src/max/max_args.c` | Meta/struct mechanics (not a config field) |

### 7) `prm.<field>` usages left that are neither CTL-based nor TOML-backed

| PRM field | Count | Files | Notes |
|---|---:|---|---|
| `heap_offset` | 3 | `src/max/max_init.c` | PRM layout/internal |
| `id` | 1 | `src/max/max_init.c` | PRM layout/internal |
| `version` | 1 | `src/max/max_init.c` | PRM layout/internal |

### 8) Raw PRM call sites (Not Inside `ngcfg*()`) outside `max/`

Found `0` raw PRM callsites outside `max/` across `0` unique `PRM(...)` expressions.

#### 8.1) TOML-backed raw PRM call sites outside `max/`

None.

#### 8.2) CTL-based raw PRM call sites outside `max/` with no TOML backing

None.

#### 8.3) Raw PRM call sites outside `max/` that are neither CTL-based nor TOML-backed

None.

### 9) Direct `prm.<field>` usages outside `max/`

Found `554` total `prm.<field>` hits outside `max/` across `169` unique fields.

#### 9.1) TOML-backed `prm.<field>` usages outside `max/`

Runtime-shipping subset:

Notes:

- `src/libs/prot/` still references some `prm.<field>` values, but only as the *fallback argument* to `ngcfg_get_*()` calls (already TOML-first).
- The only remaining runtime-shipping TOML-backed usage that is **not** already wrapped as a fallback inside `ngcfg_get_*()` is `carrier_mask` (via the `carrier_flag` macro in `src/libs/slib/prog.h`).

| PRM field | Count | Files |
|---|---:|---|
| `carrier_mask` | 1 | `src/libs/slib/prog.h` |

#### 9.2) CTL-based `prm.<field>` usages outside `max/` with no TOML backing

| PRM field | Count | Files |
|---|---:|---|
| `cls` | 22 | `src/utils/util/s_access.c`, `src/utils/util/s_misc.c`, `src/utils/util/s_parse.c`, `src/utils/util/s_sessio.c` |
| `dlall_priv` | 1 | `src/utils/util/s_sessio.c` |
| `h` | 1 | `src/utils/util/maid.c` |
| `ul_reward` | 1 | `src/utils/util/s_sessio.c` |
| `ulbbs_priv` | 1 | `src/utils/util/s_sessio.c` |

#### 9.3) `prm.<field>` usages outside `max/` that are neither CTL-based nor TOML-backed

| PRM field | Count | Files | Notes |
|---|---:|---|---|
| `adat_name` | 4 | `src/utils/util/s_area.c`, `src/utils/util/s_parse.c`, `src/utils/util/silt.c` | Obsolete compatibility |
| `aidx_name` | 4 | `src/utils/util/s_area.c`, `src/utils/util/s_parse.c`, `src/utils/util/silt.c` | Obsolete compatibility |
| `heap_offset` | 3 | `src/utils/util/maid.c`, `src/utils/util/s_parse.c` | PRM layout/internal |
| `id` | 2 | `src/utils/util/maid.c`, `src/utils/util/s_parse.c` | PRM layout/internal |
| `max_ulist` | 2 | `src/utils/util/s_parse.c`, `src/utils/util/s_sessio.c` | Obsolete compatibility |
| `min_ulist` | 2 | `src/utils/util/s_parse.c`, `src/utils/util/s_sessio.c` | Obsolete compatibility |
| `ratio_threshold` | 1 | `src/utils/util/s_sessio.c` | Obsolete compatibility |
| `version` | 2 | `src/utils/util/maid.c`, `src/utils/util/s_parse.c` | PRM layout/internal |
