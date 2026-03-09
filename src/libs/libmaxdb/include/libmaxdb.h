/*
 * libmaxdb.h — Public API header for SQLite user DB
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

#ifndef LIBMAXDB_H
#define LIBMAXDB_H

#include <time.h>
#include "typedefs.h"  /* For word, dword, etc. */
#include "stamp.h"     /* For SCOMBO */

/* Connection Management */
typedef struct MaxDB MaxDB;

#define MAXDB_OPEN_READONLY   0x01
#define MAXDB_OPEN_READWRITE  0x02
#define MAXDB_OPEN_CREATE     0x04

/**
 * @brief Open a database connection.
 *
 * @param db_path  Path to the SQLite database file.
 * @param flags    Combination of MAXDB_OPEN_* flags.
 * @return Database handle on success, or NULL on failure.
 */
MaxDB* maxdb_open(const char *db_path, int flags);

/**
 * @brief Close a database connection and free resources.
 *
 * @param db  Database handle (NULL-safe).
 */
void maxdb_close(MaxDB *db);

/**
 * @brief Begin an explicit database transaction.
 *
 * @param db  Database handle.
 * @return MAXDB_OK on success, or MAXDB_ERROR.
 */
int maxdb_begin_transaction(MaxDB *db);

/**
 * @brief Commit the current transaction.
 *
 * @param db  Database handle.
 * @return MAXDB_OK on success, or MAXDB_ERROR.
 */
int maxdb_commit(MaxDB *db);

/**
 * @brief Roll back the current transaction.
 *
 * @param db  Database handle.
 * @return MAXDB_OK on success, or MAXDB_ERROR.
 */
int maxdb_rollback(MaxDB *db);

/**
 * @brief Return the last error message string.
 *
 * @param db  Database handle.
 * @return Error message, or "No error" if none.
 */
const char* maxdb_error(MaxDB *db);

/* Schema Management */

/**
 * @brief Query the current schema version (PRAGMA user_version).
 *
 * @param db  Database handle.
 * @return Schema version number, or -1 on error.
 */
int maxdb_schema_version(MaxDB *db);

/**
 * @brief Upgrade the database schema to the target version.
 *
 * @param db              Database handle.
 * @param target_version  Desired schema version.
 * @return MAXDB_OK on success, or MAXDB_ERROR.
 */
int maxdb_schema_upgrade(MaxDB *db, int target_version);

/* User Operations - Full Column Access */
typedef struct {
    int id;                    /* SQLite rowid, matches legacy record offset */
    
    /* Identity */
    char name[36];
    char city[36];
    char alias[21];
    char phone[15];
    char dataphone[19];
    
    /* Authentication */
    char pwd[16];
    byte pwd_encrypted;        /* BITS_ENCRYPT flag */
    
    /* Demographics */
    word dob_year;             /* Year (1900-) */
    byte dob_month;            /* Month (1-12) */
    byte dob_day;              /* Day (1-31) */
    byte sex;                  /* SEX_MALE, SEX_FEMALE, SEX_UNKNOWN */
    
    /* Access control */
    word priv;                 /* Privilege level */
    dword xkeys;               /* Access keys (32 bits) */
    
    /* Expiry settings */
    word xp_priv;              /* Priv to demote to on expiry */
    SCOMBO xp_date;            /* Expiry date */
    dword xp_mins;             /* Minutes until expiry */
    byte xp_flag;              /* XFLAG_* flags */
    
    /* Statistics */
    word times;                /* Number of previous calls */
    word call;                 /* Number of calls today */
    dword msgs_posted;         /* Total messages posted */
    dword msgs_read;           /* Total messages read */
    dword nup;                 /* Number of files uploaded */
    dword ndown;               /* Number of files downloaded */
    sdword ndowntoday;         /* Number of files downloaded today */
    dword up;                  /* KB uploaded, all calls */
    dword down;                /* KB downloaded, all calls */
    sdword downtoday;          /* KB downloaded today */
    
    /* Session info */
    SCOMBO ludate;             /* Last call date */
    SCOMBO date_1stcall;       /* First call date */
    SCOMBO date_pwd_chg;       /* Last password change date */
    SCOMBO date_newfile;       /* Last new-files check date */
    word time;                 /* Time online today (minutes) */
    word time_added;           /* Time credited for today */
    word timeremaining;        /* Time left for current call */
    
    /* Preferences */
    byte video;                /* Video mode (GRAPH_*) */
    byte lang;                 /* Language number */
    byte width;                /* Screen width */
    byte len;                  /* Screen height */
    byte help;                 /* Help level */
    byte nulls;                /* Nulls after CR */
    sbyte def_proto;           /* Default file transfer protocol */
    byte compress;             /* Default compression program */
    
    /* Message/File area tracking */
    word lastread_ptr;         /* Lastread pointer offset */
    char msg[64];              /* Current message area */
    char files[64];            /* Current file area */
    
    /* Credits/Points */
    word credit;               /* Matrix credit (cents) */
    word debit;                /* Matrix debit (cents) */
    dword point_credit;        /* Total points allocated */
    dword point_debit;         /* Total points used */
    
    /* Flags */
    byte bits;                 /* BITS_* flags */
    word bits2;                /* BITS2_* flags */
    word delflag;              /* UFLAG_* flags */
    
    /* Misc */
    word group;                /* Group number (not implemented) */
    dword extra;               /* Extra field */
    
    /* Bookkeeping */
    time_t created_at;
    time_t updated_at;
    
} MaxDBUser;

/* Find operations */

/** @brief Find a user by exact name (case-insensitive). */
MaxDBUser* maxdb_user_find_by_name(MaxDB *db, const char *name);

/** @brief Find a user by alias (case-insensitive). */
MaxDBUser* maxdb_user_find_by_alias(MaxDB *db, const char *alias);

/** @brief Find a user by numeric ID. */
MaxDBUser* maxdb_user_find_by_id(MaxDB *db, int id);

/* CRUD operations */

/** @brief Create a new user with auto-assigned ID. */
int maxdb_user_create(MaxDB *db, const MaxDBUser *user, int *out_id);

/** @brief Create a new user with an explicit ID (for import/migration). */
int maxdb_user_create_with_id(MaxDB *db, const MaxDBUser *user);

/** @brief Update an existing user record by ID. */
int maxdb_user_update(MaxDB *db, const MaxDBUser *user);

/** @brief Delete a user record by ID. */
int maxdb_user_delete(MaxDB *db, int id);

/* ID helpers */

/** @brief Compute the next available user ID (max + 1). */
int maxdb_user_next_id(MaxDB *db, int *out_id);

/* Navigation */

/** @brief Find the user with the smallest ID greater than the given ID. */
MaxDBUser* maxdb_user_find_next_after_id(MaxDB *db, int id);

/** @brief Find the user with the largest ID less than the given ID. */
MaxDBUser* maxdb_user_find_prev_before_id(MaxDB *db, int id);

/* Iteration */
typedef struct MaxDBUserCursor MaxDBUserCursor;

/** @brief Open a cursor over all users ordered by ID. */
MaxDBUserCursor* maxdb_user_find_all(MaxDB *db);

/** @brief Advance the cursor and return the next user (caller must free). */
MaxDBUser* maxdb_user_cursor_next(MaxDBUserCursor *cursor);

/** @brief Close a user cursor and release its resources. */
void maxdb_user_cursor_close(MaxDBUserCursor *cursor);

/* Statistics */

/** @brief Return the total number of user records. */
int maxdb_user_count(MaxDB *db);

/* Free returned objects */

/** @brief Free a heap-allocated MaxDBUser returned by find/cursor functions. */
void maxdb_user_free(MaxDBUser *user);

/* Return codes */
#define MAXDB_OK           0
#define MAXDB_ERROR       -1
#define MAXDB_NOTFOUND    -2
#define MAXDB_EXISTS      -3
#define MAXDB_CONSTRAINT  -4

#endif /* LIBMAXDB_H */
