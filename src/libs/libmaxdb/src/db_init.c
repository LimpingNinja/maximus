/*
 * libmaxdb - Connection management and schema versioning
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 * https://github.com/LimpingNinja
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db_internal.h"

/* SQL statements for schema creation */
const char *SQL_CREATE_USERS_TABLE = 
    "CREATE TABLE IF NOT EXISTS users ("
    "  id INTEGER PRIMARY KEY,"
    "  name TEXT NOT NULL COLLATE NOCASE,"
    "  city TEXT,"
    "  alias TEXT COLLATE NOCASE,"
    "  phone TEXT,"
    "  dataphone TEXT,"
    "  pwd BLOB,"
    "  pwd_encrypted INTEGER DEFAULT 0,"
    "  dob_year INTEGER,"
    "  dob_month INTEGER,"
    "  dob_day INTEGER,"
    "  sex INTEGER DEFAULT 0,"
    "  priv INTEGER DEFAULT 0,"
    "  xkeys INTEGER DEFAULT 0,"
    "  xp_priv INTEGER DEFAULT 0,"
    "  xp_date_date INTEGER,"
    "  xp_date_time INTEGER,"
    "  xp_mins INTEGER DEFAULT 0,"
    "  xp_flag INTEGER DEFAULT 0,"
    "  times INTEGER DEFAULT 0,"
    "  call INTEGER DEFAULT 0,"
    "  msgs_posted INTEGER DEFAULT 0,"
    "  msgs_read INTEGER DEFAULT 0,"
    "  nup INTEGER DEFAULT 0,"
    "  ndown INTEGER DEFAULT 0,"
    "  ndowntoday INTEGER DEFAULT 0,"
    "  up INTEGER DEFAULT 0,"
    "  down INTEGER DEFAULT 0,"
    "  downtoday INTEGER DEFAULT 0,"
    "  ludate_date INTEGER,"
    "  ludate_time INTEGER,"
    "  date_1stcall_date INTEGER,"
    "  date_1stcall_time INTEGER,"
    "  date_pwd_chg_date INTEGER,"
    "  date_pwd_chg_time INTEGER,"
    "  date_newfile_date INTEGER,"
    "  date_newfile_time INTEGER,"
    "  time INTEGER DEFAULT 0,"
    "  time_added INTEGER DEFAULT 0,"
    "  timeremaining INTEGER DEFAULT 0,"
    "  video INTEGER DEFAULT 0,"
    "  lang INTEGER DEFAULT 0,"
    "  width INTEGER DEFAULT 80,"
    "  len INTEGER DEFAULT 24,"
    "  help INTEGER DEFAULT 0,"
    "  nulls INTEGER DEFAULT 0,"
    "  def_proto INTEGER DEFAULT 0,"
    "  compress INTEGER DEFAULT 0,"
    "  lastread_ptr INTEGER DEFAULT 0,"
    "  msg TEXT,"
    "  files TEXT,"
    "  credit INTEGER DEFAULT 0,"
    "  debit INTEGER DEFAULT 0,"
    "  point_credit INTEGER DEFAULT 0,"
    "  point_debit INTEGER DEFAULT 0,"
    "  bits INTEGER DEFAULT 0,"
    "  bits2 INTEGER DEFAULT 0,"
    "  delflag INTEGER DEFAULT 0,"
    "  grp INTEGER DEFAULT 0,"
    "  extra INTEGER DEFAULT 0,"
    "  created_at_unix INTEGER NOT NULL DEFAULT (unixepoch()),"
    "  updated_at_unix INTEGER NOT NULL DEFAULT (unixepoch())"
    ")";

const char *SQL_CREATE_USERS_NAME_INDEX = 
    "CREATE UNIQUE INDEX IF NOT EXISTS users_name_idx ON users(name COLLATE NOCASE)";

const char *SQL_CREATE_USERS_ALIAS_INDEX = 
    "CREATE INDEX IF NOT EXISTS users_alias_idx ON users(alias COLLATE NOCASE)";

/* Helper functions */
void maxdb_set_error(MaxDB *db, const char *msg) {
    if (db->error_msg) {
        free(db->error_msg);
    }
    db->error_msg = msg ? strdup(msg) : NULL;
}

void maxdb_clear_error(MaxDB *db) {
    if (db->error_msg) {
        free(db->error_msg);
        db->error_msg = NULL;
    }
    db->last_error = MAXDB_OK;
}

/* Open database connection */
MaxDB* maxdb_open(const char *db_path, int flags) {
    MaxDB *db;
    int sqlite_flags = 0;
    int rc;
    
    if (!db_path) {
        return NULL;
    }
    
    db = calloc(1, sizeof(MaxDB));
    if (!db) {
        return NULL;
    }
    
    /* Convert flags */
    if (flags & MAXDB_OPEN_READONLY) {
        sqlite_flags |= SQLITE_OPEN_READONLY;
    }
    if (flags & MAXDB_OPEN_READWRITE) {
        sqlite_flags |= SQLITE_OPEN_READWRITE;
    }
    if (flags & MAXDB_OPEN_CREATE) {
        sqlite_flags |= SQLITE_OPEN_CREATE;
    }
    
    /* Default to read-write if nothing specified */
    if (sqlite_flags == 0) {
        sqlite_flags = SQLITE_OPEN_READWRITE;
    }
    
    rc = sqlite3_open_v2(db_path, &db->db, sqlite_flags, NULL);
    if (rc != SQLITE_OK) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        sqlite3_close(db->db);
        free(db);
        return NULL;
    }
    
    /* Enable WAL mode for better concurrency */
    sqlite3_exec(db->db, "PRAGMA journal_mode=WAL", NULL, NULL, NULL);
    
    /* Enable foreign keys */
    sqlite3_exec(db->db, "PRAGMA foreign_keys=ON", NULL, NULL, NULL);
    
    /* Set busy timeout to 5 seconds for multi-node scenarios */
    sqlite3_busy_timeout(db->db, 5000);
    
    return db;
}

/* Close database connection */
void maxdb_close(MaxDB *db) {
    if (!db) {
        return;
    }
    
    if (db->db) {
        sqlite3_close(db->db);
    }
    
    if (db->error_msg) {
        free(db->error_msg);
    }
    
    free(db);
}

/* Get last error message */
const char* maxdb_error(MaxDB *db) {
    if (!db) {
        return "Invalid database handle";
    }
    return db->error_msg ? db->error_msg : "No error";
}

/* Begin transaction */
int maxdb_begin_transaction(MaxDB *db) {
    int rc;
    
    if (!db || !db->db) {
        return MAXDB_ERROR;
    }
    
    rc = sqlite3_exec(db->db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        return MAXDB_ERROR;
    }
    
    return MAXDB_OK;
}

/* Commit transaction */
int maxdb_commit(MaxDB *db) {
    int rc;
    
    if (!db || !db->db) {
        return MAXDB_ERROR;
    }
    
    rc = sqlite3_exec(db->db, "COMMIT", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        return MAXDB_ERROR;
    }
    
    return MAXDB_OK;
}

/* Rollback transaction */
int maxdb_rollback(MaxDB *db) {
    int rc;
    
    if (!db || !db->db) {
        return MAXDB_ERROR;
    }
    
    rc = sqlite3_exec(db->db, "ROLLBACK", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        return MAXDB_ERROR;
    }
    
    return MAXDB_OK;
}

/* Get schema version */
int maxdb_schema_version(MaxDB *db) {
    sqlite3_stmt *stmt;
    int version = 0;
    int rc;
    
    if (!db || !db->db) {
        return -1;
    }
    
    rc = sqlite3_prepare_v2(db->db, "PRAGMA user_version", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        version = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return version;
}

/* Upgrade schema to target version */
int maxdb_schema_upgrade(MaxDB *db, int target_version) {
    int current_version;
    int rc;
    char version_sql[64];
    
    if (!db || !db->db) {
        return MAXDB_ERROR;
    }
    
    current_version = maxdb_schema_version(db);
    if (current_version < 0) {
        maxdb_set_error(db, "Failed to get current schema version");
        return MAXDB_ERROR;
    }
    
    if (current_version >= target_version) {
        return MAXDB_OK;  /* Already at or above target */
    }
    
    /* Begin transaction for schema upgrade */
    if (maxdb_begin_transaction(db) != MAXDB_OK) {
        return MAXDB_ERROR;
    }
    
    /* Upgrade from version 0 to 1 */
    if (current_version == 0 && target_version >= 1) {
        /* Create users table */
        rc = sqlite3_exec(db->db, SQL_CREATE_USERS_TABLE, NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            maxdb_set_error(db, sqlite3_errmsg(db->db));
            maxdb_rollback(db);
            return MAXDB_ERROR;
        }
        
        /* Create indexes */
        rc = sqlite3_exec(db->db, SQL_CREATE_USERS_NAME_INDEX, NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            maxdb_set_error(db, sqlite3_errmsg(db->db));
            maxdb_rollback(db);
            return MAXDB_ERROR;
        }
        
        rc = sqlite3_exec(db->db, SQL_CREATE_USERS_ALIAS_INDEX, NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            maxdb_set_error(db, sqlite3_errmsg(db->db));
            maxdb_rollback(db);
            return MAXDB_ERROR;
        }
        
        /* Set version to 1 */
        snprintf(version_sql, sizeof(version_sql), "PRAGMA user_version = 1");
        rc = sqlite3_exec(db->db, version_sql, NULL, NULL, NULL);
        if (rc != SQLITE_OK) {
            maxdb_set_error(db, sqlite3_errmsg(db->db));
            maxdb_rollback(db);
            return MAXDB_ERROR;
        }
        
        current_version = 1;
    }
    
    /* Future version upgrades would go here */
    
    /* Commit the upgrade */
    if (maxdb_commit(db) != MAXDB_OK) {
        return MAXDB_ERROR;
    }
    
    return MAXDB_OK;
}

/* Helper: Convert SCOMBO to unix timestamp */
time_t scombo_to_unix(const SCOMBO *sc) {
    struct tm tm_time;
    
    if (!sc || (sc->msg_st.date.da == 0 && sc->msg_st.date.mo == 0 && sc->msg_st.date.yr == 0)) {
        return 0;
    }
    
    memset(&tm_time, 0, sizeof(tm_time));
    tm_time.tm_mday = sc->msg_st.date.da;
    tm_time.tm_mon = sc->msg_st.date.mo - 1;  /* tm_mon is 0-11 */
    tm_time.tm_year = sc->msg_st.date.yr + 80;  /* tm_year is years since 1900, SCOMBO is since 1980 */
    tm_time.tm_hour = sc->msg_st.time.hh;
    tm_time.tm_min = sc->msg_st.time.mm;
    tm_time.tm_sec = sc->msg_st.time.ss << 1;  /* SCOMBO stores seconds/2 */
    
    return mktime(&tm_time);
}

/* Helper: Convert unix timestamp to SCOMBO */
void unix_to_scombo(time_t t, SCOMBO *sc) {
    struct tm *tm_time;
    
    if (!sc) {
        return;
    }
    
    if (t == 0) {
        memset(sc, 0, sizeof(SCOMBO));
        return;
    }
    
    tm_time = localtime(&t);
    if (!tm_time) {
        memset(sc, 0, sizeof(SCOMBO));
        return;
    }
    
    sc->msg_st.date.da = tm_time->tm_mday;
    sc->msg_st.date.mo = tm_time->tm_mon + 1;  /* tm_mon is 0-11 */
    sc->msg_st.date.yr = tm_time->tm_year - 80;  /* tm_year is years since 1900, SCOMBO is since 1980 */
    sc->msg_st.time.hh = tm_time->tm_hour;
    sc->msg_st.time.mm = tm_time->tm_min;
    sc->msg_st.time.ss = tm_time->tm_sec >> 1;  /* SCOMBO stores seconds/2 */
}
