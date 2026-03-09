/*
 * ui_form.c — Form runner
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

#define MAX_INCL_COMMS

#define MAX_LANG_global
#define MAX_LANG_m_area
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "prog.h"
#include "keys.h"
#include "mm.h"
#include "protod.h"

#include "ui_form.h"
#include "ui_field.h"

/**
 * @brief Hide the terminal cursor if the user has ANSI/AVATAR video.
 *
 * @param did_hide Receives 1 if cursor was hidden, 0 otherwise.
 */
static void near ui_form_hide_cursor(int *did_hide)
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

/**
 * @brief Restore the terminal cursor if it was previously hidden.
 *
 * @param did_hide Value from ui_form_hide_cursor().
 */
static void near ui_form_show_cursor(int did_hide)
{
  if (!did_hide)
    return;

  if (usr.video == GRAPH_ANSI || usr.video == GRAPH_AVATAR)
    Printf("\x1b[?25h");
}

/**
 * @brief Initialize a ui_form_style_t with sensible defaults.
 *
 * @param style Style struct to initialize.
 */
void ui_form_style_default(ui_form_style_t *style)
{
  if (!style)
    return;
  
  style->label_attr = 0x0e;      /* yellow */
  style->normal_attr = 0x07;     /* white */
  style->focus_attr = 0x1e;      /* yellow on blue */
  style->save_mode = UI_FORM_SAVE_CTRL_S;
  style->wrap = 1;
  style->required_msg = "Required field is empty";
  style->required_x = 1;
  style->required_y = 24;
  style->required_attr = 0x0c;   /* light red */
}

/**
 * @brief Calculate the horizontal center of a form field.
 *
 * @param f Form field.
 * @return  Center X coordinate as a float.
 */
static float near ui_form_field_center_x(const ui_form_field_t *f)
{
  return (float)f->x + ((float)f->width - 1.0f) / 2.0f;
}

/**
 * @brief Calculate the X position where a field's label should start.
 *
 * @param f Form field.
 * @return  Column for the label (accounts for "Label: " prefix).
 */
static int near ui_form_field_label_x(const ui_form_field_t *f)
{
  int label_len;
  
  if (!f->label || !*f->label)
    return f->x;
  
  label_len = (int)strlen(f->label);
  return f->x - (label_len + 2);  /* "Label: " */
}

/**
 * @brief Draw a single form field (label + value) with focus styling.
 *
 * @param f       Form field definition.
 * @param focused Non-zero if this field has focus.
 * @param style   Form style for default colors.
 */
static void near ui_form_draw_field(const ui_form_field_t *f, int focused, const ui_form_style_t *style)
{
  byte label_attr;
  byte field_attr;
  char display_buf[PATHLEN];
  int i;
  
  if (!f || !style)
    return;
  
  /* Resolve colors */
  label_attr = f->label_attr ? f->label_attr : style->label_attr;
  field_attr = focused ? 
    (f->focus_attr ? f->focus_attr : style->focus_attr) :
    (f->normal_attr ? f->normal_attr : style->normal_attr);
  
  /* Draw label if present */
  if (f->label && *f->label)
  {
    ui_set_attr(label_attr);
    ui_goto(f->y, ui_form_field_label_x(f));
    Printf("%s: ", f->label);
  }
  
  /* Draw field value */
  ui_set_attr(field_attr);
  ui_goto(f->y, f->x);
  
  if (f->field_type == UI_FIELD_MASKED && f->value && *f->value)
  {
    /* Password field - show asterisks */
    int len = (int)strlen(f->value);
    for (i = 0; i < len && i < f->width; i++)
      Putc('*');
    for (; i < f->width; i++)
      Putc(' ');
  }
  else if (f->field_type == UI_FIELD_FORMAT && f->format_mask && *f->format_mask)
  {
    /* Format mask field */
    ui_mask_apply(f->value ? f->value : "", f->format_mask, display_buf, sizeof(display_buf));
    for (i = 0; i < f->width && display_buf[i]; i++)
      Putc(display_buf[i]);
    for (; i < f->width; i++)
      Putc(' ');
  }
  else
  {
    /* Plain text field */
    const char *val = f->value ? f->value : "";
    for (i = 0; i < f->width && val[i]; i++)
      Putc(val[i]);
    for (; i < f->width; i++)
      Putc(' ');
  }
  
  vbuf_flush();
}

/**
 * @brief Redraw all form fields, highlighting the selected one.
 *
 * @param fields      Array of form fields.
 * @param field_count Number of fields.
 * @param selected    Index of the currently selected field.
 * @param style       Form style.
 */
static void near ui_form_redraw(ui_form_field_t *fields, int field_count, int selected, const ui_form_style_t *style)
{
  int i;
  
  for (i = 0; i < field_count; i++)
    ui_form_draw_field(&fields[i], i == selected, style);
}

/**
 * @brief Find the nearest field neighbor in a given direction.
 *
 * @param fields      Array of form fields.
 * @param field_count Number of fields.
 * @param current     Index of the current field.
 * @param direction   "up", "down", "left", or "right".
 * @param wrap        Non-zero to wrap around edges.
 * @return            Index of the best neighbor, or -1 if none.
 */
static int near ui_form_find_neighbor(ui_form_field_t *fields, int field_count, int current, const char *direction, int wrap)
{
  float cur_cx, cx;
  int cur_y, dy, dx;
  int i;
  int best_idx = -1;
  float best_primary = 999999.0f;
  float best_secondary = 999999.0f;
  
  if (!fields || field_count < 2 || current < 0 || current >= field_count)
    return -1;
  
  cur_cx = ui_form_field_center_x(&fields[current]);
  cur_y = fields[current].y;
  
  /* Find candidates in the specified direction */
  for (i = 0; i < field_count; i++)
  {
    float primary, secondary;
    
    if (i == current)
      continue;
    
    cx = ui_form_field_center_x(&fields[i]);
    dy = fields[i].y - cur_y;
    dx = (int)(cx - cur_cx);
    
    if (strcmp(direction, "down") == 0 && dy > 0)
    {
      primary = (float)dy;
      secondary = (float)abs(dx);
    }
    else if (strcmp(direction, "up") == 0 && dy < 0)
    {
      primary = (float)(-dy);
      secondary = (float)abs(dx);
    }
    else if (strcmp(direction, "right") == 0 && dx > 0)
    {
      primary = (float)abs(dy);
      secondary = (float)dx;
    }
    else if (strcmp(direction, "left") == 0 && dx < 0)
    {
      primary = (float)abs(dy);
      secondary = (float)(-dx);
    }
    else
      continue;
    
    /* Update best candidate */
    if (primary < best_primary || (primary == best_primary && secondary < best_secondary))
    {
      best_primary = primary;
      best_secondary = secondary;
      best_idx = i;
    }
  }
  
  if (best_idx >= 0)
    return best_idx;
  
  /* No candidate found - try wrapping if enabled */
  if (!wrap)
    return -1;
  
  /* Wrap to opposite edge */
  if (strcmp(direction, "down") == 0 || strcmp(direction, "up") == 0)
  {
    int extreme_y = (strcmp(direction, "down") == 0) ? 999999 : -999999;
    
    for (i = 0; i < field_count; i++)
    {
      if (i == current)
        continue;
      if (strcmp(direction, "down") == 0 && fields[i].y < extreme_y)
        extreme_y = fields[i].y;
      else if (strcmp(direction, "up") == 0 && fields[i].y > extreme_y)
        extreme_y = fields[i].y;
    }
    
    /* Find closest field at extreme Y */
    for (i = 0; i < field_count; i++)
    {
      float dist;
      
      if (i == current || fields[i].y != extreme_y)
        continue;
      
      cx = ui_form_field_center_x(&fields[i]);
      dist = (float)abs((int)(cx - cur_cx));
      
      if (best_idx < 0 || dist < best_secondary)
      {
        best_idx = i;
        best_secondary = dist;
      }
    }
  }
  else
  {
    float extreme_cx = (strcmp(direction, "right") == 0) ? 999999.0f : -999999.0f;
    
    for (i = 0; i < field_count; i++)
    {
      if (i == current)
        continue;
      cx = ui_form_field_center_x(&fields[i]);
      if (strcmp(direction, "right") == 0 && cx < extreme_cx)
        extreme_cx = cx;
      else if (strcmp(direction, "left") == 0 && cx > extreme_cx)
        extreme_cx = cx;
    }
    
    /* Find closest field at extreme X */
    for (i = 0; i < field_count; i++)
    {
      float dist;
      
      if (i == current)
        continue;
      
      cx = ui_form_field_center_x(&fields[i]);
      if (cx != extreme_cx)
        continue;
      
      dist = (float)abs(fields[i].y - cur_y);
      
      if (best_idx < 0 || dist < best_secondary)
      {
        best_idx = i;
        best_secondary = dist;
      }
    }
  }
  
  return best_idx;
}

/**
 * @brief Find the next or previous field in sequential (tab) order.
 *
 * @param current     Current field index.
 * @param field_count Total number of fields.
 * @param forward     Non-zero for next, zero for previous.
 * @return            New field index (wraps around).
 */
static int near ui_form_find_sequential(int current, int field_count, int forward)
{
  if (forward)
    return (current + 1) % field_count;
  else
    return (current - 1 + field_count) % field_count;
}

/**
 * @brief Check whether a required field has a valid (non-empty) value.
 *
 * @param f Form field to validate.
 * @return  Non-zero if valid or not required.
 */
static int near ui_form_field_required_ok(const ui_form_field_t *f)
{
  int needed;
  int len;
  const char *s;
  
  if (!f->required)
    return 1;

  if (!f->value)
    return 0;

  /* Treat whitespace-only as empty for plain text fields */
  if (f->field_type == UI_FIELD_TEXT)
  {
    s = f->value;
    while (*s && isspace((unsigned char)*s))
      s++;
    if (!*s)
      return 0;
  }
  else
  {
    if (!*f->value)
      return 0;
  }
  
  if (f->field_type == UI_FIELD_FORMAT && f->format_mask)
  {
    needed = ui_mask_count_positions(f->format_mask);

    /* If caller set a smaller max_len, require only up to that many positions */
    if (f->max_len > 0 && f->max_len < needed)
      needed = f->max_len;

    len = (int)strlen(f->value);
    return len >= needed;
  }
  
  return 1;
}

/**
 * @brief Find the first required field that fails validation.
 *
 * @param fields      Array of form fields.
 * @param field_count Number of fields.
 * @return            Index of first invalid field, or -1 if all valid.
 */
static int near ui_form_first_invalid_required(ui_form_field_t *fields, int field_count)
{
  int i;
  
  for (i = 0; i < field_count; i++)
  {
    if (!ui_form_field_required_ok(&fields[i]))
      return i;
  }
  
  return -1;
}

/**
 * @brief Display the "required field" splash message at the configured position.
 *
 * @param style Form style containing splash message and coordinates.
 */
static void near ui_form_show_required_splash(const ui_form_style_t *style)
{
  if (!style || !style->required_msg || !*style->required_msg)
    return;

  ui_set_attr(style->required_attr);
  ui_goto(style->required_y, style->required_x);
  Printf("%s", style->required_msg);
  vbuf_flush();
}

/**
 * @brief Clear the "required field" splash message from the screen.
 *
 * @param style Form style containing splash message and coordinates.
 */
static void near ui_form_clear_required_splash(const ui_form_style_t *style)
{
  int len;
  int i;
  
  if (!style || !style->required_msg || !*style->required_msg)
    return;
  
  len = (int)strlen(style->required_msg);
  ui_set_attr(style->required_attr);
  ui_goto(style->required_y, style->required_x);
  for (i = 0; i < len; i++)
    Putc(' ');
  vbuf_flush();
}

/**
 * @brief Enter edit mode for a single form field.
 *
 * @param f     Form field to edit.
 * @param style Form style for colors and behavior.
 * @return      ui_edit_field result code.
 */
static int near ui_form_edit_field(ui_form_field_t *f, const ui_form_style_t *style)
{
  ui_edit_field_style_t edit_style;
  byte field_attr;
  int rc;
  
  if (!f || !style)
    return UI_EDIT_ERROR;
  
  field_attr = f->focus_attr ? f->focus_attr : style->focus_attr;
  
  /* Configure edit style */
  edit_style.normal_attr = field_attr;
  edit_style.focus_attr = field_attr;
  edit_style.fill_ch = ' ';
  edit_style.flags = UI_EDIT_FLAG_FIELD_MODE;
  edit_style.format_mask = NULL;
  
  if (f->field_type == UI_FIELD_MASKED)
    edit_style.flags |= UI_EDIT_FLAG_MASK;
  else if (f->field_type == UI_FIELD_FORMAT && f->format_mask)
    edit_style.format_mask = f->format_mask;
  
  /* Run field editor */
  rc = ui_edit_field(f->y, f->x, f->width, f->max_len, f->value, f->value_cap, &edit_style);
  
  return rc;
}

/**
 * @brief Run an interactive form with keyboard navigation and field editing.
 *
 * @param fields      Array of form field definitions.
 * @param field_count Number of fields.
 * @param style       Form style configuration.
 * @return            1 = saved, 0 = cancelled, -1 = error.
 */
int ui_form_run(ui_form_field_t *fields, int field_count, const ui_form_style_t *style)
{
  int selected = 0;
  int old_selected;
  int rc;
  int ch;
  int invalid_idx;
  int did_hide_cursor = 0;
  int ret = -1;
  
  if (!fields || field_count < 1 || !style)
    return -1;
  
  ui_form_hide_cursor(&did_hide_cursor);

  /* Initial draw */
  ui_form_redraw(fields, field_count, selected, style);
  
  while (1)
  {
    old_selected = selected;
    
    /* Position cursor at selected field */
    ui_goto(fields[selected].y, fields[selected].x);
    vbuf_flush();
    
    /* Get input */
    ch = ui_read_key();
    
    /* Handle navigation */
    if (ch == K_UP)
    {
      int next = ui_form_find_neighbor(fields, field_count, selected, "up", style->wrap);
      if (next >= 0)
        selected = next;
    }
    else if (ch == K_DOWN)
    {
      int next = ui_form_find_neighbor(fields, field_count, selected, "down", style->wrap);
      if (next >= 0)
        selected = next;
    }
    else if (ch == K_LEFT)
    {
      int next = ui_form_find_neighbor(fields, field_count, selected, "left", style->wrap);
      if (next >= 0)
        selected = next;
    }
    else if (ch == K_RIGHT)
    {
      int next = ui_form_find_neighbor(fields, field_count, selected, "right", style->wrap);
      if (next >= 0)
        selected = next;
    }
    else if (ch == K_TAB)
    {
      selected = ui_form_find_sequential(selected, field_count, 1);
    }
    else if (ch == K_STAB)
    {
      selected = ui_form_find_sequential(selected, field_count, 0);
    }
    else if (ch == K_RETURN)
    {
      /* Edit current field */
      ui_form_show_cursor(did_hide_cursor);
      rc = ui_form_edit_field(&fields[selected], style);
      ui_form_hide_cursor(&did_hide_cursor);
      
      if (rc == UI_EDIT_NEXT)
        selected = ui_form_find_sequential(selected, field_count, 1);
      else if (rc == UI_EDIT_PREVIOUS)
        selected = ui_form_find_sequential(selected, field_count, 0);
      
      /* Redraw after edit */
      ui_form_redraw(fields, field_count, selected, style);
    }
    else if (ch == 19)  /* Ctrl+S */
    {
      /* Check required fields */
      invalid_idx = ui_form_first_invalid_required(fields, field_count);
      if (invalid_idx >= 0)
      {
        ui_form_show_required_splash(style);
        selected = invalid_idx;
        ui_form_redraw(fields, field_count, selected, style);
        continue;
      }
      
      /* Save and exit */
      ret = 1;
      break;
    }
    else if (ch == K_ESC)
    {
      /* Cancel */
      ret = 0;
      break;
    }
    else if (ch >= 32 && ch < 127)
    {
      /* Check for hotkey match */
      int i;
      char ch_lower = (char)tolower(ch);
      
      for (i = 0; i < field_count; i++)
      {
        if (fields[i].hotkey && tolower(fields[i].hotkey) == ch_lower)
        {
          selected = i;
          break;
        }
      }
    }
    
    /* Redraw if selection changed */
    if (old_selected != selected)
    {
      ui_form_clear_required_splash(style);
      ui_form_draw_field(&fields[old_selected], 0, style);
      ui_form_draw_field(&fields[selected], 1, style);
    }
  }

  ui_form_show_cursor(did_hide_cursor);
  return ret;
}
