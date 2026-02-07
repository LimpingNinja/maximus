/*
 * Maximus Version 3.02
 * Copyright 1989, 2002 by Lanius Corporation.  All rights reserved.
 *
 * Modifications Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 * https://github.com/LimpingNinja
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

#ifndef __UI_LIGHTBAR_H_DEFINED
#define __UI_LIGHTBAR_H_DEFINED

#include "typedefs.h"

#define UI_JUSTIFY_LEFT    0
#define UI_JUSTIFY_CENTER  1
#define UI_JUSTIFY_RIGHT   2

typedef struct {
  const char **items;
  int count;
  int x;
  int y;
  int width;        /* 0 => auto */
  int margin;       /* extra columns added to computed width */
  int justify;      /* UI_JUSTIFY_* */
  byte normal_attr;
  byte selected_attr;
  byte hotkey_attr; /* 0 => use normal_attr/selected_attr */
  byte hotkey_highlight_attr; /* 0 => no special hotkey highlight when selected */
  int wrap;
  int enable_hotkeys;
  int show_brackets; /* 1 => show [X], 0 => show just X highlighted */
} ui_lightbar_menu_t;

typedef struct {
  const char *text;
  int x;
  int y;
  int width;   /* 0 => auto */
  int justify; /* UI_JUSTIFY_* */
} ui_lightbar_item_t;

typedef struct {
  const ui_lightbar_item_t *items;
  int count;
  byte normal_attr;
  byte selected_attr;
  byte hotkey_attr; /* 0 => use normal_attr/selected_attr */
  byte hotkey_highlight_attr; /* 0 => no special hotkey highlight when selected */
  int margin;       /* extra columns added to computed per-item width */
  int wrap;
  int enable_hotkeys;
  int show_brackets; /* 1 => show [X], 0 => show just X highlighted */
} ui_lightbar_pos_menu_t;

int ui_lightbar_run(ui_lightbar_menu_t *m);

int ui_lightbar_run_hotkey(ui_lightbar_menu_t *m, int *out_key);

int ui_lightbar_run_pos_hotkey(ui_lightbar_pos_menu_t *m, int *out_key);

int ui_select_prompt(
    const char *prompt,
    const char **options,
    int option_count,
    byte prompt_attr,
    byte normal_attr,
    byte selected_attr,
    int flags,
    int margin,
    const char *separator,
    int *out_key
);

#endif /* __UI_LIGHTBAR_H_DEFINED */
