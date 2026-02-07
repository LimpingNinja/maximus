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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "prog.h"
#include "keys.h"
#include "mm.h"
#include "protod.h"
#include "ui_field.h"

int ui_field_can_fit_at(int col, int width)
{
  int tw;

  if (width < 1)
    return 0;

  tw = TermWidth();
  if (tw <= 0)
    return 0;

  if (col < 1)
    return 0;

  return (col + width - 1) <= tw;
}

int ui_field_can_fit_here(int width)
{
  return ui_field_can_fit_at((int)current_col, width);
}

/* Set attribute using Maximus AVATAR sequence */
void ui_set_attr(byte attr)
{
  Printf(attr_string, attr);
}

/* Position cursor */
void ui_goto(int row, int col)
{
  Goto(row, col);
}

/* Fill a rectangular region with a character */
void ui_fill_rect(int row, int col, int width, int height, char ch, byte attr)
{
  int r, c;
  
  ui_set_attr(attr);
  
  for (r = 0; r < height; r++)
  {
    ui_goto(row + r, col);
    for (c = 0; c < width; c++)
      Putc(ch);
  }
}

/* Write a string padded to width */
void ui_write_padded(int row, int col, int width, const char *s, byte attr)
{
  int len = s ? strlen(s) : 0;
  int i;
  
  ui_set_attr(attr);
  ui_goto(row, col);
  
  for (i = 0; i < width; i++)
  {
    if (i < len)
      Putc(s[i]);
    else
      Putc(' ');
  }
}

/* Format mask helpers */
int ui_mask_is_placeholder(char ch)
{
  return ch == '0' || ch == 'A' || ch == 'X';
}

char ui_mask_placeholder_display(char ch)
{
  if (ch == '0')
    return '0';
  return '_';
}

int ui_mask_placeholder_ok(char placeholder, char ch)
{
  if (placeholder == '0')
    return isdigit((unsigned char)ch);
  if (placeholder == 'A')
    return isalpha((unsigned char)ch);
  if (placeholder == 'X')
    return isalnum((unsigned char)ch);
  return 0;
}

int ui_mask_count_positions(const char *mask)
{
  int count = 0;
  if (!mask)
    return 0;
  while (*mask)
  {
    if (ui_mask_is_placeholder(*mask))
      count++;
    mask++;
  }
  return count;
}

void ui_mask_apply(const char *raw, const char *mask, char *out, int out_cap)
{
  int raw_i = 0;
  int out_i = 0;
  int raw_len = raw ? (int)strlen(raw) : 0;
  
  if (!mask || !out || out_cap < 1)
  {
    if (out && out_cap > 0)
      out[0] = '\0';
    return;
  }
  
  while (*mask && out_i < out_cap - 1)
  {
    if (ui_mask_is_placeholder(*mask))
    {
      if (raw_i < raw_len)
        out[out_i++] = raw[raw_i++];
      else
        out[out_i++] = ui_mask_placeholder_display(*mask);
    }
    else
    {
      out[out_i++] = *mask;
    }
    mask++;
  }
  out[out_i] = '\0';
}

int ui_read_key(void)
{
  int ch;

  while (!Mdm_keyp())
    Giveaway_Slice();

  ch = Mdm_getcw();

  if (ch == 0 || ch == K_ESC)
  {
    while (!Mdm_keyp())
      Giveaway_Slice();
    ch = Mdm_getcw();

    if (ch == '[' || ch == 'O')
    {
      while (!Mdm_keyp())
        Giveaway_Slice();
      ch = Mdm_getcw();

      if (ch == '3')
      {
        while (!Mdm_keyp())
          Giveaway_Slice();
        ch = Mdm_getcw();
        if (ch == '~')
          return UI_KEY_DELETE;
      }
    }
  }

  return ch;
}

/* Initialize ui_edit_field_style with defaults */
void ui_edit_field_style_default(ui_edit_field_style_t *style)
{
  if (!style)
    return;
  style->normal_attr = 0x07;  /* gray on black */
  style->focus_attr = 0x1e;   /* yellow on blue */
  style->fill_ch = ' ';
  style->flags = 0;
  style->format_mask = NULL;
}

/* Initialize ui_prompt_field_style with defaults */
void ui_prompt_field_style_default(ui_prompt_field_style_t *style)
{
  if (!style)
    return;
  style->prompt_attr = 0x0e;  /* yellow */
  style->field_attr = 0x1e;   /* yellow on blue */
  style->fill_ch = ' ';
  style->flags = 0;
  style->start_mode = UI_PROMPT_START_HERE;
  style->format_mask = NULL;
}

/* Bounded field editor with style struct */
int ui_edit_field(
    int row,
    int col,
    int width,
    int max_len,
    char *buf,
    int buf_cap,
    const ui_edit_field_style_t *style)
{
  int cur_pos = 0;
  int len = 0;
  int ch;
  int done = 0;
  int result = UI_EDIT_ACCEPT;
  int i;
  byte normal_attr, focus_attr, fill_ch;
  int flags;
  const char *format_mask;
  int use_format_mask = 0;
  int mask_positions = 0;
  char display_buf[PATHLEN];
  
  if (!buf || buf_cap < 1 || width < 1 || !style)
    return UI_EDIT_ERROR;

  if (!ui_field_can_fit_at(col, width))
    return UI_EDIT_NOROOM;
  
  /* Extract style fields */
  normal_attr = style->normal_attr;
  focus_attr = style->focus_attr;
  fill_ch = style->fill_ch ? style->fill_ch : ' ';
  flags = style->flags;
  format_mask = style->format_mask;
  
  /* Check if we're using format mask mode */
  if (format_mask && *format_mask)
  {
    use_format_mask = 1;
    mask_positions = ui_mask_count_positions(format_mask);
    /* For format mask, max_len is the number of editable positions */
    if (max_len > mask_positions)
      max_len = mask_positions;
    if (max_len > buf_cap - 1)
      max_len = buf_cap - 1;
    /* Width should accommodate the full mask display */
    if (width < (int)strlen(format_mask))
      width = (int)strlen(format_mask);
  }
  else
  {
    /* Ensure max_len doesn't exceed buffer or width */
    if (max_len > buf_cap - 1)
      max_len = buf_cap - 1;
    if (max_len > width)
      max_len = width;
  }
  
  /* Initialize from existing buffer content */
  len = (int)strlen(buf);
  if (len > max_len)
  {
    buf[max_len] = '\0';
    len = max_len;
  }
  cur_pos = len;
  
  /* Pre-paint the field background with normal_attr */
  ui_set_attr(normal_attr);
  ui_goto(row, col);
  for (i = 0; i < width; i++)
    Putc(' ');
  
  /* Paint the field content with focus_attr */
  ui_set_attr(focus_attr);
  ui_goto(row, col);
  
  if (use_format_mask)
  {
    /* Format mask mode - render through mask */
    ui_mask_apply(buf, format_mask, display_buf, sizeof(display_buf));
    for (i = 0; i < width && display_buf[i]; i++)
      Putc(display_buf[i]);
    for (; i < width; i++)
      Putc(' ');
  }
  else if (flags & UI_EDIT_FLAG_MASK)
  {
    /* Masked field - show fill_ch for each character */
    for (i = 0; i < width; i++)
    {
      if (i < len)
        Putc(fill_ch);
      else
        Putc(' ');
    }
  }
  else
  {
    /* Normal field - show actual content */
    for (i = 0; i < width; i++)
    {
      if (i < len)
        Putc(buf[i]);
      else
        Putc(' ');
    }
  }
  
  /* Position cursor - for format mask, find the display position */
  if (use_format_mask)
  {
    /* Find display position for raw cursor position */
    int raw_i = 0;
    int disp_i = 0;
    const char *m = format_mask;
    while (*m && raw_i < cur_pos)
    {
      if (ui_mask_is_placeholder(*m))
        raw_i++;
      m++;
      disp_i++;
    }
    /* Skip to next placeholder if we're on a literal */
    while (*m && !ui_mask_is_placeholder(*m))
    {
      m++;
      disp_i++;
    }
    ui_goto(row, col + disp_i);
  }
  else
  {
    ui_goto(row, col + cur_pos);
  }
  vbuf_flush();
  
  while (!done)
  {
    ch = ui_read_key();
    
    switch (ch)
    {
      case K_RETURN:
        result = UI_EDIT_ACCEPT;
        done = 1;
        break;

 #if K_DEL != 8 && K_DEL != K_BS
      case K_DEL:
 #endif
      case UI_KEY_DELETE:
        if (cur_pos < len)
        {
          memmove(buf + cur_pos, buf + cur_pos + 1, len - cur_pos);
          len--;

          ui_set_attr(focus_attr);

          if (use_format_mask)
          {
            ui_goto(row, col);
            ui_mask_apply(buf, format_mask, display_buf, sizeof(display_buf));
            for (i = 0; i < width && display_buf[i]; i++)
              Putc(display_buf[i]);
            for (; i < width; i++)
              Putc(' ');

            {
              int raw_i = 0, disp_i = 0;
              const char *m = format_mask;
              while (*m && raw_i < cur_pos)
              {
                if (ui_mask_is_placeholder(*m))
                  raw_i++;
                m++;
                disp_i++;
              }
              ui_goto(row, col + disp_i);
            }
          }
          else
          {
            ui_goto(row, col + cur_pos);
            if (flags & UI_EDIT_FLAG_MASK)
            {
              for (i = cur_pos; i < width; i++)
                Putc((i < len) ? fill_ch : ' ');
            }
            else
            {
              for (i = cur_pos; i < width; i++)
                Putc((i < len) ? buf[i] : ' ');
            }
            ui_goto(row, col + cur_pos);
          }

          vbuf_flush();
        }
        break;
      
      case K_ESC:
        if (flags & UI_EDIT_FLAG_ALLOW_CANCEL)
        {
          result = UI_EDIT_CANCEL;
          done = 1;
        }
        break;
      
      case K_UP:
      case K_STAB:
        if (flags & UI_EDIT_FLAG_FIELD_MODE)
        {
          result = UI_EDIT_PREVIOUS;
          done = 1;
        }
        break;
      
      case K_DOWN:
      case K_TAB:
        if (flags & UI_EDIT_FLAG_FIELD_MODE)
        {
          result = UI_EDIT_NEXT;
          done = 1;
        }
        break;
      
      case K_LEFT:
        if (cur_pos > 0)
        {
          cur_pos--;
          if (use_format_mask)
          {
            /* Find display position for raw cursor */
            int raw_i = 0, disp_i = 0;
            const char *m = format_mask;
            while (*m && raw_i < cur_pos)
            {
              if (ui_mask_is_placeholder(*m))
                raw_i++;
              m++;
              disp_i++;
            }
            ui_goto(row, col + disp_i);
          }
          else
          {
            ui_goto(row, col + cur_pos);
          }
          vbuf_flush();
        }
        break;
      
      case K_RIGHT:
        if (cur_pos < len)
        {
          cur_pos++;
          if (use_format_mask)
          {
            /* Find display position for raw cursor */
            int raw_i = 0, disp_i = 0;
            const char *m = format_mask;
            while (*m && raw_i < cur_pos)
            {
              if (ui_mask_is_placeholder(*m))
                raw_i++;
              m++;
              disp_i++;
            }
            /* Skip literals to next placeholder */
            while (*m && !ui_mask_is_placeholder(*m))
            {
              m++;
              disp_i++;
            }
            ui_goto(row, col + disp_i);
          }
          else
          {
            ui_goto(row, col + cur_pos);
          }
          vbuf_flush();
        }
        break;
      
      case K_HOME:
        cur_pos = 0;
        if (use_format_mask)
        {
          /* Find first placeholder position */
          int disp_i = 0;
          const char *m = format_mask;
          while (*m && !ui_mask_is_placeholder(*m))
          {
            m++;
            disp_i++;
          }
          ui_goto(row, col + disp_i);
        }
        else
        {
          ui_goto(row, col);
        }
        vbuf_flush();
        break;
      
      case K_END:
        cur_pos = len;
        if (use_format_mask)
        {
          /* Find display position for end of raw data */
          int raw_i = 0, disp_i = 0;
          const char *m = format_mask;
          while (*m && raw_i < len)
          {
            if (ui_mask_is_placeholder(*m))
              raw_i++;
            m++;
            disp_i++;
          }
          ui_goto(row, col + disp_i);
        }
        else
        {
          ui_goto(row, col + cur_pos);
        }
        vbuf_flush();
        break;
      
#if K_BS != 8
      case 8:
#endif
#if K_BS != 0x7f
      case 0x7f:
#endif
      case K_BS:
        if (cur_pos > 0)
        {
          /* Delete char before cursor */
          memmove(buf + cur_pos - 1, buf + cur_pos, len - cur_pos + 1);
          len--;
          cur_pos--;
          
          ui_set_attr(focus_attr);
          
          if (use_format_mask)
          {
            /* Redraw entire masked field */
            ui_goto(row, col);
            ui_mask_apply(buf, format_mask, display_buf, sizeof(display_buf));
            for (i = 0; i < width && display_buf[i]; i++)
              Putc(display_buf[i]);
            for (; i < width; i++)
              Putc(' ');
            /* Position cursor */
            int raw_i = 0, disp_i = 0;
            const char *m = format_mask;
            while (*m && raw_i < cur_pos)
            {
              if (ui_mask_is_placeholder(*m))
                raw_i++;
              m++;
              disp_i++;
            }
            ui_goto(row, col + disp_i);
          }
          else
          {
            ui_goto(row, col + cur_pos);
            if (flags & UI_EDIT_FLAG_MASK)
            {
              for (i = cur_pos; i < width; i++)
                Putc((i < len) ? fill_ch : ' ');
            }
            else
            {
              for (i = cur_pos; i < width; i++)
                Putc((i < len) ? buf[i] : ' ');
            }
            ui_goto(row, col + cur_pos);
          }
          vbuf_flush();
        }
        break;
      
      default:
        /* Printable character */
        if (ch >= 32 && ch < 127 && len < max_len)
        {
          if (use_format_mask)
          {
            /* Find the placeholder at current position and validate */
            int raw_i = 0;
            const char *m = format_mask;
            char placeholder = 0;
            while (*m && raw_i < cur_pos)
            {
              if (ui_mask_is_placeholder(*m))
                raw_i++;
              m++;
            }
            /* Find next placeholder */
            while (*m && !ui_mask_is_placeholder(*m))
              m++;
            if (*m)
              placeholder = *m;
            
            /* Validate character against placeholder type */
            if (placeholder && ui_mask_placeholder_ok(placeholder, (char)ch))
            {
              /* Insert character at cursor */
              memmove(buf + cur_pos + 1, buf + cur_pos, len - cur_pos + 1);
              buf[cur_pos] = (char)ch;
              len++;
              cur_pos++;
              
              ui_set_attr(focus_attr);
              ui_goto(row, col);
              ui_mask_apply(buf, format_mask, display_buf, sizeof(display_buf));
              for (i = 0; i < width && display_buf[i]; i++)
                Putc(display_buf[i]);
              for (; i < width; i++)
                Putc(' ');
              
              /* Position cursor at next placeholder */
              raw_i = 0;
              int disp_i = 0;
              m = format_mask;
              while (*m && raw_i < cur_pos)
              {
                if (ui_mask_is_placeholder(*m))
                  raw_i++;
                m++;
                disp_i++;
              }
              /* Skip literals */
              while (*m && !ui_mask_is_placeholder(*m))
              {
                m++;
                disp_i++;
              }
              ui_goto(row, col + disp_i);
              vbuf_flush();
            }
          }
          else
          {
            /* Insert character at cursor */
            memmove(buf + cur_pos + 1, buf + cur_pos, len - cur_pos + 1);
            buf[cur_pos] = (char)ch;
            len++;
            cur_pos++;
            
            ui_set_attr(focus_attr);
            ui_goto(row, col + (cur_pos - 1));

            if (flags & UI_EDIT_FLAG_MASK)
            {
              for (i = cur_pos - 1; i < width; i++)
                Putc((i < len) ? fill_ch : ' ');
            }
            else
            {
              for (i = cur_pos - 1; i < width; i++)
                Putc((i < len) ? buf[i] : ' ');
            }

            ui_goto(row, col + cur_pos);
            vbuf_flush();
          }
        }
        break;
    }
  }
  
  /* Ensure null termination */
  buf[len] = '\0';
  
  return result;
}


/* Inline prompt editor with style struct */
int ui_prompt_field(
    const char *prompt,
    int width,
    int max_len,
    char *buf,
    int buf_cap,
    const ui_prompt_field_style_t *style)
{
  int start_col;
  int prompt_len;
  int field_col;
  int row;
  int col;
  int rc;
  byte save_attr;
  ui_edit_field_style_t edit_style;
  byte prompt_attr, field_attr, fill_ch;
  int flags, start_mode;

  if (!prompt)
    prompt = "";

  if (!buf || buf_cap < 1 || !style)
    return UI_EDIT_ERROR;

  /* Extract style fields */
  prompt_attr = style->prompt_attr;
  field_attr = style->field_attr;
  fill_ch = style->fill_ch ? style->fill_ch : ' ';
  flags = style->flags;
  start_mode = style->start_mode;

  if (usr.video != GRAPH_ANSI && usr.video != GRAPH_AVATAR)
  {
    if (flags & UI_EDIT_FLAG_MASK)
      InputGetseNH(buf, (char)fill_ch, (char *)prompt);
    else
      InputGetsWNH(buf, (char *)prompt);
    return UI_EDIT_ACCEPT;
  }

  switch (start_mode)
  {
    case UI_PROMPT_START_CR:
      Putc('\r');
      break;

    case UI_PROMPT_START_CLBOL:
      Putc('\r');
      Puts(CLEOL);
      Putc('\r');
      break;
  }

  start_col = (int)current_col;
  prompt_len = stravtlen((char *)prompt);
  field_col = start_col + prompt_len;

  if (!ui_field_can_fit_at(field_col, width))
  {
    if (flags & UI_EDIT_FLAG_MASK)
      InputGetseNH(buf, (char)fill_ch, (char *)prompt);
    else
      InputGetsWNH(buf, (char *)prompt);
    return UI_EDIT_ACCEPT;
  }

  if (prompt_attr != (byte)-1)
    ui_set_attr(prompt_attr);

  Printf("%s", (char *)prompt);

  row = (int)current_line;
  col = (int)current_col;
  
  save_attr = (byte)mdm_attr;

  /* Build edit_field style from prompt_field style */
  edit_style.normal_attr = field_attr;
  edit_style.focus_attr = field_attr;
  edit_style.fill_ch = fill_ch;
  edit_style.flags = flags;
  edit_style.format_mask = style->format_mask;

  rc = ui_edit_field(row, col, width, max_len, buf, buf_cap, &edit_style);

  if (save_attr != (byte)-1)
    Printf(attr_string, save_attr);

  Putc('\n');

  return rc;
}
