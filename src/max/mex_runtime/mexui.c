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

#define MAX_LANG_m_area
#include "mexall.h"
#include "ui_field.h"
#include "ui_lightbar.h"
#include "ui_form.h"
#include "ui_scroll.h"

static char * near MexDupVMString(const IADDR *pia)
{
  char *vm_str;
  word wLen;
  char *rc;

  if (!pia)
    return NULL;

  vm_str = (char *)MexFetch(FormString, (IADDR *)pia);
  if (!vm_str)
    return NULL;

  wLen = *(word *)vm_str;
  vm_str += sizeof(word);

  rc = (char *)malloc((size_t)wLen + 1);
  if (!rc)
    return NULL;

  memcpy(rc, vm_str, wLen);
  rc[wLen] = 0;
  return rc;
}

static char * near MexDupEmptyString(void)
{
  char *rc;

  rc = (char *)malloc(1);
  if (!rc)
    return NULL;

  rc[0] = 0;
  return rc;
}

#ifdef MEX

typedef struct mex_scroll_region_obj
{
  char *key;
  ui_scrolling_region_t r;
  struct mex_scroll_region_obj *next;
} mex_scroll_region_obj_t;

typedef struct mex_text_viewer_obj
{
  char *key;
  ui_text_viewer_t v;
  struct mex_text_viewer_obj *next;
} mex_text_viewer_obj_t;

static mex_scroll_region_obj_t *near g_scroll_regions = NULL;
static mex_text_viewer_obj_t *near g_text_viewers = NULL;

static mex_scroll_region_obj_t *near mex_find_scroll_region(const char *key)
{
  mex_scroll_region_obj_t *cur;

  if (!key || !*key)
    return NULL;

  for (cur = g_scroll_regions; cur; cur = cur->next)
    if (cur->key && strcmp(cur->key, key) == 0)
      return cur;

  return NULL;
}

static mex_text_viewer_obj_t *near mex_find_text_viewer(const char *key)
{
  mex_text_viewer_obj_t *cur;

  if (!key || !*key)
    return NULL;

  for (cur = g_text_viewers; cur; cur = cur->next)
    if (cur->key && strcmp(cur->key, key) == 0)
      return cur;

  return NULL;
}

/* ui_goto(row, col) - Position cursor */
word EXPENTRY intrin_ui_goto(void)
{
  MA ma;
  int row, col;
  
  MexArgBegin(&ma);
  row = (int)MexArgGetWord(&ma);
  col = (int)MexArgGetWord(&ma);
  
  ui_goto(row, col);
  
  if (pmisThis->pmid->instant_video)
    vbuf_flush();
  
  return MexArgEnd(&ma);
}

/**
 * @brief ui_read_key() - Return a MaxUI decoded key code.
 */
word EXPENTRY intrin_ui_read_key(void)
{
  regs_2[0] = (word)ui_read_key();
  return 0;
}

word EXPENTRY intrin_ui_lightbar_pos(void)
{
  MA ma;
  struct mex_ui_lightbar_item *items_ref;
  struct mex_ui_lightbar_style *style;
  int count;
  ui_lightbar_item_t *items = NULL;
  char **text_array = NULL;
  ui_lightbar_pos_menu_t menu;
  int i;
  int result;
  int out_key = 0;

  MexArgBegin(&ma);
  items_ref = (struct mex_ui_lightbar_item *)MexArgGetRef(&ma);
  count = (int)MexArgGetWord(&ma);
  style = (struct mex_ui_lightbar_style *)MexArgGetRef(&ma);

  if (!items_ref || count < 1 || !style)
  {
    regs_2[0] = (word)-1;
    return MexArgEnd(&ma);
  }

  items = (ui_lightbar_item_t *)calloc((size_t)count, sizeof(ui_lightbar_item_t));
  text_array = (char **)calloc((size_t)count, sizeof(char *));
  if (!items || !text_array)
  {
    free(items);
    free(text_array);
    regs_2[0] = (word)-1;
    return MexArgEnd(&ma);
  }

  for (i = 0; i < count; i++)
  {
    text_array[i] = MexDupVMString(&items_ref[i].text);
    if (!text_array[i])
      text_array[i] = MexDupEmptyString();

    items[i].text = text_array[i];
    items[i].x = (int)items_ref[i].x;
    items[i].y = (int)items_ref[i].y;
    items[i].width = (int)items_ref[i].width;
    items[i].justify = (int)items_ref[i].justify;
  }

  menu.items = (const ui_lightbar_item_t *)items;
  menu.count = count;
  menu.normal_attr = style->normal_attr;
  menu.selected_attr = style->selected_attr;
  menu.hotkey_attr = style->hotkey_attr;
  menu.hotkey_highlight_attr = style->hotkey_highlight_attr;
  menu.margin = (int)style->margin;
  menu.wrap = style->wrap;
  menu.enable_hotkeys = style->enable_hotkeys;
  menu.show_brackets = style->show_brackets;

  result = ui_lightbar_run_pos_hotkey(&menu, &out_key);

  for (i = 0; i < count; i++)
    if (text_array[i])
      free(text_array[i]);
  free(text_array);
  free(items);

  if (result >= 0)
  {
    style->out_hotkey = (word)out_key;
    regs_2[0] = (word)(result + 1);
  }
  else
  {
    style->out_hotkey = 0;
    regs_2[0] = (word)-1;
  }

  if (pmisThis->pmid->instant_video)
    vbuf_flush();

  return MexArgEnd(&ma);
}

word EXPENTRY intrin_ui_select_prompt_hotkey(void)
{
  MA ma;
  char *prompt;
  IADDR *options_ref;
  int count, flags;
  byte prompt_attr, normal_attr, selected_attr;
  word *hotkey_ref;
  char **options_array = NULL;
  int i;
  int result;
  int out_key = 0;

   MexArgBegin(&ma);
   prompt = MexArgGetString(&ma, FALSE);
   options_ref = (IADDR *)MexArgGetRef(&ma);
   count = (int)MexArgGetWord(&ma);
   prompt_attr = (byte)MexArgGetWord(&ma);
   normal_attr = (byte)MexArgGetWord(&ma);
   selected_attr = (byte)MexArgGetWord(&ma);
   flags = (int)MexArgGetWord(&ma);
   hotkey_ref = (word *)MexArgGetRef(&ma);

   if (hotkey_ref)
     *hotkey_ref = 0;

   if (!options_ref || count < 1)
   {
     if (prompt)
       free(prompt);
     regs_2[0] = (word)-1;
     return MexArgEnd(&ma);
   }

   options_array = (char **)malloc((size_t)count * sizeof(char *));
   if (!options_array)
   {
     if (prompt)
       free(prompt);
     regs_2[0] = (word)-1;
     return MexArgEnd(&ma);
   }

   for (i = 0; i < count; i++)
   {
     options_array[i] = MexDupVMString(&options_ref[i]);
     if (!options_array[i])
       options_array[i] = MexDupEmptyString();
   }

   result = ui_select_prompt(
     prompt ? prompt : "",
     (const char **)options_array,
     count,
     prompt_attr,
     normal_attr,
     selected_attr,
     flags,
     0,
     "",
     &out_key
   );

   if (prompt)
     free(prompt);

   for (i = 0; i < count; i++)
     if (options_array[i])
       free(options_array[i]);
   free(options_array);

   if (result >= 0)
   {
     if (hotkey_ref)
       *hotkey_ref = (word)out_key;
     regs_2[0] = (word)(result + 1);
   }
   else
   {
     if (hotkey_ref)
       *hotkey_ref = 0;
     regs_2[0] = (word)-1;
   }

   if (pmisThis->pmid->instant_video)
     vbuf_flush();

   return MexArgEnd(&ma);
 }

 word EXPENTRY intrin_ui_lightbar_hotkey(void)
 {
   MA ma;
   IADDR *items_ref;
   word *hotkey_ref;
   int count, x, y, width, justify, wrap, enable_hotkeys;
   byte normal_attr, selected_attr;
   char **items_array = NULL;
   int i;
   int result;
   int out_key = 0;
   ui_lightbar_menu_t menu;

   MexArgBegin(&ma);
   items_ref = (IADDR *)MexArgGetRef(&ma);
   count = (int)MexArgGetWord(&ma);
   x = (int)MexArgGetWord(&ma);
   y = (int)MexArgGetWord(&ma);
   width = (int)MexArgGetWord(&ma);
   justify = (int)MexArgGetWord(&ma);
   normal_attr = (byte)MexArgGetWord(&ma);
   selected_attr = (byte)MexArgGetWord(&ma);
   wrap = (int)MexArgGetWord(&ma);
   enable_hotkeys = (int)MexArgGetWord(&ma);
   hotkey_ref = (word *)MexArgGetRef(&ma);

   if (hotkey_ref)
     *hotkey_ref = 0;

   if (!items_ref || count < 1)
   {
     regs_2[0] = (word)-1;
     return MexArgEnd(&ma);
   }

   items_array = (char **)malloc((size_t)count * sizeof(char *));
   if (!items_array)
   {
     regs_2[0] = (word)-1;
     return MexArgEnd(&ma);
   }

   for (i = 0; i < count; i++)
   {
     items_array[i] = MexDupVMString(&items_ref[i]);
     if (!items_array[i])
       items_array[i] = MexDupEmptyString();
   }

   menu.items = (const char **)items_array;
   menu.count = count;
   menu.x = x;
   menu.y = y;
   menu.width = width;
   menu.margin = 0;
   menu.justify = justify;
   menu.normal_attr = normal_attr;
   menu.selected_attr = selected_attr;
   menu.hotkey_attr = 0;
   menu.hotkey_highlight_attr = 0;
   menu.wrap = wrap;
   menu.enable_hotkeys = enable_hotkeys;
   menu.show_brackets = 1;

   result = ui_lightbar_run_hotkey(&menu, &out_key);

   for (i = 0; i < count; i++)
     if (items_array[i])
       free(items_array[i]);
   free(items_array);

   if (result >= 0)
   {
     if (hotkey_ref)
       *hotkey_ref = (word)out_key;
     regs_2[0] = (word)(result + 1);
   }
   else
   {
     if (hotkey_ref)
       *hotkey_ref = 0;
     regs_2[0] = (word)-1;
   }

   if (pmisThis->pmid->instant_video)
     vbuf_flush();

   return MexArgEnd(&ma);
 }

/* ui_set_attr(attr) - Set display attribute */
word EXPENTRY intrin_ui_set_attr(void)
{
  MA ma;
  byte attr;
  
  MexArgBegin(&ma);
  attr = (byte)MexArgGetWord(&ma);
  
  ui_set_attr(attr);
  
  if (pmisThis->pmid->instant_video)
    vbuf_flush();
  
  return MexArgEnd(&ma);
}

/* ui_make_attr(fg, bg) - Return attribute byte composed from foreground/background */
word EXPENTRY intrin_ui_make_attr(void)
{
  MA ma;
  byte fg;
  byte bg;

  MexArgBegin(&ma);
  fg = (byte)MexArgGetWord(&ma);
  bg = (byte)MexArgGetWord(&ma);

  regs_2[0] = (word)((fg & 0x0f) | ((bg & 0x0f) << 4));
  return MexArgEnd(&ma);
}

/* ui_fill_rect(row, col, width, height, ch, attr) - Fill rectangle */
word EXPENTRY intrin_ui_fill_rect(void)
{
  MA ma;
  int row, col, width, height;
  char ch;
  byte attr;
  
  MexArgBegin(&ma);
  row = (int)MexArgGetWord(&ma);
  col = (int)MexArgGetWord(&ma);
  width = (int)MexArgGetWord(&ma);
  height = (int)MexArgGetWord(&ma);
  ch = (char)MexArgGetByte(&ma);
  attr = (byte)MexArgGetWord(&ma);
  
  ui_fill_rect(row, col, width, height, ch, attr);
  
  if (pmisThis->pmid->instant_video)
    vbuf_flush();
  
  return MexArgEnd(&ma);
}

/* ui_write_padded(row, col, width, s, attr) - Write padded string */
word EXPENTRY intrin_ui_write_padded(void)
{
  MA ma;
  int row, col, width;
  byte attr;
  char *s;
  
  MexArgBegin(&ma);
  row = (int)MexArgGetWord(&ma);
  col = (int)MexArgGetWord(&ma);
  width = (int)MexArgGetWord(&ma);
  s = MexArgGetString(&ma, FALSE);
  attr = (byte)MexArgGetWord(&ma);
  
  if (s)
  {
    ui_write_padded(row, col, width, s, attr);
    free(s);
  }
  
  if (pmisThis->pmid->instant_video)
    vbuf_flush();
  
  return MexArgEnd(&ma);
}

/* ui_prompt_field(prompt, width, max_len, buf, ref style) - Prompt with field using style struct */
word EXPENTRY intrin_ui_prompt_field(void)
{
  MA ma;
  int width, max_len;
  char *prompt;
  char *buf;
  char *format_mask_str = NULL;
  char local_buf[PATHLEN];
  int result;
  struct mex_ui_prompt_field_style *mex_style;
  ui_prompt_field_style_t style;
  
  MexArgBegin(&ma);
  prompt = MexArgGetString(&ma, FALSE);
  width = (int)MexArgGetWord(&ma);
  max_len = (int)MexArgGetWord(&ma);
  
  /* Get initial buffer content */
  if ((buf = MexArgGetString(&ma, FALSE)) != NULL)
  {
    strncpy(local_buf, buf, PATHLEN - 1);
    local_buf[PATHLEN - 1] = '\0';
    free(buf);
  }
  else
  {
    local_buf[0] = '\0';
  }
  
  /* Get style struct reference */
  mex_style = (struct mex_ui_prompt_field_style *)MexArgGetRef(&ma);
  
  if (!mex_style)
  {
    if (prompt)
      free(prompt);
    regs_2[0] = (word)UI_EDIT_ERROR;
    return MexArgEnd(&ma);
  }
  
  /* Convert MEX style to C style */
  style.prompt_attr = (byte)mex_style->prompt_attr;
  style.field_attr = (byte)mex_style->field_attr;
  style.fill_ch = mex_style->fill_ch ? mex_style->fill_ch : ' ';
  style.flags = (int)mex_style->flags;
  style.start_mode = (int)mex_style->start_mode;
  
  /* Get format_mask string if present */
  format_mask_str = MexDupVMString(&mex_style->format_mask);
  style.format_mask = format_mask_str;
  
  /* Call the prompt field */
  if (prompt)
  {
    result = ui_prompt_field(prompt, width, max_len, local_buf, PATHLEN, &style);
    free(prompt);
  }
  else
  {
    result = UI_EDIT_ERROR;
  }
  
  if (format_mask_str)
    free(format_mask_str);
  
  /* Return the edited string */
  MexReturnString(local_buf);
  
  /* Return result code in regs_2[0] */
  regs_2[0] = (word)result;
  
  return MexArgEnd(&ma);
}

/* ui_edit_field(row, col, width, max_len, buf, ref style) - Edit field using style struct */
word EXPENTRY intrin_ui_edit_field(void)
{
  MA ma;
  int row, col, width, max_len;
  char *buf;
  char *format_mask_str = NULL;
  char local_buf[PATHLEN];
  int result;
  struct mex_ui_edit_field_style *mex_style;
  ui_edit_field_style_t style;
  
  MexArgBegin(&ma);
  row = (int)MexArgGetWord(&ma);
  col = (int)MexArgGetWord(&ma);
  width = (int)MexArgGetWord(&ma);
  max_len = (int)MexArgGetWord(&ma);
  
  /* Get initial buffer content */
  if ((buf = MexArgGetString(&ma, FALSE)) != NULL)
  {
    strncpy(local_buf, buf, PATHLEN - 1);
    local_buf[PATHLEN - 1] = '\0';
    free(buf);
  }
  else
  {
    local_buf[0] = '\0';
  }
  
  /* Get style struct reference */
  mex_style = (struct mex_ui_edit_field_style *)MexArgGetRef(&ma);
  
  if (!mex_style)
  {
    regs_2[0] = (word)UI_EDIT_ERROR;
    return MexArgEnd(&ma);
  }
  
  /* Convert MEX style to C style */
  style.normal_attr = (byte)mex_style->normal_attr;
  style.focus_attr = (byte)mex_style->focus_attr;
  style.fill_ch = mex_style->fill_ch ? mex_style->fill_ch : ' ';
  style.flags = (int)mex_style->flags;
  
  /* Get format_mask string if present */
  format_mask_str = MexDupVMString(&mex_style->format_mask);
  style.format_mask = format_mask_str;
  
  /* Call the editor */
  result = ui_edit_field(row, col, width, max_len, local_buf, PATHLEN, &style);
  
  if (format_mask_str)
    free(format_mask_str);
  
  /* Return the edited string */
  MexReturnString(local_buf);
  
  /* Return result code in regs_2[0] */
  regs_2[0] = (word)result;
  
  return MexArgEnd(&ma);
}

/* ui_edit_field_style_default(ref style) - Initialize edit field style with defaults */
word EXPENTRY intrin_ui_edit_field_style_default(void)
{
  MA ma;
  struct mex_ui_edit_field_style *style;
  
  MexArgBegin(&ma);
  style = (struct mex_ui_edit_field_style *)MexArgGetRef(&ma);
  
  if (style)
  {
    style->normal_attr = 0x07;  /* gray on black */
    style->focus_attr = 0x1e;   /* yellow on blue */
    style->fill_ch = ' ';
    style->flags = 0;
    memset(&style->format_mask, 0, sizeof(style->format_mask));
  }
  
  return MexArgEnd(&ma);
}

/* ui_prompt_field_style_default(ref style) - Initialize prompt field style with defaults */
word EXPENTRY intrin_ui_prompt_field_style_default(void)
{
  MA ma;
  struct mex_ui_prompt_field_style *style;
  
  MexArgBegin(&ma);
  style = (struct mex_ui_prompt_field_style *)MexArgGetRef(&ma);
  
  if (style)
  {
    style->prompt_attr = 0x0e;  /* yellow */
    style->field_attr = 0x1e;   /* yellow on blue */
    style->fill_ch = ' ';
    style->flags = 0;
    style->start_mode = 0;      /* UI_PROMPT_START_HERE */
    memset(&style->format_mask, 0, sizeof(style->format_mask));
  }
  
  return MexArgEnd(&ma);
}

 /* ui_lightbar(ref array of string: items, int: count, int: x, int: y, int: width, ref struct ui_lightbar_style: style) - Vertical lightbar menu */
word EXPENTRY intrin_ui_lightbar(void)
{
  MA ma;
  IADDR *items_ref;
  struct mex_ui_lightbar_style *style;
  int count, x, y, width;
  char **items_array = NULL;
  int i;
  int result;
  int out_key = 0;
  ui_lightbar_menu_t menu;
  
  MexArgBegin(&ma);
  items_ref = (IADDR *)MexArgGetRef(&ma);
  count = (int)MexArgGetWord(&ma);
  x = (int)MexArgGetWord(&ma);
  y = (int)MexArgGetWord(&ma);
  width = (int)MexArgGetWord(&ma);
  style = (struct mex_ui_lightbar_style *)MexArgGetRef(&ma);
  
  if (!items_ref || count < 1 || !style)
  {
    regs_2[0] = (word)-1;
    return MexArgEnd(&ma);
  }
  
  items_array = (char **)malloc((size_t)count * sizeof(char *));
  if (!items_array)
  {
    regs_2[0] = (word)-1;
    return MexArgEnd(&ma);
  }
  
  for (i = 0; i < count; i++)
  {
    items_array[i] = MexDupVMString(&items_ref[i]);
    if (!items_array[i])
      items_array[i] = MexDupEmptyString();
  }
  
  menu.items = (const char **)items_array;
  menu.count = count;
  menu.x = x;
  menu.y = y;
  menu.width = width;
  menu.justify = style->justify;
  menu.normal_attr = style->normal_attr;
  menu.selected_attr = style->selected_attr;
  menu.hotkey_attr = style->hotkey_attr;
  menu.hotkey_highlight_attr = style->hotkey_highlight_attr;
  menu.margin = (int)style->margin;
  menu.wrap = style->wrap;
  menu.enable_hotkeys = style->enable_hotkeys;
  menu.show_brackets = style->show_brackets;
  
  result = ui_lightbar_run_hotkey(&menu, &out_key);

  for (i = 0; i < count; i++)
    if (items_array[i])
      free(items_array[i]);
  free(items_array);
  
  if (result >= 0)
  {
    style->out_hotkey = (word)out_key;
    regs_2[0] = (word)(result + 1);
  }
  else
  {
    style->out_hotkey = 0;
    regs_2[0] = (word)-1;
  }
  
  if (pmisThis->pmid->instant_video)
    vbuf_flush();
  
  return MexArgEnd(&ma);
}

 /* ui_select_prompt(string: prompt, ref array of string: options, int: count, ref struct ui_select_prompt_style: style) - Inline select prompt */
word EXPENTRY intrin_ui_select_prompt(void)
{
  MA ma;
  char *prompt;
  IADDR *options_ref;
  int count;
  struct mex_ui_select_prompt_style *style;
  char **options_array = NULL;
  char *separator_str = NULL;
  int i;
  int result;
  int out_key = 0;
  int flags = 0;
  
  MexArgBegin(&ma);
  prompt = MexArgGetString(&ma, FALSE);
  options_ref = (IADDR *)MexArgGetRef(&ma);
  count = (int)MexArgGetWord(&ma);
  style = (struct mex_ui_select_prompt_style *)MexArgGetRef(&ma);
  
  if (!options_ref || count < 1 || !style)
  {
    if (prompt)
      free(prompt);
    regs_2[0] = (word)-1;
    return MexArgEnd(&ma);
  }
  
  options_array = (char **)malloc((size_t)count * sizeof(char *));
  if (!options_array)
  {
    if (prompt)
      free(prompt);
    regs_2[0] = (word)-1;
    return MexArgEnd(&ma);
  }
  
  for (i = 0; i < count; i++)
  {
    options_array[i] = MexDupVMString(&options_ref[i]);
    if (!options_array[i])
      options_array[i] = MexDupEmptyString();
  }

  if (style->show_brackets == 0)
    flags |= UI_SP_FLAG_STRIP_BRACKETS;

  if (style->hotkey_attr)
    flags |= ((int)(style->hotkey_attr & 0xff) << UI_SP_HOTKEY_ATTR_SHIFT);

  if (style->default_index)
    flags |= ((int)(style->default_index & 0xff) << UI_SP_DEFAULT_SHIFT);

  separator_str = MexDupVMString(&style->separator);
  if (!separator_str)
    separator_str = MexDupEmptyString();
  
  result = ui_select_prompt(
    prompt ? prompt : "",
    (const char **)options_array,
    count,
    (byte)style->prompt_attr,
    (byte)style->normal_attr,
    (byte)style->selected_attr,
    flags,
    (int)style->margin,
    separator_str,
    &out_key
  );
  
  if (prompt)
    free(prompt);

  for (i = 0; i < count; i++)
    if (options_array[i])
      free(options_array[i]);
  free(options_array);

  if (separator_str)
    free(separator_str);

  if (result >= 0)
  {
    style->out_hotkey = (word)out_key;
    regs_2[0] = (word)(result + 1);
  }
  else
  {
    style->out_hotkey = 0;
    regs_2[0] = (word)-1;
  }
  
  if (pmisThis->pmid->instant_video)
    vbuf_flush();
  
  return MexArgEnd(&ma);
}

/* ui_lightbar_style_default(ref struct ui_lightbar_style: s) - Initialize with defaults */
word EXPENTRY intrin_ui_lightbar_style_default(void)
{
  MA ma;
  struct mex_ui_lightbar_style *style;
  
  MexArgBegin(&ma);
  style = (struct mex_ui_lightbar_style *)MexArgGetRef(&ma);
  
  if (style)
  {
    style->justify = 0;           /* UI_JUSTIFY_LEFT */
    style->wrap = 1;
    style->enable_hotkeys = 1;
    style->show_brackets = 1;     /* UI_BRACKET_SQUARE */
    style->normal_attr = 0x07;    /* UI_GRAY */
    style->selected_attr = 0x1e;  /* UI_YELLOWONBLUE */
    style->hotkey_attr = 0;       /* use selected_attr */
    style->hotkey_highlight_attr = 0;
    style->margin = 0;
    style->out_hotkey = 0;
  }
  
  return MexArgEnd(&ma);
}

/* ui_select_prompt_style_default(ref struct ui_select_prompt_style: s) - Initialize with defaults */
word EXPENTRY intrin_ui_select_prompt_style_default(void)
{
  MA ma;
  struct mex_ui_select_prompt_style *style;
  
  MexArgBegin(&ma);
  style = (struct mex_ui_select_prompt_style *)MexArgGetRef(&ma);
  
  if (style)
  {
    style->prompt_attr = 0x0e;    /* UI_YELLOW */
    style->normal_attr = 0x03;    /* UI_CYAN */
    style->selected_attr = 0x1e;  /* UI_YELLOWONBLUE */
    style->hotkey_attr = 0;       /* use selected_attr */
    style->show_brackets = 1;     /* UI_BRACKET_SQUARE */
    style->margin = 0;
    memset(&style->separator, 0, sizeof(style->separator));
    style->default_index = 0;     /* 0 = first option */
    style->out_hotkey = 0;
  }
  
  return MexArgEnd(&ma);
}

/* ui_form_style_default */
word EXPENTRY intrin_ui_form_style_default(void)
{
  MA ma;
  struct mex_ui_form_style *style;
  
  MexArgBegin(&ma);
  
  style = (struct mex_ui_form_style *)MexArgGetRef(&ma);
  
  if (style)
  {
    style->label_attr = 0x0e;      /* yellow */
    style->normal_attr = 0x07;     /* white */
    style->focus_attr = 0x1e;      /* yellow on blue */
    style->save_mode = 0;          /* UI_FORM_SAVE_CTRL_S */
    style->wrap = 1;
    memset(&style->required_msg, 0, sizeof(style->required_msg));
    style->required_x = 1;
    style->required_y = 24;
    style->required_attr = 0x0c;   /* light red */
  }
  
  return MexArgEnd(&ma);
}

/* ui_form_run */
word EXPENTRY intrin_ui_form_run(void)
{
  MA ma;
  struct mex_ui_form_field *fields_ref;
  word field_count;
  struct mex_ui_form_style *mex_style;
  ui_form_field_t *fields = NULL;
  ui_form_style_t style;
  int i;
  int rc = -1;
  
  MexArgBegin(&ma);
  
  fields_ref = (struct mex_ui_form_field *)MexArgGetRef(&ma);
  field_count = MexArgGetWord(&ma);
  mex_style = (struct mex_ui_form_style *)MexArgGetRef(&ma);
  
  if (!fields_ref || field_count < 1 || !mex_style)
  {
    regs_2[0] = (word)-1;
    return MexArgEnd(&ma);
  }
  
  /* Allocate C field array */
  fields = (ui_form_field_t *)calloc((size_t)field_count, sizeof(ui_form_field_t));
  if (!fields)
  {
    regs_2[0] = (word)-1;
    return MexArgEnd(&ma);
  }
  
  /* Convert MEX fields to C fields */
  for (i = 0; i < (int)field_count; i++)
  {
    char *value_str;

    fields[i].name = MexDupVMString(&fields_ref[i].name);
    fields[i].label = MexDupVMString(&fields_ref[i].label);
    fields[i].x = (int)fields_ref[i].x;
    fields[i].y = (int)fields_ref[i].y;
    fields[i].width = (int)fields_ref[i].width;
    fields[i].max_len = (int)fields_ref[i].max_len;
    fields[i].field_type = (int)fields_ref[i].field_type;
    fields[i].hotkey = (char)fields_ref[i].hotkey;
    fields[i].required = (int)fields_ref[i].required;
    fields[i].label_attr = (byte)fields_ref[i].label_attr;
    fields[i].normal_attr = (byte)fields_ref[i].normal_attr;
    fields[i].focus_attr = (byte)fields_ref[i].focus_attr;
    fields[i].format_mask = MexDupVMString(&fields_ref[i].format_mask);
    
    /* Allocate value buffer and copy initial value */
    value_str = MexDupVMString(&fields_ref[i].value);
    fields[i].value_cap = fields[i].max_len + 1;
    if (fields[i].value_cap < 256)
      fields[i].value_cap = 256;
    fields[i].value = (char *)calloc((size_t)fields[i].value_cap, 1);
    if (fields[i].value && value_str)
    {
      strncpy(fields[i].value, value_str, (size_t)(fields[i].value_cap - 1));
      fields[i].value[fields[i].value_cap - 1] = 0;
    }
    if (value_str)
      free(value_str);
  }
  
  /* Convert MEX style to C style */
  style.label_attr = (byte)mex_style->label_attr;
  style.normal_attr = (byte)mex_style->normal_attr;
  style.focus_attr = (byte)mex_style->focus_attr;
  style.save_mode = (int)mex_style->save_mode;
  style.wrap = (int)mex_style->wrap;
  style.required_msg = MexDupVMString(&mex_style->required_msg);
  style.required_x = (int)mex_style->required_x;
  style.required_y = (int)mex_style->required_y;
  style.required_attr = (byte)mex_style->required_attr;
  
  /* Run form */
  rc = ui_form_run(fields, (int)field_count, &style);
  
  /* Copy values back to MEX fields */
  if (rc == 1)
  {
    for (i = 0; i < (int)field_count; i++)
    {
      if (!fields[i].value)
        continue;

      MexKillStructString(mex_ui_form_field, &fields_ref[i], value);
      StoreString(MexPtrToVM(&fields_ref[i]), struct mex_ui_form_field, value, fields[i].value);
    }
  }
  
  /* Cleanup */
  if (style.required_msg)
    free((void *)style.required_msg);
  
  for (i = 0; i < (int)field_count; i++)
  {
    if (fields[i].name)
      free((void *)fields[i].name);
    if (fields[i].label)
      free((void *)fields[i].label);
    if (fields[i].format_mask)
      free((void *)fields[i].format_mask);
    if (fields[i].value)
      free(fields[i].value);
  }
  free(fields);
  
  regs_2[0] = (word)rc;
  return MexArgEnd(&ma);
}

word EXPENTRY intrin_ui_scroll_region_style_default(void)
{
  MA ma;
  struct mex_ui_scroll_region_style *mex_style;
  ui_scrolling_region_style_t style;

  MexArgBegin(&ma);
  mex_style = (struct mex_ui_scroll_region_style *)MexArgGetRef(&ma);

  ui_scrolling_region_style_default(&style);

  if (mex_style)
  {
    mex_style->attr = (word)style.attr;
    mex_style->scrollbar_attr = (word)style.scrollbar_attr;
    mex_style->flags = (word)style.flags;
  }

  return MexArgEnd(&ma);
}

word EXPENTRY intrin_ui_scroll_region_create(void)
{
  MA ma;
  char *key;
  int x;
  int y;
  int width;
  int height;
  int max_lines;
  struct mex_ui_scroll_region_style *mex_style;
  ui_scrolling_region_style_t style;
  mex_scroll_region_obj_t *obj;

  MexArgBegin(&ma);
  key = MexArgGetString(&ma, FALSE);
  x = (int)MexArgGetWord(&ma);
  y = (int)MexArgGetWord(&ma);
  width = (int)MexArgGetWord(&ma);
  height = (int)MexArgGetWord(&ma);
  max_lines = (int)MexArgGetWord(&ma);
  mex_style = (struct mex_ui_scroll_region_style *)MexArgGetRef(&ma);

  regs_2[0] = (word)-1;

  if (!key || !*key || !mex_style)
  {
    if (key)
      free(key);
    return MexArgEnd(&ma);
  }

  if (mex_find_scroll_region(key))
  {
    free(key);
    regs_2[0] = (word)-2;
    return MexArgEnd(&ma);
  }

  ui_scrolling_region_style_default(&style);
  style.attr = (byte)mex_style->attr;
  style.scrollbar_attr = (byte)mex_style->scrollbar_attr;
  style.flags = (int)mex_style->flags;

  obj = (mex_scroll_region_obj_t *)calloc(1, sizeof(*obj));
  if (!obj)
  {
    free(key);
    regs_2[0] = (word)-3;
    return MexArgEnd(&ma);
  }

  obj->key = key;
  ui_scrolling_region_init(&obj->r, x, y, width, height, max_lines, &style);
  obj->next = g_scroll_regions;
  g_scroll_regions = obj;

  regs_2[0] = 0;
  return MexArgEnd(&ma);
}

word EXPENTRY intrin_ui_scroll_region_destroy(void)
{
  MA ma;
  char *key;
  mex_scroll_region_obj_t *cur;
  mex_scroll_region_obj_t *prev;

  MexArgBegin(&ma);
  key = MexArgGetString(&ma, FALSE);

  regs_2[0] = (word)-1;

  if (!key)
    return MexArgEnd(&ma);

  prev = NULL;
  for (cur = g_scroll_regions; cur; prev = cur, cur = cur->next)
    if (cur->key && strcmp(cur->key, key) == 0)
      break;

  free(key);

  if (!cur)
    return MexArgEnd(&ma);

  if (prev)
    prev->next = cur->next;
  else
    g_scroll_regions = cur->next;

  ui_scrolling_region_free(&cur->r);
  if (cur->key)
    free(cur->key);
  free(cur);

  regs_2[0] = 0;
  return MexArgEnd(&ma);
}

word EXPENTRY intrin_ui_scroll_region_append(void)
{
  MA ma;
  char *key;
  char *text;
  int flags;
  mex_scroll_region_obj_t *obj;

  MexArgBegin(&ma);
  key = MexArgGetString(&ma, FALSE);
  text = MexArgGetString(&ma, FALSE);
  flags = (int)MexArgGetWord(&ma);

  regs_2[0] = (word)-1;

  obj = mex_find_scroll_region(key);
  if (obj)
  {
    ui_scrolling_region_append(&obj->r, text ? text : "", flags);
    regs_2[0] = 0;
  }

  if (key)
    free(key);
  if (text)
    free(text);

  return MexArgEnd(&ma);
}

word EXPENTRY intrin_ui_scroll_region_render(void)
{
  MA ma;
  char *key;
  mex_scroll_region_obj_t *obj;

  MexArgBegin(&ma);
  key = MexArgGetString(&ma, FALSE);

  regs_2[0] = (word)-1;

  obj = mex_find_scroll_region(key);
  if (obj)
  {
    ui_scrolling_region_render(&obj->r);
    if (pmisThis->pmid->instant_video)
      vbuf_flush();
    regs_2[0] = 0;
  }

  if (key)
    free(key);

  return MexArgEnd(&ma);
}

word EXPENTRY intrin_ui_scroll_region_handle_key(void)
{
  MA ma;
  char *key;
  int keycode;
  mex_scroll_region_obj_t *obj;

  MexArgBegin(&ma);
  key = MexArgGetString(&ma, FALSE);
  keycode = (int)MexArgGetWord(&ma);

  regs_2[0] = 0;

  obj = mex_find_scroll_region(key);
  if (obj)
    regs_2[0] = (word)ui_scrolling_region_handle_key(&obj->r, keycode);

  if (key)
    free(key);

  return MexArgEnd(&ma);
}

word EXPENTRY intrin_ui_text_viewer_style_default(void)
{
  MA ma;
  struct mex_ui_text_viewer_style *mex_style;
  ui_text_viewer_style_t style;

  MexArgBegin(&ma);
  mex_style = (struct mex_ui_text_viewer_style *)MexArgGetRef(&ma);

  ui_text_viewer_style_default(&style);

  if (mex_style)
  {
    mex_style->attr = (word)style.attr;
    mex_style->status_attr = (word)style.status_attr;
    mex_style->scrollbar_attr = (word)style.scrollbar_attr;
    mex_style->flags = (word)style.flags;
  }

  return MexArgEnd(&ma);
}

word EXPENTRY intrin_ui_text_viewer_create(void)
{
  MA ma;
  char *key;
  int x;
  int y;
  int width;
  int height;
  struct mex_ui_text_viewer_style *mex_style;
  ui_text_viewer_style_t style;
  mex_text_viewer_obj_t *obj;

  MexArgBegin(&ma);
  key = MexArgGetString(&ma, FALSE);
  x = (int)MexArgGetWord(&ma);
  y = (int)MexArgGetWord(&ma);
  width = (int)MexArgGetWord(&ma);
  height = (int)MexArgGetWord(&ma);
  mex_style = (struct mex_ui_text_viewer_style *)MexArgGetRef(&ma);

  regs_2[0] = (word)-1;

  if (!key || !*key || !mex_style)
  {
    if (key)
      free(key);
    return MexArgEnd(&ma);
  }

  if (mex_find_text_viewer(key))
  {
    free(key);
    regs_2[0] = (word)-2;
    return MexArgEnd(&ma);
  }

  ui_text_viewer_style_default(&style);
  style.attr = (byte)mex_style->attr;
  style.status_attr = (byte)mex_style->status_attr;
  style.scrollbar_attr = (byte)mex_style->scrollbar_attr;
  style.flags = (int)mex_style->flags;

  obj = (mex_text_viewer_obj_t *)calloc(1, sizeof(*obj));
  if (!obj)
  {
    free(key);
    regs_2[0] = (word)-3;
    return MexArgEnd(&ma);
  }

  obj->key = key;
  ui_text_viewer_init(&obj->v, x, y, width, height, &style);
  obj->next = g_text_viewers;
  g_text_viewers = obj;

  regs_2[0] = 0;
  return MexArgEnd(&ma);
}

word EXPENTRY intrin_ui_text_viewer_destroy(void)
{
  MA ma;
  char *key;
  mex_text_viewer_obj_t *cur;
  mex_text_viewer_obj_t *prev;

  MexArgBegin(&ma);
  key = MexArgGetString(&ma, FALSE);

  regs_2[0] = (word)-1;

  if (!key)
    return MexArgEnd(&ma);

  prev = NULL;
  for (cur = g_text_viewers; cur; prev = cur, cur = cur->next)
    if (cur->key && strcmp(cur->key, key) == 0)
      break;

  free(key);

  if (!cur)
    return MexArgEnd(&ma);

  if (prev)
    prev->next = cur->next;
  else
    g_text_viewers = cur->next;

  ui_text_viewer_free(&cur->v);
  if (cur->key)
    free(cur->key);
  free(cur);

  regs_2[0] = 0;
  return MexArgEnd(&ma);
}

word EXPENTRY intrin_ui_text_viewer_set_text(void)
{
  MA ma;
  char *key;
  char *text;
  mex_text_viewer_obj_t *obj;

  MexArgBegin(&ma);
  key = MexArgGetString(&ma, FALSE);
  text = MexArgGetString(&ma, FALSE);

  regs_2[0] = (word)-1;

  obj = mex_find_text_viewer(key);
  if (obj)
  {
    ui_text_viewer_set_text(&obj->v, text ? text : "");
    regs_2[0] = 0;
  }

  if (key)
    free(key);
  if (text)
    free(text);

  return MexArgEnd(&ma);
}

word EXPENTRY intrin_ui_text_viewer_render(void)
{
  MA ma;
  char *key;
  mex_text_viewer_obj_t *obj;

  MexArgBegin(&ma);
  key = MexArgGetString(&ma, FALSE);

  regs_2[0] = (word)-1;

  obj = mex_find_text_viewer(key);
  if (obj)
  {
    ui_text_viewer_render(&obj->v);
    if (pmisThis->pmid->instant_video)
      vbuf_flush();
    regs_2[0] = 0;
  }

  if (key)
    free(key);

  return MexArgEnd(&ma);
}

word EXPENTRY intrin_ui_text_viewer_handle_key(void)
{
  MA ma;
  char *key;
  int keycode;
  mex_text_viewer_obj_t *obj;

  MexArgBegin(&ma);
  key = MexArgGetString(&ma, FALSE);
  keycode = (int)MexArgGetWord(&ma);

  regs_2[0] = 0;

  obj = mex_find_text_viewer(key);
  if (obj)
    regs_2[0] = (word)ui_text_viewer_handle_key(&obj->v, keycode);

  if (key)
    free(key);

  return MexArgEnd(&ma);
}

word EXPENTRY intrin_ui_text_viewer_read_key(void)
{
  MA ma;
  char *key;
  mex_text_viewer_obj_t *obj;
  int k;

  MexArgBegin(&ma);
  key = MexArgGetString(&ma, FALSE);

  regs_2[0] = 0;

  obj = mex_find_text_viewer(key);
  if (obj)
  {
    k = ui_text_viewer_read_key(&obj->v);
    regs_2[0] = (word)k;
  }

  if (key)
    free(key);

  return MexArgEnd(&ma);
}

#endif /* MEX */
