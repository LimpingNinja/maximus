# libmaxdb - Maximus SQLite Data Access Library

## Overview

`libmaxdb` provides a clean, type-safe C API for accessing Maximus runtime data stores backed by SQLite. This library abstracts all SQL operations and provides structured access to user records, with future support for lastread pointers, caller logs, and other data stores.

## Architecture

- **Full column-based schema**: All `struct _usr` fields mapped to proper SQL columns
- **Type-safe API**: No raw SQL in application code
- **Transaction support**: ACID guarantees for data integrity
- **Multi-node safe**: WAL mode with busy timeout for concurrent access
- **Extensible**: Easy to add new tables and operations

## API Usage

### Opening a Database

```c
#include "libmaxdb.h"

MaxDB *db = maxdb_open("/path/to/user.db", 
                       MAXDB_OPEN_READWRITE | MAXDB_OPEN_CREATE);
if (!db) {
    fprintf(stderr, "Failed to open database\n");
    return 1;
}
```

### Finding Users

```c
/* Find by name */
MaxDBUser *user = maxdb_user_find_by_name(db, "John Doe");
if (user) {
    printf("User ID: %d, Calls: %d\n", user->id, user->times);
    maxdb_user_free(user);
}

/* Find by alias */
user = maxdb_user_find_by_alias(db, "JDoe");

/* Find by ID */
user = maxdb_user_find_by_id(db, 1);
```

### Creating Users

```c
MaxDBUser new_user = {0};
strncpy(new_user.name, "Jane Smith", sizeof(new_user.name));
strncpy(new_user.city, "New York", sizeof(new_user.city));
new_user.priv = 100;
new_user.width = 80;
new_user.len = 24;

int new_id;
if (maxdb_user_create(db, &new_user, &new_id) == MAXDB_OK) {
    printf("Created user with ID: %d\n", new_id);
}
```

### Updating Users

```c
MaxDBUser *user = maxdb_user_find_by_name(db, "Jane Smith");
if (user) {
    user->times++;
    user->msgs_read += 5;
    
    if (maxdb_user_update(db, user) == MAXDB_OK) {
        printf("User updated\n");
    }
    
    maxdb_user_free(user);
}
```

### Iterating All Users

```c
MaxDBUserCursor *cursor = maxdb_user_find_all(db);
MaxDBUser *user;

while ((user = maxdb_user_cursor_next(cursor)) != NULL) {
    printf("%4d %-30s %d calls\n", user->id, user->name, user->times);
    maxdb_user_free(user);
}

maxdb_user_cursor_close(cursor);
```

### Transactions

```c
maxdb_begin_transaction(db);

/* Multiple operations */
maxdb_user_create(db, &user1, NULL);
maxdb_user_create(db, &user2, NULL);

if (error_occurred) {
    maxdb_rollback(db);
} else {
    maxdb_commit(db);
}
```

### Error Handling

```c
if (maxdb_user_create(db, &user, &id) != MAXDB_OK) {
    fprintf(stderr, "Error: %s\n", maxdb_error(db));
}
```

### Closing Database

```c
maxdb_close(db);
```

## Return Codes

- `MAXDB_OK` (0) - Success
- `MAXDB_ERROR` (-1) - General error
- `MAXDB_NOTFOUND` (-2) - Record not found
- `MAXDB_EXISTS` (-3) - Record already exists
- `MAXDB_CONSTRAINT` (-4) - Constraint violation

## Schema

The user table maps all fields from `struct _usr` (see `src/max/core/max_u.h`):

- **Identity**: name, city, alias, phone, dataphone
- **Authentication**: pwd, pwd_encrypted
- **Demographics**: dob_year, dob_month, dob_day, sex
- **Access control**: priv, xkeys
- **Statistics**: times, call, msgs_posted, msgs_read, uploads/downloads
- **Session info**: last call dates, time online
- **Preferences**: video mode, language, screen dimensions
- **Area tracking**: lastread_ptr, current msg/file areas
- **Credits/Points**: credit, debit, point_credit, point_debit
- **Flags**: bits, bits2, delflag

## Building

The library is built as part of the main Maximus build:

```bash
make build
```

The static library `libmaxdb.a` will be created and linked into the Max binary and utilities.

## Integration with Legacy Code

Legacy code using `UserFile*` API in `src/libs/slib/userapi.c` can continue to work through a compatibility shim that wraps `libmaxdb`. New code should use `libmaxdb` directly.

## Future Extensions

- **Milestone 2**: Lastread pointer table
- **Milestone 3**: Caller log table
- Additional data stores as needed

## Thread Safety

`libmaxdb` uses SQLite's default serialized threading mode. Each `MaxDB*` handle should be used by a single thread, but multiple handles can be opened to the same database file safely (WAL mode enables concurrent readers and a single writer).
