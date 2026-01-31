/*
 * libmaxdb - Internal definitions and helpers
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 * https://github.com/LimpingNinja
 */

#ifndef DB_INTERNAL_H
#define DB_INTERNAL_H

#include "sqlite3.h"
#include "libmaxdb.h"

/* Internal MaxDB structure */
struct MaxDB {
    sqlite3 *db;
    char *error_msg;
    int last_error;
};

/* Internal cursor structure */
struct MaxDBUserCursor {
    MaxDB *db;
    sqlite3_stmt *stmt;
    int done;
};

/* Helper functions */
void maxdb_set_error(MaxDB *db, const char *msg);
void maxdb_clear_error(MaxDB *db);

/* SQL statement strings */
extern const char *SQL_CREATE_USERS_TABLE;
extern const char *SQL_CREATE_USERS_NAME_INDEX;
extern const char *SQL_CREATE_USERS_ALIAS_INDEX;

extern const char *SQL_INSERT_USER;
extern const char *SQL_INSERT_USER_WITH_ID;
extern const char *SQL_UPDATE_USER;
extern const char *SQL_DELETE_USER;
extern const char *SQL_FIND_USER_BY_ID;
extern const char *SQL_FIND_USER_BY_NAME;
extern const char *SQL_FIND_USER_BY_ALIAS;
extern const char *SQL_FIND_NEXT_AFTER_ID;
extern const char *SQL_FIND_PREV_BEFORE_ID;
extern const char *SQL_NEXT_USER_ID;
extern const char *SQL_FIND_ALL_USERS;
extern const char *SQL_COUNT_USERS;

/* Helper: Bind MaxDBUser to prepared statement */
int bind_user_to_stmt(sqlite3_stmt *stmt, const MaxDBUser *user);

/* Helper: Extract MaxDBUser from result row */
MaxDBUser* extract_user_from_stmt(sqlite3_stmt *stmt);

/* Helper: Convert SCOMBO to unix timestamp */
time_t scombo_to_unix(const SCOMBO *sc);

/* Helper: Convert unix timestamp to SCOMBO */
void unix_to_scombo(time_t t, SCOMBO *sc);

#endif /* DB_INTERNAL_H */
