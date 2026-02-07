# LibMexDB C API Design

## Overview

C API layer for MEX database abstraction. Provides SQLite3 backend with dBASE-like simplicity for BBS door game development.

## Core Type Mapping

MEX uses these fundamental types that map to C:
- `byte` (8-bit)
- `word` (16-bit) 
- `dword` (32-bit)
- `string` (IADDR pointer to string data)
- `struct` (TYPEDESC with symbol table)
- `array` (TYPEDESC with bounds + element type)

## C API Structure

### libmexdb_table.h - Database abstraction layer

```c
#include "mex.h"
#include <sqlite3.h>
#include "libmaxdb.h"

// Field type constants (exposed to MEX)
#define DB_STRING  1
#define DB_INT     2
#define DB_FLOAT   3
#define DB_DATE    4
#define DB_BOOL    5

// Database handle (opaque to MEX)
// Owns the sqlite connection and the last error state (Pattern 1).
typedef struct {
    MaxDB *maxdb;
} mexdb_db_t;

// Table handle (opaque to MEX)
// References its owning db and caches prepared statements.
typedef struct {
    mexdb_db_t *db;
    char *table_name;
    int field_count;
    struct {
        char *name;
        int type;
        int size;  // For strings
    } *fields;
    sqlite3_stmt *insert_stmt;
    sqlite3_stmt *update_stmt;
    sqlite3_stmt *delete_stmt;
} mexdb_table_t;

// Cursor handle (opaque to MEX)
// Implements the iterator query model from the spec.
typedef struct {
    mexdb_table_t *table;
    sqlite3_stmt *stmt;
    int done;
} mexdb_cursor_t;

//=============================================================================
// MEX-callable API functions
// These get registered as MEX intrinsics
//=============================================================================

// db_open(filename) -> db handle
// Enables WAL, configures busy handling, and initializes error state.
mexdb_db_t* mex_db_open(const char *filename);

// db_close(db) -> void
void mex_db_close(mexdb_db_t *db);

// db_table(db, name, fields) -> table handle
// fields = array of [name, type, size] arrays
mexdb_table_t* mex_db_table(
    mexdb_db_t *db,
    const char *name,      // From MEX string
    void *fields_array,    // MEX array of arrays
    int field_count
);

// table_insert(table, rec_ref) -> int (id)
int mex_table_insert(mexdb_table_t *table, void *rec_ref);

// table_update(table, rec_ref) -> int (rows)
int mex_table_update(mexdb_table_t *table, void *rec_ref);

// table_delete(table, rec_ref) -> int (rows)
int mex_table_delete(mexdb_table_t *table, void *rec_ref);

// table_delete_by_id(table, id) -> int (rows)
int mex_table_delete_by_id(mexdb_table_t *table, int id);

// table_get(table, id, rec_ref) -> bool
int mex_table_get(mexdb_table_t *table, int id, void *rec_ref);

// Cursor queries
mexdb_cursor_t* mex_table_find_open(
    mexdb_table_t *table,
    void *filter_ref,
    void *order_ref,
    int limit,
    int offset
);

int mex_table_find_next(mexdb_cursor_t *cur, void *rec_ref);
void mex_table_find_close(mexdb_cursor_t *cur);

// Named statements ("stored procedure" helpers)
int mex_db_proc_register(mexdb_db_t *db, const char *name, const char *sql);
int mex_db_proc_exec(mexdb_db_t *db, const char *name, void *params_ref);
mexdb_cursor_t* mex_db_proc_query_open(mexdb_db_t *db, const char *name, void *params_ref);

//=============================================================================
// MEX VM Integration Notes
//=============================================================================
//
// LibMexDB intrinsics should use the existing MEX runtime helpers:
// - MexArgBegin / MexArgGetString / MexArgGetWord / MexArgGetDword / MexArgGetRef
// - MexKillString
// - MexKillStructString + StoreString(...) for writing struct string fields
// - MexDupVMString(...) for reading strings from VM/struct fields
// - MexReturnString / MexReturnStringBytes for returning strings
//
// Intrinsics are registered via the existing intrinsic dispatch table (see mex.c).
```

## Implementation Example

### libmexdb_table.c

```c
mexdb_db_t* mex_db_open(const char *filename) {
    mexdb_db_t *db = calloc(1, sizeof(*db));
    if (!db) return NULL;

    db->maxdb = maxdb_open(filename, 0);
    if (!db->maxdb) {
        free(db);
        return NULL;
    }

    // Ensure WAL is enabled (libmaxdb already does this).
    // Busy handling is configured with a short busy timeout plus bounded retries/backoff.
    // Note: the LibMexDB helper layer should live inside the libmaxdb source tree (or as a tightly-coupled sibling)
    // so it can access MaxDB internals (including the underlying sqlite3* and last-error state) without expanding
    // libmaxdb's public API.
    return db;
}

mexdb_table_t* mex_db_table(mexdb_db_t *db, const char *name, void *fields_array, int field_count) {
    mexdb_table_t *tbl = calloc(1, sizeof(*tbl));
    if (!tbl) return NULL;
    tbl->db = db;
    tbl->table_name = strdup(name);
    tbl->field_count = field_count;
    tbl->fields = calloc(field_count, sizeof(*tbl->fields));
    
    // Parse MEX array of field definitions
    // fields_array points to MEX array: [[name, type, size], ...]
    for (int i = 0; i < field_count; i++) {
        void *field_def = get_array_element(fields_array, i);
        tbl->fields[i].name = get_array_string(field_def, 0);
        tbl->fields[i].type = get_array_int(field_def, 1);
        tbl->fields[i].size = get_array_int(field_def, 2);
    }
    
    // Generate CREATE TABLE IF NOT EXISTS + ALTER TABLE ADD COLUMN for missing fields.
    char sql[4096];
    sprintf(sql, "CREATE TABLE IF NOT EXISTS %s (id INTEGER PRIMARY KEY", name);
    for (int i = 0; i < field_count; i++) {
        strcat(sql, ", ");
        strcat(sql, tbl->fields[i].name);
        switch (tbl->fields[i].type) {
            case DB_STRING: sprintf(sql + strlen(sql), " TEXT"); break;
            case DB_INT: strcat(sql, " INTEGER"); break;
            case DB_FLOAT: strcat(sql, " REAL"); break;
            // ...
        }
    }
    strcat(sql, ")");
    
    // Execute schema SQL against the underlying sqlite3 connection owned by MaxDB.
    sqlite3_exec(/* sqlite3* from tbl->db->maxdb */, sql, NULL, NULL, NULL);
    
    // Prepare statements (cached per table)
    // INSERT/UPDATE/DELETE statements are generated from the field list.
    
    return tbl;
}

int mex_table_insert(mexdb_table_t *table, void *rec_ref) {
    // Bind fields from the caller-provided MEX struct and execute cached INSERT.
    // String fields must be read via VM helpers (e.g. MexDupVMString).
    // On success, set rec.id using StoreString/MexStore* APIs as appropriate.
    return 0;
}

mexdb_cursor_t* mex_table_find_open(mexdb_table_t *table, void *filter_ref, void *order_ref, int limit, int offset) {
    // Compile filter/order structs into a parameterized SELECT, prepare a statement, and return a cursor.
    return NULL;
}

int mex_table_find_next(mexdb_cursor_t *cur, void *rec_ref) {
    // sqlite3_step + copy row columns into the provided MEX record struct.
    // For struct string fields: MexKillStructString + StoreString.
    return 0;
}

void mex_table_find_close(mexdb_cursor_t *cur) {
    // Finalize stmt and free cursor.
}
```

## Key Points

1. **DB owns connection** - `mexdb_db_t` owns the SQLite connection (via `MaxDB`). Tables reference the db.
2. **Cursor iterator model** - Queries return cursors, and rows are copied into a caller-provided record struct.
3. **No C record/resultset objects** - The record is a MEX struct, managed by the script. LibMexDB populates it in-place.
4. **Real string lifetime rules** - Reading strings requires VM-aware helpers; writing struct string fields requires `MexKillStructString` + `StoreString`.
5. **Pattern 1 errors** - Return values only; call `db_errstr(db)` / `db_errcode(db)` for details.
6. **WAL + busy strategy** - Enable WAL. Use a short busy timeout plus bounded retries/backoff for `BUSY`/`LOCKED`.

The MEX side would look like the original design, but the C layer handles all SQL generation and type marshaling transparently.
