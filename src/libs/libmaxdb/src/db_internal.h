/*
 * db_internal.h — Internal database header
 *
 * Copyright 2026 by Kevin Morgan.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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

/** @brief Set the last error message on the database handle. */
void maxdb_set_error(MaxDB *db, const char *msg);

/** @brief Clear the last error message on the database handle. */
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

/**
 * @brief Bind all MaxDBUser fields to a prepared INSERT or UPDATE statement.
 *
 * @param stmt  Prepared statement with positional placeholders.
 * @param user  User record to bind.
 * @return SQLITE_OK on success.
 */
int bind_user_to_stmt(sqlite3_stmt *stmt, const MaxDBUser *user);

/**
 * @brief Extract a MaxDBUser from the current result row.
 *
 * @param stmt  Stepped statement positioned on a row.
 * @return Heap-allocated user (caller must free), or NULL on OOM.
 */
MaxDBUser* extract_user_from_stmt(sqlite3_stmt *stmt);

/**
 * @brief Convert an SCOMBO date/time to a Unix timestamp.
 *
 * @param sc  Source SCOMBO value.
 * @return Unix timestamp, or 0 if sc is NULL/zeroed.
 */
time_t scombo_to_unix(const SCOMBO *sc);

/**
 * @brief Convert a Unix timestamp to an SCOMBO date/time.
 *
 * @param t   Unix timestamp (0 produces a zeroed SCOMBO).
 * @param sc  Output SCOMBO.
 */
void unix_to_scombo(time_t t, SCOMBO *sc);

#endif /* DB_INTERNAL_H */
