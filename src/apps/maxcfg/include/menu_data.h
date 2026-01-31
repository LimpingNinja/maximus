/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * menu_data.h - Menu configuration data structures
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#ifndef MENU_DATA_H
#define MENU_DATA_H

#include <stdbool.h>
#include "prog.h"

typedef struct MaxCfgToml MaxCfgToml;

/* Menu flags (from max.h) */
#define MFLAG_MF_NOVICE   0x0001  /* MenuFile for novice users */
#define MFLAG_MF_REGULAR  0x0002  /* MenuFile for regular users */
#define MFLAG_MF_EXPERT   0x0004  /* MenuFile for expert users */
#define MFLAG_MF_RIP      0x0400  /* MenuFile for RIP users */
#define MFLAG_MF_ALL      (MFLAG_MF_NOVICE | MFLAG_MF_REGULAR | MFLAG_MF_EXPERT)

#define MFLAG_HF_NOVICE   0x0010  /* HeaderFile for novice users */
#define MFLAG_HF_REGULAR  0x0020  /* HeaderFile for regular users */
#define MFLAG_HF_EXPERT   0x0040  /* HeaderFile for expert users */
#define MFLAG_HF_RIP      0x0800  /* HeaderFile for RIP users */
#define MFLAG_HF_ALL      (MFLAG_HF_NOVICE | MFLAG_HF_REGULAR | MFLAG_HF_EXPERT)

#define MFLAG_SILENT      0x1000  /* Silent menu header */

/* Option flags (from max.h) */
#define OFLAG_NODSP       0x0001  /* Don't display on menu (hidden) */
#define OFLAG_CTL         0x0002  /* Produce .CTL file for external command */
#define OFLAG_NOCLS       0x0004  /* Don't clear screen for Display_Menu */
#define OFLAG_THEN        0x0008  /* Execute only if last IF was true */
#define OFLAG_ELSE        0x0010  /* Execute only if last IF was false */
#define OFLAG_ULOCAL      0x0020  /* Local users only */
#define OFLAG_UREMOTE     0x0040  /* Remote users only */
#define OFLAG_REREAD      0x0080  /* Re-read LASTUSER.BBS after external */
#define OFLAG_STAY        0x0100  /* Don't perform menu cleanup */
#define OFLAG_RIP         0x0200  /* RIP callers only */
#define OFLAG_NORIP       0x0400  /* Non-RIP callers only */

/* Area type flags for menu options */
#define ATYPE_NONE        0x00
#define ATYPE_LOCAL       0x01
#define ATYPE_MATRIX      0x02
#define ATYPE_ECHO        0x04
#define ATYPE_CONF        0x08

/* Menu option structure */
typedef struct {
    char *command;           /* Command name (e.g., "Display_Menu") */
    char *arguments;         /* Command arguments (NULL if none) */
    char *priv_level;        /* Privilege level string */
    char *description;       /* Menu option description */
    char *key_poke;          /* Optional key-poke text (quoted) */
    word flags;              /* OFLAG_* flags */
    byte areatype;           /* ATYPE_* flags for area restrictions */
} MenuOption;

/* Menu definition structure */
typedef struct {
    char *name;              /* Menu name (e.g., "MAIN") */
    char *title;             /* Display title */
    char *header_file;       /* HeaderFile path (NULL if none) */
    word header_flags;       /* MFLAG_HF_* flags */
    char *menu_file;         /* MenuFile path (NULL if none) */
    word menu_flags;         /* MFLAG_MF_* flags */
    int menu_length;         /* MenuFile line count (0 if none) */
    int menu_color;          /* AVATAR color (-1 = none) */
    int opt_width;           /* Option width (0 = default 20) */
    
    MenuOption **options;    /* Array of menu option pointers (owned) */
    int option_count;        /* Number of options */
    int option_capacity;     /* Allocated capacity */
} MenuDefinition;

/* Parse menus.ctl and return array of menu definitions */
MenuDefinition **parse_menus_ctl(const char *sys_path, int *menu_count, char *err, size_t err_len);

#if 0
/* Save menu definitions to menus.ctl */
bool save_menus_ctl(const char *sys_path, MenuDefinition **menus, int menu_count, char *err, size_t err_len);
#endif

/* Load menu definitions from TOML files under <systempath>/config/menus/<name>.toml.
 * Returns allocated arrays for menus, paths, and prefixes (same index).
 * Caller frees with free_menu_definitions(menus, count) and free() each string in
 * paths/prefixes plus the arrays.
 */
bool load_menus_toml(MaxCfgToml *toml,
                     const char *sys_path,
                     MenuDefinition ***out_menus,
                     char ***out_paths,
                     char ***out_prefixes,
                     int *out_count,
                     char *err,
                     size_t err_len);

/* Save one menu to a specific TOML file, and reload its prefix into toml.
 * toml_prefix should be of the form "menus.<basename>".
 */
bool save_menu_toml(MaxCfgToml *toml,
                    const char *toml_path,
                    const char *toml_prefix,
                    const MenuDefinition *menu,
                    char *err,
                    size_t err_len);

/* Free menu definitions */
void free_menu_definitions(MenuDefinition **menus, int menu_count);

/* Free a single menu definition */
void free_menu_definition(MenuDefinition *menu);

/* Create a new empty menu definition */
MenuDefinition *create_menu_definition(const char *name);

/* Add an option to a menu */
bool add_menu_option(MenuDefinition *menu, MenuOption *option);

/* Insert an option at a specific position in a menu */
bool insert_menu_option(MenuDefinition *menu, MenuOption *option, int index);

/* Remove an option from a menu */
bool remove_menu_option(MenuDefinition *menu, int index);

/* Create a new empty menu option */
MenuOption *create_menu_option(void);

/* Free a menu option */
void free_menu_option(MenuOption *option);

#endif /* MENU_DATA_H */
