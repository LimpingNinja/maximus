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

#include "prog.h"
#include "keys.h"

#include "ui_field.h"
#include "ui_scroll.h"

/**
 * @brief Duplicate a byte range into a new NUL-terminated string.
 */
static char *near ui_scroll_strdup_n(const char *s, int n)
{
  char *out;

  if (!s)
    s = "";

  if (n < 0)
    n = 0;

  out = (char *)malloc((size_t)n + 1);
  if (!out)
    return NULL;

  if (n > 0)
    memcpy(out, s, (size_t)n);

  out[n] = '\0';
  return out;
}

/**
 * @brief Free all wrapped cell lines.
 */
static void near ui_scroll_free_lines(ui_cell_line_t **lines, int *line_count)
{
  int i;

  if (!lines || !*lines || !line_count)
    return;

  for (i = 0; i < *line_count; i++)
    if ((*lines)[i].cells)
      ui_shadowbuf_free_cells((*lines)[i].cells);

  free(*lines);
  *lines = NULL;
  *line_count = 0;
}

/**
 * @brief Append one wrapped cell line to the line buffer.
 */
static int near ui_scroll_push_line(ui_cell_line_t **lines, int *line_count, const ui_shadow_cell_t *cells, int len)
{
  ui_cell_line_t *new_lines;
  ui_shadow_cell_t *dup;

  if (!lines || !line_count)
    return 0;

  dup = NULL;
  if (len > 0)
  {
    dup = (ui_shadow_cell_t *)malloc((size_t)len * sizeof(ui_shadow_cell_t));
    if (!dup)
      return 0;
    memcpy(dup, cells, (size_t)len * sizeof(ui_shadow_cell_t));
  }

  new_lines = (ui_cell_line_t *)realloc(*lines, (size_t)(*line_count + 1) * sizeof(ui_cell_line_t));
  if (!new_lines)
  {
    if (dup)
      free(dup);
    return 0;
  }

  new_lines[*line_count].cells = dup;
  new_lines[*line_count].len = len;
  *lines = new_lines;
  (*line_count)++;
  return 1;
}

/**
 * @brief Clamp view_top to a valid range for a given buffer/viewport.
 */
static void near ui_scroll_clamp_view(int *view_top, int line_count, int height, int *out_at_bottom)
{
  int max_top;

  if (!view_top)
    return;

  if (height < 1)
    height = 1;

  max_top = (line_count > height) ? (line_count - height) : 0;

  if (*view_top < 0)
    *view_top = 0;
  if (*view_top > max_top)
    *view_top = max_top;

  if (out_at_bottom)
    *out_at_bottom = (*view_top >= max_top);
}

void ui_scrolling_region_style_default(ui_scrolling_region_style_t *style)
{
  if (!style)
    return;

  style->attr = 0x07;
  style->scrollbar_attr = 0x07;
  style->flags = UI_SCROLL_REGION_AUTO_FOLLOW;
}

int ui_scrolling_region_overlay_push(const ui_scrolling_region_t *r, ui_shadow_overlay_t *ov)
{
  if (!r || !ov)
    return 0;

  if (!r->sb_valid)
    return 0;

  return ui_shadowbuf_overlay_push(&r->sb, 1, 1, r->sb.width, r->sb.height, ov);
}

void ui_scrolling_region_overlay_pop(ui_scrolling_region_t *r, ui_shadow_overlay_t *ov)
{
  if (!r || !ov)
    return;

  if (!r->sb_valid)
    return;

  ui_shadowbuf_overlay_pop(&r->sb, ov);
}

void ui_scrolling_region_init(ui_scrolling_region_t *r, int x, int y, int width, int height, int max_lines, const ui_scrolling_region_style_t *style)
{
  if (!r)
    return;

  memset(r, 0, sizeof(*r));

  r->x = x;
  r->y = y;
  r->width = (width > 0) ? width : 1;
  r->height = (height > 0) ? height : 1;
  r->max_lines = (max_lines > 0) ? max_lines : 1000;

  if (style)
    r->style = *style;
  else
    ui_scrolling_region_style_default(&r->style);

  r->lines = NULL;
  r->line_count = 0;
  r->view_top = 0;
  r->at_bottom = 1;

  r->sb_valid = 0;
  memset(&r->sb, 0, sizeof(r->sb));

  r->last_thumb_top = -1;
  r->last_thumb_len = -1;
}

void ui_scrolling_region_free(ui_scrolling_region_t *r)
{
  if (!r)
    return;

  ui_scroll_free_lines(&r->lines, &r->line_count);
  if (r->sb_valid)
    ui_shadowbuf_free(&r->sb);
  r->sb_valid = 0;
  r->view_top = 0;
  r->at_bottom = 1;
}

void ui_scrolling_region_clear(ui_scrolling_region_t *r)
{
  if (!r)
    return;

  ui_scroll_free_lines(&r->lines, &r->line_count);
  r->view_top = 0;
  r->at_bottom = 1;
  r->last_thumb_top = -1;
  r->last_thumb_len = -1;
}

/**
 * @brief Drop oldest lines to enforce max_lines.
 */
static void near ui_scrolling_region_trim(ui_scrolling_region_t *r)
{
  int excess;
  int i;

  if (!r || r->max_lines <= 0)
    return;

  if (r->line_count <= r->max_lines)
    return;

  excess = r->line_count - r->max_lines;
  if (excess <= 0)
    return;

  for (i = 0; i < excess; i++)
    if (r->lines[i].cells)
      ui_shadowbuf_free_cells(r->lines[i].cells);

  memmove(r->lines, r->lines + excess, (size_t)(r->line_count - excess) * sizeof(ui_cell_line_t));
  r->line_count -= excess;

  r->view_top -= excess;
  if (r->view_top < 0)
    r->view_top = 0;
}

int ui_scrolling_region_append(ui_scrolling_region_t *r, const char *text, int append_flags)
{
  const char *p;
  const char *line_start;
  int force_follow = 0;
  int force_nofollow = 0;
  byte cur_attr;

  if (!r)
    return 0;

  if (!text)
    text = "";

  force_follow = (append_flags & UI_SCROLL_APPEND_FOLLOW) != 0;
  force_nofollow = (append_flags & UI_SCROLL_APPEND_NOFOLLOW) != 0;

  cur_attr = r->style.attr;

  line_start = text;
  p = text;

  while (1)
  {
    if (*p == '\n' || *p == '\0')
    {
      int n = (int)(p - line_start);
      char *line;
      ui_shadow_cell_t *cells = NULL;
      int cell_count = 0;
      byte end_attr = cur_attr;
      int pos;

      /* Strip trailing CR if present. */
      if (n > 0 && line_start[n - 1] == '\r')
        n--;

      line = ui_scroll_strdup_n(line_start, n);
      if (!line)
        return 0;

      if (!ui_shadowbuf_normalize_line(line, cur_attr, r->style.attr, &cells, &cell_count, &end_attr))
      {
        free(line);
        return 0;
      }

      free(line);

      if (cell_count <= 0)
      {
        if (!ui_scroll_push_line(&r->lines, &r->line_count, NULL, 0))
        {
          if (cells)
            ui_shadowbuf_free_cells(cells);
          return 0;
        }
      }

      for (pos = 0; pos < cell_count; pos += r->width)
      {
        int seg_len = cell_count - pos;
        if (seg_len > r->width)
          seg_len = r->width;

        if (!ui_scroll_push_line(&r->lines, &r->line_count, cells + pos, seg_len))
        {
          ui_shadowbuf_free_cells(cells);
          return 0;
        }
      }

      if (cells)
        ui_shadowbuf_free_cells(cells);
      cur_attr = end_attr;

      if (*p == '\0')
        break;

      line_start = p + 1;
    }

    if (*p == '\0')
      break;

    p++;
  }

  ui_scrolling_region_trim(r);

  if (force_follow)
    r->at_bottom = 1;
  else if (force_nofollow)
    r->at_bottom = 0;

  if ((r->style.flags & UI_SCROLL_REGION_AUTO_FOLLOW) && r->at_bottom)
  {
    r->view_top = (r->line_count > r->height) ? (r->line_count - r->height) : 0;
    r->at_bottom = 1;
  }
  else
  {
    ui_scroll_clamp_view(&r->view_top, r->line_count, r->height, &r->at_bottom);
  }

  return 1;
}

static void near ui_scrolling_region_ensure_sb(ui_scrolling_region_t *r)
{
  int want_w;
  int want_h;

  if (!r)
    return;

  want_w = r->width + ((r->style.flags & UI_SCROLL_REGION_SHOW_SCROLLBAR) ? 1 : 0);
  want_h = r->height;
  if (want_w < 1)
    want_w = 1;
  if (want_h < 1)
    want_h = 1;

  if (!r->sb_valid || r->sb.width != want_w || r->sb.height != want_h)
  {
    if (r->sb_valid)
      ui_shadowbuf_free(&r->sb);

    ui_shadowbuf_init(&r->sb, want_w, want_h, r->style.attr);
    r->sb_valid = 1;
  }

  r->sb.default_attr = r->style.attr;
}

void ui_scrolling_region_render(ui_scrolling_region_t *r)
{
  int row;
  int idx;
  int text_w;

  if (!r)
    return;

  ui_scrolling_region_ensure_sb(r);
  ui_shadowbuf_clear(&r->sb, r->style.attr);

  text_w = r->width;
  if (text_w < 1)
    text_w = 1;

  for (row = 0; row < r->height; row++)
  {
    idx = r->view_top + row;

    ui_shadowbuf_goto(&r->sb, row + 1, 1);
    {
      int col;
      int len = (idx >= 0 && idx < r->line_count) ? r->lines[idx].len : 0;
      ui_shadow_cell_t *cells = (idx >= 0 && idx < r->line_count) ? r->lines[idx].cells : NULL;

      for (col = 0; col < text_w; col++)
      {
        if (cells && col < len)
        {
          ui_shadowbuf_set_attr(&r->sb, cells[col].attr);
          ui_shadowbuf_putc(&r->sb, (int)cells[col].ch);
        }
        else
        {
          ui_shadowbuf_set_attr(&r->sb, r->style.attr);
          ui_shadowbuf_putc(&r->sb, ' ');
        }
      }
    }
  }

  if (r->style.flags & UI_SCROLL_REGION_SHOW_SCROLLBAR)
  {
    int thumb_top;
    int thumb_len;
    int max_top;
    int usable;

    if (r->line_count <= r->height)
    {
      thumb_top = 0;
      thumb_len = r->height;
    }
    else
    {
      max_top = (r->line_count > r->height) ? (r->line_count - r->height) : 1;

      thumb_len = (r->height * r->height) / (r->line_count > 0 ? r->line_count : 1);
      if (thumb_len < 1)
        thumb_len = 1;
      if (thumb_len > r->height)
        thumb_len = r->height;

      usable = r->height - thumb_len;
      if (usable < 1)
        usable = 1;

      thumb_top = (r->view_top * usable) / (max_top > 0 ? max_top : 1);
      if (thumb_top < 0)
        thumb_top = 0;
      if (thumb_top > r->height - thumb_len)
        thumb_top = r->height - thumb_len;
    }

    ui_shadowbuf_set_attr(&r->sb, r->style.scrollbar_attr);
    {
      int sb_col = r->width + 1;
      int sb_row;
      for (sb_row = 0; sb_row < r->height; sb_row++)
      {
        ui_shadowbuf_goto(&r->sb, sb_row + 1, sb_col);
        ui_shadowbuf_putc(&r->sb, (sb_row >= thumb_top && sb_row < (thumb_top + thumb_len)) ? (char)219 : (char)176);
      }
    }

    r->last_thumb_top = thumb_top;
    r->last_thumb_len = thumb_len;
  }

  ui_shadowbuf_paint_region(&r->sb, r->x, r->y, 1, 1, r->sb.width, r->sb.height);
}

int ui_scrolling_region_handle_key(ui_scrolling_region_t *r, int key)
{
  int old_top;

  if (!r)
    return 0;

  old_top = r->view_top;

  switch (key)
  {
    case K_UP:
      r->view_top -= 1;
      break;

    case K_DOWN:
      r->view_top += 1;
      break;

    case K_PGUP:
    case 0x15: /* Ctrl+U */
      r->view_top -= r->height;
      break;

    case K_PGDN:
    case 0x04: /* Ctrl+D */
      r->view_top += r->height;
      break;

    case K_HOME:
    case 0x08: /* Ctrl+H */
      r->view_top = 0;
      break;

    case K_END:
    case 0x05: /* Ctrl+E */
      r->view_top = (r->line_count > r->height) ? (r->line_count - r->height) : 0;
      break;

    default:
      return 0;
  }

  ui_scroll_clamp_view(&r->view_top, r->line_count, r->height, &r->at_bottom);

  return (r->view_top != old_top);
}

void ui_text_viewer_style_default(ui_text_viewer_style_t *style)
{
  if (!style)
    return;

  style->attr = 0x07;
  style->status_attr = 0x07;
  style->scrollbar_attr = 0x07;
  style->flags = UI_TBV_SHOW_STATUS | UI_TBV_SHOW_SCROLLBAR;
}

int ui_text_viewer_overlay_push(const ui_text_viewer_t *v, ui_shadow_overlay_t *ov)
{
  if (!v || !ov)
    return 0;

  if (!v->sb_valid)
    return 0;

  return ui_shadowbuf_overlay_push(&v->sb, 1, 1, v->sb.width, v->sb.height, ov);
}

void ui_text_viewer_overlay_pop(ui_text_viewer_t *v, ui_shadow_overlay_t *ov)
{
  if (!v || !ov)
    return;

  if (!v->sb_valid)
    return;

  ui_shadowbuf_overlay_pop(&v->sb, ov);
}

void ui_text_viewer_init(ui_text_viewer_t *v, int x, int y, int width, int height, const ui_text_viewer_style_t *style)
{
  if (!v)
    return;

  memset(v, 0, sizeof(*v));

  v->x = x;
  v->y = y;
  v->width = (width > 0) ? width : 1;
  v->height = (height > 0) ? height : 1;

  if (style)
    v->style = *style;
  else
    ui_text_viewer_style_default(&v->style);

  v->lines = NULL;
  v->line_count = 0;
  v->view_top = 0;

  v->sb_valid = 0;
  memset(&v->sb, 0, sizeof(v->sb));

  v->last_thumb_top = -1;
  v->last_thumb_len = -1;
}

void ui_text_viewer_free(ui_text_viewer_t *v)
{
  if (!v)
    return;

  ui_scroll_free_lines(&v->lines, &v->line_count);
  if (v->sb_valid)
    ui_shadowbuf_free(&v->sb);
  v->sb_valid = 0;
  v->view_top = 0;
  v->last_thumb_top = -1;
  v->last_thumb_len = -1;
}

int ui_text_viewer_set_text(ui_text_viewer_t *v, const char *text)
{
  const char *p;
  const char *line_start;
  byte cur_attr;

  if (!v)
    return 0;

  ui_scroll_free_lines(&v->lines, &v->line_count);

  if (!text)
    text = "";

  line_start = text;
  p = text;

  cur_attr = v->style.attr;

  while (1)
  {
    if (*p == '\n' || *p == '\0')
    {
      int n = (int)(p - line_start);
      char *line;
      ui_shadow_cell_t *cells = NULL;
      int cell_count = 0;
      byte end_attr = cur_attr;
      int pos;

      if (n > 0 && line_start[n - 1] == '\r')
        n--;

      line = ui_scroll_strdup_n(line_start, n);
      if (!line)
        return 0;

      if (!ui_shadowbuf_normalize_line(line, cur_attr, v->style.attr, &cells, &cell_count, &end_attr))
      {
        free(line);
        return 0;
      }

      free(line);

      if (cell_count <= 0)
      {
        if (!ui_scroll_push_line(&v->lines, &v->line_count, NULL, 0))
        {
          if (cells)
            ui_shadowbuf_free_cells(cells);
          return 0;
        }
      }

      for (pos = 0; pos < cell_count; pos += v->width)
      {
        int seg_len = cell_count - pos;
        if (seg_len > v->width)
          seg_len = v->width;

        if (!ui_scroll_push_line(&v->lines, &v->line_count, cells + pos, seg_len))
        {
          ui_shadowbuf_free_cells(cells);
          return 0;
        }
      }

      ui_shadowbuf_free_cells(cells);
      cur_attr = end_attr;

      if (*p == '\0')
        break;

      line_start = p + 1;
    }

    if (*p == '\0')
      break;

    p++;
  }

  if (v->line_count < 1)
  {
    if (!ui_scroll_push_line(&v->lines, &v->line_count, NULL, 0))
      return 0;
  }

  v->view_top = 0;
  v->last_thumb_top = -1;
  v->last_thumb_len = -1;

  return 1;
}

static int near ui_text_viewer_text_height(const ui_text_viewer_t *v)
{
  if (!v)
    return 1;

  if ((v->style.flags & UI_TBV_SHOW_STATUS) && v->height > 1)
    return v->height - 1;

  return v->height;
}

static void near ui_text_viewer_render_status(ui_text_viewer_t *v)
{
  char buf[96];
  int total;
  int cur;
  int percent;
  int len;
  int i;

  if (!v)
    return;

  total = v->line_count;
  cur = v->view_top + 1;
  if (total > 1)
    percent = (v->view_top * 100) / (total - 1);
  else
    percent = 0;

  sprintf(buf, " Line %d/%d (%d%%) ", cur, total, percent);
  len = (int)strlen(buf);
  if (len > v->width)
    buf[v->width] = '\0';

  if (!v->sb_valid)
    return;

  ui_shadowbuf_goto(&v->sb, v->height, 1);
  ui_shadowbuf_set_attr(&v->sb, v->style.status_attr);
  for (i = 0; i < v->width; i++)
    ui_shadowbuf_putc(&v->sb, (i < len && buf[i]) ? buf[i] : ' ');

  if (v->style.flags & UI_TBV_SHOW_SCROLLBAR)
  {
    ui_shadowbuf_goto(&v->sb, v->height, v->width + 1);
    ui_shadowbuf_set_attr(&v->sb, v->style.attr);
    ui_shadowbuf_putc(&v->sb, ' ');
  }
}

static void near ui_text_viewer_ensure_sb(ui_text_viewer_t *v)
{
  int want_w;
  int want_h;

  if (!v)
    return;

  want_w = v->width + ((v->style.flags & UI_TBV_SHOW_SCROLLBAR) ? 1 : 0);
  want_h = v->height;
  if (want_w < 1)
    want_w = 1;
  if (want_h < 1)
    want_h = 1;

  if (!v->sb_valid || v->sb.width != want_w || v->sb.height != want_h)
  {
    if (v->sb_valid)
      ui_shadowbuf_free(&v->sb);
    ui_shadowbuf_init(&v->sb, want_w, want_h, v->style.attr);
    v->sb_valid = 1;
  }

  v->sb.default_attr = v->style.attr;
}

void ui_text_viewer_render(ui_text_viewer_t *v)
{
  int row;
  int idx;
  int th;
  int text_w;

  if (!v)
    return;

  ui_text_viewer_ensure_sb(v);
  ui_shadowbuf_clear(&v->sb, v->style.attr);

  th = ui_text_viewer_text_height(v);
  if (th < 1)
    th = 1;

  text_w = v->width;
  if (text_w < 1)
    text_w = 1;

  ui_scroll_clamp_view(&v->view_top, v->line_count, th, NULL);

  for (row = 0; row < th; row++)
  {
    idx = v->view_top + row;

    ui_shadowbuf_goto(&v->sb, row + 1, 1);
    {
      int col;
      int len = (idx >= 0 && idx < v->line_count) ? v->lines[idx].len : 0;
      ui_shadow_cell_t *cells = (idx >= 0 && idx < v->line_count) ? v->lines[idx].cells : NULL;

      for (col = 0; col < text_w; col++)
      {
        if (cells && col < len)
        {
          ui_shadowbuf_set_attr(&v->sb, cells[col].attr);
          ui_shadowbuf_putc(&v->sb, (int)cells[col].ch);
        }
        else
        {
          ui_shadowbuf_set_attr(&v->sb, v->style.attr);
          ui_shadowbuf_putc(&v->sb, ' ');
        }
      }
    }
  }

  if (v->style.flags & UI_TBV_SHOW_SCROLLBAR)
  {
    int thumb_top;
    int thumb_len;
    int max_top;
    int usable;

    if (v->line_count <= th)
    {
      thumb_top = 0;
      thumb_len = th;
    }
    else
    {
      max_top = (v->line_count > th) ? (v->line_count - th) : 1;

      thumb_len = (th * th) / (v->line_count > 0 ? v->line_count : 1);
      if (thumb_len < 1)
        thumb_len = 1;
      if (thumb_len > th)
        thumb_len = th;

      usable = th - thumb_len;
      if (usable < 1)
        usable = 1;

      thumb_top = (v->view_top * usable) / (max_top > 0 ? max_top : 1);
      if (thumb_top < 0)
        thumb_top = 0;
      if (thumb_top > th - thumb_len)
        thumb_top = th - thumb_len;
    }

    ui_shadowbuf_set_attr(&v->sb, v->style.scrollbar_attr);
    {
      int sb_col = v->width + 1;
      int sb_row;
      for (sb_row = 0; sb_row < th; sb_row++)
      {
        ui_shadowbuf_goto(&v->sb, sb_row + 1, sb_col);
        ui_shadowbuf_putc(&v->sb, (sb_row >= thumb_top && sb_row < (thumb_top + thumb_len)) ? (char)219 : (char)176);
      }
    }

    v->last_thumb_top = thumb_top;
    v->last_thumb_len = thumb_len;
  }

  if (v->style.flags & UI_TBV_SHOW_STATUS)
    ui_text_viewer_render_status(v);

  ui_shadowbuf_paint_region(&v->sb, v->x, v->y, 1, 1, v->sb.width, v->sb.height);
}

int ui_text_viewer_handle_key(ui_text_viewer_t *v, int key)
{
  int old_top;
  int th;

  if (!v)
    return 0;

  th = ui_text_viewer_text_height(v);
  if (th < 1)
    th = 1;

  old_top = v->view_top;

  switch (key)
  {
    case K_UP:
      v->view_top -= 1;
      break;

    case K_DOWN:
      v->view_top += 1;
      break;

    case K_PGUP:
    case 0x15: /* Ctrl+U */
      v->view_top -= th;
      break;

    case K_PGDN:
    case 0x04: /* Ctrl+D */
      v->view_top += th;
      break;

    case K_HOME:
    case 0x08: /* Ctrl+H */
      v->view_top = 0;
      break;

    case K_END:
    case 0x05: /* Ctrl+E */
      v->view_top = (v->line_count > th) ? (v->line_count - th) : 0;
      break;

    default:
      return 0;
  }

  ui_scroll_clamp_view(&v->view_top, v->line_count, th, NULL);

  return (v->view_top != old_top);
}

int ui_text_viewer_read_key(ui_text_viewer_t *v)
{
  int key;

  if (!v)
    return ui_read_key();

  key = ui_read_key();

  if (ui_text_viewer_handle_key(v, key))
  {
    ui_text_viewer_render(v);
    return 0;
  }

  return key;
}
