/*
 * ui_field.h — Edit field primitives header
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

/**
 * @brief Set the current display attribute using AVATAR sequence.
 *
 * @param attr PC/DOS attribute byte.
 */
void ui_set_attr(byte attr);

/**
 * @brief Position cursor at the specified row and column.
 *
 * @param row 1-based row.
 * @param col 1-based column.
 */
void ui_goto(int row, int col);

/**
 * @brief Fill a rectangular region with a character and attribute.
 *
 * @param row    Top-left row (1-based).
 * @param col    Top-left column (1-based).
 * @param width  Width in columns.
 * @param height Height in rows.
 * @param ch     Fill character.
 * @param attr   PC attribute for the fill.
 */
void ui_fill_rect(int row, int col, int width, int height, char ch, byte attr);

/**
 * @brief Write a string padded to a fixed width at the given position.
 *
 * @param row   Row (1-based).
 * @param col   Column (1-based).
 * @param width Display width to pad to.
 * @param s     String to write (may be NULL).
 * @param attr  PC attribute.
 */
void ui_write_padded(int row, int col, int width, const char *s, byte attr);

/**
 * @brief Check whether a field of given width fits at a specific column.
 *
 * @param col   1-based starting column.
 * @param width Field width in columns.
 * @return      Non-zero if the field fits within the terminal width.
 */
int ui_field_can_fit_at(int col, int width);

/**
 * @brief Check whether a field of given width fits at the current cursor column.
 *
 * @param width Field width in columns.
 * @return      Non-zero if the field fits.
 */
int ui_field_can_fit_here(int width);

/**
 * @brief Test if a mask character is an editable placeholder (0, A, or X).
 *
 * @param ch Mask character.
 * @return   Non-zero if ch is a placeholder.
 */
int ui_mask_is_placeholder(char ch);

/**
 * @brief Return the display character for an unfilled placeholder position.
 *
 * @param ch Placeholder character ('0' returns '0', others return '_').
 * @return   Display character.
 */
char ui_mask_placeholder_display(char ch);

/**
 * @brief Validate whether a typed character satisfies a placeholder type.
 *
 * @param placeholder Mask placeholder character ('0'=digit, 'A'=alpha, 'X'=alnum).
 * @param ch          Character typed by the user.
 * @return            Non-zero if the character is acceptable.
 */
int ui_mask_placeholder_ok(char placeholder, char ch);

/**
 * @brief Count the number of editable placeholder positions in a format mask.
 *
 * @param mask Format mask string.
 * @return     Number of placeholder positions.
 */
int ui_mask_count_positions(const char *mask);

/**
 * @brief Apply raw input data through a format mask for display.
 *
 * @param raw     Raw input characters (digits/letters only).
 * @param mask    Format mask (e.g. "(000) 000-0000").
 * @param out     Output buffer for the formatted result.
 * @param out_cap Capacity of out.
 */
void ui_mask_apply(const char *raw, const char *mask, char *out, int out_cap);

/**
 * @brief Read a key with ESC-sequence decoding for arrow/function keys.
 *
 * Handles both DOS scan-code style input and ANSI terminal escape sequences,
 * translating them into internal K_* key constants.
 *
 * @return Key code (K_UP, K_DOWN, K_LEFT, K_RIGHT, K_HOME, K_END, etc.).
 */
int ui_read_key(void);

/**
 * @brief Bounded inline field editor with full style control.
 *
 * Presents an editable text field at the given screen position. Supports
 * insert mode, format masks, password masking, and field-mode navigation.
 *
 * @param row     Screen row (1-based).
 * @param col     Screen column (1-based).
 * @param width   Display width of the field.
 * @param max_len Maximum input length.
 * @param buf     Input/output buffer (pre-populated value, edited in-place).
 * @param buf_cap Capacity of buf.
 * @param style   Style configuration (colors, flags, mask).
 * @return        UI_EDIT_ACCEPT, UI_EDIT_CANCEL, UI_EDIT_PREVIOUS, UI_EDIT_NEXT, etc.
 */
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

/**
 * @brief Inline prompt-and-edit field with configurable start mode.
 *
 * Displays a prompt string followed by an editable field. Falls back
 * to line-mode input for non-ANSI terminals.
 *
 * @param prompt  Prompt text displayed before the field.
 * @param width   Display width of the editable area.
 * @param max_len Maximum input length.
 * @param buf     Input/output buffer.
 * @param buf_cap Capacity of buf.
 * @param style   Style configuration (colors, flags, start mode).
 * @return        UI_EDIT_ACCEPT or UI_EDIT_ERROR.
 */
int ui_prompt_field(
    const char *prompt,
    int width,
    int max_len,
    char *buf,
    int buf_cap,
    const ui_prompt_field_style_t *style
);

/**
 * @brief Initialize a ui_edit_field_style_t with sensible defaults.
 *
 * @param style Style struct to initialize.
 */
void ui_edit_field_style_default(ui_edit_field_style_t *style);

/**
 * @brief Initialize a ui_prompt_field_style_t with sensible defaults.
 *
 * @param style Style struct to initialize.
 */
void ui_prompt_field_style_default(ui_prompt_field_style_t *style);

#endif /* __UI_FIELD_H_DEFINED */
