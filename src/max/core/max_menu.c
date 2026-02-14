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

#ifndef __GNUC__
#pragma off(unreferenced)
static char rcs_id[]="$Id: max_menu.c,v 1.4 2004/01/28 06:38:10 paltas Exp $";
#pragma on(unreferenced)
#endif

/*# name=Menu server
*/

#define MAX_INCL_COMMS

#define MAX_LANG_global
#define MAX_LANG_m_area
#define MAX_LANG_sysop
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "prog.h"
#include "mm.h"
#include "max_msg.h"
#include "max_file.h"
#include "max_menu.h"
#include "display.h"
#include "ui_lightbar.h"


static char *pszMenuName=NULL;

char *CurrentMenuName(void)
{
  return pszMenuName ? pszMenuName : blank_str;
}


static void near ProcessMenuName(char *name, char *menu_name)
{
  pszMenuName=menu_name;

  if (strchr(name, '%'))
    Parse_Outside_Cmd(name, menu_name);
  else strcpy(menu_name, name);

  Convert_Star_To_Task(menu_name);
  menu_name[MAX_MENUNAME-1]=0;
}


int DoDspFile(byte help, word flag)
{
  return ((help==NOVICE   && (flag & MFLAG_MF_NOVICE))  ||
          (help==REGULAR  && (flag & MFLAG_MF_REGULAR)) ||
          (help==EXPERT   && (flag & MFLAG_MF_EXPERT))  ||
          (hasRIP() && (flag & MFLAG_MF_RIP)));
}

static int near DoHdrFile(byte help, word flag)
{
  return ((help==NOVICE   && (flag & MFLAG_HF_NOVICE))  ||
          (help==REGULAR  && (flag & MFLAG_HF_REGULAR)) ||
          (help==EXPERT   && (flag & MFLAG_HF_EXPERT))  ||
          (hasRIP() && (flag & MFLAG_HF_RIP)));
}

/* This shows the header part of the menu (in general, the headerfile)      *
 * to the user.                                                             */

static void near ShowMenuHeader(PAMENU pam, byte help, int first_time)
{
  char *filename;

  filename=MNU(*pam, m.headfile);

  if (!*filename || !DoHdrFile(help, pam->m.flag))
  {
    if (hasRIP())
      Putc('\n');
    else
      Puts("\n\n");
  }

  /* Is it a MEX file? */

  if (*filename==':')
  {
    char temp[PATHLEN];

    /* Run the MEX file, passing it an argument stating whether or not    *
     * this is the first time we've been through the menu.                */

    sprintf(temp, "%s %d", filename+1, first_time);

    Mex(temp);
  }
  else if (Display_File(DISPLAY_HOTMENU | DISPLAY_MENUHELP, NULL, filename)==-1)
  {
    logit(cantfind, filename);
  }
}

static void near ShowMenuFile(PAMENU pam, char *filename)
{
  if (! *linebuf &&
      Display_File(hasRIP() ? DISPLAY_MENUHELP : (DISPLAY_HOTMENU | DISPLAY_MENUHELP),
                   NULL,
                   filename)==-1)
  {
    logit(cantfind, filename);
  }

  if (pam->m.hot_colour != -1)
  {
    Printf(attr_string, pam->m.hot_colour & 0x7f);
    Puts(pam->m.hot_colour > 0x7f ? BLINK : blank_str);
  }
}


/* Returns TRUE if the next waiting keystroke is a non-junk menu option */

static int near GotMenuStroke(void)
{
  int ch;

  while ((ch=Mdm_kpeek())==8 || ch==0x7f)
    Mdm_getcw();

  return (ch != -1);
}


/* Show one individual menu command */

static void near ShowMenuCommand(PAMENU pam, struct _opt *popt, int eol, int first_opt, byte help)
{
  char *optname=(pam->menuheap + popt->name);
  int nontty;

  switch (help)
  {
    default: /* novice */
      nontty = usr.video != GRAPH_TTY;

      {
        int field_w = pam->m.opt_width + nontty - 3;
        const char *txt = optname + 1;
        int txt_len = 0;
        int pad_l = 0;
        int pad_r;

        if (field_w < 0)
          field_w = 0;

        while (txt_len < field_w && txt[txt_len])
          txt_len++;

        if (pam->cm_enabled)
        {
          if (pam->cm_option_justify == 1)
            pad_l = (field_w - txt_len) / 2;
          else if (pam->cm_option_justify == 2)
            pad_l = (field_w - txt_len);
        }

        if (pad_l < 0)
          pad_l = 0;

        pad_r = field_w - pad_l - txt_len;
        if (pad_r < 0)
          pad_r = 0;

        Printf("%s%*s%s%c%s%s%.*s%*s%c",
               menu_opt_col,
               pad_l,
               blank_str,
               menu_high_col,
               *optname,
               menu_opt_col,
               ")" + nontty,
               txt_len,
               txt,
               pad_r,
               blank_str,
               eol ? '\n' : ' ');
      }
      break;

    case REGULAR:
      Printf("%s%c", " " + !!first_opt, *optname);

    case EXPERT:
      break;
  }
}


/* This shows all of the canned menu commands for the menu body */

static void near ShowMenuCanned(PAMENU pam, byte help, char *title, char *menuname)
{
  struct _opt *popt, *eopt;
  int opts_per_line, num_opts, num_shown;
  int first_opt=TRUE;

  /* Exit if we have stacked input */

  if (*linebuf || ((usr.bits & BITS_HOTKEYS) && GotMenuStroke()))
    return;

  Printf("%s%s%c", menu_name_col, title,
         help==NOVICE ? '\n' : ' ');

  if (help==REGULAR)
    Printf(menu_start);

  if (!pam->m.opt_width)
    pam->m.opt_width=DEFAULT_OPT_WIDTH;
  opts_per_line = (TermWidth()+1) / pam->m.opt_width;
  if (opts_per_line<=0)
    opts_per_line=1;
  num_opts=0;

  for (popt=pam->opt, eopt=popt + pam->m.num_options, num_shown=0;
       popt < eopt && !brk_trapped && !mdm_halt() &&
       ((usr.bits & BITS_HOTKEYS)==0 || !GotMenuStroke());
       popt++)
  {
    if (popt->type && OptionOkay(pam, popt, TRUE, NULL, &mah, &fah, menuname))
    {
      if (++num_opts < opts_per_line)
        ShowMenuCommand(pam, popt, FALSE, first_opt, help);
      else
      {
        ShowMenuCommand(pam, popt, TRUE, first_opt, help);

        if (pam->cm_enabled && pam->cm_option_spacing && help==NOVICE)
          Putc('\n');

        num_opts=0;
      }

      num_shown++;

      first_opt=FALSE;
    }
  }

  switch (help)
  {
    case REGULAR: Printf(menu_end);  break;
    case NOVICE:  Printf(ATTR "%s%s", CWHITE,
                         (num_shown % opts_per_line)==0 ? "" : "\n",
                         select_p); break;
  }

  Puts(GRAY);
}


/* Bounded canned menu renderer.
 *
 * Same as ShowMenuCanned but positions each option within configured boundaries.
 * Uses the same ShowMenuCommand() logic with explicit cursor positioning.
 */
static void near ShowMenuCannedBounded(PAMENU pam, byte help, char *title, char *menuname)
{
  struct _opt *popt, *eopt;
  int width, height, opts_per_row;
  int num_shown, first_opt;
  int cell_w;
  int row_spacing;
  int row_step;
  int max_rows;
  int boundary_width;
  int boundary_height;
  int base_x = 0;
  int base_x_inited = 0;
  int total_valid = -1;
  int total_rows = 0;
  int last_row_cols = 0;
  int spread_w = 0;
  int spread_h = 0;
  int spread_gap_y = 0;
  int spread_off_y = 0;
  int vjust_off_y = 0;

  /* Exit if we have stacked input */
  if (*linebuf || ((usr.bits & BITS_HOTKEYS) && GotMenuStroke()))
    return;

  if (!pam->m.opt_width)
    pam->m.opt_width=DEFAULT_OPT_WIDTH;

  /* Canned bounded renderer uses the legacy menu cell width (opt_width). */
  cell_w = (int)pam->m.opt_width;

  width = (int)(pam->cm_x2 - pam->cm_x1 + 1);
  height = (int)(pam->cm_y2 - pam->cm_y1 + 1);
  opts_per_row = width / pam->m.opt_width;
  if (opts_per_row <= 0)
    opts_per_row = 1;

  row_spacing = (pam->cm_enabled && pam->cm_option_spacing) ? 1 : 0;
  row_step = 1 + row_spacing;
  max_rows = (height + row_step - 1) / row_step;
  boundary_width = (int)(pam->cm_x2 - pam->cm_x1 + 1);
  boundary_height = (int)(pam->cm_y2 - pam->cm_y1 + 1);

  /* Spread modes:
   * - 2: spread (full)
   * - 3: spread_width
   * - 4: spread_height
   */
  if (pam->cm_enabled)
  {
    if (pam->cm_boundary_layout == 2)
    {
      spread_w = 1;
      spread_h = 1;
    }
    else if (pam->cm_boundary_layout == 3)
      spread_w = 1;
    else if (pam->cm_boundary_layout == 4)
      spread_h = 1;
  }

  /* For tight/spread layouts we need to know how many options will actually be shown. */
  if (pam->cm_enabled && (pam->cm_boundary_layout == 1 || spread_w || spread_h || pam->cm_boundary_vjustify != 0))
  {
    total_valid = 0;
    for (popt=pam->opt, eopt=popt + pam->m.num_options;
         popt < eopt && !brk_trapped && !mdm_halt() &&
         ((usr.bits & BITS_HOTKEYS)==0 || !GotMenuStroke());
         popt++)
    {
      if (popt->type && OptionOkay(pam, popt, TRUE, NULL, &mah, &fah, menuname))
        total_valid++;
    }

    total_rows = (total_valid + opts_per_row - 1) / opts_per_row;
    last_row_cols = (total_valid % opts_per_row);
    if (last_row_cols == 0)
      last_row_cols = opts_per_row;
  }

  /* Apply vertical boundary justification for non-vertical-spread layouts.
   * (spread_height/spread already includes vertical justification via spread_off_y)
   */
  if (pam->cm_enabled && !spread_h && pam->cm_boundary_vjustify != 0 && total_valid >= 0)
  {
    int R = total_rows;
    int Rdisp = R;
    int content_h;
    int span_y;

    if (Rdisp > max_rows)
      Rdisp = max_rows;

    if (Rdisp > 1)
      content_h = Rdisp + (Rdisp - 1) * row_spacing;
    else if (Rdisp == 1)
      content_h = 1;
    else
      content_h = 0;

    span_y = boundary_height - content_h;
    if (span_y < 0)
      span_y = 0;

    if (pam->cm_boundary_vjustify == 1)
      vjust_off_y = span_y / 2;
    else if (pam->cm_boundary_vjustify == 2)
      vjust_off_y = span_y;
    else
      vjust_off_y = 0;
  }

  /* Vertical spread pre-compute.
   * sticky-first when option_spacing=false: at most 1 extra blank line per gap.
   * spacing-first when option_spacing=true: distribute span evenly per gap.
   */
  if (pam->cm_enabled && spread_h && total_valid >= 0)
  {
    int R = total_rows;

    if (R <= 0)
      R = 0;

    if (R <= 1)
    {
      int span_y = boundary_height - 1;
      if (span_y < 0)
        span_y = 0;

      if (pam->cm_boundary_vjustify == 1)
        spread_off_y = span_y / 2;
      else if (pam->cm_boundary_vjustify == 2)
        spread_off_y = span_y;
      else
        spread_off_y = 0;
      spread_gap_y = 0;
    }
    else
    {
      int base_row_gap = row_spacing;
      int content_h = R + (R - 1) * base_row_gap;
      int span_y = boundary_height - content_h;
      int gaps = R - 1;
      int leftover_y;
      int offset_y;

      if (span_y < 0)
        span_y = 0;

      if (row_spacing)
        spread_gap_y = span_y / gaps;
      else
        spread_gap_y = (span_y >= gaps) ? 1 : 0;

      leftover_y = span_y - (spread_gap_y * gaps);
      if (leftover_y < 0)
        leftover_y = 0;

      if (pam->cm_boundary_vjustify == 1)
        offset_y = leftover_y / 2;
      else if (pam->cm_boundary_vjustify == 2)
        offset_y = leftover_y;
      else
        offset_y = 0;

      spread_off_y = offset_y;
    }
  }

  /* Title at configured location */
  if (pam->cm_show_title)
  {
    if (pam->cm_title_x > 0 && pam->cm_title_y > 0)
      Goto(pam->cm_title_y, pam->cm_title_x);

    Printf("%s%s%c", menu_name_col, title,
           help==NOVICE ? '\n' : ' ');

    if (help==REGULAR)
      Printf(menu_start);
  }

  /* Render options with positioning */
  num_shown = 0;
  first_opt = TRUE;
  for (popt=pam->opt, eopt=popt + pam->m.num_options;
       popt < eopt && !brk_trapped && !mdm_halt() &&
       ((usr.bits & BITS_HOTKEYS)==0 || !GotMenuStroke());
       popt++)
  {
    if (popt->type && OptionOkay(pam, popt, TRUE, NULL, &mah, &fah, menuname))
    {
      int row = num_shown / opts_per_row;
      int col = num_shown % opts_per_row;
      int cols_in_row = opts_per_row;
      int grid_w;
      int x;
      int eol;

      if (pam->cm_enabled && (pam->cm_boundary_layout == 1 || spread_w || spread_h) && total_valid >= 0)
      {
        if (row == total_rows - 1)
          cols_in_row = last_row_cols;
      }

      eol = (col == cols_in_row - 1);

      grid_w = cols_in_row * cell_w;

      if (pam->cm_enabled && spread_w)
      {
        int span = boundary_width - (cols_in_row * cell_w);

        if (span <= 0)
        {
          x = (int)pam->cm_x1 + (col * cell_w);
        }
        else if (cols_in_row <= 1)
        {
          int offset = 0;

          if (pam->cm_boundary_justify == 1)
            offset = span / 2;
          else if (pam->cm_boundary_justify == 2)
            offset = span;

          x = (int)pam->cm_x1 + offset;
        }
        else
        {
          int gaps = cols_in_row - 1;
          int gap = span / gaps;
          int leftover = span - (gap * gaps);
          int offset = 0;

          if (pam->cm_boundary_justify == 1)
            offset = leftover / 2;
          else if (pam->cm_boundary_justify == 2)
            offset = leftover;

          x = (int)pam->cm_x1 + offset + (col * (cell_w + gap));
        }
      }
      else
      {
        if (!base_x_inited || (pam->cm_enabled && pam->cm_boundary_layout == 1))
        {
          /* Default layout is "grid"; "tight" re-computes per-row. */
          if (pam->cm_enabled && pam->cm_boundary_layout != 1)
            grid_w = opts_per_row * pam->m.opt_width;

          if (grid_w >= boundary_width)
            base_x = (int)pam->cm_x1;
          else if (pam->cm_enabled && pam->cm_boundary_justify == 1)
            base_x = (int)pam->cm_x1 + (boundary_width - grid_w) / 2;
          else if (pam->cm_enabled && pam->cm_boundary_justify == 2)
            base_x = (int)pam->cm_x2 - grid_w + 1;
          else
            base_x = (int)pam->cm_x1;

          base_x_inited = 1;
        }

        x = base_x + (col * cell_w);
      }

      if (row < max_rows)
      {
        int y;

        if (pam->cm_enabled && spread_h && total_valid >= 0)
          y = (int)pam->cm_y1 + spread_off_y + (row * (1 + row_spacing + spread_gap_y));
        else
          y = (int)pam->cm_y1 + vjust_off_y + row + (row * row_spacing);

        Goto(y, x);
        ShowMenuCommand(pam, popt, eol, first_opt, help);
        num_shown++;
        first_opt = FALSE;
      }
    }
  }

  /* Print select prompt at prompt_location for NOVICE */
  switch (help)
  {
    case REGULAR: Printf(menu_end); break;
    case NOVICE:
      if (pam->cm_prompt_x > 0 && pam->cm_prompt_y > 0)
      {
        Goto(pam->cm_prompt_y, pam->cm_prompt_x);
        Printf(ATTR "%s", CWHITE, select_p);
      }
      else
      {
        Printf(ATTR "%s%s", CWHITE,
               (num_shown % opts_per_row)==0 ? "" : "\n",
               select_p);
      }
      break;
  }

  Puts(GRAY);
}


/* This displays the body of a menu to the user */

static void near ShowMenuBody(PAMENU pam, byte help, char *title, char *menuname)
{
  char *filename=MNU(*pam, m.dspfile);

  /* Bounded NOVICE lightbar frame: let ui_lightbar paint the items. */
  if (help == NOVICE && pam->cm_enabled && pam->cm_lightbar_menu &&
      pam->cm_x1 > 0 && pam->cm_y1 > 0 && pam->cm_x2 >= pam->cm_x1 && pam->cm_y2 >= pam->cm_y1)
  {
    /* If there is a custom menu file to be displayed */
    if (*filename && DoDspFile(help, pam->m.flag))
    {
      ShowMenuFile(pam, filename);

      /* If configured, skip canned output */
      if (pam->cm_enabled && pam->cm_skip_canned_menu)
        return;
    }

    /* Exit if we have stacked input */
    if (*linebuf || ((usr.bits & BITS_HOTKEYS) && GotMenuStroke()))
      return;

    /* Title at configured location */
    if (pam->cm_show_title)
    {
      if (pam->cm_title_x > 0 && pam->cm_title_y > 0)
        Goto(pam->cm_title_y, pam->cm_title_x);

      Printf("%s%s%c", menu_name_col, title,
             help==NOVICE ? '\n' : ' ');

      if (help==REGULAR)
        Printf(menu_start);
    }

    /* Suppress the legacy select prompt; lightbar is the prompt. */
    Puts(GRAY);
    return;
  }

  /* If there is a custom menu file to be displayed */
  if (*filename && DoDspFile(help, pam->m.flag))
  {
    ShowMenuFile(pam, filename);

    /* If configured, skip canned output */
    if (pam->cm_enabled && pam->cm_skip_canned_menu)
      return;
  }

  /* Render canned menu - bounded if boundaries configured, normal otherwise */
  if (help == NOVICE && pam->cm_enabled && pam->cm_x1 > 0 && pam->cm_y1 > 0 && 
      pam->cm_x2 >= pam->cm_x1 && pam->cm_y2 >= pam->cm_y1)
    ShowMenuCannedBounded(pam, help, title, menuname);
  else
    ShowMenuCanned(pam, help, title, menuname);
}


static int near GetMenuResponseLightbarBounded(PAMENU pam, byte help, char *menuname)
{
  struct _opt *popt, *eopt;
  ui_lightbar_item_t *items = NULL;
  char **texts = NULL;
  int count = 0;
  int cap = 0;
  int out_key = 0;

  int lb_margin;
  int cell_w;

  int nontty;

  int width, height, opts_per_row;
  int row_spacing;
  int row_step;
  int max_rows;
  int boundary_width;
  int boundary_height;
  int base_x = 0;
  int base_x_inited = 0;
  int total_valid = -1;
  int total_rows = 0;
  int last_row_cols = 0;
  int spread_w = 0;
  int spread_h = 0;
  int spread_gap_y = 0;
  int spread_off_y = 0;
  int vjust_off_y = 0;

  if (pam == NULL)
    return -1;

  if (usr.video == GRAPH_TTY)
    return -1;

  nontty = usr.video != GRAPH_TTY;

  if (!pam->m.opt_width)
    pam->m.opt_width = DEFAULT_OPT_WIDTH;

  lb_margin = (pam->cm_enabled) ? (int)pam->cm_lightbar_margin : 1;
  if (lb_margin < 0)
    lb_margin = 0;
  cell_w = (int)pam->m.opt_width + (lb_margin * 2);
  if (cell_w < 1)
    cell_w = 1;

  width = (int)(pam->cm_x2 - pam->cm_x1 + 1);
  height = (int)(pam->cm_y2 - pam->cm_y1 + 1);
  opts_per_row = width / cell_w;
  if (opts_per_row <= 0)
    opts_per_row = 1;

  row_spacing = (pam->cm_enabled && pam->cm_option_spacing) ? 1 : 0;
  row_step = 1 + row_spacing;
  max_rows = (height + row_step - 1) / row_step;
  boundary_width = (int)(pam->cm_x2 - pam->cm_x1 + 1);
  boundary_height = (int)(pam->cm_y2 - pam->cm_y1 + 1);

  if (pam->cm_enabled)
  {
    if (pam->cm_boundary_layout == 2)
    {
      spread_w = 1;
      spread_h = 1;
    }
    else if (pam->cm_boundary_layout == 3)
      spread_w = 1;
    else if (pam->cm_boundary_layout == 4)
      spread_h = 1;
  }

  if (pam->cm_enabled && (pam->cm_boundary_layout == 1 || spread_w || spread_h || pam->cm_boundary_vjustify != 0))
  {
    total_valid = 0;
    for (popt=pam->opt, eopt=popt + pam->m.num_options;
         popt < eopt && !brk_trapped && !mdm_halt() &&
         ((usr.bits & BITS_HOTKEYS)==0 || !GotMenuStroke());
         popt++)
    {
      if (popt->type && OptionOkay(pam, popt, TRUE, NULL, &mah, &fah, menuname))
      {
        char *optname = pam->menuheap + popt->name;
        if (optname && *optname && *optname != '`')
          total_valid++;
      }
    }

    total_rows = (total_valid + opts_per_row - 1) / opts_per_row;
    last_row_cols = (total_valid % opts_per_row);
    if (last_row_cols == 0)
      last_row_cols = opts_per_row;
  }

  if (pam->cm_enabled && !spread_h && pam->cm_boundary_vjustify != 0 && total_valid >= 0)
  {
    int R = total_rows;
    int Rdisp = R;
    int content_h;
    int span_y;

    if (Rdisp > max_rows)
      Rdisp = max_rows;

    if (Rdisp > 1)
      content_h = Rdisp + (Rdisp - 1) * row_spacing;
    else if (Rdisp == 1)
      content_h = 1;
    else
      content_h = 0;

    span_y = boundary_height - content_h;
    if (span_y < 0)
      span_y = 0;

    if (pam->cm_boundary_vjustify == 1)
      vjust_off_y = span_y / 2;
    else if (pam->cm_boundary_vjustify == 2)
      vjust_off_y = span_y;
    else
      vjust_off_y = 0;
  }

  if (pam->cm_enabled && spread_h && total_valid >= 0)
  {
    int R = total_rows;

    if (R <= 0)
      R = 0;

    if (R <= 1)
    {
      int span_y = boundary_height - 1;
      if (span_y < 0)
        span_y = 0;

      if (pam->cm_boundary_vjustify == 1)
        spread_off_y = span_y / 2;
      else if (pam->cm_boundary_vjustify == 2)
        spread_off_y = span_y;
      else
        spread_off_y = 0;
      spread_gap_y = 0;
    }
    else
    {
      int base_row_gap = row_spacing;
      int content_h = R + (R - 1) * base_row_gap;
      int span_y = boundary_height - content_h;
      int gaps = R - 1;
      int leftover_y;
      int offset_y;

      if (span_y < 0)
        span_y = 0;

      if (row_spacing)
        spread_gap_y = span_y / gaps;
      else
        spread_gap_y = (span_y >= gaps) ? 1 : 0;

      leftover_y = span_y - (spread_gap_y * gaps);
      if (leftover_y < 0)
        leftover_y = 0;

      if (pam->cm_boundary_vjustify == 1)
        offset_y = leftover_y / 2;
      else if (pam->cm_boundary_vjustify == 2)
        offset_y = leftover_y;
      else
        offset_y = 0;

      spread_off_y = offset_y;
    }
  }

  cap = (total_valid >= 0) ? total_valid : (int)pam->m.num_options;
  if (cap < 1)
    cap = 1;

  items = (ui_lightbar_item_t *)calloc((size_t)cap, sizeof(ui_lightbar_item_t));
  texts = (char **)calloc((size_t)cap, sizeof(char *));
  if (items == NULL || texts == NULL)
    goto done;

  for (popt=pam->opt, eopt=popt + pam->m.num_options;
       popt < eopt && !brk_trapped && !mdm_halt() &&
       ((usr.bits & BITS_HOTKEYS)==0 || !GotMenuStroke());
       popt++)
  {
    if (popt->type && OptionOkay(pam, popt, TRUE, NULL, &mah, &fah, menuname))
    {
      char *optname = pam->menuheap + popt->name;
      int hk;
      const char *txt;
      int field_w;
      int txt_len;
      int pad_l;
      int pad_r;
      int row;
      int col;
      int cols_in_row;
      int grid_w;
      int x;
      int y;
      int justify;

      if (optname == NULL || *optname == '\0')
        continue;

      if (*optname == '`')
        continue;

      hk = (int)toupper((unsigned char)*optname);
      if (hk == '[' || hk == ']')
        continue;
      if (!isprint((unsigned char)hk) || hk == ' ')
        continue;

      txt = optname + 1;
      field_w = (int)pam->m.opt_width + nontty - 3;
      if (field_w < 0)
        field_w = 0;
      txt_len = 0;
      while (txt_len < field_w && txt[txt_len])
        txt_len++;

      pad_l = 0;
      if (pam->cm_enabled)
      {
        if (pam->cm_option_justify == 1)
          pad_l = (field_w - txt_len) / 2;
        else if (pam->cm_option_justify == 2)
          pad_l = (field_w - txt_len);
      }
      if (pad_l < 0)
        pad_l = 0;
      pad_r = field_w - pad_l - txt_len;
      if (pad_r < 0)
        pad_r = 0;

      row = count / opts_per_row;
      col = count % opts_per_row;
      cols_in_row = opts_per_row;

      if (pam->cm_enabled && (pam->cm_boundary_layout == 1 || spread_w || spread_h) && total_valid >= 0)
      {
        if (row == total_rows - 1)
          cols_in_row = last_row_cols;
      }

      grid_w = cols_in_row * cell_w;

      if (pam->cm_enabled && spread_w)
      {
        int span = boundary_width - (cols_in_row * cell_w);

        if (span <= 0)
        {
          x = (int)pam->cm_x1 + (col * cell_w);
        }
        else if (cols_in_row <= 1)
        {
          int offset = 0;

          if (pam->cm_boundary_justify == 1)
            offset = span / 2;
          else if (pam->cm_boundary_justify == 2)
            offset = span;

          x = (int)pam->cm_x1 + offset;
        }
        else
        {
          int gaps = cols_in_row - 1;
          int gap = span / gaps;
          int leftover = span - (gap * gaps);
          int offset = 0;

          if (pam->cm_boundary_justify == 1)
            offset = leftover / 2;
          else if (pam->cm_boundary_justify == 2)
            offset = leftover;

          x = (int)pam->cm_x1 + offset + (col * (cell_w + gap));
        }
      }
      else
      {
        if (!base_x_inited || (pam->cm_enabled && pam->cm_boundary_layout == 1))
        {
          if (pam->cm_enabled && pam->cm_boundary_layout != 1)
            grid_w = opts_per_row * cell_w;

          if (grid_w >= boundary_width)
            base_x = (int)pam->cm_x1;
          else if (pam->cm_enabled && pam->cm_boundary_justify == 1)
            base_x = (int)pam->cm_x1 + (boundary_width - grid_w) / 2;
          else if (pam->cm_enabled && pam->cm_boundary_justify == 2)
            base_x = (int)pam->cm_x2 - grid_w + 1;
          else
            base_x = (int)pam->cm_x1;

          base_x_inited = 1;
        }

        x = base_x + (col * cell_w);
      }

      if (row >= max_rows)
        break;

      if (pam->cm_enabled && spread_h && total_valid >= 0)
        y = (int)pam->cm_y1 + spread_off_y + (row * (1 + row_spacing + spread_gap_y));
      else
        y = (int)pam->cm_y1 + vjust_off_y + row + (row * row_spacing);

      justify = UI_JUSTIFY_LEFT;
      /* Padding is baked into the string to match ShowMenuCommand() output. */

      {
        size_t need = (size_t)pad_l + 3 + (size_t)txt_len + (size_t)pad_r + 1;
        char *s = (char *)malloc(need);
        if (s == NULL)
          goto done;

        if (pad_l > 0)
          memset(s, ' ', (size_t)pad_l);
        snprintf(s + pad_l, need - (size_t)pad_l, "[%c]%.*s", (char)hk, txt_len, txt);
        if (pad_r > 0)
          memset(s + pad_l + 3 + (size_t)txt_len, ' ', (size_t)pad_r);
        s[need - 1] = '\0';

        texts[count] = s;
        items[count].text = s;
        items[count].x = x;
        items[count].y = y;
        items[count].width = pam->m.opt_width;
        items[count].justify = justify;
      }

      count++;
      if (count >= cap)
        break;
    }
  }

  if (count > 0)
  {
    ui_lightbar_pos_menu_t m;
    memset(&m, 0, sizeof(m));
    m.items = items;
    m.count = count;
    m.normal_attr = pam->cm_lightbar_normal_attr;
    m.selected_attr = pam->cm_lightbar_selected_attr;
    m.hotkey_attr = pam->cm_lightbar_high_attr;
    m.hotkey_highlight_attr = pam->cm_lightbar_high_selected_attr;
    m.margin = lb_margin;
    m.wrap = 1;
    m.enable_hotkeys = 1;
    m.show_brackets = 0;
    (void)ui_lightbar_run_pos_hotkey(&m, &out_key);
  }

done:
  if (texts)
  {
    int i;
    for (i = 0; i < count; i++)
      free(texts[i]);
  }
  free(texts);
  free(items);

  if (out_key > 0)
  {
    /* Move cursor to prompt location before processing command to avoid overwriting menu */
    if (pam->cm_prompt_x > 0 && pam->cm_prompt_y > 0)
      Goto(pam->cm_prompt_y, pam->cm_prompt_x);
    
    return (int)toupper((unsigned char)out_key);
  }

  return -1;
}


/* Get menu response from user */

static option near GetMenuResponse(char *title)
{
  char prompt[PATHLEN];
  int ch;

  snprintf(prompt, sizeof(prompt), "%s%s: " GRAY, menu_name_col, title);

  do
  {
    ch = Input_Char(CINPUT_NOUPPER | CINPUT_PROMPT | CINPUT_P_CTRLC |
                    CINPUT_NOXLT | CINPUT_DUMP | CINPUT_MSGREAD | CINPUT_SCAN,
                    prompt);

    if (ch==10 || ch==13 || ch==0)
      ch='|';
  }
  while (ch==8 || ch==9 || ch==0x7f);

  return ch;
}


/* Display the option that was selected by the user */

static void near ShowOption(int ch, byte help, word flag)
{
  if ((usr.bits & BITS_HOTKEYS) && *linebuf==0)
  {
    if (!hasRIP() || !DoDspFile(help, flag))
      Putc(ch=='|' ? ' ' : ch);
    Putc('\n');
  }
}



/* Process the user's keystroke */

static int near ProcessMenuResponse(PAMENU pam, int *piSameMenu, char *name,
                                    XMSG *msg, byte *pbHelp, unsigned int ch,
                                    int *piRanOpt, char *menuname)
{
  struct _opt *popt, *eopt;
  unsigned upper_ch=toupper(ch);
  int shown=FALSE;
  unsigned flag;
  char *p;

  *piRanOpt=FALSE;

  if (ch=='.')
  {
    *pbHelp=NOVICE;
    return 0;
  }

  for (popt=pam->opt, eopt=popt + pam->m.num_options;
       popt < eopt;
       popt++)
  {
    int scan=-1;

    /* Handle cursor keys and other keys that use scan codes */

    if (ch > 255 && pam->menuheap[popt->name]=='`')
      scan = atoi(pam->menuheap+popt->name+1) << 8;

    if ((upper_ch==toupper(pam->menuheap[popt->name]) || ch==scan) &&
        upper_ch != '`' &&
        OptionOkay(pam, popt, FALSE, NULL, &mah, &fah, menuname))
    {

      if (popt->type != read_individual && !shown)
      {
        shown=TRUE;
        ShowOption(ch, *pbHelp, pam->m.flag);
      }

      if (pam->m.flag & MFLAG_RESET)
        RipReset();

      *pbHelp=usr.help;
      *piRanOpt=TRUE;

      next_menu_char=-1;


      p=RunOption(pam, popt, upper_ch, msg, &flag, menuname);

      if (flag & RO_NEWMENU)
      {
        *piSameMenu=FALSE;
        strcpy(name, p);
      }

      if (flag & RO_QUIT)
        return -1;

      if (flag & RO_SAVE)
        return 1;
    }
  }

  if (!*piRanOpt && ch != '|' && ch != 0x7f && ch <= 255)
  {
    ShowOption(ch, *pbHelp, pam->m.flag);
    { char _cb[2] = { (char)upper_ch, '\0' };
      LangPrintf(dontunderstand, _cb); }
    mdm_dump(DUMP_INPUT);
    ResetAttr();
    Clear_KBuffer();
    vbuf_flush();

    switch (*pbHelp)
    {
      case REGULAR: *pbHelp=NOVICE; break;
      case EXPERT: *pbHelp=REGULAR; break;
    }
  }

  return 0;
}



/* Perform menu-name substitutions based on the menu to be entered */

static int near EnterMenu(char *name, char *menu_name)
{
  static char old_replace[PATHLEN];
  static char old_name[PATHLEN];
  int rc=FALSE;

  /* The old_replace/name fields are used to hold the menuname/menureplace  *
   * strings of the area which caused us to shift into a custom menu.  If   *
   * these fields are non-null, they mean that we ARE currently in a custom *
   * menu.                                                                  *
   *                                                                        *
   * The idea behind this code is to cover the changes in the current menu  *
   * when switching between areas.  The first test checks to see if the     *
   * custom menu name is still equal to the current menu name.  If this     *
   * is true, we are either still in the current area (and no action        *
   * needs to be taken), or we are in the process of leaving that area      *
   * (and the menu must be restored).                                       *
   *                                                                        *
   * If the menunames of the current message and file areas do not match    *
   * the current menu name, we know that we have switched message/file      *
   * areas, so we restore the menu name to that which was in use            *
   * originally.  This is what was stored in the old_replace value.         */

  if (*old_name)
  {
    if (eqstri(old_name, menu_name))
    {
      if (fah.heap &&
          !eqstri(FAS(fah, menuname), menu_name) &&
          !eqstri(MAS(mah, menuname), menu_name))
      {
        strcpy(menu_name, old_replace);
        strcpy(name, menu_name);
        *old_name=*old_replace=0;
        rc=TRUE;
      }
    }
    else
    {
      /* We have switched to a different menu completely, as opposed to     *
       * just changing areas, so clear the save/restore information.        */

      *old_name=*old_replace=0;
    }
  }

  /* If the current menu is to be replaced with a custom name, and          *
   * replace-name is not same as menu-name, then switch to the new menu.    */

  if (fah.heap &&
      eqstri(menu_name, FAS(fah, menureplace)) &&
      !eqstri(menu_name, FAS(fah, menuname)))
  {
    strcpy(old_name, FAS(fah, menuname));

    if (! *old_replace)
      strcpy(old_replace, FAS(fah, menureplace));

    strcpy(name, FAS(fah, menuname));
    ProcessMenuName(name, menu_name);
    rc=TRUE;
  }

  /* Repeat same for file areas */

  if (eqstri(menu_name, MAS(mah, menureplace)) &&
      !eqstri(menu_name, MAS(mah, menuname)))
  {
    strcpy(old_name, MAS(mah, menuname));

    if (! *old_replace)
      strcpy(old_replace, MAS(mah, menureplace));

    strcpy(name, MAS(mah, menuname));
    ProcessMenuName(name, menu_name);
    rc=TRUE;
  }


  /* If we have an active msg area pushed on the stack, and if we are       *
   * supposed to be using a barricade priv level                            */

  if (lam && mah.bi.use_barpriv)
    if (lam->biOldPriv.use_barpriv)
    {
      if (!eqstri(CurrentMenuName(), MAS(mah, barricademenu)))
        ExitMsgAreaBarricade();
    }
    else EnterMsgAreaBarricade();

  if (laf && fah.bi.use_barpriv)
    if (laf->biOldPriv.use_barpriv && fah.heap)
    {
      if (!eqstri(CurrentMenuName(), FAS(fah, barricademenu)))
        ExitFileAreaBarricade();
    }
    else EnterFileAreaBarricade();

  return rc;
}



static int near RiteArea(int areatype,int attrib)
{
  if ((attrib & MA_NET) && (areatype & AREATYPE_MATRIX)==0)
    return FALSE;

  if ((attrib & MA_ECHO) && (areatype & AREATYPE_ECHO)==0)
    return FALSE;

  if ((attrib & MA_CONF) && (areatype & AREATYPE_CONF)==0)
    return FALSE;

  if ((attrib & (MA_SHARED | MA_NET))==0 && (areatype & AREATYPE_LOCAL)==0)
    return FALSE;

  return TRUE;
}


static int near MagnEtOkay(struct _opt *opt)
{
  if (inmagnet)
  {
    switch (opt->type)
    {
      case edit_save:
      case edit_abort:
      case edit_list:
      case edit_edit:
      case edit_insert:
      case edit_delete:
      case edit_quote:
      case display_file:
        return FALSE;
    }
  }

  return TRUE;
}


/* Check the priv level of the option against the standard priv level       *
 * required, in addition to checking override priv levels.                  */

static int near OverridePrivOkay(struct _amenu *menu, struct _opt *popt,
                                 PMAH pmah, PFAH pfah, char *menuname)
{
  int i;
  char name=toupper(menu->menuheap[popt->name]);
  char szNewName[PATHLEN];

  if (pmah)
  {
    for (i=0; i < pmah->ma.num_override; i++)
    {
      Parse_Outside_Cmd(pmah->heap + pmah->pov[i].menuname, szNewName);

      if (pmah->pov[i].opt==popt->type &&
          eqstri(szNewName, menuname) &&
          (!pmah->pov[i].name ||
           toupper(pmah->pov[i].name)==name))
      {
        return PrivOK(pmah->heap + pmah->pov[i].acs, FALSE);
      }
    }
  }

  if (pfah)
  {
    for (i=0; i < pfah->fa.num_override; i++)
    {
      Parse_Outside_Cmd(pfah->heap + pfah->pov[i].menuname, szNewName);

      if (pfah->pov[i].opt==popt->type &&
          eqstri(szNewName, menuname) &&
          (!pfah->pov[i].name ||
           toupper(pfah->pov[i].name)==name))
      {
        return PrivOK(pfah->heap + pfah->pov[i].acs, FALSE);
      }
    }
  }

  return PrivOK(menu->menuheap + popt->priv, FALSE);
}

int OptionOkay(struct _amenu *menu, struct _opt *popt, int displaying,
               char *barricade, PMAH pmah, PFAH pfah, char *menuname)
{
  BARINFO biSave;
  BARINFO bi;
  int rc;

  bi.use_barpriv=FALSE;

  /* See if we have to temporarily adjust user's priv level because of      *
   * an extended barricade.                                                 */

  if (barricade && *barricade &&
      GetBarPriv(barricade, FALSE, pmah, pfah, &bi, TRUE) &&
      bi.use_barpriv)
  {
    /* Save user's old priv level */

    biSave.priv=usr.priv;
    biSave.keys=usr.xkeys;

    /* Temporarily use this new priv level */

    usr.priv=bi.priv;
    usr.xkeys=bi.keys;
  }

  rc = (OverridePrivOkay(menu, popt, pmah, pfah, menuname) &&
        RiteArea(popt->areatype, pmah->ma.attribs) &&
        MagnEtOkay(popt) &&
        (local ? (popt->flag & OFLAG_UREMOTE)==0
               : (popt->flag & OFLAG_ULOCAL)==0) &&
        (!displaying || (popt->flag & OFLAG_NODSP)==0) &&
        (hasRIP() ? (popt->flag & OFLAG_NORIP)==0
                  : (popt->flag & OFLAG_RIP)==0));

  /* Restore user's priv level */

  if (bi.use_barpriv)
  {
    usr.priv=biSave.priv;
    usr.xkeys=biSave.keys;
  }

  return rc;
}



/* The main menu handler */

int Display_Options(char *first_name, XMSG *msg)
{
  char name[PATHLEN];                   /* Name of current menu */
  char menu_name[PATHLEN];              /* Name of current menu, after P_O_C */
  struct _amenu menu;                   /* Current menu */
  char title[PATHLEN];
  char *title_temp;
  int same_menu, first_time, opt_rc;
  byte help, orig_help;                 /* Current help level */

  next_menu_char=-1;

  /* Initialize the current menu and get menu name */

  Initialize_Menu(&menu);

  strcpy(name, first_name);

  halt();

  /* Keep displaying until we are told to exit */

  do
  {
    /* We are just about to enter the named menu */

    ProcessMenuName(name, menu_name);
    EnterMenu(name, menu_name);

    Free_Menu(&menu);

    /* Read current menu into memory */

    if (Read_Menu(&menu, menu_name) != 0)
    {
      cant_open(menu_name);
      quit(2);
    }

    same_menu=first_time=TRUE;
    help=usr.help;

    do
    {
      int ran_opt;
      unsigned ch;

      /* Get and display the title of this menu */

      title_temp=menu.m.title ? MNU(menu, m.title) : name;
      Parse_Outside_Cmd(title_temp, title);

      if (nullptrcheck())
        Got_A_Null_Pointer(blank_str, menu_name);

      if (next_menu_char==-1)
      {
        menuhelp=help;
        ShowMenuHeader(&menu, help, first_time);
        ShowMenuBody(&menu, help, title, menu_name);
      }

      do
      {
        /* Get a keystroke from the user */

        if (next_menu_char == -1)
        {
          if (help == NOVICE && menu.cm_enabled && menu.cm_lightbar_menu && !menu.cm_skip_canned_menu &&
              menu.cm_x1 > 0 && menu.cm_y1 > 0 && menu.cm_x2 >= menu.cm_x1 && menu.cm_y2 >= menu.cm_y1)
          {
            int lbch = GetMenuResponseLightbarBounded(&menu, help, menu_name);
            if (lbch < 0)
              ch = GetMenuResponse(title);
            else
              ch = (unsigned)lbch;
          }
          else
            ch=GetMenuResponse(title);
        }
        else
        {
          ch=next_menu_char;
          next_menu_char=-1;
        }

        /* Save the user's current help level */

        orig_help=usr.help;

        opt_rc=ProcessMenuResponse(&menu, &same_menu, name, msg, &help,
                                   ch, &ran_opt, menu_name);

        /* If the user's help level changed, update it for this menu */

        if (usr.help != orig_help)
          help=usr.help;

        /* If we have to switch to a new menu due to the current msg/file   *
         * area, do so now.                                                 */

        ProcessMenuName(name, menu_name);

        if (EnterMenu(name, menu_name))
        {
          ran_opt=TRUE;
          same_menu=FALSE;
        }

        /* Don't display error messages for invalid f-keys or cursor        *
         * keys -- just ignore them.                                        */
      }
      while (!ran_opt && ch > 255 && opt_rc==0);

      first_time=FALSE;
    }
    while (same_menu && opt_rc==0);
  }
  while (opt_rc==0);

  Free_Menu(&menu);

  return opt_rc==-1 ? ABORT : SAVE;
}



