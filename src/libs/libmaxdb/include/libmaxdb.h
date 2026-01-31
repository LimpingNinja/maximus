/*
 * libmaxdb - Maximus SQLite Data Access Library
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 * https://github.com/LimpingNinja
 *
 * This library provides a clean, type-safe API for accessing Maximus
 * runtime data stores (users, lastread pointers, caller logs, etc.)
 * backed by SQLite.
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
MaxDBUser* maxdb_user_find_by_name(MaxDB *db, const char *name);
MaxDBUser* maxdb_user_find_by_alias(MaxDB *db, const char *alias);
MaxDBUser* maxdb_user_find_by_id(MaxDB *db, int id);

/* CRUD operations */
int maxdb_user_create(MaxDB *db, const MaxDBUser *user, int *out_id);
int maxdb_user_create_with_id(MaxDB *db, const MaxDBUser *user);
int maxdb_user_update(MaxDB *db, const MaxDBUser *user);
int maxdb_user_delete(MaxDB *db, int id);

/* ID helpers */
int maxdb_user_next_id(MaxDB *db, int *out_id);

/* Navigation */
MaxDBUser* maxdb_user_find_next_after_id(MaxDB *db, int id);
MaxDBUser* maxdb_user_find_prev_before_id(MaxDB *db, int id);

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
