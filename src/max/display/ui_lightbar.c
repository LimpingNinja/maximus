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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "prog.h"
#include "keys.h"
#include "mm.h"
#include "protod.h"

#include "ui_field.h"
#include "ui_lightbar.h"

typedef struct {
  char *disp;
  char *orig;
  int hotkey; /* lowercase ascii or 0 */
  int hotkey_pos; /* index into disp (or -1) */
} ui_lb_item_t;

 typedef struct {
   ui_lb_item_t it;
   int x;
   int y;
   int width;
   int justify;
   int width_used;
   long cx2;
 } ui_lb_pos_item_t;

static int near ui_lb_is_printable(int ch)
{
  return (ch >= 32 && ch <= 126);
}

static void near ui_lb_hide_cursor(int *did_hide)
{
  if (did_hide)
    *did_hide = 0;

  if (usr.video == GRAPH_ANSI || usr.video == GRAPH_AVATAR)
  {
    Printf("\x1b[?25l");
    if (did_hide)
      *did_hide = 1;
  }
}

static void near ui_lb_show_cursor(int did_hide)
{
  if (!did_hide)
    return;

  if (usr.video == GRAPH_ANSI || usr.video == GRAPH_AVATAR)
    Printf("\x1b[?25h");
}

static char *near ui_lb_strdup(const char *s)
{
  char *rc;
  size_t n;

  if (!s)
    s = "";

  n = strlen(s);
  rc = (char *)malloc(n + 1);
  if (!rc)
    return NULL;

  memcpy(rc, s, n);
  rc[n] = '\0';
  return rc;
}

static char *near ui_lb_strip_marker(const char *s, int *out_hotkey, int *out_hotkey_pos)
{
  const char *p;
  char *rc;
  size_t n;
  size_t i;
  int hk = 0;

  if (out_hotkey)
    *out_hotkey = 0;

  if (out_hotkey_pos)
    *out_hotkey_pos = -1;

  if (!s)
    return ui_lb_strdup("");

  p = strchr(s, '[');
  if (p && p[1] && p[2] == ']')
  {
    hk = tolower((unsigned char)p[1]);

    n = strlen(s);
    rc = (char *)malloc(n + 1);
    if (!rc)
      return NULL;

    /* Copy everything except the "[X]" marker */
    i = 0;
    while (*s)
    {
      if (s == p)
      {
        if (out_hotkey_pos)
          *out_hotkey_pos = (int)i;
        rc[i++] = p[1];
        s += 3;
        continue;
      }
      rc[i++] = *s++;
    }
    rc[i] = '\0';

    if (out_hotkey)
      *out_hotkey = hk;

    return rc;
  }

  return ui_lb_strdup(s);
}

static void near ui_lb_autohotkey(ui_lb_item_t *it, byte used[256])
{
  const char *s;

  if (!it || it->hotkey)
    return;

  if (!it->disp)
    return;

  s = it->disp;
  while (*s)
  {
    if (isalpha((unsigned char)*s))
    {
      int hk = tolower((unsigned char)*s);
      if (!used[(byte)hk])
      {
        it->hotkey = hk;
        used[(byte)hk] = 1;
        return;
      }
    }
    s++;
  }
}

static void near ui_lb_pos_compute_geometry(ui_lightbar_pos_menu_t *m, ui_lb_pos_item_t *items, int idx)
{
  const char *text;
  int len;
  int margin;

  if (!m || !items || idx < 0 || idx >= m->count)
    return;

  text = m->show_brackets ? (items[idx].it.orig ? items[idx].it.orig : "") : (items[idx].it.disp ? items[idx].it.disp : "");
  len = (int)strlen(text);

  margin = (m && m->margin > 0) ? m->margin : 0;
  items[idx].width_used = (items[idx].width > 0) ? items[idx].width : len;
  items[idx].width_used += margin;
  if (items[idx].width_used < 1)
    items[idx].width_used = 1;

  items[idx].cx2 = (long)(2 * items[idx].x + (items[idx].width_used - 1));
}

static void near ui_lb_draw_pos_item(ui_lightbar_pos_menu_t *m, ui_lb_pos_item_t *items, int idx, int selected)
{
  const char *s;
  int len;
  int width;
  int pad;
  int left_pad;
  int right_pad;
  int i;
  byte attr;
  byte hk_attr;
  int hotkey;
  int justify;

  if (!m || !items || idx < 0 || idx >= m->count)
    return;

  s = m->show_brackets ? (items[idx].it.orig ? items[idx].it.orig : "") : (items[idx].it.disp ? items[idx].it.disp : "");
  hotkey = items[idx].it.hotkey;
  justify = items[idx].justify;

  len = (int)strlen(s);
  width = items[idx].width_used;
  if (width < 1)
    width = 1;

  if (len > width)
    len = width;

  pad = width - len;
  if (pad < 0)
    pad = 0;

  switch (justify)
  {
    case UI_JUSTIFY_RIGHT:
      left_pad = pad;
      right_pad = 0;
      break;

    case UI_JUSTIFY_CENTER:
      left_pad = pad / 2;
      right_pad = pad - left_pad;
      break;

    default:
      left_pad = 0;
      right_pad = pad;
      break;
  }

  attr = selected ? m->selected_attr : m->normal_attr;
  hk_attr = selected
    ? (m->hotkey_highlight_attr ? m->hotkey_highlight_attr : attr)
    : (m->hotkey_attr ? m->hotkey_attr : attr);

  ui_set_attr(attr);
  ui_goto(items[idx].y, items[idx].x);

  for (i = 0; i < left_pad; i++)
    Putc(' ');

  if (hotkey && ((selected && m->hotkey_highlight_attr) || (!selected && m->hotkey_attr)))
  {
    int hk_index = -1;

    if (m->show_brackets)
    {
      const char *p = strchr(s, '[');
      if (p && p[1] && p[2] == ']')
        hk_index = (int)(p - s + 1);
    }
    else
    {
      hk_index = items[idx].it.hotkey_pos;
    }

    if (hk_index < 0 || hk_index >= len)
    {
      for (i = 0; i < len; i++)
      {
        if (tolower((unsigned char)s[i]) == hotkey)
        {
          hk_index = i;
          break;
        }
      }
    }

    for (i = 0; i < len; i++)
    {
      if (i == hk_index)
      {
        ui_set_attr(hk_attr);
        Putc(s[i]);
        ui_set_attr(attr);
      }
      else
        Putc(s[i]);
    }
  }
  else
  {
    for (i = 0; i < len; i++)
      Putc(s[i]);
  }

  for (i = 0; i < right_pad; i++)
    Putc(' ');
}

static void near ui_sp_draw_option(int row, int col, const ui_lb_item_t *opt, int selected, byte normal_attr, byte selected_attr, byte hotkey_attr, int strip_brackets);

static void near ui_sp_draw_option_margined(int row, int col, const ui_lb_item_t *opt, int selected, byte normal_attr, byte selected_attr, byte hotkey_attr, int strip_brackets, int margin)
{
  int i;
  int safe_margin;

  safe_margin = (margin > 0) ? margin : 0;

  ui_set_attr(selected ? selected_attr : normal_attr);
  ui_goto(row, col);

  for (i = 0; i < safe_margin; i++)
    Putc(' ');

  ui_sp_draw_option(row, col + safe_margin, opt, selected, normal_attr, selected_attr, hotkey_attr, strip_brackets);

  for (i = 0; i < safe_margin; i++)
    Putc(' ');
}

static int near ui_lb_find_neighbor_pos(ui_lb_pos_item_t *items, int count, int cur, int direction, int wrap)
{
  long cur_cx2;
  int cur_y;
  int i;
  long best_p = 0;
  long best_s = 0;
  int best_i = -1;
  int have_best = 0;

  if (!items || count < 1 || cur < 0 || cur >= count)
    return -1;

  cur_cx2 = items[cur].cx2;
  cur_y = items[cur].y;

  for (i = 0; i < count; i++)
  {
    long dx2;
    long adx2;
    long dy;
    long ady;
    long p;
    long s;

    if (i == cur)
      continue;

    dx2 = items[i].cx2 - cur_cx2;
    dy = (long)items[i].y - (long)cur_y;
    adx2 = dx2 < 0 ? -dx2 : dx2;
    ady = dy < 0 ? -dy : dy;

    switch (direction)
    {
      case K_DOWN:
        if (dy <= 0)
          continue;
        p = dy;
        s = adx2;
        break;
      case K_UP:
        if (dy >= 0)
          continue;
        p = -dy;
        s = adx2;
        break;
      case K_RIGHT:
        if (dx2 <= 0)
          continue;
        p = dx2;
        s = ady;
        break;
      case K_LEFT:
        if (dx2 >= 0)
          continue;
        p = -dx2;
        s = ady;
        break;
      default:
        continue;
    }

    if (!have_best || p < best_p || (p == best_p && (s < best_s || (s == best_s && i < best_i))))
    {
      have_best = 1;
      best_p = p;
      best_s = s;
      best_i = i;
    }
  }

  if (have_best)
    return best_i;

  if (!wrap)
    return -1;

  if (direction == K_DOWN || direction == K_UP)
  {
    int extreme_y;
    long best_dx = 0;
    int best = -1;
    int have = 0;

    extreme_y = items[cur].y;
    if (direction == K_DOWN)
    {
      for (i = 0; i < count; i++)
        if (i != cur && items[i].y < extreme_y)
          extreme_y = items[i].y;
    }
    else
    {
      for (i = 0; i < count; i++)
        if (i != cur && items[i].y > extreme_y)
          extreme_y = items[i].y;
    }

    for (i = 0; i < count; i++)
    {
      long dx;

      if (i == cur || items[i].y != extreme_y)
        continue;

      dx = items[i].cx2 - cur_cx2;
      if (dx < 0)
        dx = -dx;

      if (!have || dx < best_dx || (dx == best_dx && i < best))
      {
        have = 1;
        best_dx = dx;
        best = i;
      }
    }

    return best;
  }
  else
  {
    long extreme;
    long best_dy = 0;
    int best = -1;
    int have = 0;

    extreme = items[cur].cx2;
    if (direction == K_RIGHT)
    {
      for (i = 0; i < count; i++)
        if (i != cur && items[i].cx2 < extreme)
          extreme = items[i].cx2;
    }
    else
    {
      for (i = 0; i < count; i++)
        if (i != cur && items[i].cx2 > extreme)
          extreme = items[i].cx2;
    }

    for (i = 0; i < count; i++)
    {
      long dy;

      if (i == cur || items[i].cx2 != extreme)
        continue;

      dy = (long)items[i].y - (long)cur_y;
      if (dy < 0)
        dy = -dy;

      if (!have || dy < best_dy || (dy == best_dy && i < best))
      {
        have = 1;
        best_dy = dy;
        best = i;
      }
    }

    return best;
  }
}

int ui_lightbar_run_pos_hotkey(ui_lightbar_pos_menu_t *m, int *out_key)
{
  ui_lb_pos_item_t *items = NULL;
  byte used[256];
  int i;
  int selected = 0;
  int ch;
  int did_hide_cursor = 0;

  if (out_key)
    *out_key = 0;

  if (!m || !m->items || m->count < 1)
    return -1;

  memset(used, 0, sizeof(used));

  items = (ui_lb_pos_item_t *)calloc((size_t)m->count, sizeof(ui_lb_pos_item_t));
  if (!items)
    return -1;

  for (i = 0; i < m->count; i++)
  {
    const char *src = m->items[i].text ? m->items[i].text : "";
    items[i].it.orig = ui_lb_strdup(src);
    items[i].it.disp = ui_lb_strip_marker(src, &items[i].it.hotkey, &items[i].it.hotkey_pos);
    if (items[i].it.hotkey)
      used[(byte)items[i].it.hotkey] = 1;

    items[i].x = m->items[i].x;
    items[i].y = m->items[i].y;
    items[i].width = m->items[i].width;
    items[i].justify = m->items[i].justify;
  }

  if (m->enable_hotkeys)
  {
    for (i = 0; i < m->count; i++)
      ui_lb_autohotkey(&items[i].it, used);
  }

  for (i = 0; i < m->count; i++)
    ui_lb_pos_compute_geometry(m, items, i);

  ui_lb_hide_cursor(&did_hide_cursor);

  for (i = 0; i < m->count; i++)
    ui_lb_draw_pos_item(m, items, i, (i == selected));

  ui_goto(items[selected].y, items[selected].x);
  vbuf_flush();

  while (1)
  {
    ch = ui_read_key();

    switch (ch)
    {
      case K_RETURN:
        if (out_key)
          *out_key = items[selected].it.hotkey;
        for (i = 0; i < m->count; i++)
        {
          free(items[i].it.disp);
          free(items[i].it.orig);
        }
        free(items);
        ui_lb_show_cursor(did_hide_cursor);
        return selected;

      case K_ESC:
        for (i = 0; i < m->count; i++)
        {
          free(items[i].it.disp);
          free(items[i].it.orig);
        }
        free(items);
        ui_lb_show_cursor(did_hide_cursor);
        return -1;

      case K_UP:
      case K_DOWN:
      case K_LEFT:
      case K_RIGHT:
      {
        int next = ui_lb_find_neighbor_pos(items, m->count, selected, ch, m->wrap);
        if (next >= 0 && next != selected)
        {
          ui_lb_draw_pos_item(m, items, selected, 0);
          selected = next;
          ui_lb_draw_pos_item(m, items, selected, 1);
          ui_goto(items[selected].y, items[selected].x);
          vbuf_flush();
        }
        break;
      }

      default:
        if (m->enable_hotkeys && ui_lb_is_printable(ch))
        {
          int key = tolower((unsigned char)ch);
          for (i = 0; i < m->count; i++)
          {
            if (items[i].it.hotkey && items[i].it.hotkey == key)
            {
              int j;
              if (out_key)
                *out_key = items[i].it.hotkey;
              for (j = 0; j < m->count; j++)
              {
                free(items[j].it.disp);
                free(items[j].it.orig);
              }
              free(items);
              ui_lb_show_cursor(did_hide_cursor);
              return i;
            }
          }
        }
        break;
    }
  }
}

static void near ui_lb_draw_item(
    ui_lightbar_menu_t *m,
    ui_lb_item_t *items,
    int idx,
    int selected,
    int width)
{
  const char *s;
  int len;
  int left_pad = 0;
  int right_pad = 0;
  int pad;
  int i;
  byte attr;
  byte hk_attr;
  int hotkey;

  if (!m || !items || idx < 0 || idx >= m->count)
    return;

  s = m->show_brackets ? (items[idx].orig ? items[idx].orig : "") : (items[idx].disp ? items[idx].disp : "");
  hotkey = items[idx].hotkey;
  len = (int)strlen(s);
  if (len > width)
    len = width;

  pad = width - len;
  if (pad < 0)
    pad = 0;

  switch (m->justify)
  {
    case UI_JUSTIFY_RIGHT:
      left_pad = pad;
      right_pad = 0;
      break;
    case UI_JUSTIFY_CENTER:
      left_pad = pad / 2;
      right_pad = pad - left_pad;
      break;
    default:
      left_pad = 0;
      right_pad = pad;
      break;
  }

  attr = selected ? m->selected_attr : m->normal_attr;
  hk_attr = selected
    ? (m->hotkey_highlight_attr ? m->hotkey_highlight_attr : attr)
    : (m->hotkey_attr ? m->hotkey_attr : attr);
  ui_set_attr(attr);
  ui_goto(m->y + idx, m->x);

  for (i = 0; i < left_pad; i++)
    Putc(' ');

  if (hotkey && ((selected && m->hotkey_highlight_attr) || (!selected && m->hotkey_attr)))
  {
    int hk_index = -1;

    if (m->show_brackets)
    {
      const char *p = strchr(s, '[');
      if (p && p[1] && p[2] == ']')
        hk_index = (int)(p - s + 1);
    }
    else
    {
      hk_index = items[idx].hotkey_pos;
    }

    if (hk_index < 0 || hk_index >= len)
    {
      for (i = 0; i < len; i++)
      {
        if (tolower((unsigned char)s[i]) == hotkey)
        {
          hk_index = i;
          break;
        }
      }
    }

    for (i = 0; i < len; i++)
    {
      if (i == hk_index)
      {
        ui_set_attr(hk_attr);
        Putc(s[i]);
        ui_set_attr(attr);
      }
      else
        Putc(s[i]);
    }
  }
  else
  {
    for (i = 0; i < len; i++)
      Putc(s[i]);
  }

  for (i = 0; i < right_pad; i++)
    Putc(' ');
}

int ui_lightbar_run(ui_lightbar_menu_t *m)
{
  ui_lb_item_t *items = NULL;
  byte used[256];
  int i;
  int width;
  int margin;
  int selected = 0;
  int ch;
  int did_hide_cursor = 0;

  if (!m || !m->items || m->count < 1)
    return -1;

  memset(used, 0, sizeof(used));

  items = (ui_lb_item_t *)calloc((size_t)m->count, sizeof(ui_lb_item_t));
  if (!items)
    return -1;

  /* Parse items */
  for (i = 0; i < m->count; i++)
  {
    items[i].orig = ui_lb_strdup(m->items[i]);
    items[i].disp = ui_lb_strip_marker(m->items[i], &items[i].hotkey, &items[i].hotkey_pos);
    if (items[i].hotkey)
      used[(byte)items[i].hotkey] = 1;
  }

  if (m->enable_hotkeys)
  {
    for (i = 0; i < m->count; i++)
      ui_lb_autohotkey(&items[i], used);
  }

  ui_lb_hide_cursor(&did_hide_cursor);

  /* Determine width */
  margin = (m && m->margin > 0) ? m->margin : 0;
  width = m->width;
  if (width <= 0)
  {
    width = 1;
    for (i = 0; i < m->count; i++)
    {
      const char *text = m->show_brackets ? items[i].orig : items[i].disp;
      int len = text ? (int)strlen(text) : 0;
      if (len > width)
        width = len;
    }
  }
  width += margin;
  if (width < 1)
    width = 1;

  /* Initial paint */
  for (i = 0; i < m->count; i++)
    ui_lb_draw_item(m, items, i, (i == selected), width);

  ui_goto(m->y + selected, m->x);
  vbuf_flush();

  while (1)
  {
    ch = ui_read_key();

    switch (ch)
    {
      case K_RETURN:
        for (i = 0; i < m->count; i++)
        {
          free(items[i].disp);
          free(items[i].orig);
        }
        free(items);
        ui_lb_show_cursor(did_hide_cursor);
        return selected;

      case K_ESC:
        for (i = 0; i < m->count; i++)
        {
          free(items[i].disp);
          free(items[i].orig);
        }
        free(items);
        ui_lb_show_cursor(did_hide_cursor);
        return -1;

      case K_UP:
        if (selected > 0)
        {
          ui_lb_draw_item(m, items, selected, 0, width);
          selected--;
          ui_lb_draw_item(m, items, selected, 1, width);
          ui_goto(m->y + selected, m->x);
          vbuf_flush();
        }
        else if (m->wrap)
        {
          ui_lb_draw_item(m, items, selected, 0, width);
          selected = m->count - 1;
          ui_lb_draw_item(m, items, selected, 1, width);
          ui_goto(m->y + selected, m->x);
          vbuf_flush();
        }
        break;

      case K_DOWN:
        if (selected < m->count - 1)
        {
          ui_lb_draw_item(m, items, selected, 0, width);
          selected++;
          ui_lb_draw_item(m, items, selected, 1, width);
          ui_goto(m->y + selected, m->x);
          vbuf_flush();
        }
        else if (m->wrap)
        {
          ui_lb_draw_item(m, items, selected, 0, width);
          selected = 0;
          ui_lb_draw_item(m, items, selected, 1, width);
          ui_goto(m->y + selected, m->x);
          vbuf_flush();
        }
        break;

      default:
        if (m->enable_hotkeys && ui_lb_is_printable(ch))
        {
          int key = tolower((unsigned char)ch);
          for (i = 0; i < m->count; i++)
          {
            if (items[i].hotkey && items[i].hotkey == key)
            {
              for (ch = 0; ch < m->count; ch++)
              {
                free(items[ch].disp);
                free(items[ch].orig);
              }
              free(items);
              ui_lb_show_cursor(did_hide_cursor);
              return i;
            }
          }
        }
        break;
    }
  }
}

int ui_lightbar_run_hotkey(ui_lightbar_menu_t *m, int *out_key)
{
  ui_lb_item_t *items = NULL;
  byte used[256];
  int i;
  int width;
  int selected = 0;
  int ch;
  int did_hide_cursor = 0;

  if (out_key)
    *out_key = 0;

  if (!m || !m->items || m->count < 1)
    return -1;

  memset(used, 0, sizeof(used));

  items = (ui_lb_item_t *)calloc((size_t)m->count, sizeof(ui_lb_item_t));
  if (!items)
    return -1;

  /* Parse items */
  for (i = 0; i < m->count; i++)
  {
    items[i].orig = ui_lb_strdup(m->items[i]);
    items[i].disp = ui_lb_strip_marker(m->items[i], &items[i].hotkey, &items[i].hotkey_pos);
    if (items[i].hotkey)
      used[(byte)items[i].hotkey] = 1;
  }

  if (m->enable_hotkeys)
  {
    for (i = 0; i < m->count; i++)
      ui_lb_autohotkey(&items[i], used);
  }

  ui_lb_hide_cursor(&did_hide_cursor);

  /* Determine width */
  width = m->width;
  if (width <= 0)
  {
    width = 1;
    for (i = 0; i < m->count; i++)
    {
      const char *text = m->show_brackets ? items[i].orig : items[i].disp;
      int len = text ? (int)strlen(text) : 0;
      if (len > width)
        width = len;
    }
  }

  /* Initial paint */
  for (i = 0; i < m->count; i++)
    ui_lb_draw_item(m, items, i, (i == selected), width);

  ui_goto(m->y + selected, m->x);
  vbuf_flush();

  while (1)
  {
    ch = ui_read_key();

    switch (ch)
    {
      case K_RETURN:
        if (out_key)
          *out_key = items[selected].hotkey;
        for (i = 0; i < m->count; i++)
        {
          free(items[i].disp);
          free(items[i].orig);
        }
        free(items);
        ui_lb_show_cursor(did_hide_cursor);
        return selected;

      case K_ESC:
        for (i = 0; i < m->count; i++)
        {
          free(items[i].disp);
          free(items[i].orig);
        }
        free(items);
        ui_lb_show_cursor(did_hide_cursor);
        return -1;

      case K_UP:
        if (selected > 0)
        {
          ui_lb_draw_item(m, items, selected, 0, width);
          selected--;
          ui_lb_draw_item(m, items, selected, 1, width);
          ui_goto(m->y + selected, m->x);
          vbuf_flush();
        }
        else if (m->wrap)
        {
          ui_lb_draw_item(m, items, selected, 0, width);
          selected = m->count - 1;
          ui_lb_draw_item(m, items, selected, 1, width);
          ui_goto(m->y + selected, m->x);
          vbuf_flush();
        }
        break;

      case K_DOWN:
        if (selected < m->count - 1)
        {
          ui_lb_draw_item(m, items, selected, 0, width);
          selected++;
          ui_lb_draw_item(m, items, selected, 1, width);
          ui_goto(m->y + selected, m->x);
          vbuf_flush();
        }
        else if (m->wrap)
        {
          ui_lb_draw_item(m, items, selected, 0, width);
          selected = 0;
          ui_lb_draw_item(m, items, selected, 1, width);
          ui_goto(m->y + selected, m->x);
          vbuf_flush();
        }
        break;

      default:
        if (m->enable_hotkeys && ui_lb_is_printable(ch))
        {
          int key = tolower((unsigned char)ch);
          for (i = 0; i < m->count; i++)
          {
            if (items[i].hotkey && items[i].hotkey == key)
            {
              if (out_key)
                *out_key = items[i].hotkey;
              for (ch = 0; ch < m->count; ch++)
              {
                free(items[ch].disp);
                free(items[ch].orig);
              }
              free(items);
              ui_lb_show_cursor(did_hide_cursor);
              return i;
            }
          }
        }
        break;
    }
  }
}

#define UI_SP_FLAG_STRIP_BRACKETS 0x0001
#define UI_SP_HOTKEY_ATTR_SHIFT   8

static void near ui_sp_draw_option(int row, int col, const ui_lb_item_t *opt, int selected, byte normal_attr, byte selected_attr, byte hotkey_attr, int strip_brackets)
{
  const char *text;
  int len;
  int i;
  byte attr;
  byte hk_attr;
  int hotkey;

  if (!opt)
    return;

  text = strip_brackets ? (opt->disp ? opt->disp : "") : (opt->orig ? opt->orig : "");
  hotkey = opt->hotkey;
  len = (int)strlen(text);

  attr = selected ? selected_attr : normal_attr;
  hk_attr = selected ? attr : (hotkey_attr ? hotkey_attr : attr);

  ui_set_attr(attr);
  ui_goto(row, col);

  if (hotkey && hotkey_attr)
  {
    int hk_index = -1;

    if (!strip_brackets)
    {
      const char *p = strchr(text, '[');
      if (p && p[1] && p[2] == ']')
        hk_index = (int)(p - text + 1);
    }
    else
    {
      hk_index = opt->hotkey_pos;
    }

    if (hk_index < 0 || hk_index >= len)
    {
      for (i = 0; i < len; i++)
      {
        if (tolower((unsigned char)text[i]) == hotkey)
        {
          hk_index = i;
          break;
        }
      }
    }

    for (i = 0; i < len; i++)
    {
      if (i == hk_index)
      {
        ui_set_attr(hk_attr);
        Putc(text[i]);
        ui_set_attr(attr);
      }
      else
        Putc(text[i]);
    }
  }
  else
  {
    for (i = 0; i < len; i++)
      Putc(text[i]);
  }
}

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
    int *out_key)
{
  ui_lb_item_t *opts = NULL;
  int i;
  int selected = 0;
  int ch;
  int row;
  int col;
  int *start_col = NULL;
  int *opt_width = NULL;
  int strip_brackets;
  byte hk_attr;
  int did_hide_cursor = 0;
  int sep_len;
  int safe_margin;
  const char *sep;

  strip_brackets = (flags & UI_SP_FLAG_STRIP_BRACKETS) ? 1 : 0;
  hk_attr = (byte)((flags >> UI_SP_HOTKEY_ATTR_SHIFT) & 0xff);

  if (out_key)
    *out_key = 0;

  if (!options || option_count < 1)
    return -1;

  opts = (ui_lb_item_t *)calloc((size_t)option_count, sizeof(ui_lb_item_t));
  start_col = (int *)calloc((size_t)option_count, sizeof(int));
  opt_width = (int *)calloc((size_t)option_count, sizeof(int));

  if (!opts || !start_col || !opt_width)
  {
    free(opts);
    free(start_col);
    free(opt_width);
    return -1;
  }

  for (i = 0; i < option_count; i++)
  {
    opts[i].orig = ui_lb_strdup(options[i]);
    opts[i].disp = ui_lb_strip_marker(options[i], &opts[i].hotkey, &opts[i].hotkey_pos);
  }

  ui_lb_hide_cursor(&did_hide_cursor);

  /* Prompt at current position */
  ui_set_attr(prompt_attr);
  Printf("%s", prompt ? prompt : "");

  row = (int)current_line;
  col = (int)current_col;

  sep = separator ? separator : "";
  sep_len = (int)strlen(sep);
  safe_margin = (margin > 0) ? margin : 0;

  /* Layout options horizontally */
  for (i = 0; i < option_count; i++)
  {
    const char *text = strip_brackets ? (opts[i].disp ? opts[i].disp : "") : (opts[i].orig ? opts[i].orig : "");
    int len = (int)strlen(text);
    if (!opts[i].hotkey)
    {
      if (opts[i].disp && opts[i].disp[0])
        opts[i].hotkey = tolower((unsigned char)opts[i].disp[0]);
    }

    start_col[i] = col;
    opt_width[i] = (len + (safe_margin * 2));
    col += opt_width[i];
    if (i != option_count - 1)
      col += sep_len;
  }

  /* Draw all options */
  for (i = 0; i < option_count; i++)
  {
    ui_sp_draw_option_margined(row, start_col[i], &opts[i], (i == selected), normal_attr, selected_attr, hk_attr, strip_brackets, safe_margin);
    if (i != option_count - 1 && sep_len > 0)
    {
      ui_set_attr(normal_attr);
      ui_goto(row, start_col[i] + opt_width[i]);
      Printf("%s", sep);
    }
  }

  ui_goto(row, start_col[selected] + safe_margin);
  vbuf_flush();

  while (1)
  {
    ch = ui_read_key();

    switch (ch)
    {
      case K_RETURN:
        if (out_key)
          *out_key = opts[selected].hotkey;
        for (i = 0; i < option_count; i++)
        {
          free(opts[i].disp);
          free(opts[i].orig);
        }
        free(opts);
        free(start_col);
        free(opt_width);
        ui_lb_show_cursor(did_hide_cursor);
        return selected;

      case 27:
        for (i = 0; i < option_count; i++)
        {
          free(opts[i].disp);
          free(opts[i].orig);
        }
        free(opts);
        free(start_col);
        free(opt_width);
        ui_lb_show_cursor(did_hide_cursor);
        return -1;

      case K_LEFT:
      case K_UP:
        if (selected > 0)
        {
          ui_sp_draw_option_margined(row, start_col[selected], &opts[selected], 0, normal_attr, selected_attr, hk_attr, strip_brackets, safe_margin);

          selected--;

          ui_sp_draw_option_margined(row, start_col[selected], &opts[selected], 1, normal_attr, selected_attr, hk_attr, strip_brackets, safe_margin);
          vbuf_flush();
        }
        else
        {
          /* wrap */
          ui_sp_draw_option_margined(row, start_col[selected], &opts[selected], 0, normal_attr, selected_attr, hk_attr, strip_brackets, safe_margin);

          selected = option_count - 1;

          ui_sp_draw_option_margined(row, start_col[selected], &opts[selected], 1, normal_attr, selected_attr, hk_attr, strip_brackets, safe_margin);
          vbuf_flush();
        }
        break;

      case K_RIGHT:
      case K_DOWN:
        if (selected < option_count - 1)
        {
          ui_sp_draw_option_margined(row, start_col[selected], &opts[selected], 0, normal_attr, selected_attr, hk_attr, strip_brackets, safe_margin);

          selected++;

          ui_sp_draw_option_margined(row, start_col[selected], &opts[selected], 1, normal_attr, selected_attr, hk_attr, strip_brackets, safe_margin);
          vbuf_flush();
        }
        else
        {
          ui_sp_draw_option_margined(row, start_col[selected], &opts[selected], 0, normal_attr, selected_attr, hk_attr, strip_brackets, safe_margin);

          selected = 0;

          ui_sp_draw_option_margined(row, start_col[selected], &opts[selected], 1, normal_attr, selected_attr, hk_attr, strip_brackets, safe_margin);
          vbuf_flush();
        }
        break;

      default:
        if (ui_lb_is_printable(ch))
        {
          int key = tolower((unsigned char)ch);
          for (i = 0; i < option_count; i++)
          {
            if (opts[i].hotkey && opts[i].hotkey == key)
            {
              int match = i;
              int j;
              if (out_key)
                *out_key = opts[match].hotkey;
              for (j = 0; j < option_count; j++)
              {
                free(opts[j].disp);
                free(opts[j].orig);
              }
              free(opts);
              free(start_col);
              free(opt_width);
              ui_lb_show_cursor(did_hide_cursor);
              return match;
            }
          }
        }
        break;
    }
  }
}
