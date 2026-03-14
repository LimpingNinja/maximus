/*
 * Maximus Version 3.02
 * Copyright 1989, 2002 by Lanius Corporation.  All rights reserved.
 * Modifications Copyright (C) 2025 Kevin Morgan (Limping Ninja)
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
static char rcs_id[]="$Id: med_scrn.c,v 1.4 2004/01/28 06:38:11 paltas Exp $";
#pragma on(unreferenced)
#endif

/*# name=MaxEd editor: Routines for manipulating the screen
*/

#define MAX_INCL_COMMS

#define MAX_LANG_global
#define MAX_LANG_m_area
#define MAX_LANG_max_bor
#include "maxed.h"
#include "mci.h"

static byte redraw_statusline_active=FALSE;

/**
 * @brief Collect the fixed positional parameter set for the editor header.
 *
 * The language template is free to omit any of these slots, but the call site
 * always binds all six so sparse `|!N`/`|#N` usage stays safe.
 */
static void near MagnEt_GetHeaderParams(char *area_buf, size_t area_sz,
                                        char *msgnum_buf, size_t msgnum_sz,
                                        char *from_buf, size_t from_sz,
                                        char *to_buf, size_t to_sz,
                                        char *subj_buf, size_t subj_sz,
                                        char *replyto_buf, size_t replyto_sz)
{
  const char *area_name;
  long replyto_msgnum=0;

  area_name=(mah.heap && *PMAS(&mah, name)) ? PMAS(&mah, name) : usr.msg;

  snprintf(area_buf, area_sz, "%s", area_name ? area_name : "");

  if (magnet_msgnum > 0)
    snprintf(msgnum_buf, msgnum_sz, "%ld", magnet_msgnum);
  else
  {
    long next_msgnum=MsgGetHighMsg(sq);
    snprintf(msgnum_buf, msgnum_sz, "%ld", UIDnum(next_msgnum) + 1L);
  }

  if (mmsg && mmsg->replyto)
  {
    int use_umsgids=ngcfg_get_bool("general.session.use_umsgids");
    replyto_msgnum=use_umsgids ? (long)mmsg->replyto
                               : MsgUidToMsgn(sq, mmsg->replyto, UID_EXACT);
  }

  if (replyto_msgnum > 0)
    snprintf(replyto_buf, replyto_sz, "%ld", replyto_msgnum);
  else
    *replyto_buf='\0';

  snprintf(from_buf, from_sz, "%s", (mmsg && mmsg->from) ? mmsg->from : "");
  snprintf(to_buf, to_sz, "%s", (mmsg && mmsg->to) ? mmsg->to : "");
  snprintf(subj_buf, subj_sz, "%s", (mmsg && mmsg->subj) ? mmsg->subj : "");
}


/**
 * @brief Count newline-delimited display rows in an expanded header buffer.
 */
static byte near MagnEt_HeaderExpandedHeight(const char *expanded)
{
  byte lines;

  if (expanded==NULL || *expanded=='\0')
    return 0;

  lines=1;

  while (*expanded)
  {
    if (*expanded=='\n')
      lines++;
    expanded++;
  }

  return lines;
}


/**
 * @brief Return the configured editor header height for the current session.
 */
byte MagnEt_CalcHeaderHeight(void)
{
  const char *tmpl;
  char expanded[1024];
  char area_buf[PATHLEN];
  char msgnum_buf[32];
  char from_buf[PATHLEN];
  char to_buf[PATHLEN];
  char subj_buf[PATHLEN];
  char replyto_buf[32];
  byte lines;

  tmpl=maxlang_get(g_current_lang, "editor.header");
  if (tmpl==NULL || *tmpl=='\0')
    return 0;

  MagnEt_GetHeaderParams(area_buf, sizeof(area_buf),
                         msgnum_buf, sizeof(msgnum_buf),
                         from_buf, sizeof(from_buf),
                         to_buf, sizeof(to_buf),
                         subj_buf, sizeof(subj_buf),
                         replyto_buf, sizeof(replyto_buf));

  LangSprintf(expanded, sizeof(expanded), tmpl,
              area_buf, msgnum_buf, from_buf, to_buf, subj_buf, replyto_buf);

  lines=MagnEt_HeaderExpandedHeight(expanded);

  if (lines > (byte)(TermLength() > 4 ? TermLength()-4 : 0))
    lines=(byte)(TermLength() > 4 ? TermLength()-4 : 0);

  return lines;
}


/**
 * @brief Draw the editor header above the text area.
 *
 * The entire layout is defined by the "editor.header" language template
 * in delta_english.toml using MCI codes for colors, formatting operators
 * ($c, $R, $L, $D, $X) for padding/centering, and terminal control codes
 * (|[<, |[>, |[H, |[C##, |[D##) for cursor repositioning within lines.
 *
 * This function simply expands the template with the six positional
 * parameters and prints each newline-delimited row at the correct
 * screen position.
 */
void MagnEt_DrawHeader(void)
{
  const char *tmpl;
  const char *line_start;
  const char *line_end;
  char area_buf[PATHLEN];
  char msgnum_buf[32];
  char from_buf[PATHLEN];
  char to_buf[PATHLEN];
  char subj_buf[PATHLEN];
  char replyto_buf[32];
  byte row;

  if (header_height==0)
    return;

  tmpl=maxlang_get(g_current_lang, "editor.header");
  if (tmpl==NULL || *tmpl=='\0')
    return;

  MagnEt_GetHeaderParams(area_buf, sizeof(area_buf),
                         msgnum_buf, sizeof(msgnum_buf),
                         from_buf, sizeof(from_buf),
                         to_buf, sizeof(to_buf),
                         subj_buf, sizeof(subj_buf),
                         replyto_buf, sizeof(replyto_buf));

  /* Expand the template with all six positional parameters and print
   * each newline-delimited row.  LangPrintfForce handles MCI expansion
   * including format ops, cursor codes, and pipe colors. */

  line_start=tmpl;

  for (row=1; row <= header_height && *line_start; row++)
  {
    char linebuf[512];
    size_t len;

    line_end=strchr(line_start, '\n');
    len=(size_t)(line_end ? (line_end - line_start) : strlen(line_start));

    if (len >= sizeof(linebuf))
      len=sizeof(linebuf) - 1;

    memcpy(linebuf, line_start, len);
    linebuf[len]='\0';

    Goto(row, 1);
    LangPrintfForce(linebuf,
                    area_buf, msgnum_buf, from_buf, to_buf, subj_buf, replyto_buf);

    if (!line_end)
      break;

    line_start=line_end + 1;
  }

  EMIT_MSG_TEXT_COL();
}


static word near MagnEt_ColorCodeLenAt(const char *p)
{
  int code;

  if (p==NULL || p[0] != '|' || !isdigit((unsigned char)p[1]) || !isdigit((unsigned char)p[2]))
    return 0;

  code=((int)(p[1]-'0') * 10) + (int)(p[2]-'0');
  return (code >= 0 && code <= 31) ? 3 : 0;
}

static void near MagnEt_EmitAttr(byte attr)
{
  Putc('\x16');
  Putc('\x01');
  Putc((int)attr);
}

static void near MagnEt_RenderEditorLine(const char *line)
{
  word pos;
  word cc_len;
  char token[4];
  byte attr=Mci2Attr(msg_text_col, 0x07);

  if (line==NULL)
    return;

  pos=1;

  while (line[pos])
  {
    cc_len=MagnEt_ColorCodeLenAt(line+pos);

    if (cc_len)
    {
      token[0]=line[pos];
      token[1]=line[pos+1];
      token[2]=line[pos+2];
      token[3]='\0';
      attr=Mci2Attr(token, attr);
      MagnEt_EmitAttr(attr);
      pos += cc_len;
      continue;
    }

    Putc(line[pos]);
    pos++;
  }
}


word MagnEt_LineVisibleLen(const char *line)
{
  word pos;
  word len;
  word cc_len;

  if (line==NULL)
    return 0;

  pos=1;
  len=0;

  while (line[pos])
  {
    cc_len=MagnEt_ColorCodeLenAt(line+pos);

    if (cc_len)
    {
      pos += cc_len;
      continue;
    }

    pos++;
    len++;
  }

  return len;
}


word MagnEt_BufferPosFromVisibleCol(const char *line, word col)
{
  word pos;
  word visible;
  word cc_len;

  if (line==NULL)
    return 1;

  if (col < 1)
    col=1;

  pos=1;
  visible=1;

  while (line[pos] && visible < col)
  {
    cc_len=MagnEt_ColorCodeLenAt(line+pos);

    if (cc_len)
    {
      pos += cc_len;
      continue;
    }

    pos++;
    visible++;
  }

  while ((cc_len=MagnEt_ColorCodeLenAt(line+pos)) != 0)
    pos += cc_len;

  return pos;
}


word MagnEt_VisibleColFromBufferPos(const char *line, word pos)
{
  word cur_pos;
  word col;
  word cc_len;

  if (line==NULL || pos <= 1)
    return 1;

  cur_pos=1;
  col=1;

  while (line[cur_pos] && cur_pos < pos)
  {
    cc_len=MagnEt_ColorCodeLenAt(line+cur_pos);

    if (cc_len)
    {
      cur_pos += cc_len;
      continue;
    }

    cur_pos++;
    col++;
  }

  return col;
}


char MagnEt_VisibleCharAt(const char *line, word col)
{
  word pos;

  if (line==NULL || col < 1 || col > MagnEt_LineVisibleLen(line))
    return '\0';

  pos=MagnEt_BufferPosFromVisibleCol(line, col);
  return line[pos];
}


int MagnEt_LineHasColorCodes(const char *line)
{
  word pos;

  if (line==NULL)
    return FALSE;

  for (pos=1; line[pos]; pos++)
    if (MagnEt_ColorCodeLenAt(line+pos))
      return TRUE;

  return FALSE;
}


void MagnEt_ClampCursor(void)
{
  word max_col;

  if (offset+cursor_x > num_lines || screen[offset+cursor_x]==NULL)
  {
    if (cursor_y < 1)
      cursor_y=1;
    return;
  }

  max_col=MagnEt_LineVisibleLen(screen[offset+cursor_x])+1;

  if (max_col > usrwidth)
    max_col=usrwidth;

  if (cursor_y < 1)
    cursor_y=1;
  else if (cursor_y > max_col)
    cursor_y=max_col;
}

void MagnEt_DrawFooterDivider(void)
{
  int col;

  Goto(MAGNET_DIVIDER_ROW, 1);
  PutsForce("|br");

  for (col=0; col < usrwidth; col++)
    Putc('\xC4');

  Puts(CLEOL);
}


void MagnEt_ClearContextLine(void)
{
  Goto(MAGNET_CONTEXT_ROW, 1);
  Puts(CLEOL);
}

void Redraw_Text(void)
{
  word x;

  /* Make the entire refresh buffer "dirty" */
  
  EMIT_MSG_TEXT_COL();

  if (update_table)
    for (x=0; x < UPDATEBUF_LEN; x++)
      update_table[x]=TRUE;
}



void Redraw_StatusLine(void)
{
  char linebuf[8];
  char colbuf[8];

  redraw_statusline_active=TRUE;
  MagnEt_ClampCursor();

  snprintf(linebuf, sizeof(linebuf), "%u", offset + cursor_x);
  snprintf(colbuf, sizeof(colbuf), "%u", cursor_y);

  Goto(MAGNET_STATUS_ROW, 1);

  LangPrintfForce(max_status,
             mmsg->to,
             mmsg->subj+(strnicmp(mmsg->subj, "Re:", 3)==0 ? 4 : 0),
             linebuf,
             colbuf);

  if (insert)
  {
    Goto(MAGNET_STATUS_ROW, usrwidth-13);

    PutsForce(status_insert);
  }

  Update_Position();

  Puts(CLEOL);
  EMIT_MSG_TEXT_COL();

  GOTO_TEXT(cursor_x, cursor_y);
  redraw_statusline_active=FALSE;
}


void Redraw_Quote(void)
{
  /* Stub — popup quote window (med_qpop.c) handles its own rendering. */
}



void Update_Line(word cx, word cy, word inc, word update_cursor)
{
  word display_row;

  /* Only update lines which are on-screen! */

  if (cx > offset && cx < offset+usrlen)
  {
    if (!Mdm_keyp() || cx==cursor_x+offset)
    {
      display_row=cx-offset;

      if (cx != max_lines)
      {
        GOTO_TEXT(display_row,1);
        EMIT_MSG_TEXT_COL();

        if (cx > max_lines || screen[cx]==NULL)
        {
          Puts(CLEOL);
        }
        else
        {
          MagnEt_RenderEditorLine(screen[cx]);
          Puts(CLEOL);
        }
      }
      else
      {
        GOTO_TEXT(display_row,1);
        LangPrintfForce(end_widget, msg_text_col);
        Puts(CLEOL);
      }

      if (update_cursor)
        GOTO_TEXT(display_row,cy+inc);

      if ((word)update_table[cx] >= cy)
        update_table[cx]=FALSE;
    }
    else update_table[cx]=TRUE;
  }
}


/* Ultra-ugly wordwrap routine (but it works!) */

int Word_Wrap(word mode)
{
  word cx, cy, y;
  byte *scx, *lastpos;
  word zerolen;
  word col;

  
  /* Get over any ending whitespace on this line */
  
  scx=screen[offset+cursor_x];

  for (col=usrwidth; is_wd(scx[col]); col--)
    ;

  /* Now find the beginning of the word */

  for (; col >= (word)(usrwidth-MAX_WRAPLEN); col--)
  {
    if (is_wd(scx[col]) || col==(word)(usrwidth-MAX_WRAPLEN))
    {
      /* Found it! */
      
      /* 'col' used to point to the whitespace; we want it to point to the    *
       * start of the word.                                                 */
      
      col++;

      /* Put the cursor here, unless we're told not to move it */
      
      if (mode != MODE_NOUPDATE)
        GOTO_TEXT(cursor_x, cursor_y-(usrwidth-col));

      /* Update virtual cursor position to reflect placement of new word */
      
      if ((cursor_y -= (usrwidth-col)) < 1)
        cursor_y=1;

      cx=cursor_x+1;

      /* If we need to allocate any more lines at the end, then do so. */
      
      if (offset+cx > num_lines && offset+cx < max_lines)
      {
        for (y=num_lines+1; y <= cx+offset; y++)
        {
          if (Allocate_Line(y))
            EdMemOvfl();
        }
      }
      else if (offset+cx==max_lines || screen[offset+cursor_x][0]==HARD_CR)
      {
        /* else we're trying to insert a line in the middle of the text,   *
         * so plop it in the right spot.                                   */
        
        if (! Insert_Line_Before(cx)) /* If we can't insert (end of text?) */
        {
          screen[offset+cx][col]='\0';
          Update_Line(offset+cx,1,0,TRUE);
          return col;
        }
      }
      
      scx=screen[offset+cursor_x];
      
      lastpos=scx+strlen(scx)-1;
      
      /* Make sure that there's a space between words when wrapping */
      
      if (! is_wd(*lastpos) && scx[0]==SOFT_CR)
      {
        *++lastpos=' ';
        *++lastpos='\0';
      }

      /* Now state that we've wordwrapped */

      scx[0]=SOFT_CR;

      /* If there's something on the next line to wrap... */

      if (screen[offset+cx][1])
      {
        if (strlen(scx+col) >= usrwidth)
          *(scx+col+usrwidth)='\0';

        /* Move line over, to make room for new word */

        strocpy(screen[offset+cx]+1 + strlen(scx+col),
                screen[offset+cx]+1);
              
        /* Put new word in place */

        memmove(screen[offset+cx]+1,
                scx+col,
                y=strlen(scx+col));

        /* Remove the first word from the prior line */

        scx[col]='\0';

        /* Display that on-screen */

        if (cursor_y < usrwidth && mode != MODE_NOUPDATE)
          Puts(CLEOL);

        /* If this causes a scroll, then do so */

        if ((mode==MODE_SCROLL /*|| mode==MODE_UPDATE*/) &&
            (cursor_x+1 >= usrlen))
          Scroll_Down(SCROLL_LINES,cursor_x-SCROLL_LINES);

        /* If the word we just moved was zero-length */

        if (y==0 || (col <= (word)strlen(screen[offset+cursor_x]+1)+1 &&
                     ! is_wd(screen[offset+cursor_x][col]) &&
                     screen[offset+cx][1]))
        {
          zerolen=TRUE;
        }
        else zerolen=FALSE;

        /* Go to a new line. */

        New_Line(y);
      }
      else /* else there's nothing on the next line, so just copy */
      {
        memmove(screen[offset+cx]+1,
                scx+col,
                y=strlen(scx+col)+1);

        /* Get rid of the original word */

        scx[col]='\0';

        /* ...and show the deletion on-screen. */

        if (cursor_y < usrwidth)
          Puts(CLEOL);

        /* Scroll, if necessary */
        
        if ((mode==MODE_SCROLL /*|| mode==MODE_UPDATE*/) &&
            (cursor_x+1 >= usrlen))
        {
          Scroll_Down(SCROLL_LINES,cursor_x-SCROLL_LINES);
          cx=cursor_x;
        }

        /* Go to a new line */

        New_Line(--y);

        /* ...and set a flag, based on the length of this word */

        if (y==0)
          zerolen=TRUE;
        else zerolen=FALSE;
      }

      /* If this has caused the *current* line to be in need of a wrap... */
      if (strlen(screen[offset+cursor_x]+1) >= usrwidth)
      {
        /* Dummy up the cursor position accordingly */

        cy=cursor_y;
        cx=cursor_x;
        cursor_y=usrwidth;

        /* And make a recursive call to ourselves */

        Word_Wrap(MODE_NOUPDATE);

        /* Fix cursor position back to where we were */

        cursor_x=cx;
        cursor_y=cy+1;
      }
      
      /* Put cursor in final spot */

      GOTO_TEXT(cx,cursor_y);

      /* Indicate that this line was updated */
      update_table[offset+cursor_x]=TRUE;

      /* And move cursor appropriately. */
      if (! zerolen)
        cursor_y++;
      break;
    }
  }

  /* If the word was the maximum length allowable */
  
  #ifdef NEVER
  if (col==usrwidth-MAX_WRAPLEN)
  {
    /* Chop off anything */
    col=cursor_y;
    screen[offset+cursor_x][usrwidth-1]='\0';
    
    if (cursor_y==usrwidth)
      Carriage_Return(FALSE);
    else
    {
      screen[offset+cursor_x][usrwidth-2]='\0';
      Update_Line(offset+cursor_x, cursor_y, 0, FALSE);
    }
  }
  #endif

  /* If this is the last recursive call, then update our position */
  if (mode==MODE_UPDATE || mode==MODE_SCROLL)
    Update_Position();

  return col;
}




void Toggle_Insert(void)
{
  insert=(char)!insert;

  Goto(MAGNET_STATUS_ROW,usrwidth-13);

  PutsForce(insert ? status_insert : insrt_ovrwrt);

  MagnEt_ClampCursor();
  GOTO_TEXT(cursor_x, cursor_y);
  EMIT_MSG_TEXT_COL();
}





void Update_Position(void)
{
  MagnEt_ClampCursor();

  if (! Mdm_keyp() && !skip_update)
  {
    pos_to_be_updated=FALSE;

    if (!redraw_statusline_active)
      Redraw_StatusLine();
  }
  else pos_to_be_updated=TRUE;

#ifdef OS_2
#ifdef TTYVIDEO
  if (displaymode==VIDEO_IBM)
#endif
    Vidfcur();
#endif
}


void NoFF_CLS(void)
{
  Goto(1,1);

  Printf(CYAN "\x16\x19\x03" CLEOL "\n%c", TermLength());

  Goto(1,1);
}




void Fix_MagnEt(void)
{
  PutsForce(maxed_init);

  if (!(usr.bits2 & BITS2_CLS))
    NoFF_CLS();

  MagnEt_DrawHeader();
  Redraw_Text();
  MagnEt_DrawFooterDivider();
  MagnEt_ClearContextLine();
  Redraw_StatusLine();
  Redraw_Quote();

  (void)mdm_ctrlc(0);
  Mdm_Flow_Off();
}




void Do_Update(void)
{
  static word lastofs=-1;
  word x, y;
  word updated;
  word last_update=FALSE;

  updated=FALSE;

  y=offset+usrlen;

  for (x=offset; x < y && ! Mdm_keyp(); x++)
  {
    /* If it's in the update buffer, update it in the normal manner */

    if (x < UPDATEBUF_LEN)
    {
      if (update_table[x])
      {
        if (updated && offset+cursor_x==x)
          GOTO_TEXT(cursor_x,cursor_y);

        Update_Line(x, 1, 0, FALSE);
        updated=TRUE;
        last_update=TRUE;
      }
      else if (x != max_lines)
        last_update=FALSE;
    }
    else
    {
      /* Otherwise, it's not on the update buffer, so just clear it, if   *
       * the last line on-screen WAS updated.                             */

      if (last_update || offset != lastofs)
      {
        GOTO_TEXT(x-offset, 1);
        Puts(CLEOL);
      }
    }
  }

  lastofs=offset;

  if (updated)
  {
    GOTO_TEXT(cursor_x, cursor_y);
    vbuf_flush();
  }
}
