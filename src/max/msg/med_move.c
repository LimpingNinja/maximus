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
static char rcs_id[]="$Id: med_move.c,v 1.4 2004/01/28 06:38:10 paltas Exp $";
#pragma on(unreferenced)
#endif

/*# name=MaxEd editor: Routines for moving cursor around the screen
*/

#define MAX_LANG_m_area
#include "maxed.h"

static void near Up_a_Line(word *cx, word *cy);
static void near Down_a_Line(word *cx, word *cy);

void Cursor_Left(void)
{
  if (cursor_y > 1)
  {
    MagnEt_SpellClear();
    cursor_y--;
    GOTO_TEXT(cursor_x,cursor_y);
    Update_Position();
  }
}




void Cursor_Right(void)
{
  word max_col=MagnEt_LineVisibleLen(screen[offset+cursor_x])+1;

  if (max_col > usrwidth)
    max_col=usrwidth;

  if (cursor_y < max_col)
  {
    MagnEt_SpellClear();
    cursor_y++;
    GOTO_TEXT(cursor_x,cursor_y);
    Update_Position();
  }
}




void Cursor_Up(void)
{
  if (offset+cursor_x > 1)
  {
    MagnEt_SpellClear();
    if (cursor_x==1 && offset != 0)
      Scroll_Up(SCROLL_CASUAL,cursor_x+SCROLL_CASUAL);
    else cursor_x--;

    MagnEt_ClampCursor();
    GOTO_TEXT(cursor_x,cursor_y);
    Update_Position();
  }
}






void Cursor_Down(int update_pos)
{
  if (cursor_x+offset < num_lines)
  {
    MagnEt_SpellClear();
    if (cursor_x+1 >= usrlen)
      Scroll_Down(SCROLL_CASUAL,cursor_x-SCROLL_CASUAL);
    else cursor_x++;

    if (update_pos)
    {
      MagnEt_ClampCursor();
      GOTO_TEXT(cursor_x,cursor_y);
      Update_Position();
    }
  }
}




void Cursor_BeginLine(void)
{
  MagnEt_SpellClear();
  GOTO_TEXT(cursor_x,cursor_y=1);
  Update_Position();
}




void Cursor_EndLine(void)
{
  MagnEt_SpellClear();
  GOTO_TEXT(cursor_x,cursor_y=MagnEt_LineVisibleLen(screen[offset+cursor_x])+1);
  Update_Position();
}



void Word_Left(void)
{
  word cx=cursor_x;
  word cy=cursor_y;
  char ch;

  if (cy==1 && offset+cx != 1)
  {
    Up_a_Line(&cx,&cy);
  }

  if (cy > 1)
    cy--;

  while (cx+offset > 1 || cy > 1)
  {
    ch=MagnEt_VisibleCharAt(screen[offset+cx], cy);

    if (isalnumpunct(ch))
      break;

    if (cy > 1)
      cy--;
    else
      Up_a_Line(&cx,&cy);
  }

  while (cy > 1)
  {
    ch=MagnEt_VisibleCharAt(screen[offset+cx], cy-1);

    if (!isalnumpunct(ch))
      break;

    cy--;
  }

  if (cx != cursor_x || cy != cursor_y)
  {
    MagnEt_SpellClear();
    cursor_x=cx;
    cursor_y=cy;

    if ((sword)cursor_x <= 0 && offset != 0)
      Scroll_Up(SCROLL_CASUAL,cursor_x+SCROLL_CASUAL);

    GOTO_TEXT(cursor_x,cursor_y);
    Update_Position();
  }
}


void Word_Right(void)
{
  word cx=cursor_x;
  word cy=cursor_y;
  word max_col;
  char ch;

  for (;;)
  {
    max_col=MagnEt_LineVisibleLen(screen[offset+cx])+1;
    ch=MagnEt_VisibleCharAt(screen[offset+cx], cy);

    if (isalnumpunct(ch))
    {
      while (cy < max_col && isalnumpunct(MagnEt_VisibleCharAt(screen[offset+cx], cy)))
        cy++;
    }

    while (cy < max_col && !isalnumpunct(MagnEt_VisibleCharAt(screen[offset+cx], cy)) &&
           MagnEt_VisibleCharAt(screen[offset+cx], cy) != '\0')
      cy++;

    if (cy < max_col || cx+offset == num_lines)
      break;

    Down_a_Line(&cx,&cy);
  }

  if (cx != cursor_x || cy != cursor_y)
  {
    MagnEt_SpellClear();
    cursor_x=cx;
    cursor_y=cy;

    if (cursor_x >= usrlen)
      Scroll_Down(SCROLL_CASUAL,cursor_x-SCROLL_CASUAL);

    GOTO_TEXT(cursor_x,cursor_y);
    Update_Position();
  }
}




static void near Up_a_Line(word *cx, word *cy)
{
  if ((*cx)+offset != 1)
  {
    *cx=*cx-1;
    *cy=MagnEt_LineVisibleLen(screen[offset+(*cx)])+1;
  }
  else *cy=1;
}



static void near Down_a_Line(word *cx, word *cy)
{
  if (*cx+offset != num_lines)
  {
    *cx=*cx+1;
    *cy=1;
  }
  else *cy=MagnEt_LineVisibleLen(screen[offset+*cx])+1;
}



void Scroll_Up(int n,int location)
{
  MagnEt_SpellClear();
  if ((sword)offset-(sword)n >= 0)
    offset -= n;
  else offset=0;

  cursor_x=location;
  Redraw_Text();
}




void Scroll_Down(int n,int location)
{
  MagnEt_SpellClear();
  if ((sword)offset+n >= (sword)num_lines)
  {
    offset=num_lines;
    location=1;
  }
  else offset += n;

  cursor_x=location;
  Redraw_Text();
}





void Page_Up(void)
{
  MagnEt_SpellClear();
  if (offset==0)
  {
    GOTO_TEXT(cursor_x=1,cursor_y);
  }
  else Scroll_Up(usrlen-1,cursor_x);

  Update_Position();
}




void Page_Down(void)
{
  MagnEt_SpellClear();
  if (offset+usrlen > num_lines)
    GOTO_TEXT(cursor_x=num_lines-offset,cursor_y);
  else
  {
    offset += usrlen-1;

    if (offset > num_lines)
      offset=num_lines-1;

    if (offset+cursor_x > num_lines)
      cursor_x=num_lines-offset;

    Redraw_Text();
  }

  Update_Position();
}


