/*
 * libmaxdb - User CRUD operations
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 * https://github.com/LimpingNinja
 */

#include <stdlib.h>
#include <string.h>
#include "db_internal.h"

/* SQL statements */
const char *SQL_INSERT_USER = 
    "INSERT INTO users (id, name, city, alias, phone, dataphone, pwd, pwd_encrypted, "
    "dob_year, dob_month, dob_day, sex, priv, xkeys, xp_priv, xp_date_date, xp_date_time, "
    "xp_mins, xp_flag, times, call, msgs_posted, msgs_read, nup, ndown, ndowntoday, "
    "up, down, downtoday, ludate_date, ludate_time, date_1stcall_date, date_1stcall_time, "
    "date_pwd_chg_date, date_pwd_chg_time, date_newfile_date, date_newfile_time, "
    "time, time_added, timeremaining, video, lang, width, len, help, nulls, def_proto, "
    "compress, lastread_ptr, msg, files, credit, debit, point_credit, point_debit, "
    "bits, bits2, delflag, grp, extra) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
    "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
    "?, ?, ?, ?, ?)";

const char *SQL_INSERT_USER_WITH_ID = 
    "INSERT INTO users (id, name, city, alias, phone, dataphone, pwd, pwd_encrypted, "
    "dob_year, dob_month, dob_day, sex, priv, xkeys, xp_priv, xp_date_date, xp_date_time, "
    "xp_mins, xp_flag, times, call, msgs_posted, msgs_read, nup, ndown, ndowntoday, "
    "up, down, downtoday, ludate_date, ludate_time, date_1stcall_date, date_1stcall_time, "
    "date_pwd_chg_date, date_pwd_chg_time, date_newfile_date, date_newfile_time, "
    "time, time_added, timeremaining, video, lang, width, len, help, nulls, def_proto, "
    "compress, lastread_ptr, msg, files, credit, debit, point_credit, point_debit, "
    "bits, bits2, delflag, grp, extra) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
    "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
    "?, ?, ?, ?, ?)";

const char *SQL_UPDATE_USER = 
    "UPDATE users SET name=?, city=?, alias=?, phone=?, dataphone=?, pwd=?, pwd_encrypted=?, "
    "dob_year=?, dob_month=?, dob_day=?, sex=?, priv=?, xkeys=?, xp_priv=?, xp_date_date=?, "
    "xp_date_time=?, xp_mins=?, xp_flag=?, times=?, call=?, msgs_posted=?, msgs_read=?, "
    "nup=?, ndown=?, ndowntoday=?, up=?, down=?, downtoday=?, ludate_date=?, ludate_time=?, "
    "date_1stcall_date=?, date_1stcall_time=?, date_pwd_chg_date=?, date_pwd_chg_time=?, "
    "date_newfile_date=?, date_newfile_time=?, time=?, time_added=?, timeremaining=?, "
    "video=?, lang=?, width=?, len=?, help=?, nulls=?, def_proto=?, compress=?, "
    "lastread_ptr=?, msg=?, files=?, credit=?, debit=?, point_credit=?, point_debit=?, "
    "bits=?, bits2=?, delflag=?, grp=?, extra=?, updated_at_unix=unixepoch() "
    "WHERE id=?";

const char *SQL_DELETE_USER = "DELETE FROM users WHERE id=?";

const char *SQL_FIND_USER_BY_ID = "SELECT * FROM users WHERE id=?";

const char *SQL_FIND_USER_BY_NAME = "SELECT * FROM users WHERE name=? COLLATE NOCASE";

const char *SQL_FIND_USER_BY_ALIAS = "SELECT * FROM users WHERE alias=? COLLATE NOCASE";

const char *SQL_FIND_ALL_USERS = "SELECT * FROM users ORDER BY id";

const char *SQL_COUNT_USERS = "SELECT COUNT(*) FROM users";

const char *SQL_NEXT_USER_ID =
    "SELECT COALESCE(MAX(id), -1) + 1 FROM users";

const char *SQL_FIND_NEXT_AFTER_ID =
    "SELECT * FROM users WHERE id > ? ORDER BY id ASC LIMIT 1";

const char *SQL_FIND_PREV_BEFORE_ID =
    "SELECT * FROM users WHERE id < ? ORDER BY id DESC LIMIT 1";

/* Helper: Bind MaxDBUser to prepared statement (for INSERT/UPDATE) */
int bind_user_to_stmt(sqlite3_stmt *stmt, const MaxDBUser *user) {
    int idx = 1;
    
    /* For INSERT, bind id first; for UPDATE, id is last */
    int is_update = (strstr(sqlite3_sql(stmt), "UPDATE") != NULL);
    
    if (!is_update) {
        sqlite3_bind_int(stmt, idx++, user->id);
    }
    
    sqlite3_bind_text(stmt, idx++, user->name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, idx++, user->city, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, idx++, user->alias, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, idx++, user->phone, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, idx++, user->dataphone, -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, idx++, user->pwd, sizeof(user->pwd), SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, idx++, user->pwd_encrypted);
    
    sqlite3_bind_int(stmt, idx++, user->dob_year);
    sqlite3_bind_int(stmt, idx++, user->dob_month);
    sqlite3_bind_int(stmt, idx++, user->dob_day);
    sqlite3_bind_int(stmt, idx++, user->sex);
    
    sqlite3_bind_int(stmt, idx++, user->priv);
    sqlite3_bind_int(stmt, idx++, user->xkeys);
    
    sqlite3_bind_int(stmt, idx++, user->xp_priv);
    sqlite3_bind_int(stmt, idx++, user->xp_date.dos_st.date);
    sqlite3_bind_int(stmt, idx++, user->xp_date.dos_st.time);
    sqlite3_bind_int(stmt, idx++, user->xp_mins);
    sqlite3_bind_int(stmt, idx++, user->xp_flag);
    
    sqlite3_bind_int(stmt, idx++, user->times);
    sqlite3_bind_int(stmt, idx++, user->call);
    sqlite3_bind_int(stmt, idx++, user->msgs_posted);
    sqlite3_bind_int(stmt, idx++, user->msgs_read);
    sqlite3_bind_int(stmt, idx++, user->nup);
    sqlite3_bind_int(stmt, idx++, user->ndown);
    sqlite3_bind_int(stmt, idx++, user->ndowntoday);
    sqlite3_bind_int(stmt, idx++, user->up);
    sqlite3_bind_int(stmt, idx++, user->down);
    sqlite3_bind_int(stmt, idx++, user->downtoday);
    
    sqlite3_bind_int(stmt, idx++, user->ludate.dos_st.date);
    sqlite3_bind_int(stmt, idx++, user->ludate.dos_st.time);
    sqlite3_bind_int(stmt, idx++, user->date_1stcall.dos_st.date);
    sqlite3_bind_int(stmt, idx++, user->date_1stcall.dos_st.time);
    sqlite3_bind_int(stmt, idx++, user->date_pwd_chg.dos_st.date);
    sqlite3_bind_int(stmt, idx++, user->date_pwd_chg.dos_st.time);
    sqlite3_bind_int(stmt, idx++, user->date_newfile.dos_st.date);
    sqlite3_bind_int(stmt, idx++, user->date_newfile.dos_st.time);
    
    sqlite3_bind_int(stmt, idx++, user->time);
    sqlite3_bind_int(stmt, idx++, user->time_added);
    sqlite3_bind_int(stmt, idx++, user->timeremaining);
    
    sqlite3_bind_int(stmt, idx++, user->video);
    sqlite3_bind_int(stmt, idx++, user->lang);
    sqlite3_bind_int(stmt, idx++, user->width);
    sqlite3_bind_int(stmt, idx++, user->len);
    sqlite3_bind_int(stmt, idx++, user->help);
    sqlite3_bind_int(stmt, idx++, user->nulls);
    sqlite3_bind_int(stmt, idx++, user->def_proto);
    sqlite3_bind_int(stmt, idx++, user->compress);
    
    sqlite3_bind_int(stmt, idx++, user->lastread_ptr);
    sqlite3_bind_text(stmt, idx++, user->msg, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, idx++, user->files, -1, SQLITE_TRANSIENT);
    
    sqlite3_bind_int(stmt, idx++, user->credit);
    sqlite3_bind_int(stmt, idx++, user->debit);
    sqlite3_bind_int(stmt, idx++, user->point_credit);
    sqlite3_bind_int(stmt, idx++, user->point_debit);
    
    sqlite3_bind_int(stmt, idx++, user->bits);
    sqlite3_bind_int(stmt, idx++, user->bits2);
    sqlite3_bind_int(stmt, idx++, user->delflag);
    sqlite3_bind_int(stmt, idx++, user->group);
    sqlite3_bind_int(stmt, idx++, user->extra);
    
    if (is_update) {
        sqlite3_bind_int(stmt, idx++, user->id);
    }
    
    return SQLITE_OK;
}

/* Helper: Extract MaxDBUser from result row */
MaxDBUser* extract_user_from_stmt(sqlite3_stmt *stmt) {
    MaxDBUser *user;
    const unsigned char *text;
    
    user = calloc(1, sizeof(MaxDBUser));
    if (!user) {
        return NULL;
    }
    
    user->id = sqlite3_column_int(stmt, 0);
    
    text = sqlite3_column_text(stmt, 1);
    if (text) strncpy(user->name, (char*)text, sizeof(user->name) - 1);
    
    text = sqlite3_column_text(stmt, 2);
    if (text) strncpy(user->city, (char*)text, sizeof(user->city) - 1);
    
    text = sqlite3_column_text(stmt, 3);
    if (text) strncpy(user->alias, (char*)text, sizeof(user->alias) - 1);
    
    text = sqlite3_column_text(stmt, 4);
    if (text) strncpy(user->phone, (char*)text, sizeof(user->phone) - 1);
    
    text = sqlite3_column_text(stmt, 5);
    if (text) strncpy(user->dataphone, (char*)text, sizeof(user->dataphone) - 1);
    
    {
        const void *blob = sqlite3_column_blob(stmt, 6);
        int blob_len = sqlite3_column_bytes(stmt, 6);
        if (blob && blob_len > 0) {
            int copy_len = (blob_len < (int)sizeof(user->pwd)) ? blob_len : (int)sizeof(user->pwd);
            memcpy(user->pwd, blob, copy_len);
        }
    }
    
    user->pwd_encrypted = sqlite3_column_int(stmt, 7);
    
    user->dob_year = sqlite3_column_int(stmt, 8);
    user->dob_month = sqlite3_column_int(stmt, 9);
    user->dob_day = sqlite3_column_int(stmt, 10);
    user->sex = sqlite3_column_int(stmt, 11);
    
    user->priv = sqlite3_column_int(stmt, 12);
    user->xkeys = sqlite3_column_int(stmt, 13);
    
    user->xp_priv = sqlite3_column_int(stmt, 14);
    user->xp_date.dos_st.date = sqlite3_column_int(stmt, 15);
    user->xp_date.dos_st.time = sqlite3_column_int(stmt, 16);
    user->xp_mins = sqlite3_column_int(stmt, 17);
    user->xp_flag = sqlite3_column_int(stmt, 18);
    
    user->times = sqlite3_column_int(stmt, 19);
    user->call = sqlite3_column_int(stmt, 20);
    user->msgs_posted = sqlite3_column_int(stmt, 21);
    user->msgs_read = sqlite3_column_int(stmt, 22);
    user->nup = sqlite3_column_int(stmt, 23);
    user->ndown = sqlite3_column_int(stmt, 24);
    user->ndowntoday = sqlite3_column_int(stmt, 25);
    user->up = sqlite3_column_int(stmt, 26);
    user->down = sqlite3_column_int(stmt, 27);
    user->downtoday = sqlite3_column_int(stmt, 28);
    
    user->ludate.dos_st.date = sqlite3_column_int(stmt, 29);
    user->ludate.dos_st.time = sqlite3_column_int(stmt, 30);
    user->date_1stcall.dos_st.date = sqlite3_column_int(stmt, 31);
    user->date_1stcall.dos_st.time = sqlite3_column_int(stmt, 32);
    user->date_pwd_chg.dos_st.date = sqlite3_column_int(stmt, 33);
    user->date_pwd_chg.dos_st.time = sqlite3_column_int(stmt, 34);
    user->date_newfile.dos_st.date = sqlite3_column_int(stmt, 35);
    user->date_newfile.dos_st.time = sqlite3_column_int(stmt, 36);
    
    user->time = sqlite3_column_int(stmt, 37);
    user->time_added = sqlite3_column_int(stmt, 38);
    user->timeremaining = sqlite3_column_int(stmt, 39);
    
    user->video = sqlite3_column_int(stmt, 40);
    user->lang = sqlite3_column_int(stmt, 41);
    user->width = sqlite3_column_int(stmt, 42);
    user->len = sqlite3_column_int(stmt, 43);
    user->help = sqlite3_column_int(stmt, 44);
    user->nulls = sqlite3_column_int(stmt, 45);
    user->def_proto = sqlite3_column_int(stmt, 46);
    user->compress = sqlite3_column_int(stmt, 47);
    
    user->lastread_ptr = sqlite3_column_int(stmt, 48);
    
    text = sqlite3_column_text(stmt, 49);
    if (text) strncpy(user->msg, (char*)text, sizeof(user->msg) - 1);
    
    text = sqlite3_column_text(stmt, 50);
    if (text) strncpy(user->files, (char*)text, sizeof(user->files) - 1);
    
    user->credit = sqlite3_column_int(stmt, 51);
    user->debit = sqlite3_column_int(stmt, 52);
    user->point_credit = sqlite3_column_int(stmt, 53);
    user->point_debit = sqlite3_column_int(stmt, 54);
    
    user->bits = sqlite3_column_int(stmt, 55);
    user->bits2 = sqlite3_column_int(stmt, 56);
    user->delflag = sqlite3_column_int(stmt, 57);
    user->group = sqlite3_column_int(stmt, 58);
    user->extra = sqlite3_column_int(stmt, 59);
    
    user->created_at = sqlite3_column_int64(stmt, 60);
    user->updated_at = sqlite3_column_int64(stmt, 61);
    
    return user;
}

/* Find user by name */
MaxDBUser* maxdb_user_find_by_name(MaxDB *db, const char *name) {
    sqlite3_stmt *stmt;
    MaxDBUser *user = NULL;
    int rc;
    
    if (!db || !db->db || !name) {
        return NULL;
    }
    
    rc = sqlite3_prepare_v2(db->db, SQL_FIND_USER_BY_NAME, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        return NULL;
    }
    
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        user = extract_user_from_stmt(stmt);
    } else if (rc != SQLITE_DONE) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
    }
    
    sqlite3_finalize(stmt);
    return user;
}

/* Find user by alias */
MaxDBUser* maxdb_user_find_by_alias(MaxDB *db, const char *alias) {
    sqlite3_stmt *stmt;
    MaxDBUser *user = NULL;
    int rc;
    
    if (!db || !db->db || !alias) {
        return NULL;
    }
    
    rc = sqlite3_prepare_v2(db->db, SQL_FIND_USER_BY_ALIAS, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        return NULL;
    }
    
    sqlite3_bind_text(stmt, 1, alias, -1, SQLITE_TRANSIENT);
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        user = extract_user_from_stmt(stmt);
    } else if (rc != SQLITE_DONE) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
    }
    
    sqlite3_finalize(stmt);
    return user;
}

/* Find user by ID */
MaxDBUser* maxdb_user_find_by_id(MaxDB *db, int id) {
    sqlite3_stmt *stmt;
    MaxDBUser *user = NULL;
    int rc;
    
    if (!db || !db->db) {
        return NULL;
    }
    
    rc = sqlite3_prepare_v2(db->db, SQL_FIND_USER_BY_ID, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        return NULL;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        user = extract_user_from_stmt(stmt);
    } else if (rc != SQLITE_DONE) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
    }
    
    sqlite3_finalize(stmt);
    return user;
}

/* Create user */
int maxdb_user_create(MaxDB *db, const MaxDBUser *user, int *out_id) {
    MaxDBUser tmp;
    int new_id;
    int rc;
    
    if (!db || !db->db || !user)
        return MAXDB_ERROR;

    rc = maxdb_user_next_id(db, &new_id);
    if (rc != MAXDB_OK)
        return rc;

    memcpy(&tmp, user, sizeof(MaxDBUser));
    tmp.id = new_id;

    rc = maxdb_user_create_with_id(db, &tmp);
    if (rc == MAXDB_OK && out_id)
        *out_id = new_id;

    return rc;
}

/* Create user with explicit ID (legacy record offset compatibility) */
int maxdb_user_create_with_id(MaxDB *db, const MaxDBUser *user) {
    sqlite3_stmt *stmt;
    int rc;
    
    if (!db || !db->db || !user)
        return MAXDB_ERROR;

    rc = sqlite3_prepare_v2(db->db, SQL_INSERT_USER_WITH_ID, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        return MAXDB_ERROR;
    }

    bind_user_to_stmt(stmt, user);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        sqlite3_finalize(stmt);
        if (rc == SQLITE_CONSTRAINT)
            return MAXDB_CONSTRAINT;
        return MAXDB_ERROR;
    }

    sqlite3_finalize(stmt);
    return MAXDB_OK;
}

/* Determine next available legacy-style ID */
int maxdb_user_next_id(MaxDB *db, int *out_id) {
    sqlite3_stmt *stmt;
    int rc;
    int id;
    
    if (!db || !db->db || !out_id)
        return MAXDB_ERROR;

    rc = sqlite3_prepare_v2(db->db, SQL_NEXT_USER_ID, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        return MAXDB_ERROR;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        id = sqlite3_column_int(stmt, 0);
        *out_id = id;
        sqlite3_finalize(stmt);
        return MAXDB_OK;
    }

    maxdb_set_error(db, sqlite3_errmsg(db->db));
    sqlite3_finalize(stmt);
    return MAXDB_ERROR;
}

MaxDBUser* maxdb_user_find_next_after_id(MaxDB *db, int id) {
    sqlite3_stmt *stmt;
    MaxDBUser *user = NULL;
    int rc;
    
    if (!db || !db->db)
        return NULL;

    rc = sqlite3_prepare_v2(db->db, SQL_FIND_NEXT_AFTER_ID, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        user = extract_user_from_stmt(stmt);
    } else if (rc != SQLITE_DONE) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
    }

    sqlite3_finalize(stmt);
    return user;
}

MaxDBUser* maxdb_user_find_prev_before_id(MaxDB *db, int id) {
    sqlite3_stmt *stmt;
    MaxDBUser *user = NULL;
    int rc;
    
    if (!db || !db->db)
        return NULL;

    rc = sqlite3_prepare_v2(db->db, SQL_FIND_PREV_BEFORE_ID, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        user = extract_user_from_stmt(stmt);
    } else if (rc != SQLITE_DONE) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
    }

    sqlite3_finalize(stmt);
    return user;
}

/* Update user */
int maxdb_user_update(MaxDB *db, const MaxDBUser *user) {
    sqlite3_stmt *stmt;
    int rc;
    
    if (!db || !db->db || !user) {
        return MAXDB_ERROR;
    }
    
    rc = sqlite3_prepare_v2(db->db, SQL_UPDATE_USER, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        return MAXDB_ERROR;
    }
    
    bind_user_to_stmt(stmt, user);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        sqlite3_finalize(stmt);
        return MAXDB_ERROR;
    }
    
    if (sqlite3_changes(db->db) == 0) {
        sqlite3_finalize(stmt);
        return MAXDB_NOTFOUND;
    }
    
    sqlite3_finalize(stmt);
    return MAXDB_OK;
}

/* Delete user */
int maxdb_user_delete(MaxDB *db, int id) {
    sqlite3_stmt *stmt;
    int rc;
    
    if (!db || !db->db) {
        return MAXDB_ERROR;
    }
    
    rc = sqlite3_prepare_v2(db->db, SQL_DELETE_USER, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        return MAXDB_ERROR;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        sqlite3_finalize(stmt);
        return MAXDB_ERROR;
    }
    
    if (sqlite3_changes(db->db) == 0) {
        sqlite3_finalize(stmt);
        return MAXDB_NOTFOUND;
    }
    
    sqlite3_finalize(stmt);
    return MAXDB_OK;
}

/* Find all users (returns cursor) */
MaxDBUserCursor* maxdb_user_find_all(MaxDB *db) {
    MaxDBUserCursor *cursor;
    int rc;
    
    if (!db || !db->db) {
        return NULL;
    }
    
    cursor = calloc(1, sizeof(MaxDBUserCursor));
    if (!cursor) {
        return NULL;
    }
    
    cursor->db = db;
    
    rc = sqlite3_prepare_v2(db->db, SQL_FIND_ALL_USERS, -1, &cursor->stmt, NULL);
    if (rc != SQLITE_OK) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        free(cursor);
        return NULL;
    }
    
    return cursor;
}

/* Get next user from cursor */
MaxDBUser* maxdb_user_cursor_next(MaxDBUserCursor *cursor) {
    int rc;
    
    if (!cursor || !cursor->stmt || cursor->done) {
        return NULL;
    }
    
    rc = sqlite3_step(cursor->stmt);
    if (rc == SQLITE_ROW) {
        return extract_user_from_stmt(cursor->stmt);
    }
    
    if (rc != SQLITE_DONE) {
        maxdb_set_error(cursor->db, sqlite3_errmsg(cursor->db->db));
    }
    
    cursor->done = 1;
    return NULL;
}

/* Close cursor */
void maxdb_user_cursor_close(MaxDBUserCursor *cursor) {
    if (!cursor) {
        return;
    }
    
    if (cursor->stmt) {
        sqlite3_finalize(cursor->stmt);
    }
    
    free(cursor);
}

/* Get user count */
int maxdb_user_count(MaxDB *db) {
    sqlite3_stmt *stmt;
    int count = 0;
    int rc;
    
    if (!db || !db->db) {
        return -1;
    }
    
    rc = sqlite3_prepare_v2(db->db, SQL_COUNT_USERS, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        maxdb_set_error(db, sqlite3_errmsg(db->db));
        return -1;
    }
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count;
}

/* Free user object */
void maxdb_user_free(MaxDBUser *user) {
    if (user) {
        free(user);
    }
}
