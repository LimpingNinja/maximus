/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * area_parse.h - Message and file area CTL parser
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#ifndef AREA_PARSE_H
#define AREA_PARSE_H

#include <stdbool.h>
#include "treeview.h"

/* Message area style flags */
typedef enum {
    MSGSTYLE_SQUISH     = 0x0001,
    MSGSTYLE_DOTMSG     = 0x0002,
    MSGSTYLE_LOCAL      = 0x0004,
    MSGSTYLE_NET        = 0x0008,
    MSGSTYLE_ECHO       = 0x0010,
    MSGSTYLE_CONF       = 0x0020,
    MSGSTYLE_PVT        = 0x0040,
    MSGSTYLE_PUB        = 0x0080,
    MSGSTYLE_HIBIT      = 0x0100,
    MSGSTYLE_ANON       = 0x0200,
    MSGSTYLE_NORNK      = 0x0400,
    MSGSTYLE_REALNAME   = 0x0800,
    MSGSTYLE_ALIAS      = 0x1000,
    MSGSTYLE_AUDIT      = 0x2000,
    MSGSTYLE_READONLY   = 0x4000,
    MSGSTYLE_HIDDEN     = 0x8000,
    MSGSTYLE_ATTACH     = 0x10000,
    MSGSTYLE_NOMAILCHK  = 0x20000
} MsgStyleFlags;

/* Message area data */
typedef struct {
    char *name;           /* Area name (e.g., "MUF" or "2") */
    char *tag;            /* EchoMail tag */
    char *path;           /* Message base path */
    char *desc;           /* Description */
    char *acs;            /* Access control string */
    char *owner;          /* Default owner */
    char *origin;         /* Custom origin line */
    char *attachpath;     /* File attachment path */
    char *barricade;      /* Barricade file */
    char *menuname;       /* Alternate menu */
    unsigned int style;   /* Style flags */
    int renum_max;        /* Max messages (0 = not set) */
    int renum_days;       /* Max days (0 = not set) */
} MsgAreaData;

typedef struct {
    char *acs;
    char *display_file;
} DivisionData;

/* File area data */
typedef struct {
    char *name;           /* Area name */
    char *desc;           /* Description */
    char *acs;            /* Access control string */
    char *download;       /* Download path */
    char *upload;         /* Upload path */
    char *filelist;       /* Custom FILES.BBS path */
    char *barricade;      /* Barricade file */
    char *menuname;       /* Alternate menu */
    bool type_slow;       /* Slow access medium */
    bool type_staged;     /* Use staging */
    bool type_nonew;      /* Exclude from new file checks */
} FileAreaData;

/* Parse msgarea.ctl and build tree structure
 * Returns array of root nodes and sets *count
 */
TreeNode **parse_msgarea_ctl(const char *sys_path, int *count, char *err, size_t err_sz);

/* Parse filearea.ctl and build tree structure
 * Returns array of root nodes and sets *count
 */
TreeNode **parse_filearea_ctl(const char *sys_path, int *count, char *err, size_t err_sz);

#if 0
/* Save msgarea.ctl from tree structure
 * Creates .bak backup if it doesn't exist
 * Returns true on success
 */
bool save_msgarea_ctl(const char *sys_path, TreeNode **roots, int count, char *err, size_t err_sz);

/* Save filearea.ctl from tree structure
 * Creates .bak backup if it doesn't exist
 * Returns true on success
 */
bool save_filearea_ctl(const char *sys_path, TreeNode **roots, int count, char *err, size_t err_sz);
#endif

/* Free area data structures */
void msgarea_data_free(MsgAreaData *data);
void filearea_data_free(FileAreaData *data);

void division_data_free(DivisionData *data);

void free_msg_tree(TreeNode **roots, int count);
void free_file_tree(TreeNode **roots, int count);

#endif /* AREA_PARSE_H */
