/**
 * @file local_term_ansi.c
 * @brief ANSI + UTF-8 local terminal backend for modern terminals.
 *
 * Converts PC/DOS attribute bytes to ANSI SGR sequences and
 * maps CP437 high-byte characters (0x80-0xFF) to their Unicode
 * equivalents, emitted as UTF-8.  Output goes directly to stdout.
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdio.h>
#include <string.h>
#include "local_term.h"

/* ------------------------------------------------------------------ */
/* Internal state                                                     */
/* ------------------------------------------------------------------ */

/** Current PC attribute (-1 = unknown / force next set_attr). */
static int s_cur_attr = -1;

/** Internal write buffer — batches output to minimize syscalls. */
#define LT_BUF_SIZE 4096
static char s_buf[LT_BUF_SIZE];
static int  s_buf_len = 0;

/**
 * @brief Append bytes to the internal buffer, flushing if needed.
 */
static void buf_append(const char *data, int len)
{
  while (len > 0)
  {
    int avail = LT_BUF_SIZE - s_buf_len;
    int chunk = (len < avail) ? len : avail;
    memcpy(s_buf + s_buf_len, data, (size_t)chunk);
    s_buf_len += chunk;
    data += chunk;
    len -= chunk;

    if (s_buf_len >= LT_BUF_SIZE)
    {
      fwrite(s_buf, 1, (size_t)s_buf_len, stdout);
      s_buf_len = 0;
    }
  }
}

/**
 * @brief Append a single byte to the internal buffer.
 */
static void buf_putc(char ch)
{
  s_buf[s_buf_len++] = ch;
  if (s_buf_len >= LT_BUF_SIZE)
  {
    fwrite(s_buf, 1, (size_t)s_buf_len, stdout);
    s_buf_len = 0;
  }
}

/**
 * @brief Append a NUL-terminated string to the internal buffer.
 */
static void buf_puts(const char *s)
{
  buf_append(s, (int)strlen(s));
}


/* ------------------------------------------------------------------ */
/* CP437 → UTF-8 translation table                                    */
/*                                                                    */
/* 0x00-0x1F: CP437 has printable glyphs for these (smiley, etc.)     */
/*            We map the displayable ones and keep whitespace as-is.  */
/* 0x20-0x7E: Standard ASCII — identity mapping.                      */
/* 0x7F:      House (⌂)                                               */
/* 0x80-0xFF: Full CP437 upper half.                                  */
/* ------------------------------------------------------------------ */

const char *cp437_to_utf8[256] = {
  /* 0x00-0x0F */
  " ",           "\xe2\x98\xba", "\xe2\x98\xbb", "\xe2\x99\xa5",
  "\xe2\x99\xa6","\xe2\x99\xa3", "\xe2\x99\xa0", "\xe2\x80\xa2",
  "\xe2\x97\x98","\xe2\x97\x8b", "\xe2\x97\x99", "\xe2\x99\x82",
  "\xe2\x99\x80","\xe2\x99\xaa", "\xe2\x99\xab", "\xe2\x98\xbc",

  /* 0x10-0x1F */
  "\xe2\x96\xba", "\xe2\x97\x84", "\xe2\x86\x95", "\xe2\x80\xbc",
  "\xc2\xb6",     "\xc2\xa7",     "\xe2\x96\xac", "\xe2\x86\xa8",
  "\xe2\x86\x91", "\xe2\x86\x93", "\xe2\x86\x92", "\xe2\x86\x90",
  "\xe2\x88\x9f", "\xe2\x86\x94", "\xe2\x96\xb2", "\xe2\x96\xbc",

  /* 0x20-0x2F  (ASCII) */
  " ", "!", "\"", "#", "$", "%", "&", "'",
  "(", ")", "*",  "+", ",", "-", ".", "/",

  /* 0x30-0x3F */
  "0", "1", "2", "3", "4", "5", "6", "7",
  "8", "9", ":", ";", "<", "=", ">", "?",

  /* 0x40-0x4F */
  "@", "A", "B", "C", "D", "E", "F", "G",
  "H", "I", "J", "K", "L", "M", "N", "O",

  /* 0x50-0x5F */
  "P", "Q", "R", "S", "T", "U", "V", "W",
  "X", "Y", "Z", "[", "\\", "]", "^", "_",

  /* 0x60-0x6F */
  "`", "a", "b", "c", "d", "e", "f", "g",
  "h", "i", "j", "k", "l", "m", "n", "o",

  /* 0x70-0x7F */
  "p", "q", "r", "s", "t", "u", "v", "w",
  "x", "y", "z", "{", "|", "}", "~", "\xe2\x8c\x82",

  /* 0x80-0x8F */
  "\xc3\x87",     "\xc3\xbc",     "\xc3\xa9",     "\xc3\xa2",
  "\xc3\xa4",     "\xc3\xa0",     "\xc3\xa5",     "\xc3\xa7",
  "\xc3\xaa",     "\xc3\xab",     "\xc3\xa8",     "\xc3\xaf",
  "\xc3\xae",     "\xc3\xac",     "\xc3\x84",     "\xc3\x85",

  /* 0x90-0x9F */
  "\xc3\x89",     "\xc3\xa6",     "\xc3\x86",     "\xc3\xb4",
  "\xc3\xb6",     "\xc3\xb2",     "\xc3\xbb",     "\xc3\xb9",
  "\xc3\xbf",     "\xc3\x96",     "\xc3\x9c",     "\xc2\xa2",
  "\xc2\xa3",     "\xc2\xa5",     "\xe2\x82\xa7", "\xc6\x92",

  /* 0xA0-0xAF */
  "\xc3\xa1",     "\xc3\xad",     "\xc3\xb3",     "\xc3\xba",
  "\xc3\xb1",     "\xc3\x91",     "\xc2\xaa",     "\xc2\xba",
  "\xc2\xbf",     "\xe2\x8c\x90", "\xc2\xac",     "\xc2\xbd",
  "\xc2\xbc",     "\xc2\xa1",     "\xc2\xab",     "\xc2\xbb",

  /* 0xB0-0xBF  (box-drawing light shades + corners) */
  "\xe2\x96\x91", "\xe2\x96\x92", "\xe2\x96\x93", "\xe2\x94\x82",
  "\xe2\x94\xa4", "\xe2\x95\xa1", "\xe2\x95\xa2", "\xe2\x95\x96",
  "\xe2\x95\x95", "\xe2\x95\xa3", "\xe2\x95\x91", "\xe2\x95\x97",
  "\xe2\x95\x9d", "\xe2\x95\x9c", "\xe2\x95\x9b", "\xe2\x94\x90",

  /* 0xC0-0xCF */
  "\xe2\x94\x94", "\xe2\x94\xb4", "\xe2\x94\xac", "\xe2\x94\x9c",
  "\xe2\x94\x80", "\xe2\x94\xbc", "\xe2\x95\x9e", "\xe2\x95\x9f",
  "\xe2\x95\x9a", "\xe2\x95\x94", "\xe2\x95\xa9", "\xe2\x95\xa6",
  "\xe2\x95\xa0", "\xe2\x95\x90", "\xe2\x95\xac", "\xe2\x95\xa7",

  /* 0xD0-0xDF */
  "\xe2\x95\xa8", "\xe2\x95\xa4", "\xe2\x95\xa5", "\xe2\x95\x99",
  "\xe2\x95\x98", "\xe2\x95\x92", "\xe2\x95\x93", "\xe2\x95\xab",
  "\xe2\x95\xaa", "\xe2\x94\x98", "\xe2\x94\x8c", "\xe2\x96\x88",
  "\xe2\x96\x84", "\xe2\x96\x8c", "\xe2\x96\x90", "\xe2\x96\x80",

  /* 0xE0-0xEF  (Greek / math) */
  "\xce\xb1",     "\xc3\x9f",     "\xce\x93",     "\xcf\x80",
  "\xce\xa3",     "\xcf\x83",     "\xc2\xb5",     "\xcf\x84",
  "\xce\xa6",     "\xce\x98",     "\xce\xa9",     "\xce\xb4",
  "\xe2\x88\x9e", "\xcf\x86",     "\xce\xb5",     "\xe2\x88\xa9",

  /* 0xF0-0xFF */
  "\xe2\x89\xa1", "\xc2\xb1",     "\xe2\x89\xa5", "\xe2\x89\xa4",
  "\xe2\x8c\xa0", "\xe2\x8c\xa1", "\xc3\xb7",     "\xe2\x89\x88",
  "\xc2\xb0",     "\xe2\x88\x99", "\xc2\xb7",     "\xe2\x88\x9a",
  "\xe2\x81\xbf", "\xc2\xb2",     "\xe2\x96\xa0", "\xc2\xa0"
};


/* ------------------------------------------------------------------ */
/* ANSI color mapping — PC color index → ANSI SGR color number.       */
/*                                                                    */
/* PC order:  Black Blue Green Cyan Red Magenta Brown/Yellow White     */
/* ANSI order: Black Red Green Yellow Blue Magenta Cyan White          */
/* ------------------------------------------------------------------ */

static const int pc_to_ansi_fg[8] = { 30, 34, 32, 36, 31, 35, 33, 37 };
static const int pc_to_ansi_bg[8] = { 40, 44, 42, 46, 41, 45, 43, 47 };


/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

/**
 * @brief Emit an ANSI SGR sequence for a PC attribute byte.
 *
 * Generates an efficient delta SGR when possible, or a full
 * reset+set when the old attribute is unknown.
 */
static void emit_sgr(byte pc_attr)
{
  int fg_idx   = pc_attr & 0x07;
  int bright   = (pc_attr & 0x08) ? 1 : 0;
  int bg_idx   = (pc_attr >> 4) & 0x07;
  int blink    = (pc_attr & 0x80) ? 1 : 0;

  /* Build a full SGR reset + set — simple and robust. */
  char buf[40];
  int pos = 0;

  buf[pos++] = '\x1b';
  buf[pos++] = '[';
  buf[pos++] = '0';                     /* reset */

  if (bright)
  {
    buf[pos++] = ';';
    buf[pos++] = '1';
  }

  if (blink)
  {
    buf[pos++] = ';';
    buf[pos++] = '5';
  }

  /* Foreground */
  pos += sprintf(buf + pos, ";%d", pc_to_ansi_fg[fg_idx]);

  /* Background (skip if black — reset already set it) */
  if (bg_idx != 0)
    pos += sprintf(buf + pos, ";%d", pc_to_ansi_bg[bg_idx]);

  buf[pos++] = 'm';
  buf[pos] = '\0';

  buf_append(buf, pos);
}

/**
 * @brief Write a UTF-8 string for a CP437 byte to stdout.
 */
static void emit_cp437(int ch)
{
  const char *utf8 = cp437_to_utf8[(unsigned char)ch];
  if (utf8)
    buf_puts(utf8);
}


/* ------------------------------------------------------------------ */
/* Backend implementation                                             */
/* ------------------------------------------------------------------ */

static void lt_ansi_init(void)
{
  s_cur_attr = -1;
  s_buf_len = 0;
  /* Fully buffered — we control flushes via lt_flush(). */
  setvbuf(stdout, NULL, _IOFBF, 8192);
}

static void lt_ansi_fini(void)
{
  /* Reset terminal attributes on shutdown. */
  buf_puts("\x1b[0m");
  /* Drain buffer before exit. */
  if (s_buf_len > 0)
  {
    fwrite(s_buf, 1, (size_t)s_buf_len, stdout);
    s_buf_len = 0;
  }
  fflush(stdout);
  s_cur_attr = -1;
}

static void lt_ansi_set_attr(byte pc_attr)
{
  if ((int)pc_attr == s_cur_attr)
    return;

  emit_sgr(pc_attr);
  s_cur_attr = (int)pc_attr;
}

static void lt_ansi_goto_xy(int row, int col)
{
  char tmp[24];
  int n = sprintf(tmp, "\x1b[%d;%dH", row, col);
  buf_append(tmp, n);
}

static void lt_ansi_putc(int ch)
{
  unsigned char uch = (unsigned char)ch;

  /* Standard printable ASCII — fast path */
  if (uch >= 0x20 && uch < 0x7F)
  {
    buf_putc((char)ch);
    return;
  }

  /* CP437 high bytes → UTF-8 */
  if (uch >= 0x80)
  {
    emit_cp437(ch);
    return;
  }

  /* Tab, CR — pass through */
  if (uch == '\t' || uch == '\r')
  {
    buf_putc((char)ch);
    return;
  }

  /* LF — emit raw (caller sends CR+LF explicitly) */
  if (uch == '\n')
  {
    buf_putc('\n');
    return;
  }

  /* Backspace */
  if (uch == 0x08)
  {
    buf_putc(0x08);
    return;
  }

  /* Bell */
  if (uch == 0x07)
  {
    buf_putc(0x07);
    return;
  }

  /* CP437 control-range glyphs (0x01-0x06, 0x0E-0x1F) → UTF-8 */
  if (uch >= 0x01 && uch <= 0x06)
  {
    emit_cp437(ch);
    return;
  }
  if (uch >= 0x0E && uch <= 0x1F)
  {
    emit_cp437(ch);
    return;
  }

  /* DEL (0x7F) — CP437 house character */
  if (uch == 0x7F)
  {
    emit_cp437(ch);
    return;
  }

  /* Everything else: drop silently (NUL, etc.) */
}

static void lt_ansi_cls(void)
{
  buf_puts("\x1b[H\x1b[2J\x1b[J");
  s_cur_attr = -1;  /* force re-emit after clear */
}

static void lt_ansi_cleol(int row, int col, byte attr)
{
  (void)row;
  (void)col;
  (void)attr;
  buf_puts("\x1b[K");
}

static void lt_ansi_cursor_up(void)
{
  buf_puts("\x1b[A");
}

static void lt_ansi_cursor_down(void)
{
  buf_puts("\x1b[B");
}

static void lt_ansi_cursor_left(void)
{
  buf_puts("\x1b[D");
}

static void lt_ansi_cursor_right(void)
{
  buf_puts("\x1b[C");
}

static void lt_ansi_set_blink(void)
{
  buf_puts("\x1b[5m");
  if (s_cur_attr >= 0)
    s_cur_attr |= 0x80;
}

static void lt_ansi_raw_write(const char *s, int len)
{
  if (s && len > 0)
    buf_append(s, len);
}

static void lt_ansi_flush(void)
{
  if (s_buf_len > 0)
  {
    fwrite(s_buf, 1, (size_t)s_buf_len, stdout);
    s_buf_len = 0;
  }
  fflush(stdout);
}


/* ------------------------------------------------------------------ */
/* Public backend instance                                            */
/* ------------------------------------------------------------------ */

LocalTermOps local_term_ansi_utf8 = {
  .lt_init         = lt_ansi_init,
  .lt_fini         = lt_ansi_fini,
  .lt_set_attr     = lt_ansi_set_attr,
  .lt_goto_xy      = lt_ansi_goto_xy,
  .lt_putc         = lt_ansi_putc,
  .lt_cls          = lt_ansi_cls,
  .lt_cleol        = lt_ansi_cleol,
  .lt_cursor_up    = lt_ansi_cursor_up,
  .lt_cursor_down  = lt_ansi_cursor_down,
  .lt_cursor_left  = lt_ansi_cursor_left,
  .lt_cursor_right = lt_ansi_cursor_right,
  .lt_set_blink    = lt_ansi_set_blink,
  .lt_raw_write    = lt_ansi_raw_write,
  .lt_flush        = lt_ansi_flush,
};


/* ------------------------------------------------------------------ */
/* Null backend — no-op for headless mode                             */
/* ------------------------------------------------------------------ */

static void lt_null_noop(void) {}
static void lt_null_set_attr(byte a) { (void)a; }
static void lt_null_goto(int r, int c) { (void)r; (void)c; }
static void lt_null_putc(int ch) { (void)ch; }
static void lt_null_cleol(int r, int c, byte a) { (void)r; (void)c; (void)a; }
static void lt_null_raw(const char *s, int l) { (void)s; (void)l; }

LocalTermOps local_term_null = {
  .lt_init         = lt_null_noop,
  .lt_fini         = lt_null_noop,
  .lt_set_attr     = lt_null_set_attr,
  .lt_goto_xy      = lt_null_goto,
  .lt_putc         = lt_null_putc,
  .lt_cls          = lt_null_noop,
  .lt_cleol        = lt_null_cleol,
  .lt_cursor_up    = lt_null_noop,
  .lt_cursor_down  = lt_null_noop,
  .lt_cursor_left  = lt_null_noop,
  .lt_cursor_right = lt_null_noop,
  .lt_set_blink    = lt_null_noop,
  .lt_raw_write    = lt_null_raw,
  .lt_flush        = lt_null_noop,
};


/* ------------------------------------------------------------------ */
/* Global backend pointer — default to null until init selects one.   */
/* ------------------------------------------------------------------ */

LocalTermOps *g_local_term = &local_term_null;
