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

#ifndef __UI_FORM_H_DEFINED
#define __UI_FORM_H_DEFINED

/* Field types */
#define UI_FIELD_TEXT     0
#define UI_FIELD_MASKED   1  /* password */
#define UI_FIELD_FORMAT   2  /* format_mask */
#define UI_FIELD_OPTION   3  /* toggle/select */

/* Form save modes */
#define UI_FORM_SAVE_CTRL_S      0  /* Ctrl+S saves, ESC cancels */
#define UI_FORM_SAVE_ESC_PROMPT  1  /* ESC prompts: Edit/Save/Exit */

/* Form field definition */
typedef struct ui_form_field
{
  const char *name;        /* field identifier (for debugging/logging) */
  const char *label;       /* optional label (rendered to left of field) */
  int x;                   /* field X position (1-indexed) */
  int y;                   /* field Y position (1-indexed) */
  int width;               /* field display width */
  int max_len;             /* max input length */
  int field_type;          /* UI_FIELD_* */
  char hotkey;             /* optional hotkey character (0 = none) */
  int required;            /* 1 = required field */
  
  /* Styling (0 = use form default) */
  byte label_attr;         /* label color */
  byte normal_attr;        /* unfocused field color */
  byte focus_attr;         /* focused field color */
  
  /* Type-specific config */
  const char *format_mask; /* for UI_FIELD_FORMAT */
  const char **options;    /* for UI_FIELD_OPTION (NULL-terminated array) */
  int option_count;        /* number of options */
  
  /* Value storage (managed by form runner) */
  char *value;             /* current field value */
  int value_cap;           /* capacity of value buffer */
} ui_form_field_t;

/* Form style/config */
typedef struct ui_form_style
{
  byte label_attr;         /* default label color */
  byte normal_attr;        /* default unfocused field color */
  byte focus_attr;         /* default focused field color */
  int save_mode;           /* UI_FORM_SAVE_* */
  int wrap;                /* 1 = wrap navigation at edges */
  
  /* Required field splash */
  const char *required_msg;  /* message shown when required field empty */
  int required_x;            /* splash X position */
  int required_y;            /* splash Y position */
  byte required_attr;        /* splash color */
} ui_form_style_t;

/* Form runner */
int ui_form_run(
    ui_form_field_t *fields,
    int field_count,
    const ui_form_style_t *style
);

/* Returns:
 *   1 = saved (field values updated in-place)
 *   0 = cancelled
 *  -1 = error
 */

/* Initialize style struct with defaults */
void ui_form_style_default(ui_form_style_t *style);

#endif /* __UI_FORM_H_DEFINED */
