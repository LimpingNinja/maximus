/*
 * maxcfg.h — Main header for Maximus Configuration Editor
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

#ifndef MAXCFG_H
#define MAXCFG_H

/*
 * ncurses exposes wide-character APIs/types (cchar_t, mvadd_wch, setcchar)
 * only when X/Open extended curses is enabled.
 */
#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED 1
#endif

#if defined(__has_include)
#  if __has_include(<ncursesw/curses.h>)
#    include <ncursesw/curses.h>
#  elif __has_include(<ncursesw/ncurses.h>)
#    include <ncursesw/ncurses.h>
#  elif __has_include(<ncurses.h>)
#    include <ncurses.h>
#  else
#    include <curses.h>
#  endif
#else
#  include <ncurses.h>
#endif
#include <stdbool.h>
#include <limits.h>
#include "libmaxcfg.h"

#define MAXCFG_VERSION "1.0"
#define MAXCFG_TITLE   "MAXCFG v" MAXCFG_VERSION

/* Default paths */
#define DEFAULT_CONFIG_PATH "config"

/* Screen layout constants */
#define TITLE_ROW       0
#define MENUBAR_ROW     1
#define WORK_AREA_TOP   2
#define STATUS_ROW      (LINES - 1)

/* Maximum string lengths */
#define MAX_MENU_ITEMS  20
#define MAX_MENU_LABEL  32
#define MAX_PATH_LEN    PATH_MAX

/* Global state */
typedef struct {
    char config_path[MAX_PATH_LEN];
    bool dirty;                     /* Unsaved changes (deprecated - use ctl_modified) */
    bool ctl_modified;              /* CTL files have been modified and need compilation */
    bool tree_reload_needed;        /* Tree data needs to be reloaded from disk */
    int current_menu;               /* Currently selected top menu */
    bool menu_open;                 /* Is a dropdown open? */
} AppState;

extern AppState g_state;

extern MaxCfg *g_maxcfg;
extern MaxCfgToml *g_maxcfg_toml;
extern MaxCfgThemeColors g_theme_colors;

/* Color pair definitions */
enum ColorPairs {
    CP_SCREEN_BG = 1,       /* Cyan on cyan - screen fill */
    CP_TITLE_BAR,           /* Black on grey - top/bottom bars */
    CP_MENU_BAR,            /* Grey on black - menu bar text */
    CP_MENU_HOTKEY,         /* Bold yellow on black - menu hotkeys */
    CP_MENU_HIGHLIGHT,      /* Bold yellow on blue - selected menu */
    CP_DROPDOWN,            /* Grey on black - dropdown text */
    CP_DROPDOWN_HIGHLIGHT,  /* Bold yellow on blue - selected item */
    CP_FORM_BG,             /* Default on black - form background */
    CP_FORM_LABEL,          /* Cyan on black - form labels */
    CP_FORM_VALUE,          /* Yellow on black - form values */
    CP_FORM_HIGHLIGHT,      /* Black on white - highlighted field */
    CP_STATUS_BAR,          /* Black on grey - status line */
    CP_DIALOG_BORDER,       /* Cyan on black - dialog borders */
    CP_DIALOG_TEXT,         /* Yellow on black - dialog text */
    CP_DIALOG_TITLE,        /* Bold white on black - dialog title */
    CP_DIALOG_MSG,          /* Brown/yellow on black - dialog message */
    CP_DIALOG_BTN_BRACKET,  /* Cyan on black - button brackets */
    CP_DIALOG_BTN_HOTKEY,   /* Bold yellow on black - button hotkey */
    CP_DIALOG_BTN_TEXT,     /* Grey on black - button text */
    CP_DIALOG_BTN_SEL,      /* Bold white on blue - selected button */
    CP_ERROR,               /* Red on black - error messages */
    CP_HELP,                /* White on blue - help text */
};

#endif /* MAXCFG_H */
