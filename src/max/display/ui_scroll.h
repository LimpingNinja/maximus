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

#ifndef __UI_SCROLL_H_DEFINED
#define __UI_SCROLL_H_DEFINED

#include "typedefs.h"

#include "ui_shadowbuf.h"

/*
 * These scrolling primitives operate on attribute-first cells. Input text may
 * include ANSI SGR (ESC[...m) and/or Maximus AVATAR attribute sequences
 * (\x16\x01\xNN). Input is normalized into (ch, attr) cells prior to wrapping
 * and rendering.
 */

/**
 * @brief One wrapped display line stored as (ch, attr) cells.
 */
typedef struct ui_cell_line
{
  ui_shadow_cell_t *cells;
  int len;
} ui_cell_line_t;

/* Append flags */
#define UI_SCROLL_APPEND_DEFAULT   0x0000
#define UI_SCROLL_APPEND_FOLLOW    0x0001  /* force follow/bottom after append */
#define UI_SCROLL_APPEND_NOFOLLOW  0x0002  /* force no-follow after append */

/* ScrollingRegion flags */
#define UI_SCROLL_REGION_SHOW_SCROLLBAR 0x0001
#define UI_SCROLL_REGION_AUTO_FOLLOW    0x0002

typedef struct ui_scrolling_region_style
{
  byte attr;            /* base attribute for padding/clears */
  byte scrollbar_attr;  /* attribute for scrollbar column */
  int flags;            /* UI_SCROLL_REGION_* */
} ui_scrolling_region_style_t;

typedef struct ui_scrolling_region
{
  int x;
  int y;
  int width;
  int height;
  int max_lines;

  ui_scrolling_region_style_t style;

  /* State */
  ui_cell_line_t *lines;
  int line_count;
  int view_top;
  int at_bottom;

  /* Render buffer (includes scrollbar column when enabled) */
  ui_shadowbuf_t sb;
  int sb_valid;

  /* Scrollbar redraw state */
  int last_thumb_top;
  int last_thumb_len;
} ui_scrolling_region_t;

/**
 * @brief Snapshot the current scrolling region viewport area for an overlay.
 *
 * This operates on the region's internal shadow buffer (which represents what
 * the widget will paint). Callers should typically:
 * - ui_scrolling_region_render(r)
 * - ui_scrolling_region_overlay_push(r, &ov)
 * - draw overlay into r->sb using ui_shadowbuf_* APIs
 * - ui_shadowbuf_paint_region(&r->sb, ...)
 * - ui_scrolling_region_overlay_pop(r, &ov)
 * - ui_scrolling_region_render(r)
 */
int ui_scrolling_region_overlay_push(const ui_scrolling_region_t *r, ui_shadow_overlay_t *ov);

/**
 * @brief Restore a previously pushed scrolling region overlay snapshot.
 */
void ui_scrolling_region_overlay_pop(ui_scrolling_region_t *r, ui_shadow_overlay_t *ov);

void ui_scrolling_region_style_default(ui_scrolling_region_style_t *style);
void ui_scrolling_region_init(ui_scrolling_region_t *r, int x, int y, int width, int height, int max_lines, const ui_scrolling_region_style_t *style);
void ui_scrolling_region_free(ui_scrolling_region_t *r);

int ui_scrolling_region_append(ui_scrolling_region_t *r, const char *text, int append_flags);
void ui_scrolling_region_clear(ui_scrolling_region_t *r);
void ui_scrolling_region_render(ui_scrolling_region_t *r);

/* Returns 1 if key was consumed (scroll changed), 0 otherwise */
int ui_scrolling_region_handle_key(ui_scrolling_region_t *r, int key);

/*
 * TextBufferViewer
 */

#define UI_TBV_SHOW_STATUS    0x0001
#define UI_TBV_SHOW_SCROLLBAR 0x0002

typedef struct ui_text_viewer_style
{
  byte attr;            /* base attribute for padding/clears */
  byte status_attr;     /* attribute for status line */
  byte scrollbar_attr;  /* attribute for scrollbar */
  int flags;            /* UI_TBV_* */
} ui_text_viewer_style_t;

typedef struct ui_text_viewer
{
  int x;
  int y;
  int width;
  int height;

  ui_text_viewer_style_t style;

  /* State */
  ui_cell_line_t *lines;
  int line_count;
  int view_top;

  /* Render buffer (includes scrollbar/status when enabled) */
  ui_shadowbuf_t sb;
  int sb_valid;

  /* Scrollbar redraw state */
  int last_thumb_top;
  int last_thumb_len;
} ui_text_viewer_t;

/**
 * @brief Snapshot the current viewer area for an overlay.
 */
int ui_text_viewer_overlay_push(const ui_text_viewer_t *v, ui_shadow_overlay_t *ov);

/**
 * @brief Restore a previously pushed viewer overlay snapshot.
 */
void ui_text_viewer_overlay_pop(ui_text_viewer_t *v, ui_shadow_overlay_t *ov);

void ui_text_viewer_style_default(ui_text_viewer_style_t *style);
void ui_text_viewer_init(ui_text_viewer_t *v, int x, int y, int width, int height, const ui_text_viewer_style_t *style);
void ui_text_viewer_free(ui_text_viewer_t *v);

int ui_text_viewer_set_text(ui_text_viewer_t *v, const char *text);
void ui_text_viewer_render(ui_text_viewer_t *v);

/* Returns 1 if key consumed, 0 otherwise */
int ui_text_viewer_handle_key(ui_text_viewer_t *v, int key);

/*
 * Convenience: reads a key, consumes only scroll/navigation keys, re-renders
 * on consume, and returns:
 * - 0 if consumed
 * - key code if not consumed
 */
int ui_text_viewer_read_key(ui_text_viewer_t *v);

#endif /* __UI_SCROLL_H_DEFINED */
