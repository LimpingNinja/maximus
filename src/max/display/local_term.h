/*
 * local_term.h — ANSI+UTF-8 local terminal backend header
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

#ifndef __LOCAL_TERM_H_DEFINED
#define __LOCAL_TERM_H_DEFINED

#include "prog.h"

/**
 * @brief Local terminal backend operations vtable.
 *
 * Each function pointer corresponds to an abstract display
 * operation that the AVATAR interpreter in Lputc invokes.
 */
typedef struct local_term_ops
{
  /** Initialize the backend (called once at startup). */
  void (*lt_init)(void);

  /** Shut down the backend (called at exit). */
  void (*lt_fini)(void);

  /**
   * @brief Set the current display attribute.
   * @param pc_attr  PC/DOS-style attribute byte:
   *                 bits 0-3 = foreground (0-15),
   *                 bits 4-6 = background (0-7),
   *                 bit 7    = blink.
   */
  void (*lt_set_attr)(byte pc_attr);

  /**
   * @brief Move the cursor to an absolute position.
   * @param row  1-based row.
   * @param col  1-based column.
   */
  void (*lt_goto_xy)(int row, int col);

  /**
   * @brief Output a single visible character.
   *
   * For the ansi_utf8 backend, bytes 0x80-0xFF are mapped
   * from CP437 to their Unicode equivalents and emitted as
   * UTF-8.  Control characters (< 0x20) other than the
   * standard whitespace set are silently dropped.
   *
   * @param ch  Character byte (may be CP437 high byte).
   */
  void (*lt_putc)(int ch);

  /** Clear the entire screen and home the cursor. */
  void (*lt_cls)(void);

  /**
   * @brief Clear from cursor to end of line.
   * @param row   1-based row   (current cursor row).
   * @param col   1-based column (current cursor col).
   * @param attr  Attribute to fill with (used by some backends).
   */
  void (*lt_cleol)(int row, int col, byte attr);

  /** Move cursor up one row. */
  void (*lt_cursor_up)(void);

  /** Move cursor down one row. */
  void (*lt_cursor_down)(void);

  /** Move cursor left one column. */
  void (*lt_cursor_left)(void);

  /** Move cursor right one column. */
  void (*lt_cursor_right)(void);

  /** Add blink attribute to the current color. */
  void (*lt_set_blink)(void);

  /**
   * @brief Write a raw byte sequence directly to the terminal.
   *
   * Used for ANSI passthrough — the backend must NOT
   * interpret the bytes, just forward them.
   *
   * @param s    Byte string.
   * @param len  Number of bytes to write.
   */
  void (*lt_raw_write)(const char *s, int len);

  /** Flush any buffered output to the terminal. */
  void (*lt_flush)(void);

} LocalTermOps;


/* ------------------------------------------------------------------ */
/* Global backend pointer — set during init, used by Lputc.           */
/* ------------------------------------------------------------------ */

extern LocalTermOps *g_local_term;


/* ------------------------------------------------------------------ */
/* Built-in backends                                                  */
/* ------------------------------------------------------------------ */

/** ANSI + UTF-8 backend for modern terminals (macOS / Linux). */
extern LocalTermOps local_term_ansi_utf8;

/** No-op backend (headless / no local video). */
extern LocalTermOps local_term_null;


/* ------------------------------------------------------------------ */
/* CP437 → UTF-8 translation table (public for reuse).                */
/* Each entry is a NUL-terminated UTF-8 string (1-3 bytes + NUL).     */
/* Indexed 0x00..0xFF; entries 0x00-0x7F are single-byte ASCII.       */
/* ------------------------------------------------------------------ */

extern const char *cp437_to_utf8[256];


#endif /* __LOCAL_TERM_H_DEFINED */
