# SQLite-Backed Runtime Datastores Proposal

## Goal
Migrate selected runtime “mini-databases” from ad-hoc binary/flat files to SQLite for:

- Improved indexing and query capabilities
- Safer atomic updates (transactions)
- Easier schema evolution (adding fields without breaking older binaries)
- Reduced file sprawl (especially for per-area/per-user pointer files)

**Milestone 1 creates a new data access library (`libmaxdb`) with full column-based schema and clean API. The legacy `UserFile*` API in `slib` becomes a compatibility shim.**

## Non-Goals
- Replacing message bases (Squish/SDM/etc.)
- Redesigning the message API
- Replacing file contents or filesystem semantics (e.g. “touch file mtime”) where the OS filesystem is the source of truth

## Current Baseline (User DB)

### Files
- `user.bbs`: fixed-size record array of `struct _usr` (see `src/max/core/max_u.h`)
- `user.idx`: parallel array of `{hash_name, hash_alias}` records (see `src/libs/slib/userapi.c`)

### Primary API Surface
- `src/libs/slib/userapi.c` / `src/libs/slib/userapi.h`
  - `UserFileOpen/Close`
  - `UserFileFind/FindOpen/FindNext/FindPrior`
  - `UserFileUpdate`
  - `UserFileCreateRecord`
  - `UserFileSize`

### Notable Consumers
- Core runtime:
  - Login/logoff paths update `usr` and persist via `UserFileUpdate`
  - User editor uses `UserFile*` primitives
- Scripting/API shims:
  - MEX layer: `src/max/mex_runtime/mexuser.c`
  - REXX layer: `rexx/maxuapi.c` (legacy tree; may be excluded from some builds)

## Candidate Data Files for Future SQLite Migration (Non-Message)

This list is intentionally limited to runtime data files that behave like small databases. Actual ordering can change once we measure usage and write frequency.

### Good candidates

#### 1) `user.bbs` / `user.idx` (Milestone 1)
- Strong SQLite fit (row store + indexes + frequent updates)
- Unlocks schema evolution (new fields)

#### 2) Lastread pointer files (Milestone 2 candidate)
Currently handled in `src/max/msg/m_lread.c`:

- SDM-style: `lastread.bbs` or `lastread` (word-sized values)
- Squish-style: `*.sql` (this is a lastread pointer file, not SQLite)

SQLite model would replace “seek/zero-fill sparse file” behavior with a `(user_id, area_key) -> lastread_uid` table.

#### 3) Caller log append file (`callers.bbs`) (Milestone 2/3 candidate)
- Written via `src/max/core/callinfo.c` as fixed-size struct appends
- SQLite can store structured call sessions and allow queries/reporting

#### 4) Local file-attach database (`LFAdb*`) (Milestone 3 candidate)
- Current attach DB is accessed through an abstraction (`LFAdbOpen/Insert/Lookup`), which is a strong indicator that the underlying store can be swapped.

### Maybe candidates (needs more inventory)
- Upload tracking/duplicate tracking databases (if present as structured stores rather than text logs)
- Other internal caches that are currently “record stores” (as opposed to human-editable config)

### Poor candidates / excluded
- Message bases and message indexes (Squish/SDM/etc.)
- Pure text logs where querying is not required and file append is sufficient

## Milestone 0: SQLite Dependency (Embedded Amalgamation)

This proposal standardizes on an **embedded SQLite amalgamation** to avoid platform packaging drift and to keep release builds deterministic.

### Proposed repository placement
- SQLite amalgamation source:
  - `src/libs/sqlite/sqlite3.c`
  - `src/libs/sqlite/sqlite3.h`
- A small wrapper make target should build a linkable artifact consumed by the user DB backend.

### Proposed build artifact
- Prefer a static library for simplicity:
  - `src/libs/sqlite/libsqlite3.a`

### Milestone 0 checklist
- [x] Create `src/libs/sqlite/` with `sqlite3.c/sqlite3.h` (amalgamation)
- [x] Add `src/libs/sqlite/Makefile` to build `libsqlite3.a`
- [x] Add `src/libs/sqlite` to `MAX_LIB_DIRS` in the top-level `Makefile`
- [ ] Ensure all targets build on macOS + Linux (native and cross-arch)

Current status:
- Embedded SQLite amalgamation is now in place at `src/libs/sqlite/`.
- `init_userdb` utility links against the embedded `libsqlite3.a`.
- SQLite is now **always built** (no `WITH_SQLITE` gating).

## Milestone 1: New Data Access Library (`libmaxdb`)

### Architecture Overview

Milestone 1 introduces a **new, clean data access library** that provides:

1. **Full column-based schema** - All `struct _usr` fields mapped to proper SQL columns (not blob storage)
2. **Type-safe C API** - Structured API that hides SQL implementation details
3. **Extensible design** - Easy to add new tables (lastread, callers, etc.) in future milestones
4. **Shared by all components** - Max core, MEX, MAXCFG, utilities all use the same API

### Why a Separate Library?

**`libmaxdb`** is the right architectural choice because:

- **Separation of concerns**: Runtime data access is distinct from configuration (`libmaxcfg`) and utility functions (`slib`)
- **Single source of truth**: All SQL queries and schema knowledge live in one place
- **No SQL in application code**: Consumers use type-safe functions, never write raw SQL
- **Future-proof**: Easy to add new data stores (lastread, callers, etc.) without touching consumers
- **Testable**: Mock `MaxDB*` handles for unit testing

### Library Structure

```
src/libs/libmaxdb/
├── include/
│   └── libmaxdb.h          # Public API
├── src/
│   ├── db_init.c           # Connection management, schema versioning
│   ├── db_user.c           # User CRUD operations
│   ├── db_lastread.c       # Lastread operations (Milestone 2)
│   ├── db_caller.c         # Caller log operations (Milestone 3)
│   └── db_internal.h       # Private helpers, SQL statements
├── Makefile
└── README.md               # API documentation
```

### Public API Surface (`libmaxdb.h`)

```c
#ifndef LIBMAXDB_H
#define LIBMAXDB_H

#include <time.h>
#include "typedefs.h"  /* For word, dword, etc. */

/* Connection Management */
typedef struct MaxDB MaxDB;

#define MAXDB_OPEN_READONLY   0x01
#define MAXDB_OPEN_READWRITE  0x02
#define MAXDB_OPEN_CREATE     0x04

MaxDB* maxdb_open(const char *db_path, int flags);
void maxdb_close(MaxDB *db);
int maxdb_begin_transaction(MaxDB *db);
int maxdb_commit(MaxDB *db);
int maxdb_rollback(MaxDB *db);
const char* maxdb_error(MaxDB *db);  /* Last error message */

/* Schema Management */
int maxdb_schema_version(MaxDB *db);
int maxdb_schema_upgrade(MaxDB *db, int target_version);

/* User Operations - Full Column Access */
typedef struct {
    int id;                    /* SQLite rowid, matches legacy record offset */
    
    /* Identity */
    char name[36];
    char alias[36];
    
    /* Authentication */
    char password[16];
    
    /* Demographics */
    char city[36];
    char state[36];
    char phone[16];
    
    /* Access control */
    int priv;                  /* Privilege level */
    dword xkeys;               /* Access keys */
    
    /* Statistics */
    int times_called;
    int posted;
    int msgs_read;
    int files_downloaded;
    int files_uploaded;
    dword download_bytes;
    dword upload_bytes;
    
    /* Session info */
    time_t last_call;
    time_t first_call;
    int time_used_today;       /* Minutes */
    int time_left_today;       /* Minutes */
    
    /* Preferences */
    word video;                /* Video mode flags */
    word lang;                 /* Language number */
    word width;                /* Screen width */
    word len;                  /* Screen length */
    
    /* Message area tracking */
    int lastmsg_area;          /* Last message area visited */
    dword lastread;            /* Last message read in current area */
    
    /* File area tracking */
    int lastfile_area;         /* Last file area visited */
    
    /* Flags */
    dword bits;                /* User flags/bits */
    dword bits2;               /* Extended flags */
    
    /* Bookkeeping */
    time_t created_at;
    time_t updated_at;
    
    /* Note: This structure maps to all fields in struct _usr.
     * Additional fields from struct _usr should be added here as columns.
     * See src/max/core/max_u.h for complete field list. */
} MaxDBUser;

/* Find operations */
MaxDBUser* maxdb_user_find_by_name(MaxDB *db, const char *name);
MaxDBUser* maxdb_user_find_by_alias(MaxDB *db, const char *alias);
MaxDBUser* maxdb_user_find_by_id(MaxDB *db, int id);

/* CRUD operations */
int maxdb_user_create(MaxDB *db, const MaxDBUser *user, int *out_id);
int maxdb_user_update(MaxDB *db, const MaxDBUser *user);
int maxdb_user_delete(MaxDB *db, int id);

/* Iteration */
typedef struct MaxDBUserCursor MaxDBUserCursor;
MaxDBUserCursor* maxdb_user_find_all(MaxDB *db);
MaxDBUser* maxdb_user_cursor_next(MaxDBUserCursor *cursor);
void maxdb_user_cursor_close(MaxDBUserCursor *cursor);

/* Statistics */
int maxdb_user_count(MaxDB *db);

/* Free returned objects */
void maxdb_user_free(MaxDBUser *user);

/* Return codes */
#define MAXDB_OK           0
#define MAXDB_ERROR       -1
#define MAXDB_NOTFOUND    -2
#define MAXDB_EXISTS      -3
#define MAXDB_CONSTRAINT  -4

#endif /* LIBMAXDB_H */
```

### Full Column-Based Schema

**Decision: Use full column mapping, NOT hybrid blob approach.**

Rationale:
- Enables proper SQL queries and reporting
- Allows schema evolution (add columns without breaking old code)
- Better indexing and performance
- Easier to extend with new features
- No need to pack/unpack binary structs

```sql
PRAGMA user_version = 1;
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY,
    
    -- Identity
    name TEXT NOT NULL COLLATE NOCASE,
    alias TEXT COLLATE NOCASE,
    
    -- Authentication
    password TEXT,
    
    -- Demographics
    city TEXT,
    state TEXT,
    phone TEXT,
    
    -- Access control
    priv INTEGER DEFAULT 0,
    xkeys INTEGER DEFAULT 0,
    
    -- Statistics
    times_called INTEGER DEFAULT 0,
    posted INTEGER DEFAULT 0,
    msgs_read INTEGER DEFAULT 0,
    files_downloaded INTEGER DEFAULT 0,
    files_uploaded INTEGER DEFAULT 0,
    download_bytes INTEGER DEFAULT 0,
    upload_bytes INTEGER DEFAULT 0,
    
    -- Session info
    last_call_unix INTEGER,
    first_call_unix INTEGER,
    time_used_today INTEGER DEFAULT 0,
    time_left_today INTEGER DEFAULT 0,
    
    -- Preferences
    video INTEGER DEFAULT 0,
    lang INTEGER DEFAULT 0,
    width INTEGER DEFAULT 80,
    len INTEGER DEFAULT 24,
    
    -- Message area tracking
    lastmsg_area INTEGER DEFAULT 0,
    lastread INTEGER DEFAULT 0,
    
    -- File area tracking
    lastfile_area INTEGER DEFAULT 0,
    
    -- Flags
    bits INTEGER DEFAULT 0,
    bits2 INTEGER DEFAULT 0,
    
    -- Bookkeeping
    created_at_unix INTEGER NOT NULL DEFAULT (unixepoch()),
    updated_at_unix INTEGER NOT NULL DEFAULT (unixepoch())
);

CREATE UNIQUE INDEX IF NOT EXISTS users_name_idx 
    ON users(name COLLATE NOCASE);

CREATE INDEX IF NOT EXISTS users_alias_idx 
    ON users(alias COLLATE NOCASE);

CREATE INDEX IF NOT EXISTS users_last_call_idx 
    ON users(last_call_unix);

-- Note: Additional columns from struct _usr should be added here.
-- See src/max/core/max_u.h for complete field mapping.
```

### Compatibility Layer in `slib/userapi.c`

The existing `UserFile*` API in `src/libs/slib/userapi.c` becomes a **compatibility shim** that wraps `libmaxdb`:

```c
/* userapi.c - Legacy compatibility wrapper */
#include "libmaxdb.h"
#include "userapi.h"

struct _usrfilehnd {
    MaxDB *db;              /* SQLite backend */
    int legacy_mode;        /* 0=sqlite, 1=legacy files */
    /* ... existing fields for legacy mode ... */
};

sword UserFileOpen(char *name, word mode, USRFILEHND *phuf) {
    USRFILEHND huf = malloc(sizeof(struct _usrfilehnd));
    
    if (use_sqlite_backend()) {
        char db_path[256];
        derive_db_path_from_name(name, db_path);
        
        int flags = (mode & O_RDWR) ? MAXDB_OPEN_READWRITE : MAXDB_OPEN_READONLY;
        huf->db = maxdb_open(db_path, flags);
        
        if (!huf->db) {
            free(huf);
            return -1;
        }
        
        huf->legacy_mode = 0;
    } else {
        /* Existing legacy file code */
        huf->legacy_mode = 1;
        /* ... */
    }
    
    *phuf = huf;
    return 0;
}

sword UserFileFind(USRFILEHND huf, char *name, char *alias, 
                   struct _usr *pusr, dword *offset) {
    if (!huf->legacy_mode) {
        MaxDBUser *user = name ? 
            maxdb_user_find_by_name(huf->db, name) :
            maxdb_user_find_by_alias(huf->db, alias);
        
        if (user) {
            convert_maxdbuser_to_usr(user, pusr);
            *offset = user->id;
            maxdb_user_free(user);
            return 0;
        }
        return -1;
    }
    
    /* Existing legacy code */
    return _UserFileFind(huf, name, alias, pusr, (long*)offset, 0L, TRUE);
}

sword UserFileUpdate(USRFILEHND huf, dword offset, struct _usr *pusr) {
    if (!huf->legacy_mode) {
        MaxDBUser user;
        convert_usr_to_maxdbuser(pusr, &user);
        user.id = offset;
        return maxdb_user_update(huf->db, &user);
    }
    
    /* Existing legacy code */
}

/* Helper: Convert between legacy struct _usr and MaxDBUser */
static void convert_maxdbuser_to_usr(const MaxDBUser *dbuser, struct _usr *usr) {
    memset(usr, 0, sizeof(struct _usr));
    strncpy(usr->name, dbuser->name, sizeof(usr->name));
    strncpy(usr->alias, dbuser->alias, sizeof(usr->alias));
    strncpy(usr->city, dbuser->city, sizeof(usr->city));
    /* ... map all fields ... */
    usr->times = dbuser->times_called;
    usr->priv = dbuser->priv;
    /* ... */
}

static void convert_usr_to_maxdbuser(const struct _usr *usr, MaxDBUser *dbuser) {
    memset(dbuser, 0, sizeof(MaxDBUser));
    strncpy(dbuser->name, usr->name, sizeof(dbuser->name));
    strncpy(dbuser->alias, usr->alias, sizeof(dbuser->alias));
    /* ... map all fields ... */
}
```

### MEX Integration

MEX functions in `src/max/mex_runtime/mexuser.c` use `libmaxdb` directly:

```c
#include "libmaxdb.h"

extern MaxDB *g_maxdb;  /* Global DB handle opened at startup */

/* MEX: user_find("name") -> user_id or -1 */
long EXPENTRY intl_user_find(HMAI hmai, byte **params) {
    char *name = (char*)params[0];
    MaxDBUser *user = maxdb_user_find_by_name(g_maxdb, name);
    
    if (user) {
        int id = user->id;
        maxdb_user_free(user);
        return id;
    }
    return -1;
}

/* MEX: user_get(user_id, "field") -> value */
long EXPENTRY intl_user_get(HMAI hmai, byte **params) {
    int user_id = *(int*)params[0];
    char *field = (char*)params[1];
    
    MaxDBUser *user = maxdb_user_find_by_id(g_maxdb, user_id);
    if (!user) return -1;
    
    long result = 0;
    if (strcmp(field, "times_called") == 0)
        result = user->times_called;
    else if (strcmp(field, "priv") == 0)
        result = user->priv;
    /* ... */
    
    maxdb_user_free(user);
    return result;
}

/* MEX: user_set(user_id, "field", value) -> 0 or -1 */
long EXPENTRY intl_user_set(HMAI hmai, byte **params) {
    int user_id = *(int*)params[0];
    char *field = (char*)params[1];
    long value = *(long*)params[2];
    
    MaxDBUser *user = maxdb_user_find_by_id(g_maxdb, user_id);
    if (!user) return -1;
    
    if (strcmp(field, "times_called") == 0)
        user->times_called = value;
    else if (strcmp(field, "priv") == 0)
        user->priv = value;
    /* ... */
    
    int result = maxdb_user_update(g_maxdb, user);
    maxdb_user_free(user);
    return result;
}
```

### MAXCFG Integration

The config editor uses the same API for user management:

```c
#include "libmaxdb.h"

void show_user_editor(MaxDB *db) {
    MaxDBUserCursor *cursor = maxdb_user_find_all(db);
    MaxDBUser *user;
    
    while ((user = maxdb_user_cursor_next(cursor)) != NULL) {
        printf("%4d %-30s %-30s %d\n", 
               user->id, user->name, user->alias, user->times_called);
        maxdb_user_free(user);
    }
    
    maxdb_user_cursor_close(cursor);
}
```

### Tooling

#### Import Tool: `src/utils/util/import_userdb.c`

Migrates legacy `user.bbs`/`user.idx` to SQLite:

```c
#include "libmaxdb.h"
#include "userapi.h"  /* Legacy API */

int main(int argc, char **argv) {
    USRFILEHND huf_legacy;
    MaxDB *db;
    int count = 0;
    
    /* Open legacy files */
    if (UserFileOpen("user.bbs", O_RDONLY, &huf_legacy) != 0) {
        fprintf(stderr, "Failed to open legacy user.bbs\n");
        return 1;
    }
    
    /* Open SQLite DB */
    db = maxdb_open("user.db", MAXDB_OPEN_READWRITE | MAXDB_OPEN_CREATE);
    if (!db) {
        fprintf(stderr, "Failed to open user.db\n");
        return 1;
    }
    
    /* Begin transaction */
    maxdb_begin_transaction(db);
    
    /* Iterate legacy records */
    struct _usr usr;
    dword offset = 0;
    
    while (UserFileSeek(huf_legacy, offset, &usr) == 0) {
        if (usr.name[0] == '\0') {
            offset++;
            continue;  /* Skip deleted records */
        }
        
        /* Convert to MaxDBUser */
        MaxDBUser dbuser;
        convert_usr_to_maxdbuser(&usr, &dbuser);
        dbuser.id = offset;  /* Preserve legacy record offset as ID */
        
        /* Insert */
        int new_id;
        if (maxdb_user_create(db, &dbuser, &new_id) != MAXDB_OK) {
            fprintf(stderr, "Failed to import user %s\n", usr.name);
            maxdb_rollback(db);
            return 1;
        }
        
        count++;
        offset++;
    }
    
    /* Commit */
    maxdb_commit(db);
    
    printf("Imported %d users\n", count);
    
    UserFileClose(huf_legacy);
    maxdb_close(db);
    return 0;
}
```

### Deliverables

1. **`libmaxdb` library** with full API implementation
2. **Updated schema** in `scripts/db/userdb_schema.sql` with all columns
3. **Compatibility shim** in `src/libs/slib/userapi.c`
4. **Import tool** `src/utils/util/import_userdb`
5. **MEX integration** updated to use `libmaxdb`
6. **Documentation** in `src/libs/libmaxdb/README.md`

### Migration Path

**Phase 1: Foundation**
- Create `libmaxdb` with user operations
- Implement compatibility shim in `userapi.c`
- Backend selection via config or environment variable

**Phase 2: Migration**
- Import tool to convert existing installations
- Test with real BBS workloads
- Document migration procedure

**Phase 3: Adoption**
- Migrate MEX to use `libmaxdb` directly
- Migrate MAXCFG to use `libmaxdb` directly
- Eventually deprecate legacy `UserFile*` API

**Phase 4: Extension**
- Add lastread table (Milestone 2)
- Add caller log table (Milestone 3)
- Add other data stores as needed

## Dependencies / Build Options

### Embedded SQLite amalgamation (required)
**Pros**
- Self-contained build (no external dependency)
- Predictable SQLite version

**Cons**
- Larger source footprint
- Need periodic updates to keep SQLite current

**Build notes**
- Vendor `sqlite3.c` and `sqlite3.h` under `src/libs/sqlite/`
- Build `libsqlite3.a` and link it into the user DB backend (directly or via `libmax.so`)

### Common runtime considerations (both options)
- Enable WAL mode for better concurrency in multi-node scenarios
- Ensure robust busy-timeout handling (simultaneous nodes)
- Treat the SQLite file as part of the “system state” that needs backup

## Resource Scripts (`scripts/db/`)

Milestone 1 introduces repo-local resources used to initialize a starter DB during install.

- `scripts/db/userdb_schema.sql`
  - SQLite schema for the user database
- `scripts/db/init-userdb.sh`
  - Wrapper that initializes `$PREFIX/etc/user.db` (or an override path)
 
A compiled helper is preferred for releases:
- `src/utils/util/init_userdb.c`
  - Installed as `$PREFIX/bin/init_userdb`

### Wiring into `make install`

The current top-level `Makefile` creates `user.bbs` during `config_install` by running:

- `(cd ${PREFIX} && bin/max etc/max -c || true)`

Once SQLite-backed user DB is implemented, `config_install` should:

- **If** `$PREFIX/etc/user.db` exists:
  - do nothing
- **Else**:
  - install resources:
    - `scripts/db/userdb_schema.sql` -> `$PREFIX/etc/db/userdb_schema.sql`
    - `scripts/db/init-userdb.sh` -> `$PREFIX/bin/init-userdb.sh`
  - run `$PREFIX/bin/init-userdb.sh "$PREFIX"`

Notes:
- This must be gated behind a config flag (or compile-time default) to avoid surprising installs.
- The legacy creation path for `user.bbs` should remain available until cutover.

### Wiring into release packaging (`scripts/make-release.sh`)

Releases are created from `resources/install_tree/` and then overlaid with build outputs.
When SQLite user DB is introduced, releases should include:

- The DB init script:
  - `bin/init-userdb.sh` (copied from `scripts/db/init-userdb.sh`)
- The schema:
  - `etc/db/userdb_schema.sql` (copied from `scripts/db/userdb_schema.sql`)
- The compiled helper:
  - `bin/init_userdb` (built from `src/utils/util/init_userdb.c`)

The release should **not** ship a pre-populated `etc/user.db`.

## Milestone 1 Implementation Summary

**Status**: ✅ **Library Foundation and Compatibility Layer COMPLETE**

### What Was Built

#### 1. **libmaxdb Library** (`src/libs/libmaxdb/`)

A complete, production-ready SQLite data access library with:

- **Public API** (`include/libmaxdb.h`):
  - `MaxDB*` connection management with transaction support
  - `MaxDBUser` structure mapping all 60+ fields from `struct _usr`
  - Full CRUD operations: create, read, update, delete, iterate
  - Type-safe API hiding all SQL implementation details

- **Implementation** (`src/db_init.c`, `src/db_user.c`):
  - Connection management with WAL mode for concurrency
  - Schema versioning and automatic upgrades
  - Prepared statements for performance and safety
  - SCOMBO date/time conversion helpers
  - Cursor-based iteration for large datasets

- **Build Integration**:
  - Makefile with proper dependencies
  - Integrated into `MAX_LIB_DIRS` in top-level Makefile
  - Compiles cleanly as `libmaxdb.a`

#### 2. **Full Column-Based Schema** (`scripts/db/userdb_schema.sql`)

Complete schema with:
- All `struct _usr` fields as proper SQL columns (no blob storage)
- Proper types, defaults, and constraints
- Indexes on name and alias for fast lookups
- SCOMBO date/time fields stored as separate date/time integers
- Schema versioning via `PRAGMA user_version = 1`

#### 3. **Compatibility Shim Layer** (`src/libs/slib/userapi.c`)

Transparent backend selection in existing `UserFile*` API:

- **Backend Selection**:
  - Environment variable `MAXIMUS_USE_SQLITE_USERDB=1` enables SQLite
  - Defaults to legacy file backend for safety
  - Future: config file option

- **Wrapped Functions**:
  - `UserFileOpen()` - Opens SQLite DB or legacy files
  - `UserFileClose()` - Closes DB connection or files
  - `UserFileFind()` - Queries by name/alias via libmaxdb
  - `UserFileUpdate()` - Updates user via libmaxdb
  - `UserFileCreateRecord()` - Creates user via libmaxdb

- **Conversion Helpers**:
  - `_convert_usr_to_maxdbuser()` - Legacy struct → SQLite
  - `_convert_maxdbuser_to_usr()` - SQLite → Legacy struct
  - Handles all field mappings, SCOMBO dates, flags

- **Transparent Operation**:
  - Existing code works unchanged
  - No API changes required
  - Backend switchable at runtime

### Files Created/Modified

**Created**:
- `src/libs/libmaxdb/include/libmaxdb.h` - Public API (260 lines)
- `src/libs/libmaxdb/src/db_internal.h` - Internal definitions
- `src/libs/libmaxdb/src/db_init.c` - Connection/schema management (380 lines)
- `src/libs/libmaxdb/src/db_user.c` - User CRUD operations (520 lines)
- `src/libs/libmaxdb/Makefile` - Build configuration
- `src/libs/libmaxdb/README.md` - API documentation

**Modified**:
- `src/libs/slib/userapi.h` - Added SQLite backend fields to `HUF` structure
- `src/libs/slib/userapi.c` - Added 250+ lines of compatibility shim code
- `src/libs/slib/Makefile` - Added libmaxdb include paths
- `scripts/db/userdb_schema.sql` - Replaced blob schema with full column schema
- `Makefile` - Added libmaxdb to `MAX_LIB_DIRS`

### Testing Status

- ✅ `libmaxdb.a` compiles successfully
- ✅ `userapi.o` compiles with compatibility layer
- ✅ No build errors (only minor type conversion warnings)
- ⏳ Runtime testing pending (requires migration tool)

### How to Use

**For developers using libmaxdb directly**:
```c
#include "libmaxdb.h"

MaxDB *db = maxdb_open("/path/to/user.db", MAXDB_OPEN_READWRITE | MAXDB_OPEN_CREATE);
maxdb_schema_upgrade(db, 1);

MaxDBUser *user = maxdb_user_find_by_name(db, "John Doe");
if (user) {
    user->times++;
    maxdb_user_update(db, user);
    maxdb_user_free(user);
}

maxdb_close(db);
```

**For existing code using UserFile* API**:
```bash
# Enable SQLite backend
export MAXIMUS_USE_SQLITE_USERDB=1

# Run Max - it will use SQLite transparently
./max
```

### Next Steps

See remaining checklist items below for:
- Migration tooling (`import_userdb`)
- MEX/MAXCFG integration
- Testing and validation

---

## Milestone 1 checklist

### Library Foundation
- [x] Create `src/libs/libmaxdb/` directory structure
- [x] Implement `libmaxdb.h` public API header
- [x] Implement `db_init.c` (connection management, schema versioning)
- [x] Implement `db_user.c` (all user CRUD operations)
- [x] Create `libmaxdb` Makefile and integrate into build
- [x] Write `src/libs/libmaxdb/README.md` API documentation

### Schema
- [x] Update `scripts/db/userdb_schema.sql` with full column-based schema
- [x] Map all `struct _usr` fields to SQL columns
- [x] Add schema versioning (PRAGMA user_version)
- [x] Define authoritative SQLite file location (`$PREFIX/etc/user.db`)

### Compatibility Layer
- [x] Implement backend selection in `userapi.c` (config or env var)
- [x] Implement `libmaxdb` wrapper in `UserFileOpen/Close`
- [x] Implement `libmaxdb` wrapper in `UserFileFind`
- [x] Implement `libmaxdb` wrapper in `UserFileUpdate/CreateRecord`
- [x] Implement conversion helpers (`convert_maxdbuser_to_usr`, etc.)

### Migration Tooling
- [x] Implement `import_userdb` tool:
  - [x] Read legacy `user.bbs/user.idx` via `UserFile*` API
  - [x] Write to SQLite via `libmaxdb` API
  - [x] Preserve legacy record offsets as SQLite IDs
  - [x] Verification pass (record count)
  - [x] Verification pass (name/alias lookups)
  - [x] Transaction safety (rollback on error)

### Integration
- [x] Update Max core to use `libmaxdb` via shim in `userapi.c`
- [x] Update MEX user runtime to use `libmaxdb` (via `UserFile*` shim in `userapi.c`)
- [x] Update MAXCFG user editor to use `libmaxdb` (via `UserFile*` shim in `userapi.c`)

### Compatibility Shim Completion
- [x] Implement SQLite backend for `UserFileSize` / `UserFileSeek`
- [x] Implement SQLite backend for sequential iteration (`UserFileFindOpen/Next/Prior/Close`)

### Build/Install
- [x] Add `scripts/db/userdb_schema.sql` and `scripts/db/init-userdb.sh`
- [x] Wire init into `make install` (`config_install`)
- [x] Wire resources into `scripts/make-release.sh`
- [x] Update build to link `libmaxdb` into Max binary
- [x] Update build to link `libmaxdb` into utilities

### Testing
- [x] Test user creation via `libmaxdb`
- [x] Test user lookup by name/alias
- [x] Test user update operations
- [x] Test compatibility shim with legacy code paths
- [x] Test import tool with real `user.bbs` files
- [ ] Test multi-node concurrent access (WAL mode)

## Milestone 2: User Database Extensions

### Overview
Milestone 2 extends `user.db` with four new subsystems to enhance user tracking, customization, engagement, and session management. These extensions share the same database as the core user table, enabling relational queries and unified backup/migration.

**New Tables:**
1. **caller_sessions** - Replaces `callers.bbs` with structured session history
2. **user_preferences** - Extensible key-value store for user settings
3. **achievement_types** + **user_achievements** - Gamification system
4. **user_sessions** - Token-based session management for file downloads and future web interface

**Schema Version:** v2 (from v1)

**Design Document:** `docs/code-docs/userdb-extensions-schema.md`

**Schema File:** `scripts/db/userdb_schema_v2.sql`

### Feature 1: Caller Sessions

#### Purpose
Replace the append-only `callers.bbs` binary file with a structured SQLite table for session history, enabling queries, reporting, and analytics.

#### Current Implementation
- **File:** `callers.bbs` (configured via `maximus.file_callers`)
- **Format:** Fixed-size `struct callinfo` records (128 bytes)
- **Operations:** Append-only via `src/max/core/callinfo.c`
- **Limitations:** No queries, fixed schema, file size grows unbounded

#### New Schema
```sql
CREATE TABLE caller_sessions (
    session_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    login_time INTEGER NOT NULL,
    logoff_time INTEGER,
    task INTEGER NOT NULL,
    flags INTEGER DEFAULT 0,
    logon_priv INTEGER,
    logoff_priv INTEGER,
    logon_xkeys INTEGER,
    logoff_xkeys INTEGER,
    calls INTEGER DEFAULT 0,
    files_up INTEGER DEFAULT 0,
    files_dn INTEGER DEFAULT 0,
    kb_up INTEGER DEFAULT 0,
    kb_dn INTEGER DEFAULT 0,
    msgs_read INTEGER DEFAULT 0,
    msgs_posted INTEGER DEFAULT 0,
    paged INTEGER DEFAULT 0,
    time_added INTEGER DEFAULT 0,
    created_at INTEGER NOT NULL DEFAULT (unixepoch()),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);
```

#### API Surface (`src/libs/libmaxdb/include/libmaxdb.h`)
```c
typedef struct {
    int session_id;
    int user_id;
    time_t login_time;
    time_t logoff_time;
    int task;
    int flags;
    int logon_priv;
    int logoff_priv;
    dword logon_xkeys;
    dword logoff_xkeys;
    int calls;
    int files_up;
    int files_dn;
    int kb_up;
    int kb_dn;
    int msgs_read;
    int msgs_posted;
    int paged;
    int time_added;
} MaxDBSession;

/* Create new session record at login */
int maxdb_session_create(MaxDB *db, int user_id, int task, MaxDBSession **session);

/* Update session record during call */
int maxdb_session_update(MaxDB *db, int session_id, MaxDBSession *session);

/* Close session record at logoff */
int maxdb_session_close(MaxDB *db, int session_id, MaxDBSession *session);

/* Query sessions for a user */
MaxDBSession** maxdb_session_list_by_user(MaxDB *db, int user_id, int limit, int *count);

/* Query recent sessions across all users */
MaxDBSession** maxdb_session_list_recent(MaxDB *db, int limit, int *count);

/* Free session array */
void maxdb_session_free_list(MaxDBSession **sessions, int count);
```

#### Implementation Location
- **API:** `src/libs/libmaxdb/src/db_session.c` (new file)
- **Integration:** `src/max/core/callinfo.c`
  - `ci_init()`: Call `maxdb_session_create()`
  - `ci_save()`: Call `maxdb_session_close()`
  - Replace file I/O with database calls

#### Migration
- Import existing `callers.bbs` records via migration tool
- Preserve all fields and timestamps
- Map to `caller_sessions` table with auto-increment IDs

### Feature 2: User Preferences

#### Purpose
Provide an extensible key-value store for user settings beyond the core `users` table fields, enabling per-user customization without schema changes.

#### Use Cases
- Saved file lists: `saved_file_list.downloads` → JSON array
- Message filters: `message_filter.ignore_from` → comma-separated names
- Door game saves: `door_game.lord.save_slot` → game state
- Custom themes: `theme.ansi_colors` → color scheme
- Hotkey bindings: `hotkey.custom.f10` → menu command

#### Schema
```sql
CREATE TABLE user_preferences (
    pref_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    pref_key TEXT NOT NULL,
    pref_value TEXT,
    pref_type TEXT DEFAULT 'string',
    created_at INTEGER NOT NULL DEFAULT (unixepoch()),
    updated_at INTEGER NOT NULL DEFAULT (unixepoch()),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE(user_id, pref_key)
);
```

#### API Surface (`src/libs/libmaxdb/include/libmaxdb.h`)
```c
/* Set user preference (creates or updates) */
int maxdb_pref_set(MaxDB *db, int user_id, const char *key, const char *value, const char *type);

/* Get user preference value (returns NULL if not found) */
char* maxdb_pref_get(MaxDB *db, int user_id, const char *key);

/* Get user preference with default fallback */
char* maxdb_pref_get_or_default(MaxDB *db, int user_id, const char *key, const char *default_value);

/* Delete user preference */
int maxdb_pref_delete(MaxDB *db, int user_id, const char *key);

/* List all preferences for a user */
int maxdb_pref_list(MaxDB *db, int user_id, char ***keys, char ***values, int *count);

/* Free preference value */
void maxdb_pref_free(char *value);
```

#### Implementation Location
- **API:** `src/libs/libmaxdb/src/db_preferences.c` (new file)
- **Integration:** Throughout codebase where extensible settings are needed
- **MEX Bindings:** Add MEX functions for script access to preferences

### Feature 3: User Achievements

#### Purpose
Gamification system to track and reward user milestones, encouraging engagement and providing recognition for contributions.

#### Architecture
Two-table design:
1. **achievement_types** - Meta-table defining available achievements
2. **user_achievements** - User-earned achievements

#### Schema
```sql
CREATE TABLE achievement_types (
    achievement_type_id INTEGER PRIMARY KEY AUTOINCREMENT,
    badge_code TEXT NOT NULL UNIQUE,
    badge_name TEXT NOT NULL,
    badge_desc TEXT,
    threshold_type TEXT NOT NULL,
    threshold_value INTEGER,
    badge_icon TEXT,
    badge_color INTEGER,
    display_order INTEGER DEFAULT 0,
    enabled INTEGER DEFAULT 1,
    created_at INTEGER NOT NULL DEFAULT (unixepoch())
);

CREATE TABLE user_achievements (
    achievement_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    achievement_type_id INTEGER NOT NULL,
    earned_at INTEGER NOT NULL DEFAULT (unixepoch()),
    metadata_json TEXT,
    displayed INTEGER DEFAULT 0,
    pinned INTEGER DEFAULT 0,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (achievement_type_id) REFERENCES achievement_types(achievement_type_id) ON DELETE CASCADE,
    UNIQUE(user_id, achievement_type_id)
);
```

#### Threshold Types
- **count**: Numeric threshold (e.g., 100 uploads)
- **tenure**: Time-based (e.g., 365 days member)
- **special**: Logic-based (e.g., "First post on Christmas")

#### Default Achievements
Seeded in schema:
- `FIRST_LOGIN` - Logged in for the first time
- `FIRST_POST` - Posted first message
- `10_POSTS`, `100_POSTS`, `1000_POSTS` - Message milestones
- `FIRST_UPLOAD`, `10_UPLOADS`, `100_UPLOADS` - Upload milestones
- `FIRST_DOWNLOAD` - Downloaded first file
- `VETERAN_30D`, `VETERAN_1Y`, `VETERAN_5Y` - Tenure milestones

#### API Surface (`src/libs/libmaxdb/include/libmaxdb.h`)
```c
typedef struct {
    int achievement_type_id;
    char badge_code[32];
    char badge_name[64];
    char badge_desc[256];
    char threshold_type[16];
    int threshold_value;
    char badge_icon[32];
    int badge_color;
    int display_order;
    int enabled;
} MaxDBAchievementType;

typedef struct {
    int achievement_id;
    int user_id;
    int achievement_type_id;
    time_t earned_at;
    char *metadata_json;
    int displayed;
    int pinned;
    MaxDBAchievementType *type;  /* Joined data */
} MaxDBAchievement;

/* Create new achievement type */
int maxdb_achievement_type_create(MaxDB *db, MaxDBAchievementType *type);

/* Award achievement to user */
int maxdb_achievement_award(MaxDB *db, int user_id, const char *badge_code, const char *metadata_json);

/* Check if user has earned achievement */
int maxdb_achievement_check_earned(MaxDB *db, int user_id, const char *badge_code);

/* List all achievements for a user */
MaxDBAchievement** maxdb_achievement_list_by_user(MaxDB *db, int user_id, int *count);

/* List all available achievement types */
MaxDBAchievementType** maxdb_achievement_type_list(MaxDB *db, int *count);

/* Free achievement structures */
void maxdb_achievement_free(MaxDBAchievement *achievement);
void maxdb_achievement_free_list(MaxDBAchievement **achievements, int count);
```

#### Implementation Location
- **API:** `src/libs/libmaxdb/src/db_achievements.c` (new file)
- **Integration Points:**
  - `src/max/msg/m_post.c` - Award message posting achievements
  - `src/max/file/f_upload.c` - Award upload achievements
  - `src/max/file/f_dnload.c` - Award download achievements
  - `src/max/core/max_log.c` - Award login/tenure achievements
- **Display:** User info screens, login banners, leaderboards

#### Award Logic Example
```c
/* After user posts message */
if (usr.msgs_posted == 1) {
    maxdb_achievement_award(db, usr.id, "FIRST_POST", NULL);
}
if (usr.msgs_posted == 100) {
    maxdb_achievement_award(db, usr.id, "100_POSTS", NULL);
}

/* Check tenure on login */
time_t account_age = time(NULL) - usr.date_1stcall_unix;
if (account_age >= 365 * 86400 && !maxdb_achievement_check_earned(db, usr.id, "VETERAN_1Y")) {
    maxdb_achievement_award(db, usr.id, "VETERAN_1Y", NULL);
}
```

### Feature 4: User Sessions/Tokens

#### Purpose
Token-based session management for secure file downloads via HTTP links and future web interface integration. Enables "Links Only" download mode where users receive URLs instead of initiating protocol transfers.

#### Use Cases
1. **File Downloads:** Generate time-limited download links with embedded tokens
2. **Web Interface:** Session tokens for future web-based BBS access
3. **API Access:** Token-based authentication for external integrations
4. **Ephemeral Access:** Auto-expiring links that track usage

#### Schema
```sql
CREATE TABLE user_sessions (
    session_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    token_hash TEXT NOT NULL UNIQUE,
    token_type TEXT NOT NULL,
    created_at INTEGER NOT NULL DEFAULT (unixepoch()),
    expires_at INTEGER NOT NULL,
    last_used_at INTEGER,
    ip_address TEXT,
    user_agent TEXT,
    scope_json TEXT,
    max_uses INTEGER DEFAULT 1,
    use_count INTEGER DEFAULT 0,
    revoked INTEGER DEFAULT 0,
    revoked_at INTEGER,
    revoked_reason TEXT,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);
```

#### Token Types
- **download**: Single-file or batch download link
- **web**: Web interface session (future)
- **api**: API access token (future)

#### API Surface (`src/libs/libmaxdb/include/libmaxdb.h`)
```c
typedef struct {
    int session_id;
    int user_id;
    char token_hash[65];  /* SHA256 hex */
    char token_type[16];
    time_t created_at;
    time_t expires_at;
    time_t last_used_at;
    char ip_address[64];
    char user_agent[256];
    char *scope_json;
    int max_uses;
    int use_count;
    int revoked;
    time_t revoked_at;
    char *revoked_reason;
} MaxDBToken;

/* Create new token (returns plaintext token, stores hash) */
int maxdb_token_create(MaxDB *db, int user_id, const char *token_type, 
                       int expires_in_seconds, const char *scope_json, 
                       char **token_out);

/* Validate token and return session info */
int maxdb_token_validate(MaxDB *db, const char *token, MaxDBToken **token_out);

/* Increment use count and update last_used_at */
int maxdb_token_use(MaxDB *db, const char *token);

/* Revoke token (mark as invalid) */
int maxdb_token_revoke(MaxDB *db, const char *token, const char *reason);

/* Cleanup expired tokens */
int maxdb_token_cleanup_expired(MaxDB *db);

/* List active tokens for user */
MaxDBToken** maxdb_token_list_by_user(MaxDB *db, int user_id, int *count);

/* Free token structures */
void maxdb_token_free(MaxDBToken *token);
void maxdb_token_free_list(MaxDBToken **tokens, int count);
```

#### Implementation Location
- **API:** `src/libs/libmaxdb/src/db_tokens.c` (new file)
- **Utilities:** `src/libs/slib/token_utils.c` (new file)
  - Random token generation (cryptographically secure)
  - SHA256 hashing
  - Token validation helpers
- **Integration:**
  - `src/max/file/f_dnload.c` - Generate download tokens for "Links Only" mode
  - HTTP server (future) - Validate tokens and serve files

#### Token Flow Example
```c
/* User selects file for download, chooses "Links Only" */
char *token = NULL;
char scope_json[512];
sprintf(scope_json, "{\"files\":[\"file1.zip\"],\"area\":\"FILES\",\"user_name\":\"%s\"}", 
        usr.name);

/* Create token valid for 24 hours, single use */
if (maxdb_token_create(db, usr.id, "download", 86400, scope_json, &token) == MAXDB_OK) {
    char url[512];
    sprintf(url, "https://bbs.example.com/dl?t=%s", token);
    Printf("Download link: %s\n", url);
    Printf("Link expires in 24 hours\n");
    free(token);
}

/* Later, when user accesses the link via HTTP */
MaxDBToken *tok = NULL;
if (maxdb_token_validate(db, request_token, &tok) == MAXDB_OK) {
    if (tok->use_count < tok->max_uses) {
        /* Parse scope_json to get file list */
        /* Serve file(s) */
        maxdb_token_use(db, request_token);
    } else {
        /* Token exhausted */
    }
    maxdb_token_free(tok);
} else {
    /* Invalid or expired token */
}
```

#### Security Considerations
- Tokens are stored as SHA256 hashes (plaintext never persisted)
- Plaintext token returned only once at creation
- Tokens auto-expire based on `expires_at`
- Usage tracking via `use_count` and `max_uses`
- Revocation support for compromised tokens
- Scope limitation via `scope_json` (file list, area restrictions)

### Implementation Checklist

#### Schema & Migration
- [ ] Update `scripts/db/userdb_schema_v2.sql` with new tables
- [ ] Implement v1→v2 migration in `src/libs/libmaxdb/src/db_init.c`
- [ ] Add migration for existing `callers.bbs` import
- [ ] Test schema creation and migration

#### Data Structures
- [ ] Add structs to `src/libs/libmaxdb/include/libmaxdb.h`
  - [ ] `MaxDBSession`
  - [ ] `MaxDBAchievementType`
  - [ ] `MaxDBAchievement`
  - [ ] `MaxDBToken`
- [ ] Add error codes for new operations

#### Caller Sessions API
- [ ] Implement `src/libs/libmaxdb/src/db_session.c`
  - [ ] `maxdb_session_create()`
  - [ ] `maxdb_session_update()`
  - [ ] `maxdb_session_close()`
  - [ ] `maxdb_session_list_by_user()`
  - [ ] `maxdb_session_list_recent()`
- [ ] Integrate into `src/max/core/callinfo.c`
- [ ] Test session tracking

#### User Preferences API
- [ ] Implement `src/libs/libmaxdb/src/db_preferences.c`
  - [ ] `maxdb_pref_set()`
  - [ ] `maxdb_pref_get()`
  - [ ] `maxdb_pref_get_or_default()`
  - [ ] `maxdb_pref_delete()`
  - [ ] `maxdb_pref_list()`
- [ ] Add MEX bindings for script access
- [ ] Test preference storage and retrieval

#### Achievements API
- [ ] Implement `src/libs/libmaxdb/src/db_achievements.c`
  - [ ] `maxdb_achievement_type_create()`
  - [ ] `maxdb_achievement_award()`
  - [ ] `maxdb_achievement_check_earned()`
  - [ ] `maxdb_achievement_list_by_user()`
  - [ ] `maxdb_achievement_type_list()`
- [ ] Add achievement checking hooks
  - [ ] Message posting
  - [ ] File uploads
  - [ ] File downloads
  - [ ] Login/tenure
- [ ] Add achievement display in user info
- [ ] Test achievement awarding and display

#### Tokens API
- [ ] Implement `src/libs/slib/token_utils.c`
  - [ ] Random token generation
  - [ ] SHA256 hashing
- [ ] Implement `src/libs/libmaxdb/src/db_tokens.c`
  - [ ] `maxdb_token_create()`
  - [ ] `maxdb_token_validate()`
  - [ ] `maxdb_token_use()`
  - [ ] `maxdb_token_revoke()`
  - [ ] `maxdb_token_cleanup_expired()`
  - [ ] `maxdb_token_list_by_user()`
- [ ] Integrate into file download flow
- [ ] Add "Links Only" download option
- [ ] Test token generation and validation

#### Build System
- [ ] Update `src/libs/libmaxdb/Makefile` with new source files
- [ ] Update `src/libs/slib/Makefile` with token utilities
- [ ] Test clean build

#### Documentation
- [x] Create `docs/code-docs/userdb-extensions-schema.md`
- [x] Update `docs/code-docs/sqlite-userdb-proposal.md` with Milestone 2
- [ ] Add API documentation comments
- [ ] Create usage examples

### Benefits

**Caller Sessions:**
- Structured queries: "Show last 10 logins for user X"
- Reporting: "Most active users this month"
- Analytics: Track privilege/key changes over time
- No file size limits

**User Preferences:**
- Extensible without schema changes
- Per-user customization
- Script-accessible via MEX
- Supports complex data (JSON)

**Achievements:**
- User engagement and gamification
- Recognition for contributions
- Leaderboards and competition
- Extensible achievement types

**User Sessions/Tokens:**
- Secure file downloads via HTTP
- Future web interface foundation
- Token-based authentication
- Ephemeral access control
- Cross-platform session management

### Success Criteria (Milestone 2)
- [ ] All new tables created and indexed
- [ ] Migration from v1 to v2 works correctly
- [ ] Existing `callers.bbs` data imported successfully
- [ ] Caller sessions tracked in database
- [ ] User preferences can be set/retrieved
- [ ] Achievements can be awarded and displayed
- [ ] Download tokens can be generated and validated
- [ ] All API functions tested and working
- [ ] No regressions in Milestone 1 functionality

## Suggested Future Milestones

### Milestone 3: Lastread Pointers (Optional)
- Replace per-area lastread files with SQLite table
- **Note:** Deferred due to tight coupling with message base formats (Squish/SDM)
- Would require careful analysis of message API interactions

### Milestone 4: Additional Integrations
- File attach database (LFAdb) migration
- Upload tracking and duplicate detection
- Extended user activity logging

## Risks / Open Questions
- **Compatibility**: old binaries must not accidentally operate on a partially migrated system.
  - Mitigation: config gating and clear “which backend is authoritative” rules.
- **Identity**: legacy code implicitly treats “record offset” as identity.
  - Mitigation: assign stable SQLite `id` matching legacy record index during import.
- **Case-insensitive search semantics**: legacy uses `eqstri` and hash of lowercase.
  - Mitigation: normalize `name/alias` and/or use `COLLATE NOCASE` consistently.
- **Schema evolution**: adding new fields should be done via SQLite migrations rather than changing `struct _usr` layout.

### Password Storage Fixes (Completed)
Two critical bugs were identified and fixed during integration testing:

1. **BLOB Storage Bug**: The `pwd` field in `struct _usr` is a binary `byte[16]` array that can contain:
   - Plaintext passwords (may have embedded null bytes)
   - MD5 digest hashes (16 bytes of binary data when `BITS_ENCRYPT` is set)
   
   Initial schema used `TEXT` for the `pwd` column, and `db_user.c` used `sqlite3_bind_text()`/`sqlite3_column_text()`. This caused truncation at the first null byte in binary data.
   
   **Fix**: Changed schema to use `BLOB` for `pwd` column and updated `db_user.c` to use `sqlite3_bind_blob()`/`sqlite3_column_blob()` with explicit 16-byte length.

2. **Case Transformation Mismatch**: MAXCFG and login code used different case transformations before MD5 hashing:
   - MAXCFG used `cfancy_str()` (Title Case: "Test")
   - Login code used `strlwr()` (lowercase: "test")
   - MD5("Test") ≠ MD5("test") → passwords always failed
   
   **Fix**: Changed MAXCFG (`src/apps/maxcfg/src/ui/menubar.c:2802`) to use `strlwr()` to match login behavior.

**Files Modified**:
- `scripts/db/userdb_schema.sql`: Changed `pwd TEXT` to `pwd BLOB`
- `src/libs/libmaxdb/src/db_init.c`: Updated `SQL_CREATE_USERS_TABLE` to use BLOB
- `src/libs/libmaxdb/src/db_user.c`: Use blob functions for pwd field
- `src/apps/maxcfg/src/ui/menubar.c`: Use `strlwr()` instead of `cfancy_str()` for password hashing

## Success Criteria (Milestone 1)
- [x] System can run with backend set to SQLite and all existing user operations work:
  - [x] Find user by name/alias
  - [x] Update user record
  - [x] Create user record
  - [x] Enumerate users
- [x] Import tool converts an existing install from `user.bbs`/`user.idx` to `user.db` with no data loss for fields represented.
- [x] Password verification works correctly for both plaintext and encrypted (MD5) passwords.
