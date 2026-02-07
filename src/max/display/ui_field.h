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

#ifndef __UI_FIELD_H_DEFINED
#define __UI_FIELD_H_DEFINED

/* Return codes for ui_edit_field */
#define UI_EDIT_ERROR     0
#define UI_EDIT_CANCEL    1
#define UI_EDIT_ACCEPT    2
#define UI_EDIT_PREVIOUS  3
#define UI_EDIT_NEXT      4
 #define UI_EDIT_NOROOM    5

/* Flags for ui_edit_field */
#define UI_EDIT_FLAG_ALLOW_CANCEL  0x0020
#define UI_EDIT_FLAG_FIELD_MODE    0x0002
#define UI_EDIT_FLAG_MASK          0x0004

/* Internal UI key codes (returned by ui_read_key) */
#define UI_KEY_DELETE              0x0100

/* Style struct for ui_edit_field */
typedef struct ui_edit_field_style
{
  byte normal_attr;       /* unfocused color */
  byte focus_attr;        /* focused/editing color */
  byte fill_ch;           /* fill/mask character (default ' ') */
  int flags;              /* UI_EDIT_FLAG_* flags */
  const char *format_mask; /* optional mask like "(000) 000-0000" */
} ui_edit_field_style_t;

/* Style struct for ui_prompt_field */
typedef struct ui_prompt_field_style
{
  byte prompt_attr;       /* prompt text color */
  byte field_attr;        /* field color */
  byte fill_ch;           /* fill/mask character (default ' ') */
  int flags;              /* UI_EDIT_FLAG_* flags */
  int start_mode;         /* UI_PROMPT_START_* mode */
  const char *format_mask; /* optional mask */
} ui_prompt_field_style_t;

/* Low-level primitives */
void ui_set_attr(byte attr);
void ui_goto(int row, int col);
void ui_fill_rect(int row, int col, int width, int height, char ch, byte attr);
void ui_write_padded(int row, int col, int width, const char *s, byte attr);

int ui_field_can_fit_at(int col, int width);
int ui_field_can_fit_here(int width);

/* Format mask helpers */
int ui_mask_is_placeholder(char ch);
char ui_mask_placeholder_display(char ch);
int ui_mask_placeholder_ok(char placeholder, char ch);
int ui_mask_count_positions(const char *mask);
void ui_mask_apply(const char *raw, const char *mask, char *out, int out_cap);

/* Read a key using common Maximus input primitives and basic ESC decoding */
int ui_read_key(void);

/* Bounded field editor with style struct */
int ui_edit_field(
    int row,
    int col,
    int width,
    int max_len,
    char *buf,
    int buf_cap,
    const ui_edit_field_style_t *style
);

/* Inline prompt editor start modes */
#define UI_PROMPT_START_HERE   0
#define UI_PROMPT_START_CR     1
#define UI_PROMPT_START_CLBOL  2

/* Inline prompt editor with style struct */
int ui_prompt_field(
    const char *prompt,
    int width,
    int max_len,
    char *buf,
    int buf_cap,
    const ui_prompt_field_style_t *style
);

/* Initialize style structs with defaults */
void ui_edit_field_style_default(ui_edit_field_style_t *style);
void ui_prompt_field_style_default(ui_prompt_field_style_t *style);

#endif /* __UI_FIELD_H_DEFINED */
