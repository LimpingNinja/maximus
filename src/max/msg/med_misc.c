/*
 * Maximus Version 3.02
 * Copyright 1989, 2002 by Lanius Corporation.  All rights reserved.
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
static char rcs_id[]="$Id: med_misc.c,v 1.4 2004/01/28 06:38:10 paltas Exp $";
#pragma on(unreferenced)
#endif

/*# name=MaxEd editor: Miscellaneous routines (overlaid)
*/

#define MAX_INCL_COMMS

#define MAX_LANG_global
#define MAX_LANG_m_area
#define MAX_LANG_max_bor
#include "maxed.h"
#include <stddef.h>
#include "keys.h"
#include "ui_lightbar.h"
#include "ui_field.h"
#include "mci.h"


/*static char * pig(char *s,char *temp);*/

static int near MagnEt_PreviewBoxLineCount(const char *s)
{
  int lines=0;

  if (s==NULL || *s=='\0')
    return 0;

  lines=1;

  while (*s)
  {
    if (*s=='\n')
      lines++;
    s++;
  }

  return lines;
}


static int near MagnEt_PreviewBoxMaxWidth(const char *s)
{
  int width=0;
  int max_width=0;

  if (s==NULL)
    return 0;

  while (*s)
  {
    if (*s=='\n')
    {
      if (width > max_width)
        max_width=width;

      width=0;
      s++;
      continue;
    }

    if (*s=='|' && s[1])
    {
      if (s[1]=='|')
      {
        width++;
        s += 2;
        continue;
      }

      if (isdigit((unsigned char)s[1]) && isdigit((unsigned char)s[2]))
      {
        s += 3;
        continue;
      }

      if (s[2])
      {
        s += 3;
        continue;
      }
    }

    width++;
    s++;
  }

  if (width > max_width)
    max_width=width;

  return max_width;
}


static void near MagnEt_DrawPreviewBox(const char *box)
{
  char linebuf[256];
  const char *line_start;
  const char *line_end;
  int box_lines;
  int box_width;
  int row;
  int col;

  if (box==NULL || *box=='\0')
    return;

  box_lines=MagnEt_PreviewBoxLineCount(box);
  box_width=MagnEt_PreviewBoxMaxWidth(box);

  if (box_lines <= 0 || box_width <= 0)
    return;

  row=((int)usrlen - box_lines) / 2 + 1;
  if (row + box_lines - 1 > usrlen)
    row=usrlen - box_lines + 1;

  if (row < 1)
    row=1;

  col=((int)usrwidth - box_width) / 2 + 1;
  if (col < 1)
    col=1;

  line_start=box;

  while (*line_start && row <= usrlen)
  {
    size_t len;

    line_end=strchr(line_start, '\n');
    len=(size_t)(line_end ? (line_end-line_start) : strlen(line_start));

    if (len >= sizeof(linebuf))
      len=sizeof(linebuf)-1;

    memcpy(linebuf, line_start, len);
    linebuf[len]='\0';

    GOTO_TEXT(row, col);
    PutsForce(linebuf);

    row++;

    if (!line_end)
      break;

    line_start=line_end+1;
  }
}


void MagnEt_Help(void)
{
  Mdm_Flow_On();
  display_line=display_col=1;
  Display_File(0, NULL, ngcfg_get_path("general.display_files.oped_help"));
  Fix_MagnEt();
}




void MagnEt_Menu(void)
{
  if (usr.bits2 & BITS2_CLS)
    Puts(CLS);
  else NoFF_CLS();

  Mdm_Flow_On();
  Bored_Menu(mmsg);
  Fix_MagnEt();
}


int MagnEt_ContextYesNo(const char *prompt)
{
  static const char *options[] = {
    "[Y]es",
    "[N]o"
  };
  int flags;
  int selected;

  flags = UI_SP_FLAG_STRIP_BRACKETS;
  flags |= ((int)Mci2Attr("|hk", 0x07) << UI_SP_HOTKEY_ATTR_SHIFT);
  flags |= (2 << UI_SP_DEFAULT_SHIFT);

  Goto(MAGNET_CONTEXT_ROW, 1);
  Puts(CLEOL);
  PutsForce((char *)(prompt ? prompt : ""));

  selected = ui_select_prompt(
    "",
    options,
    2,
    Mci2Attr("|pr", 0x07),
    Mci2Attr("|tx", 0x07),
    Mci2Attr("|lf|lb", 0x07),
    flags,
    1,
    " ",
    NULL);

  MagnEt_ClearContextLine();
  GOTO_TEXT(cursor_x, cursor_y);
  EMIT_MSG_TEXT_COL();
  vbuf_flush();

  return (selected == 0) ? YES : NO;
}


int MagnEt_EscMenu(struct _replyp *pr)
{
  static const char *options[] = {
    "[S]ave",
    "[A]bort",
    "[H]elp",
    "[Q]uote",
    "[C]olor",
    "[R]edraw"
  };
  int flags;
  int selected;

  (void)pr;

  flags = UI_SP_FLAG_STRIP_BRACKETS;
  flags |= ((int)Mci2Attr("|hk", 0x07) << UI_SP_HOTKEY_ATTR_SHIFT);

  Goto(MAGNET_CONTEXT_ROW, 1);
  Puts(CLEOL);

  selected = ui_select_prompt(
    "",
    options,
    6,
    Mci2Attr("|pr", 0x07),
    Mci2Attr("|tx", 0x07),
    Mci2Attr("|lf|lb", 0x07),
    flags,
    1,
    " ",
    NULL);

  MagnEt_ClearContextLine();
  Redraw_StatusLine();
  GOTO_TEXT(cursor_x, cursor_y);
  EMIT_MSG_TEXT_COL();
  vbuf_flush();

  switch (selected)
  {
    case 0: return MAGNET_ESC_SAVE;
    case 1: return MAGNET_ESC_ABORT;
    case 2: return MAGNET_ESC_HELP;
    case 3: return MAGNET_ESC_QUOTE;
    case 4: return MAGNET_ESC_COLOR;
    case 5: return MAGNET_ESC_REDRAW;
    default: return MAGNET_ESC_NONE;
  }
}


int MagnEt_InsertColor(void)
{
  char digits[3];
  char codebuf[4];
  const char *prompt;
  const char *preview;
  const char *preview_box;
  int len;
  int ch;
  int num;
  int i;
  int input_col;
  int sample_col;

  digits[0]='\0';
  len=0;
  input_col=21;
  sample_col=28;
  prompt=maxlang_get(g_current_lang, "max_bor.color_selector_prompt");
  preview=maxlang_get(g_current_lang, "max_bor.color_selector_preview");
  preview_box=maxlang_get(g_current_lang, "max_bor.color_selector_preview_box");

  for (;;)
  {
    MagnEt_DrawPreviewBox(preview_box);

    Goto(MAGNET_CONTEXT_ROW, 1);
    Puts(CLEOL);
    PutsForce((char *)(prompt ? prompt : ""));

    for (i=0; i < len; i++)
      Putc(digits[i]);

    if (len > 0)
    {
      num=digits[0]-'0';

      if (len==2)
        num=(num * 10) + (digits[1]-'0');

      if (num >= 0 && num <= 23)
      {
        snprintf(codebuf, sizeof(codebuf), "|%02d", num);
        Goto(MAGNET_CONTEXT_ROW, sample_col);
        if (num >= 17)
        {
          PutsForce("|pr|tbPreview: ");
          PutsForce(codebuf);
          Puts("  ");
          PutsForce("|cdSample|pr|tb");
        }
        else
        {
          LangPrintfForce((char *)(preview ? preview : ""), codebuf);
        }
      }
    }

    Goto(MAGNET_CONTEXT_ROW, input_col + len);
    vbuf_flush();

    ch=ui_read_key();

    if (ch==K_ESC)
      break;

    if (ch==K_CR)
    {
      if (len > 0)
      {
        num=digits[0]-'0';

        if (len==2)
          num=(num * 10) + (digits[1]-'0');

        if (num >= 0 && num <= 23)
        {
          snprintf(codebuf, sizeof(codebuf), "|%02d", num);

          for (i=0; codebuf[i]; i++)
            Add_Character(codebuf[i]);

          MagnEt_ClearContextLine();
          Redraw_Text();
          Redraw_StatusLine();
          GOTO_TEXT(cursor_x,cursor_y);
          EMIT_MSG_TEXT_COL();
          vbuf_flush();
          return TRUE;
        }
      }

      Putc('\a');
      continue;
    }

    if ((ch==K_BS || ch==8 || ch==127) && len > 0)
    {
      len--;
      digits[len]='\0';
      continue;
    }

    if (ch >= '0' && ch <= '9' && len < 2)
    {
      digits[len++]=(char)ch;
      digits[len]='\0';
      continue;
    }

    Putc('\a');
  }

  MagnEt_ClearContextLine();
  Redraw_Text();
  Redraw_StatusLine();
  GOTO_TEXT(cursor_x,cursor_y);
  EMIT_MSG_TEXT_COL();
  vbuf_flush();
  return FALSE;
}



#ifdef NEVER /* can cause system crashes */

void Piggy(void)
{
  word x, ofs, cx, cy;

  byte temp[MAX_LINELEN*2];
  byte temp2[PATHLEN];
  byte *o, *s, *p, *l;

  if (no_piggy)
    return;

  ofs=offset;
  cx=cursor_x;
  cy=cursor_y;

  for (x=1; x <= num_lines; x++)
  {
    for (s=screen[x]+1; *s; s++)
    {
      if (isalpha(*s))
      {
        o=s;

        p=temp;

        while (isalpha(*s) || *s=='\'')
          *p++=*s++;

        *p='\0';

        /* Make room for "ay" */

        strocpy(s+2, s);

        l=pig(temp, temp2);
        memmove(o, l, strlen(l));
        s++;

        if (s-(screen[x]+1)+strlen(l) >= (ptrdiff_t)usrwidth)
        {
          offset=x-1;
          cursor_x=1;
          cursor_y=strlen(screen[x]+1)+1;

          Word_Wrap(MODE_NOUPDATE);            /* Scroll if necessary */

          x=offset+cursor_x;
          s=screen[x];    /* the "s++" incs this later */

          if (x > num_lines || x > max_lines)
            break;
        }
      }
    }
  }

  offset=ofs;
  cursor_x=cx;
  cursor_y=cy;

  Redraw_Text();
}



static char * pig(char *s,char *temp)
{
  char *p;

  strcpy(temp,s+1);
  p=temp+strlen(temp);

  if (isupper(*s))
    *p++=(char)tolower(*s);
  else *p++=*s;

  strcpy(p,"ay");

  if (isupper(*s))
    *temp=(char)toupper(*temp);

  return temp;
}

#endif /* NEVER */



void MagnEt_Bad_Keystroke(void)
{
  Goto(MAGNET_CONTEXT_ROW,1);

  LangPrintfForce(max_no_understand,ck_for_help);

  GOTO_TEXT(cursor_x,cursor_y);
  vbuf_flush();

  if (Mdm_getcwcc()==0)
    Mdm_getcwcc();

  /*
  while (! Mdm_keyp() && !brk_trapped)
    Giveaway_Slice();
  */

  MagnEt_ClearContextLine();
  GOTO_TEXT(cursor_x,cursor_y);
  EMIT_MSG_TEXT_COL();
  vbuf_flush();
}



