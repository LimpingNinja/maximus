# LibMexDB Spec Implementation Plan Notes

This document collects implementation-oriented notes that are *not* part of the public-facing API surface, but are required to implement LibMexDB correctly and idiomatically in the Maximus + MEX runtime environment.

---

## 1. Goals and non-goals

### 1.1 Goals

- Provide a **zero-SQL** CRUD + query API to MEX door authors.
- Preserve Maximus/MEX runtime conventions:
  - **Pattern-1 error handling** (return codes + `db_errcode()` / `db_errstr()`).
  - **Caller-owned structs**, passed by reference, populated in-place by intrinsics.
  - VM-safe handling of strings when reading/writing struct fields.
- Keep multi-node use responsive:
  - WAL enabled.
  - Short busy timeout + bounded retries/backoff.

### 1.2 Non-goals (v1)

- Joins, subqueries, arbitrary SQL for normal use.
- Returning arrays of structs from C (cursor iterator model is the v1 query mechanism).
- Extending the MEX compiler type system.

---

## 2. `db_open()` path resolution and configuration

### 2.1 What door authors pass

Door code should be allowed to do:

- `db_open("doors/game.db")`

…without needing to know `sys_path` or build system layout.

### 2.2 Resolution rules

When `filename` is **absolute**:

- Use it as-is.

When `filename` is **relative**:

- Resolve relative to:
  1. `maximus.db_path` (if present and non-empty), else
  2. `maximus.misc_path`, else
  3. `maximus.sys_path`.

This matches existing “relative path under sys_path” behavior implemented in the Maximus runtime’s `ngcfg_get_path()` pattern, while allowing a DB-specific override.

### 2.3 Implementation note: prefer existing join helpers

There are already path utilities in `libmaxcfg`:

- `maxcfg_resolve_path(base_dir, path, out, out_sz)`
- `maxcfg_join_path(cfg, relative, out, out_sz)`

On the Maximus runtime side, `ngcfg_get_path("maximus.misc_path")` already resolves relative-to-`sys_path` and normalizes trailing slashes for directories.

**Recommendation:** implement `mex_db_open()` resolution by obtaining a base directory via `ngcfg_get_path()` and joining `filename` via the same safe-join logic used elsewhere (or by calling the appropriate helper if it’s available from the same layer).

### 2.4 `db_path` in TOML (config integration)

- Proposed TOML key: `maximus.db_path`.
- Not present in CTL historically.
- If you choose to add it to the TOML schema:
  - `maxcfg` can stub it during CTL->TOML conversion, defaulting it to `misc_path`.
  - It should remain optional; runtime fallback remains `misc_path`/`sys_path`.

---

## 3. SQLite open configuration: WAL + busy handling

### 3.1 WAL

On successful open, enable:

- `PRAGMA journal_mode=WAL;`

This improves read concurrency in multi-node deployments.

### 3.2 Busy/locked strategy

Use **both**:

- SQLite busy timeout: **~250ms**
- Bounded retry/backoff in LibMexDB wrapper (for operations that can be safely retried):
  - attempts: **8**
  - backoff: **10ms -> 80ms** (linear or capped exponential)

Implementation details:

- Treat `SQLITE_BUSY` and `SQLITE_LOCKED` as retryable.
- Any other SQLite failure is non-retryable and should immediately set the db error state.

---

## 4. Error handling and error state

### 4.1 Pattern

All intrinsics return *only* success/failure values.

- `0`/`FALSE`/`-1` indicates failure.
- Callers retrieve details via:
  - `db_errcode(db)`
  - `db_errstr(db)`

### 4.2 What stores the error

Implementation should store error state on the **database handle**, not transient cursors/tables, so callers always know where to ask.

This mirrors `libmaxdb`’s model (`maxdb_error(db)` etc.).

### 4.3 Error string lifetime

The error string returned by `db_errstr(db)` should remain valid until the next LibMexDB call that mutates the db error state (or until `db_close(db)`).

Implementation options:

- Store a fixed-size buffer on the db handle (truncate on write), or
- Store a heap-allocated string on the db handle (free/replace on write).

The fixed-size buffer option tends to be simplest and matches the “door code friendliness” goal.

---

## 5. MEX VM integration notes (strings, structs, refs)

These notes are critical for correctness and memory safety.

### 5.1 Strings passed by value vs by reference

MEX intrinsics typically:

- Use `MexArgGetString()` / `MexArgGetNonRefString()` to obtain a **C copy** of a string argument.
- Free the temporary C buffer after use.

### 5.2 Structs passed by reference

Records are passed as `ref struct`. C code reads/writes directly to the struct memory.

### 5.3 Writing string fields into MEX structs

When setting a string field inside a struct from C:

- First call `MexKillStructString()` on the destination string field.
- Then store the new string into VM-managed memory (e.g. via `MexStoreByteStringAt()` / `StoreString`-style helpers used by current intrinsics).

This “kill then store” pattern is used in existing MEX runtime intrinsics (e.g. user export/import paths).

### 5.4 Cursor iteration model is VM-friendly

Returning an array of structs would require allocating and populating MEX heap structures from C with correct lifetime semantics.

The cursor iterator model avoids this by:

- Having the caller allocate the record struct.
- C populates one record per `table_find_next()`.

---

## 6. Schema management and migrations (v1 rules)

### 6.1 Core rule

LibMexDB should be **non-destructive by default**.

- **Add fields** automatically (ALTER TABLE ADD COLUMN).
- **Ignore unknown columns** in the database that aren’t present in the MEX struct schema.
- **Do not drop columns** automatically.

### 6.2 Type changes

Changing types is a migration problem.

- In v1, treat as manual migration (documented limitation).

---

## 7. Type mapping notes (including strings, bool, dates)

### 7.1 `DB_STRING` (clamping + strict)

Two rules were requested:

- **Clamping on insert/update**
  - If caller provides a string longer than the schema size, the stored value is truncated.
- **Strict on read/write**
  - When reading from DB into a struct field with declared max length, enforce that the destination buffer / VM string does not exceed the declared size.

Implementation detail:

- Clamping is easiest at bind time:
  - compute `min(strlen(s), max_len)` and bind only that many bytes.
- On extract, clamp again to schema size.

Additional implementation note (recommended): enforce strictness at the database layer too.

- For `DB_STRING(size=N)` columns, create the column with a `CHECK` constraint:
  - `CHECK(length(colname) <= N)`

This prevents external tools (or future code paths) from inserting oversized strings behind LibMexDB’s back.

### 7.2 `DB_BOOL`

SQLite stores booleans as integers.

- Bind: accept `0/1` (or any int, normalized to 0/1).
- Extract: return `0/1`.

### 7.3 `DB_DATE` / time handling

Decision (v1): store dates as **Unix epoch seconds** in an `INTEGER` column.

Notes:

- This is easy to bind/extract and keeps filters (LT/GT) meaningful.
- If you later need local-time formatting, add helper functions at the MEX layer (or door code) to format/parse.

MEX already has time helpers exposed in headers (e.g. `time()` returning an epoch-like value), so door code can typically do:

- `player_rec.last_login := time();`

If you need more precision later:

- Add `DB_TIME_MS` or use epoch milliseconds, but that is a v2 decision.

---

## 8. Query compilation (non-SQL filter/order)

### 8.1 Predicates

Each `mexdb_pred` compiles to a `WHERE` fragment:

- `field OP ?`

Where `OP` is mapped from `MEXDB_OP_*`.

### 8.2 Field validation

To keep things safe:

- Verify that `pred.field` and `order.field` match known schema fields.
- Reject unknown fields with a clear error in db error state.

### 8.3 Binding values

Bind based on `pred.type`:

- `DB_INT` / `DB_BOOL` / `DB_DATE`: bind int64
- `DB_FLOAT`: bind double
- `DB_STRING`: bind text (with clamping)

### 8.4 Ordering

Ordering compiles to:

- `ORDER BY <field> ASC|DESC`

No raw SQL accepted from door code.

---

## 9. Named statements (“stored procedure” helper)

SQLite doesn’t support stored procedures. The “stored procedure helper” we discussed is simply:

- A **name -> prepared statement** registry on the db handle
- With a controlled, typed parameter binding API from MEX

This provides most of the practical benefits of “procedures” in door code:

- Door author writes SQL once (at startup) and references it by name later
- Statements are cached and reused (performance + consistency)
- Normal door code can still stay “zero SQL” for CRUD/find

### 9.1 Safety constraints (recommended)

To keep this feature from turning into “raw SQL everywhere”, constrain it:

- Only allow SQL to be registered by door code (not user input).
- Only allow a **single statement** per registration (reject strings containing `;` beyond trailing whitespace).
- Prefer allowing only `SELECT`, `INSERT`, `UPDATE`, `DELETE` in v1.
- Optionally disallow schema-changing statements here (DDL) and keep DDL under `db_table()`.

### 9.2 Parameter binding model

Use the same “array + count” calling convention used elsewhere in MEX integration.

Bind parameters positionally:

- `params[1]` binds to SQL parameter `?1`
- `params[2]` binds to SQL parameter `?2`
- etc.

Bind based on `params[i].type`:

- `DB_INT`, `DB_BOOL`, `DB_DATE`: bind as integer (int64 in SQLite)
- `DB_FLOAT`: bind as double
- `DB_STRING`: bind as text (apply clamping if a schema size is known)

If `param_count` does not match `sqlite3_bind_parameter_count(stmt)`, return failure and set db error.

### 9.3 Caching and lifecycle

Registration should ideally compile the SQL immediately (prepare) and cache the resulting `sqlite3_stmt*`.

Execution path:

- `sqlite3_reset(stmt)`
- `sqlite3_clear_bindings(stmt)`
- bind params
- step/exec

All named statements are scoped to the db handle and freed when the db is closed.

If you support re-registering the same name:

- Finalize the old statement, replace with the new one.

### 9.4 Result mapping for `db_proc_query_open()`

`db_proc_query_open()` returns a cursor, and iteration should reuse the same row-to-struct population logic as `table_find_next()`.

Recommended mapping rules:

- For each column in the result row, look up its name via `sqlite3_column_name()`.
- If that name matches a known field in the destination record schema:
  - Convert and store it into the struct field.
- If a column name is unknown:
  - Ignore it.

To avoid “stale fields” when a query returns only a subset of columns, the iterator should set missing schema fields to defaults each row:

- `DB_INT`, `DB_BOOL`, `DB_DATE`: `0`
- `DB_FLOAT`: `0`
- `DB_STRING`: `""` (using the kill-then-store VM pattern)

This makes partial-SELECT queries predictable.

### 9.5 Why we still call it a “helper”

The intent is not to replace the non-SQL query API, but to provide an escape hatch for:

- Aggregate queries (`COUNT(*)` with grouping)
- Bulk updates (`UPDATE ... WHERE ...`)
- Complex sorts/limits

…without forcing LibMexDB v1’s structured query language to grow features it doesn’t need.

---

## 10. Implementation plan (3 phases)

### Phase 1: Core handles + CRUD + path resolution

- Implement `db_open()` / `db_close()` with:
  - path resolution rules (`db_path`/`misc_path`/`sys_path`)
  - WAL enable
  - base busy timeout
  - db error state
- Implement `db_table()`:
  - schema creation
  - add-column migration rule
  - internal schema cache for field validation
- Implement CRUD:
  - `table_insert()` / `table_update()` / `table_save()`
  - `table_get()`
  - `table_delete()` / `table_delete_by_id()`
- Implement `db_errcode()` / `db_errstr()`.

**Exit criteria:** a door can open DB, ensure table, insert/update/get/delete records reliably.

### Phase 2: Cursor queries + filter/order + count

- Implement `table_find_open()` / `table_find_next()` / `table_find_close()`:
  - compile WHERE/ORDER BY
  - bind predicate values
  - populate caller-provided record struct
  - ensure all string fields follow “kill then store” rule
- Implement `table_count()` using the same filter compilation code.

**Exit criteria:** a door can iterate results without leaks or VM corruption; filters/order behave deterministically.

### Phase 3: Named statements + performance hardening

- Implement named statement registration and execution:
  - `db_proc_register()`
  - `db_proc_exec()`
  - `db_proc_query_open()`
- Add statement caching policy + eviction rules (if needed).
- Add consistent retry/backoff wrapper for BUSY/LOCKED.
- Add targeted regression tests (C-side) for:
  - string clamping
  - date comparisons
  - busy retry logic
  - cursor resource cleanup

**Exit criteria:** advanced users can safely use pre-registered statements; performance is stable under multi-node contention.
