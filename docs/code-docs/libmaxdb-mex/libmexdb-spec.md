# LibMexDB API Specification v1.0

## Overview

LibMexDB provides a dBASE-inspired database abstraction layer for MEX door game development. It eliminates SQL complexity while providing full CRUD operations, queries, and transactions backed by SQLite3.

## Design Philosophy

- **Zero SQL Knowledge Required** - Developers work with structs and arrays
- **Type Safe** - MEX compiler validates all operations at compile time
- **Automatic Schema Management** - Tables created/migrated automatically
- **Transparent Persistence** - Records are just MEX structs with magic
- **Performance First** - Prepared statements, connection pooling, lazy loading

## Core Concepts

### Tables
Tables are defined by schema and accessed via handles. Each table automatically gets an `id` field (auto-increment primary key).

### Records
Records are MEX structs that map 1:1 with database rows. Field access is direct property access.

### Cursors
Queries return cursor handles. Records are retrieved by iterating the cursor and populating a caller-provided record struct (passed by reference).

---

## API Reference

### Database Management

#### `db_open(filename)`
Opens or creates a database file.

**Parameters:**
- `filename` (string) - Path to SQLite database file

**Returns:** Database handle (dword)

**Path resolution:**
- If `filename` is an absolute path, it is used as-is.
- If `filename` is a relative path, LibMexDB resolves it relative to:
  - `maximus.db_path` (if present), else
  - `maximus.misc_path`, else
  - `maximus.sys_path`.

This keeps door code simple while ensuring databases end up under a stable, configurable base directory.

**Example:**
```mex
db := db_open("doors/trivia.db");
```

#### `db_close(db)`
Closes database connection and flushes changes.

**Parameters:**
- `db` (dword) - Database handle

**Example:**
```mex
db_close(db);
```

#### `db_begin(db)`
Starts a transaction. All operations are atomic until `db_commit()` or `db_rollback()`.

**Parameters:**
- `db` (dword) - Database handle

**Example:**
```mex
db_begin(db);
// ... multiple operations ...
db_commit(db);
```

#### `db_commit(db)`
Commits current transaction.

#### `db_rollback(db)`
Rolls back current transaction.

---

### Table Definition

#### `db_table(db, name, fields, field_count)`
Creates or opens a table with specified schema.

**Parameters:**
- `db` (dword) - Database handle
- `name` (string) - Table name
- `fields` (ref array [1..] of struct mexdb_field) - Array of field definitions
- `field_count` (int) - Number of elements in `fields`

```mex
struct mexdb_field {
  string: name;
  int: type;
  int: size;
};
```

**Field Types:**
- `DB_STRING` - Variable-length text (requires size)
- `DB_INT` - 32-bit signed integer
- `DB_FLOAT` - Double-precision float
- `DB_DATE` - Unix timestamp (stored as integer)
- `DB_BOOL` - Boolean (0/1)

**Returns:** Table handle (dword)

**Example:**
```mex
array [1..5] of struct mexdb_field: fields;

fields[1].name := "name";
fields[1].type := DB_STRING;
fields[1].size := 30;

fields[2].name := "score";
fields[2].type := DB_INT;
fields[2].size := 0;

fields[3].name := "level";
fields[3].type := DB_INT;
fields[3].size := 0;

fields[4].name := "active";
fields[4].type := DB_BOOL;
fields[4].size := 0;

fields[5].name := "last_login";
fields[5].type := DB_DATE;
fields[5].size := 0;

players := db_table(db, "players", fields, 5);
```

#### `db_table_exists(db, name)`
Checks if table exists.

**Returns:** Boolean

#### `db_table_drop(table)`
Drops table and all data. **Destructive operation.**

---

### Record Operations

Records are normal MEX structs allocated/owned by the MEX program. LibMexDB operates on records by reference and populates/reads fields in-place.

#### `table_insert(table, rec)`
Inserts `rec` into the database. On success sets `rec.id` to the generated ID.

**Parameters:**
- `table` (dword) - Table handle
- `rec` (ref struct) - Record to insert

**Returns:** Record ID (int), or `-1` on error.

#### `table_update(table, rec)`
Updates an existing row by `rec.id`.

**Parameters:**
- `table` (dword) - Table handle
- `rec` (ref struct) - Record to update

**Returns:** Rows affected (int), or `-1` on error.

#### `table_save(table, rec)`
Smart save: inserts if `rec.id == 0`, otherwise updates.

**Returns:** Record ID (int), or `-1` on error.

#### `table_delete(table, rec)`
Deletes a row by `rec.id`.

**Returns:** Rows affected (int), or `-1` on error.

#### `table_delete_by_id(table, id)`
Deletes a row by ID without loading it.

**Parameters:**
- `table` (dword) - Table handle
- `id` (int) - Record ID

**Returns:** Rows affected (int), or `-1` on error.

---

### Query Operations (Cursor Iterator Model)

LibMexDB provides a non-SQL query API. Filters and ordering are expressed using structs and constants, then compiled internally into a prepared `SELECT`.

#### Comparison and ordering constants

```mex
MEXDB_OP_EQ = 1;
MEXDB_OP_NE = 2;
MEXDB_OP_LT = 3;
MEXDB_OP_LE = 4;
MEXDB_OP_GT = 5;
MEXDB_OP_GE = 6;
MEXDB_OP_LIKE = 7;

MEXDB_LOGIC_AND = 1;
MEXDB_LOGIC_OR  = 2;

MEXDB_ORDER_ASC  = 0;
MEXDB_ORDER_DESC = 1;
```

#### Filter and order structs

```mex
struct mexdb_pred {
  string: field;
  int: op;
  int: type;    // Uses DB_* constants (DB_STRING, DB_INT, DB_FLOAT, DB_DATE, DB_BOOL)
  dword: v_int;
  string: v_str;
};

struct mexdb_order {
  string: field;
  int: dir;
};
```

#### `cursor = table_find_open(table, logic, preds, pred_count, order, limit, offset)`
Creates a cursor for iterating records.

**Parameters:**
- `table` (dword) - Table handle
- `logic` (int) - `MEXDB_LOGIC_AND` or `MEXDB_LOGIC_OR`
- `preds` (ref array [1..] of struct mexdb_pred) - Predicate array
- `pred_count` (int) - Number of predicates in `preds` (0 for no filter)
- `order` (ref struct mexdb_order) - Ordering (or `NULL`)
- `limit` (int) - Max rows (0 for unlimited)
- `offset` (int) - Offset rows (0 for none)

**Returns:** Cursor handle (dword) or `0` on error.

**Example:**
```mex
array [1..1] of struct mexdb_pred: preds;
struct mexdb_order: order;

preds[1].field := "active";
preds[1].op := MEXDB_OP_EQ;
preds[1].type := DB_BOOL;
preds[1].v_int := 1;

order.field := "score";
order.dir := MEXDB_ORDER_DESC;

cursor := table_find_open(players, MEXDB_LOGIC_AND, preds, 1, order, 10, 0);
while (table_find_next(cursor, player_rec))
{
    // ...
}
table_find_close(cursor);
```

#### `ok = table_find_next(cursor, rec)`
Fetches the next row from the cursor and populates `rec`.

**Parameters:**
- `cursor` (dword)
- `rec` (ref struct) - Record to populate

**Returns:** Boolean (true if row was read, false if no more rows or error).

#### `table_find_close(cursor)`
Closes the cursor and releases resources.

#### `ok = table_get(table, id, rec)`
Loads a row by ID into `rec`.

**Returns:** Boolean.

#### `count = table_count(table, filter)`
Counts rows matching `filter`.

**Returns:** Count (int) or `-1` on error.

---

### Named Statements (“Stored Procedures”)

SQLite does not support stored procedures. LibMexDB provides **named statements** as a convenience for advanced use cases that are not covered by the non-SQL query API.

A named statement is a **prepared SQL statement** that is:
- Registered once on a database handle under a symbolic name
- Cached on that database handle
- Executed later by name with a list of parameters

This is intended for **door authors**, not end-users. Door code can safely centralize “advanced SQL” into a few named statements without requiring SQL for normal CRUD and find operations.

#### Parameter struct

Parameters are passed as a fixed-size array plus count (matching typical MEX calling conventions):

```mex
struct mexdb_param {
  int: type;   // Uses DB_* constants (DB_STRING, DB_INT, DB_FLOAT, DB_DATE, DB_BOOL)
  dword: v_int;
  string: v_str;
};
```

#### `ok = db_proc_register(db, name, sql)`
Registers a named SQL statement on the database connection.

The SQL should use positional placeholders (`?`) for parameters.

Named statements are scoped to the database handle and are freed automatically on `db_close(db)`.

#### `rows = db_proc_exec(db, name, params, param_count)`
Executes a registered statement that does not return rows.

#### `cursor = db_proc_query_open(db, name, params, param_count)`
Executes a registered `SELECT` and returns a cursor.

**Example:**
```mex
// Register once
ok := db_proc_register(db, "award_points",
                       "UPDATE players SET score = score + ? WHERE id = ?");
if (ok = 0)
{
    print(db_errstr(db));
    return;
}

// Execute later
array [1..2] of struct mexdb_param: params;

params[1].type := DB_INT;
params[1].v_int := 50;

params[2].type := DB_INT;
params[2].v_int := player_rec.id;

rows := db_proc_exec(db, "award_points", params, 2);
if (rows < 0)
{
    print(db_errstr(db));
}
```

**Example (query cursor):**
```mex
ok := db_proc_register(db, "top_players",
                       "SELECT id, name, score FROM players ORDER BY score DESC LIMIT ?");
if (ok = 0)
{
    print(db_errstr(db));
    return;
}

array [1..1] of struct mexdb_param: params;
params[1].type := DB_INT;
params[1].v_int := 10;

cursor := db_proc_query_open(db, "top_players", params, 1);
if (cursor = 0)
{
    print(db_errstr(db));
    return;
}

while (table_find_next(cursor, player_rec))
{
    // player_rec populated from SELECT columns
}
table_find_close(cursor);
```

---

## Error Handling

LibMexDB uses Pattern 1: operations return values only. On failure they return `0`, `-1`, or `FALSE` and store an error on the database handle.

#### `db_errstr(db)`
Returns the last error message for a database connection.

#### `db_errcode(db)`
Returns the last error code.

```mex
db := db_open("doors/game.db");

cursor := table_find_open(players, MEXDB_LOGIC_AND, preds, 0, order, 0, 0);
if (cursor = 0)
{
    print(db_errstr(db));
    db_close(db);
    return;
}

while (table_find_next(cursor, player_rec))
{
    // ... use player_rec ...
}
table_find_close(cursor);

db_close(db);
```

---

## Performance Guidelines

1. **Use Indexes** - Index fields used in filters and ordering.
2. **Transactions** - Wrap multiple operations in `db_begin()`/`db_commit()`.
3. **Limit Results** - Use `limit`/`offset` on cursors.
4. **Reuse cursors carefully** - Prefer specific filters over scanning the entire table.

---

## Migration Strategy

When schema changes:

1. **Adding Fields** - Automatically added with NULL/default values
2. **Removing Fields** - Old fields ignored, data preserved
3. **Renaming Fields** - Requires manual migration (future: `table.rename_field()`)
4. **Changing Types** - Requires manual migration (future: `table.alter_field()`)

---

## Thread Safety

LibMexDB is **not thread-safe**. Each MEX script instance should have its own database connection. For multi-node BBS:

```mex
// Each node gets own connection
db := db_open("doors/game.db");
// ... operations ...
db_close(db);
```

SQLite handles file locking automatically.

---

## Concurrency and Locking

LibMexDB enables SQLite WAL mode on open.

When SQLite returns `BUSY`/`LOCKED`, LibMexDB uses:

1. A short SQLite busy timeout (default: 250ms)
2. Bounded retries with backoff (default: 8 attempts, 10ms -> 80ms)

The goal is to keep door scripts responsive while still tolerating brief lock contention in multi-node scenarios.

---

## Limitations

- **No Joins** - Use separate queries and manual correlation
- **No Subqueries** - Use multiple queries
- **Limited Filter Operations** - Predicates support basic comparisons and `LIKE` only.
- **Max String Length** - 255 chars (configurable at table creation)
- **No Array Result Sets (v1)** - Queries return cursors; MEX scripts iterate and populate a record struct.

---

## Future Enhancements

- **Raw SQL** - `db.query(sql, params)` for complex queries
- **Migrations** - `table.migrate(new_schema)` for schema changes
- **Relationships** - `has_many()`, `belongs_to()`, `has_one()`
- **Validation** - `table.validate(rules)` for data integrity
- **Callbacks** - `before_insert()`, `after_update()`, etc.
- **Full-Text Search** - `table.search(query)` using SQLite FTS5
- **JSON Fields** - `DB_JSON` type for nested data
- **Soft Deletes** - `deleted_at` timestamp instead of hard delete
- **Timestamps** - Auto `created_at`/`updated_at` fields
- **Pagination** - `table.paginate(page, per_page)`

---

## Implementation Checklist

### Phase 1: Core CRUD
- [ ] `db_open()`, `db_close()`
- [ ] `db_table()` with schema creation
- [ ] `table_insert()`, `table_update()`, `table_delete()`, `table_delete_by_id()`
- [ ] `table_get()`
- [ ] Cursor queries: `table_find_open()`, `table_find_next()`, `table_find_close()`

### Phase 2: Advanced Queries
- [ ] `table_count()`
- [ ] Named statements: `db_proc_register()`, `db_proc_exec()`, `db_proc_query_open()`

### Phase 3: Performance
- [ ] `table_index()` for single fields
- [ ] `table_index_multi()` for composite indexes
- [ ] Prepared statement caching

### Phase 4: Transactions
- [ ] `db_begin()`, `db_commit()`, `db_rollback()`
- [ ] Automatic rollback on error
- [ ] Nested transaction support (savepoints)

### Phase 5: Polish
- [ ] Error messages and codes
- [ ] Schema migration helpers
- [ ] Documentation and examples
- [ ] Performance benchmarks
- [ ] Test suite

---

## License

LibMexDB is released under GPL-2.0 to match Maximus BBS licensing.
