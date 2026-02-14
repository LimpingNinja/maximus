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

#define MAX_INCL_COMMS

#define MAX_LANG_m_area
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "prog.h"
#include "mm.h"
#include "protod.h"

#include "ui_field.h"
#include "ui_shadowbuf.h"

/**
 * @brief Convert ANSI SGR foreground color index (0-7) to DOS/PC color index.
 *
 * ANSI order (0-7): Black, Red, Green, Yellow, Blue, Magenta, Cyan, White.
 * DOS order used by Maximus attrs: Black, Blue, Green, Cyan, Red, Magenta, Brown/Yellow, Gray.
 */
static int near ui_ansi_to_pc_color(int ansi)
{
  static const int map[8] = {0, 4, 2, 6, 1, 5, 3, 7};
  int a = ansi;

  if (a < 0)
    a = 0;
  if (a > 7)
    a = 7;

  return map[a];
}

/**
 * @brief Apply a basic SGR parameter to a PC attribute.
 *
 * Supported:
 * - 0 reset
 * - 1 bright on
 * - 22 bright off
 * - 5 blink on
 * - 25 blink off
 * - 30-37 foreground
 * - 40-47 background
 */
static void near ui_shadowbuf_apply_sgr_attr(byte *cur_attr, byte default_attr, int param)
{
  int fg;
  int bg;
  int bright;
  int blink;
  int def_fg;
  int def_bg;

  if (!cur_attr)
    return;

  fg = (int)(*cur_attr & 0x0F);
  bg = (int)((*cur_attr >> 4) & 0x07);
  bright = (fg & 0x08) ? 1 : 0;
  blink = (*cur_attr & 0x80) ? 1 : 0;

  def_fg = (int)(default_attr & 0x0F);
  def_bg = (int)((default_attr >> 4) & 0x07);

  if (param == 0)
  {
    *cur_attr = default_attr;
    return;
  }

  if (param == 1)
  {
    bright = 1;
  }
  else if (param == 22)
  {
    bright = 0;
  }
  else if (param == 5)
  {
    blink = 1;
  }
  else if (param == 25)
  {
    blink = 0;
  }
  else if (param >= 30 && param <= 37)
  {
    fg = ui_ansi_to_pc_color(param - 30);
  }
  else if (param == 39)
  {
    fg = def_fg & 0x07;
    bright = (def_fg & 0x08) ? 1 : 0;
  }
  else if (param >= 40 && param <= 47)
  {
    bg = ui_ansi_to_pc_color(param - 40);
  }
  else if (param == 49)
  {
    bg = def_bg & 0x07;
  }
  else if (param == 7)
  {
    /*
     * Reverse video.
     *
     * This is intentionally simple: swap base colors and keep the intensity
     * bit on the resulting foreground. This matches typical BBS usage where
     * reverse is paired with SGR 0 to restore defaults.
     */
    {
      int fg_base = fg & 0x07;
      int bg_base = bg & 0x07;
      fg = bg_base;
      bg = fg_base;
    }
  }

  /* Compose */
  fg = (fg & 0x07) | (bright ? 0x08 : 0);
  bg = (bg & 0x07);

  *cur_attr = (byte)((bg << 4) | (fg & 0x0F) | (blink ? 0x80 : 0));
}

static void near ui_shadowbuf_apply_sgr(ui_shadowbuf_t *b, int param)
{
  if (!b)
    return;

  ui_shadowbuf_apply_sgr_attr(&b->current_attr, b->default_attr, param);
}

static int near ui_shadowbuf_idx(const ui_shadowbuf_t *b, int row, int col)
{
  return (row - 1) * b->width + (col - 1);
}

void ui_shadowbuf_init(ui_shadowbuf_t *b, int width, int height, byte default_attr)
{
  if (!b)
    return;

  memset(b, 0, sizeof(*b));

  b->width = (width > 0) ? width : 1;
  b->height = (height > 0) ? height : 1;
  b->cursor_row = 1;
  b->cursor_col = 1;
  b->default_attr = default_attr;
  b->current_attr = default_attr;

  b->cells = (ui_shadow_cell_t *)malloc((size_t)(b->width * b->height) * sizeof(ui_shadow_cell_t));
  if (!b->cells)
  {
    b->width = b->height = 0;
    return;
  }

  ui_shadowbuf_clear(b, default_attr);
}

void ui_shadowbuf_free(ui_shadowbuf_t *b)
{
  if (!b)
    return;

  if (b->cells)
    free(b->cells);

  memset(b, 0, sizeof(*b));
}

void ui_shadowbuf_clear(ui_shadowbuf_t *b, byte attr)
{
  int i;
  int n;

  if (!b || !b->cells)
    return;

  n = b->width * b->height;

  for (i = 0; i < n; i++)
  {
    b->cells[i].ch = ' ';
    b->cells[i].attr = attr;
  }

  b->cursor_row = 1;
  b->cursor_col = 1;
  b->current_attr = attr;
}

void ui_shadowbuf_goto(ui_shadowbuf_t *b, int row, int col)
{
  if (!b)
    return;

  if (row < 1)
    row = 1;
  if (col < 1)
    col = 1;
  if (row > b->height)
    row = b->height;
  if (col > b->width)
    col = b->width;

  b->cursor_row = row;
  b->cursor_col = col;
}

void ui_shadowbuf_set_attr(ui_shadowbuf_t *b, byte attr)
{
  if (!b)
    return;

  b->current_attr = attr;
}

void ui_shadowbuf_putc(ui_shadowbuf_t *b, int ch)
{
  int r;
  int c;
  int idx;

  if (!b || !b->cells)
    return;

  if (ch == '\r')
  {
    b->cursor_col = 1;
    return;
  }

  if (ch == '\n')
  {
    if (b->cursor_row < b->height)
      b->cursor_row++;
    return;
  }

  r = b->cursor_row;
  c = b->cursor_col;

  if (r < 1 || c < 1 || r > b->height || c > b->width)
    return;

  idx = ui_shadowbuf_idx(b, r, c);
  b->cells[idx].ch = (char)ch;
  b->cells[idx].attr = b->current_attr;

  b->cursor_col++;
  if (b->cursor_col > b->width)
  {
    b->cursor_col = 1;
    if (b->cursor_row < b->height)
      b->cursor_row++;
  }
}

/**
 * @brief Parse a CSI SGR sequence of the form ESC[ ... m.
 */
static const char *near ui_shadowbuf_parse_csi_sgr_attr(byte *cur_attr, byte default_attr, const char *s)
{
  int val = 0;
  int have_val = 0;

  if (!cur_attr || !s)
    return s;

  /* s points at the first byte after ESC[ */
  while (*s)
  {
    unsigned char ch = (unsigned char)*s;

    if (isdigit(ch))
    {
      val = (val * 10) + (ch - '0');
      have_val = 1;
      s++;
      continue;
    }

    if (ch == ';')
    {
      ui_shadowbuf_apply_sgr_attr(cur_attr, default_attr, have_val ? val : 0);
      val = 0;
      have_val = 0;
      s++;
      continue;
    }

    if (ch == 'm')
    {
      ui_shadowbuf_apply_sgr_attr(cur_attr, default_attr, have_val ? val : 0);
      return s + 1;
    }

    /* Unsupported CSI; skip until a final byte 0x40..0x7e. */
    if (ch >= 0x40 && ch <= 0x7e)
      return s + 1;

    s++;
  }

  return s;
}

static const char *near ui_shadowbuf_parse_csi_sgr(ui_shadowbuf_t *b, const char *s)
{
  if (!b)
    return s;

  return ui_shadowbuf_parse_csi_sgr_attr(&b->current_attr, b->default_attr, s);
}

int ui_shadowbuf_normalize_line(const char *text, byte start_attr, byte default_attr, ui_shadow_cell_t **out_cells, int *out_count, byte *out_end_attr)
{
  const unsigned char *p;
  ui_shadow_cell_t *cells;
  int cap;
  int n;
  byte cur_attr;

  if (!out_cells || !out_count)
    return 0;

  *out_cells = NULL;
  *out_count = 0;

  if (!text)
    text = "";

  /* Worst-case: one output cell per input byte. */
  cap = (int)strlen(text);
  if (cap < 1)
    cap = 1;

  cells = (ui_shadow_cell_t *)malloc((size_t)cap * sizeof(ui_shadow_cell_t));
  if (!cells)
    return 0;

  n = 0;
  cur_attr = start_attr;
  p = (const unsigned char *)text;

  while (*p && *p != '\n')
  {
    if (*p == 0x1B)
    {
      /* ANSI ESC */
      if (p[1] == '[')
      {
        const char *next = ui_shadowbuf_parse_csi_sgr_attr(&cur_attr, default_attr, (const char *)(p + 2));
        p = (const unsigned char *)next;
        continue;
      }

      /* Unknown ESC sequence; drop ESC. */
      p++;
      continue;
    }

    if (*p == 0x16)
    {
      /* AVATAR attribute sequences (subset).
       * - 0x16 0x01 <attr> : set attribute
       */
      if (p[1] == 0x01 && p[2])
      {
        cur_attr = (byte)p[2];
        p += 3;
        continue;
      }

      p++;
      continue;
    }

    /* Ignore CR in a single-line normalization context. */
    if (*p == '\r')
    {
      p++;
      continue;
    }

    if (n >= cap)
    {
      ui_shadow_cell_t *new_cells;
      cap = cap * 2;
      if (cap < 16)
        cap = 16;
      new_cells = (ui_shadow_cell_t *)realloc(cells, (size_t)cap * sizeof(ui_shadow_cell_t));
      if (!new_cells)
      {
        free(cells);
        return 0;
      }
      cells = new_cells;
    }

    cells[n].ch = (char)*p;
    cells[n].attr = cur_attr;
    n++;
    p++;
  }

  *out_cells = cells;
  *out_count = n;
  if (out_end_attr)
    *out_end_attr = cur_attr;

  return 1;
}

void ui_shadowbuf_free_cells(ui_shadow_cell_t *cells)
{
  if (cells)
    free(cells);
}

void ui_shadowbuf_write(ui_shadowbuf_t *b, const char *text)
{
  const unsigned char *p;

  if (!b || !text)
    return;

  p = (const unsigned char *)text;

  while (*p)
  {
    if (*p == 0x1B)
    {
      /* ANSI ESC */
      if (p[1] == '[')
      {
        const char *next = ui_shadowbuf_parse_csi_sgr(b, (const char *)(p + 2));
        p = (const unsigned char *)next;
        continue;
      }

      /* Unknown ESC sequence; drop ESC. */
      p++;
      continue;
    }

    if (*p == 0x16)
    {
      /* AVATAR control sequences (subset).
       * - 0x16 0x01 <attr> : set attribute
       */
      if (p[1] == 0x01 && p[2])
      {
        ui_shadowbuf_set_attr(b, (byte)p[2]);
        p += 3;
        continue;
      }

      /* Unknown/unsupported AVATAR control; drop marker. */
      p++;
      continue;
    }

    ui_shadowbuf_putc(b, (int)*p);
    p++;
  }
}

ui_shadow_block_t ui_shadowbuf_gettext(const ui_shadowbuf_t *b, int left, int top, int right, int bottom)
{
  ui_shadow_block_t out;
  int l;
  int t;
  int r;
  int bo;
  int w;
  int h;
  int rr;
  int cc;
  int idx;

  memset(&out, 0, sizeof(out));

  if (!b || !b->cells)
    return out;

  l = left;
  t = top;
  r = right;
  bo = bottom;

  if (l < 1)
    l = 1;
  if (t < 1)
    t = 1;
  if (r > b->width)
    r = b->width;
  if (bo > b->height)
    bo = b->height;

  w = (r >= l) ? (r - l + 1) : 0;
  h = (bo >= t) ? (bo - t + 1) : 0;

  out.width = w;
  out.height = h;

  if (w <= 0 || h <= 0)
    return out;

  out.cells = (ui_shadow_cell_t *)malloc((size_t)(w * h) * sizeof(ui_shadow_cell_t));
  if (!out.cells)
  {
    out.width = 0;
    out.height = 0;
    return out;
  }

  idx = 0;
  for (rr = 0; rr < h; rr++)
  {
    for (cc = 0; cc < w; cc++)
    {
      int src_idx = ui_shadowbuf_idx(b, t + rr, l + cc);
      out.cells[idx++] = b->cells[src_idx];
    }
  }

  return out;
}

void ui_shadowbuf_puttext(ui_shadowbuf_t *b, int left, int top, const ui_shadow_block_t *block)
{
  int rr;
  int cc;

  if (!b || !b->cells || !block || !block->cells)
    return;

  for (rr = 0; rr < block->height; rr++)
  {
    for (cc = 0; cc < block->width; cc++)
    {
      int dst_r = top + rr;
      int dst_c = left + cc;
      int dst_idx;
      int src_idx = rr * block->width + cc;

      if (dst_r < 1 || dst_c < 1 || dst_r > b->height || dst_c > b->width)
        continue;

      dst_idx = ui_shadowbuf_idx(b, dst_r, dst_c);
      b->cells[dst_idx] = block->cells[src_idx];
    }
  }
}

void ui_shadowbuf_free_block(ui_shadow_block_t *block)
{
  if (!block)
    return;

  if (block->cells)
    free(block->cells);

  block->cells = NULL;
  block->width = 0;
  block->height = 0;
}

int ui_shadowbuf_overlay_push(const ui_shadowbuf_t *b, int left, int top, int right, int bottom, ui_shadow_overlay_t *ov)
{
  if (!b || !ov)
    return 0;

  memset(ov, 0, sizeof(*ov));
  ov->left = left;
  ov->top = top;
  ov->under = ui_shadowbuf_gettext(b, left, top, right, bottom);

  if (ov->under.width <= 0 || ov->under.height <= 0 || !ov->under.cells)
  {
    ui_shadowbuf_free_block(&ov->under);
    return 0;
  }

  return 1;
}

void ui_shadowbuf_overlay_pop(ui_shadowbuf_t *b, ui_shadow_overlay_t *ov)
{
  if (!b || !ov)
    return;

  if (ov->under.cells)
    ui_shadowbuf_puttext(b, ov->left, ov->top, &ov->under);

  ui_shadowbuf_free_block(&ov->under);
  ov->left = 0;
  ov->top = 0;
}

void ui_shadowbuf_paint_region(const ui_shadowbuf_t *b, int screen_x, int screen_y, int left, int top, int right, int bottom)
{
  int l;
  int t;
  int r;
  int bo;
  int rr;
  int cc;
  int last_attr = -1;

  if (!b || !b->cells)
    return;

  l = left;
  t = top;
  r = right;
  bo = bottom;

  if (l < 1)
    l = 1;
  if (t < 1)
    t = 1;
  if (r > b->width)
    r = b->width;
  if (bo > b->height)
    bo = b->height;

  if (r < l || bo < t)
    return;

  for (rr = t; rr <= bo; rr++)
  {
    ui_goto(screen_y + (rr - 1), screen_x + (l - 1));

    for (cc = l; cc <= r; cc++)
    {
      int idx = ui_shadowbuf_idx(b, rr, cc);
      int a = (int)b->cells[idx].attr & 0xFF;

      if (last_attr != a)
      {
        ui_set_attr((byte)a);
        last_attr = a;
      }

      Putc((int)b->cells[idx].ch);
    }
  }

  vbuf_flush();
}
