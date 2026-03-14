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
static char rcs_id[]="$Id: med_add.c,v 1.4 2004/01/28 06:38:10 paltas Exp $";
#pragma on(unreferenced)
#endif

/*# name=MaxEd editor: Routines for adding characters and lines to message
*/

#define MAX_LANG_global
#define MAX_LANG_m_area
#include "maxed.h"

static int near Insert_Line_Before_CR(int cx);
static void near Insert_At(word cx, word cy, char ch, word inc);


/* Make sure that line is filled with real cahracters up to current          *
 * cursor posn.                                                              */

static void near AddFill(void)
{
  char *line=screen[offset+cursor_x];
  word visible_len=MagnEt_LineVisibleLen(line);
  word rawpos=MagnEt_BufferPosFromVisibleCol(line, visible_len+1);

  while (cursor_y > visible_len+1)
  {
    strocpy(line+rawpos+1, line+rawpos);
    line[rawpos++]=' ';
    visible_len++;
  }
}


/* Add a character to the current line, handling wordwrap and parawrap */

void Add_Character(int ch)
{
  char *line;
  word rawpos;
  word cx, cy;
  word oldoffset;
  word col;

  AddFill();
  line=screen[offset+cursor_x];

  if (cursor_y==usrwidth)
  {
    if (MagnEt_LineHasColorCodes(line))
    {
      if (Carriage_Return(FALSE))
        return;
    }
    else
    {
      cy=cursor_y;
      col=Word_Wrap(MODE_SCROLL);            /* Scroll if necessary */
      cursor_y=(cy-col)+1;
    }
  }

  AddFill();
  line=screen[offset+cursor_x];
  rawpos=MagnEt_BufferPosFromVisibleCol(line, cursor_y);

  /* If insert mode is on, or we're in the last column */

  if (insert || line[rawpos]=='\0')
  {
    Insert_At(cursor_x,rawpos,(char)ch,1);

    if (!MagnEt_LineHasColorCodes(line) && strlen(screen[offset+cursor_x]+1) >= usrwidth)
    {
      cx=cursor_x;
      cy=cursor_y;
      oldoffset=offset;

      cursor_y=usrwidth;

      GOTO_TEXT(cx,cursor_y);

      col=Word_Wrap(MODE_UPDATE);
      
      if (cursor_x >= usrlen)
        Scroll_Down(SCROLL_LINES,cursor_x-SCROLL_LINES);

      if (cy > strlen(screen[oldoffset+cx]+1))
      {
        /*if (cy-x+1 >= 1 && cy-col+1 <= usrwidth)*/
          GOTO_TEXT(cursor_x, cursor_y=cy-col+1);
      }
      else
      {
        GOTO_TEXT(cursor_x=cx-(offset-oldoffset), cursor_y=cy);
      }
    }
  }
  else
  {
    line[rawpos]=(char)ch;
    cursor_y=MagnEt_VisibleColFromBufferPos(line, rawpos+1);
    Update_Line(offset+cursor_x,1,0,FALSE);
  }

  Update_Position();
}


/* Insert a character at a specific position and update screen display */

static void near Insert_At(word cx, word cy, char ch, word inc)
{
  strocpy(screen[offset+cx]+cy+1,
          screen[offset+cx]+cy);

  screen[offset+cx][cy]=ch;

  Update_Line(offset+cursor_x,1,0,FALSE);
  cursor_y=MagnEt_VisibleColFromBufferPos(screen[offset+cx], cy+inc);
}


/* Sends us to a new virtual line, no matter what!  Doesn't do             *
 * word-wrapping, splitting, etc.                                          */

void New_Line(int col)
{                         
  word x;

  if (cursor_x+offset+1 < max_lines)
  {
    cursor_x++;
    cursor_y=(col==0 ? 1 : col);

    if (cursor_x+offset > num_lines)
    {
      for (x=num_lines+1; x <= cursor_x+offset; x++)
      {
        if (Allocate_Line(x))
          EdMemOvfl();
      }
    }
  }
}



word Carriage_Return(int hard)
{
  word cx;
  word temp;
  word added_line;
  word line;
  word rawpos;
    
  byte save_cr;

  if (eqstri(screen[cursor_x]+1, "^z"))
  {
    screen[cursor_x][1]='\0';
    return TRUE;
  }

  added_line=FALSE;

  if (cursor_x+1 >= usrlen)
    Scroll_Down(SCROLL_LINES,cursor_x-SCROLL_LINES);

  cx=cursor_x+1;

  /* If we're on last line of message, allocate enuf lines to fill to end */
  
  if (cx+offset+1 >= num_lines && (num_lines < max_lines-1))
  {
    /* For each one until num_lines+1, allocate a line */
    
    for (line=num_lines+1; line <= cx+offset; line++)
    {
      added_line=TRUE;

      if (Allocate_Line(line))
        EdMemOvfl();
    }
  }

  temp=1;
  rawpos=MagnEt_BufferPosFromVisibleCol(screen[offset+cursor_x], cursor_y);

  /* If we need to insert a line in the middle, and there's enough room */
  
  if (insert && num_lines < max_lines-1)
  {
    save_cr=*screen[offset+cursor_x];
    
    /* If we're supposed to do a hard C/R, then do so */

    if (hard)
      *screen[offset+cursor_x]=HARD_CR;

    if (! (cx+offset > num_lines) && !added_line)
    {
      if (Allocate_Line(num_lines+1))
        EdMemOvfl();
    }

    if (cursor_y < usrwidth)
      Puts(CLEOL);

    if ((temp=Insert_Line_Before_CR(cx)) != 0)
    {
      /* If we need to split this line in two... */

      if (rawpos <= strlen(screen[offset+cursor_x]+1))
      {
        strocpy(screen[offset+cx]+1,
                screen[offset+cursor_x]+rawpos);
      }
      else screen[offset+cx][1]='\0'; /* else blank out the next line */

      /* Copy the attribute to the next line */

      *screen[offset+cx]=save_cr;

      /* Chop off this line, where we split it. */

      screen[offset+cursor_x][rawpos]='\0';

      Update_Line(offset+cx, 1, 0, FALSE);
    }
  }

  if (temp && offset+cursor_x != max_lines-1)
  {
    GOTO_TEXT(++cursor_x, cursor_y=1);

    Update_Position();
  }

  return FALSE;
}


static int near Insert_Line_Before_CR(int cx)
{
  word x;
  char *p;

  if (num_lines < max_lines)
  {
    if (screen[num_lines][1] != '\0')
    {
      if (Allocate_Line(num_lines+1))
        EdMemOvfl();
    }

    p=screen[num_lines];

    for (x=min(num_lines-1,max_lines-1); x >= offset+cx; x--)
    {
      screen[x+1]=screen[x];
      update_table[x+1]=TRUE;
    }

    screen[offset+cx]=p;

    return 1;
  }
  else
  {
    GOTO_TEXT(cursor_x,cursor_y=1);
    return 0;
  }
}



int Insert_Line_Before(int cx)
{
  word x;
  char *p;

  if (num_lines < max_lines-1)
  {
    if (screen[num_lines][1] != '\0')
    {
      if (Allocate_Line(num_lines+1))
        EdMemOvfl();
    }

    p=screen[num_lines];

    for (x=min(num_lines-1,max_lines-1); x >= offset+cx; x--)
    {
      screen[x+1]=screen[x];
      update_table[x+1]=TRUE;
    }

    screen[offset+cx]=p;

    return 1;
  }
  else
  {
    GOTO_TEXT(cursor_x,cursor_y=1);
    return 0;
  }
}


