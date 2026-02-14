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

#ifndef __UI_SHADOWBUF_H_DEFINED
#define __UI_SHADOWBUF_H_DEFINED

#include "typedefs.h"

/**
 * @file ui_shadowbuf.h
 * @brief Attribute-based off-screen buffer for MaxUI drawing and overlays.
 *
 * Maximus is an attribute-first renderer: UI code emits PC attribute changes
 * and cursor motion; the output layer translates to ANSI/AVATAR as needed.
 *
 * ui_shadowbuf_t is a Notorious Doorkit-style ShadowScreen concept implemented
 * in Maximus terms: an in-memory grid of (character, PC attribute) cells.
 */

/**
 * @brief One shadow buffer cell.
 */
typedef struct ui_shadow_cell
{
  char ch;
  byte attr;
} ui_shadow_cell_t;

/**
 * @brief A rectangular block of shadow cells.
 */
typedef struct ui_shadow_block
{
  int width;
  int height;
  ui_shadow_cell_t *cells;
} ui_shadow_block_t;

/**
 * @brief Overlay snapshot state.
 *
 * This stores the "under" block and the top-left origin to restore it.
 */
typedef struct ui_shadow_overlay
{
  int left;
  int top;
  ui_shadow_block_t under;
} ui_shadow_overlay_t;

/**
 * @brief Off-screen shadow buffer with cursor and current attribute state.
 */
typedef struct ui_shadowbuf
{
  int width;
  int height;

  int cursor_row;  /* 1-indexed */
  int cursor_col;  /* 1-indexed */

  byte default_attr;
  byte current_attr;

  ui_shadow_cell_t *cells;
} ui_shadowbuf_t;

/**
 * @brief Normalize a single line of text (no newlines) into (ch, attr) cells.
 *
 * This is a reusable text conversion helper intended for any MaxUI module
 * that needs correct visible-width behavior while supporting styled input.
 *
 * Supported input:
 * - Plain text
 * - ANSI SGR sequences (ESC[...m) for basic color/bright/blink/reset
 * - AVATAR attribute sequences (\x16\x01\xNN)
 *
 * Unsupported escape sequences are skipped.
 *
 * @param text Input text (single line; callers should split on '\n').
 * @param start_attr Starting PC attribute.
 * @param default_attr Default PC attribute used for SGR 0 resets.
 * @param out_cells Allocated output cells (caller must free with ui_shadowbuf_free_cells()).
 * @param out_count Number of output cells.
 * @param out_end_attr Attribute after processing the input line.
 *
 * @return 1 on success, 0 on failure.
 */
int ui_shadowbuf_normalize_line(const char *text, byte start_attr, byte default_attr, ui_shadow_cell_t **out_cells, int *out_count, byte *out_end_attr);

/**
 * @brief Free an allocation returned by ui_shadowbuf_normalize_line().
 */
void ui_shadowbuf_free_cells(ui_shadow_cell_t *cells);

/**
 * @brief Initialize a shadow buffer.
 *
 * @param b Shadow buffer to initialize.
 * @param width Width in columns.
 * @param height Height in rows.
 * @param default_attr Default PC attribute used for clears and resets.
 */
void ui_shadowbuf_init(ui_shadowbuf_t *b, int width, int height, byte default_attr);

/**
 * @brief Free resources owned by a shadow buffer.
 */
void ui_shadowbuf_free(ui_shadowbuf_t *b);

/**
 * @brief Clear the buffer to spaces using the provided attribute.
 */
void ui_shadowbuf_clear(ui_shadowbuf_t *b, byte attr);

/**
 * @brief Move the shadow cursor (clamped to bounds).
 */
void ui_shadowbuf_goto(ui_shadowbuf_t *b, int row, int col);

/**
 * @brief Set the current attribute.
 */
void ui_shadowbuf_set_attr(ui_shadowbuf_t *b, byte attr);

/**
 * @brief Write a character at the current cursor position and advance.
 */
void ui_shadowbuf_putc(ui_shadowbuf_t *b, int ch);

/**
 * @brief Write text into the shadow buffer.
 *
 * The input may contain:
 * - Plain text
 * - ANSI escape sequences (SGR supported; other CSI sequences are ignored)
 * - Maximus AVATAR attribute sequences (\x16\x01\xNN)
 *
 * The text is normalized into cells and written at the current cursor.
 */
void ui_shadowbuf_write(ui_shadowbuf_t *b, const char *text);

/**
 * @brief Extract a rectangular block from the buffer.
 *
 * Coordinates are 1-indexed and inclusive.
 */
ui_shadow_block_t ui_shadowbuf_gettext(const ui_shadowbuf_t *b, int left, int top, int right, int bottom);

/**
 * @brief Restore a block into the buffer at the specified top-left.
 */
void ui_shadowbuf_puttext(ui_shadowbuf_t *b, int left, int top, const ui_shadow_block_t *block);

/**
 * @brief Free a block created by ui_shadowbuf_gettext().
 */
void ui_shadowbuf_free_block(ui_shadow_block_t *block);

/**
 * @brief Snapshot a region to support drawing an overlay.
 *
 * Typical usage:
 * - ui_shadowbuf_overlay_push(...)
 * - draw overlay into the same buffer
 * - ui_shadowbuf_paint_region(...)
 * - ui_shadowbuf_overlay_pop(...)
 * - ui_shadowbuf_paint_region(...)
 */
int ui_shadowbuf_overlay_push(const ui_shadowbuf_t *b, int left, int top, int right, int bottom, ui_shadow_overlay_t *ov);

/**
 * @brief Restore and free a previously pushed overlay snapshot.
 */
void ui_shadowbuf_overlay_pop(ui_shadowbuf_t *b, ui_shadow_overlay_t *ov);

/**
 * @brief Paint a rectangular region of the buffer to the terminal.
 *
 * This emits Maximus UI primitives (attr + goto + characters), which are
 * translated to ANSI/AVATAR by the existing output pipeline.
 *
 * @param screen_x Screen column (1-indexed) where buffer column 1 should paint.
 * @param screen_y Screen row (1-indexed) where buffer row 1 should paint.
 */
void ui_shadowbuf_paint_region(const ui_shadowbuf_t *b, int screen_x, int screen_y, int left, int top, int right, int bottom);

#endif /* __UI_SHADOWBUF_H_DEFINED */
