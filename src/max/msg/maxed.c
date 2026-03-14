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
static char rcs_id[]="$Id: maxed.c,v 1.4 2004/01/28 06:38:10 paltas Exp $";
#pragma on(unreferenced)
#endif

/*# name=Full-screen editor
*/

#define INIT_MAXED
#define MAX_INCL_COMMS

#define MAX_LANG_global
#define MAX_LANG_m_area
#define MAX_LANG_max_bor
#include "maxed.h"
#include "keys.h"
#include "m_reply.h"
#include "mci.h"

static word near Process_Scan_Code(struct _replyp *pr);
static word near Process_Cursor_Key(struct _replyp *pr);
static word near Process_Control_K(struct _replyp *pr);
static void near Init_Vars(void);


#if 1
/* Get a character and parse local ^c's as real chars */

int Mdm_getcwcc(void)
{
  int timer2;

  /* Convert any local ^Cs or ^breaks to ASCII 3, which is what we *
   * want.                                                         */

  timer2=FALSE;
  input_timeout=timerset(timeout_tics);

  while (! Mdm_keyp() && !brk_trapped)
  {
    Check_Time_Limit(&input_timeout, &timer2);
    Check_For_Message(NULL, NULL);
    Giveaway_Slice();
  }

  if (brk_trapped)
  {
    brk_trapped=0;
    return '\x03';
  }

  return (Mdm_getcw());
}
#else

int Mdm_getcwcc(void)
{
  while (Mdm_kpeek_tic(timeout_tics)==-1 && !brk_trapped)
    ;  /* Check_Time_Limit/Check_For_Message/Giveaway_Slice already done */
  if (brk_trapped)
  {
    brk_trapped=0;
    return '\x03';
  }
  return Mdm_getcw();
}
#endif

int MagnEt(XMSG *msg,HMSG msgh,long msgnum,struct _replyp *pr)
{
  long now_cl;

  sword ret;
  sword ch;
  sword state;
  word redo_status;

  char break_loop;

  mmsg=msg;
  magnet_msgnum=msgnum;
  state=0;

  if ((update_table=malloc(UPDATEBUF_LEN))==NULL)
    return ABORT;

  MciPushParseFlags(MCI_PARSE_ALL, 0);

  now_cl=coreleft();

  max_lines = (now_cl-MAXED_SAVE_MEM) / (long)MAX_LINELEN;

  if ((sword)max_lines < 0 || (sword)max_lines > MAX_LINES)
    max_lines = MAX_LINES;

  if (max_lines==0)   /* Something's really wrong here! */
  {
    logit(mem_nmsgb);
    PutsForce(mem_nmsgb);

    Press_ENTER();
  }
  else                /* Things look A-OK! */
  {
    /* Allow high-bit chars while in the editor */

    in_msghibit++;

    /* Allocate the last line, for our spiffy '-end-' widget. */

    if (Allocate_Line(max_lines))
      EdMemOvfl();

    /* Turn off modem flow controls, so we can interpret characters such  *
     * as control-C, control-S, etc.                                      */

    (void)mdm_ctrlc(0);
    Mdm_Flow_Off();

    update_table[max_lines]=TRUE;

    Init_Vars();


    if (!(usr.bits2 & BITS2_CLS))
      NoFF_CLS();

    if (Allocate_Line(offset+1))
      EdMemOvfl();

    break_loop=FALSE;

    PutsForce(maxed_init);
    MagnEt_DrawHeader();
    if (msgh)
    {
      Load_Message(msgh);
      Redraw_Text();
      Do_Update();
    }

    redo_status=FALSE;

    MagnEt_DrawFooterDivider();
    MagnEt_ClearContextLine();
    Redraw_StatusLine();
    MagnEt_SpellInit();

    if (usr.help==NOVICE)
    {
      redo_status=TRUE;

      Goto(MAGNET_CONTEXT_ROW, 1);
      PutsForce(ck_for_help);
      Puts(CLEOL);
      EMIT_MSG_TEXT_COL();
      GOTO_TEXT(cursor_x, cursor_y);
    }

    if (setjmp(jumpto)==0) /* Really ugly, but the best way to handle errs */
    {
      for (;;)
      {
        /* This loop is to update lines on the screen.  If the user wants  *
         * to do a few page-down's in a row or something, we don't want to *
         * do a FULL page-down if there is another character waiting,      *
         * which usually means the user wants something NOW.               */

        while (!break_loop)
        {
          screen[max_lines][0]=SOFT_CR;
          screen[max_lines][1]='\0';

          /* So we'll display the -end- whenever we get a chance.  Since   *
           * we don't want the '-end' continiously displaying, we only set *
           * the update table to restore the line if it isn't on-screen,   *
           * so it DOES get displayed once we DO come back on-screen.      *
           * (Confused yet?)                                               */

          if (! (offset+cursor_x >= max_lines-usrlen))
            update_table[max_lines]=TRUE;

          /* Stick the cursor in the right spot */
          if (!Mdm_keyp() && pos_to_be_updated)
            Update_Position();

          vbuf_flush();

          Do_Update();

          ch=Mdm_getcwcc();
          
          /* Redraw the status line for the user's help msg */

          if (redo_status)
          {
            MagnEt_ClearContextLine();
            GOTO_TEXT(cursor_x,cursor_y);
            EMIT_MSG_TEXT_COL();
            redo_status=FALSE;
          }

          switch (ch)
          {
            case 0:    /* Scan key! */
              switch (Process_Scan_Code(pr))
              {
                case SAVE:
                  ret=SAVE;
                  goto BackToCaller;

                case ABORT:
                  break_loop=TRUE;
                  break;
              }
              break;

            case K_CTRLA:
              MagnEt_SpellClear();
              Word_Left();
              break;

            case K_CTRLC:
              MagnEt_SpellClear();
              Page_Down();
              break;

            case K_CTRLD:
              MagnEt_SpellClear();
              Cursor_Right();
              break;

            case K_CTRLE:
              MagnEt_SpellClear();
              Cursor_Up();
              break;

            case K_CTRLF:
              MagnEt_SpellClear();
              Word_Right();
              break;

            case K_CTRLG:    /* Delete character */
              MagnEt_SpellClear();
              Delete_Char();
              break;

#ifndef UNIX
            case K_VTDEL:    /* VT 100 delete */

              /* Delete char under cusor if user has ibmchars turned on */

              if (usr.bits2 & BITS2_IBMCHARS)
              {
                MagnEt_SpellClear();
                Delete_Char();
                break;
              }
              /* else fall-through */
#endif

            case K_BS:    /* Backspace */
            case K_VTDEL: /* Also treat DEL-as-Backspace on UNIX terminals */
              MagnEt_SpellClear();
              BackSpace();
              break;

            case K_TAB:    /* Tab */
            {
              word chrs;
              
              skip_update=TRUE;

              for (chrs=8 - ((cursor_y-1) % 8); chrs--; )
              {
                if (insert)
                  Add_Character(' ');
                else Cursor_Right();
              }

              skip_update=FALSE;
              break;
            }

            case K_CTRLK:
              switch (Process_Control_K(pr))
              {
                case SAVE:
                  ret=SAVE;
                  goto BackToCaller;

                case ABORT:
                  break_loop=TRUE;
                  break;
              }
              break;

            case K_CTRLL:
              update_table[offset+cursor_x]=TRUE;
              break;

            case K_CR:    /* Return */
              MagnEt_SpellCheckCurrentWord(FALSE);
              if (Carriage_Return(TRUE))
              {
                ret=SAVE;
                goto BackToCaller;
              }
              break;

            case K_CTRLN:
            case K_CTRLO:
              MagnEt_Help();
              break;

            case K_CTRLQ:
              MagnEt_SpellClear();
              Quote_Popup(pr);
              Fix_MagnEt();
              break;

            case K_CTRLR:
              MagnEt_SpellClear();
              Page_Up();
              break;

            case K_CTRLS:
              MagnEt_SpellClear();
              Cursor_Left();
              break;

            case K_CTRLT:
              MagnEt_SpellClear();
              Delete_Word(); 
              break;

            case K_CTRLV:
              Toggle_Insert();
              break;

            case K_CTRLW:
              MagnEt_SpellClear();
              Fix_MagnEt();
              break;

            case K_CTRLX:
              MagnEt_SpellClear();
              Cursor_Down(TRUE);
              break;

            case K_CTRLY:
              MagnEt_SpellClear();
              /* If we're on the last line, simply blank it out */

              if (offset+cursor_x >= num_lines)
              {
                screen[offset+cursor_x][0]='\r';
                screen[offset+cursor_x][1]='\0';
                update_table[offset+cursor_x]=TRUE;
              }
              else
              {
                /* Otherwise, delete it and move up a line */

                Delete_Line(cursor_x);
              }

              cursor_y=1;

              GOTO_TEXT(cursor_x,cursor_y);
              break;

            case K_CTRLZ:
              ret=SAVE;
              goto BackToCaller;

            case K_ESC:
              switch (Process_Cursor_Key(pr))
              {
                case SAVE:
                  ret=SAVE;
                  goto BackToCaller;

                case ABORT:
                  break_loop=TRUE;
                  break;
              }
              break;

            case K_CTRLB:
            case K_CTRLU:
            case '\x1c':
            case '\x1e':
            case '\x1f':
              MagnEt_Bad_Keystroke();

            case K_CTRLJ:  /* Linefeed, just ignore */
              break;

            case '+':
              Puts(sp_bs);
              /* fall-through */

            default:        /* Normal character */
              {
                if (state==0 && (ch==':' || ch==';' || ch=='B' || ch=='8'))
                  state=1;
                else if (state==1 && ch=='-')
                  state=2;
                else if (state==2 && (ch=='(' || ch=='{' || ch=='['))
                {
                  Goto(MAGNET_CONTEXT_ROW,10);

                  /* Tell this deadbeat to lighten up! */
                  PutsForce(happy);
                  EMIT_MSG_TEXT_COL();


                  GOTO_TEXT(cursor_x,cursor_y);
                  state=0;
                }
                else state=0;

                if (ch >= '1' && ch <= '5' && MagnEt_SpellApplySuggestion(ch-'1'))
                  break;

                if (ch > 31 && (ch < 127 || ((mah.ma.attribs & MA_HIBIT))))
                {
                  Add_Character(ch);
                  MagnEt_SpellHandleTypedChar(ch);
                }
              }

              break;
          }
        }

        /* Make sure s/he really means it */
        if (MagnEt_ContextYesNo(isachange ? abortchange : abortmsg)==YES)
          break;  /* Get out of loop */
        else
        {
          (void)mdm_ctrlc(0);
          Mdm_Flow_Off();

          Fix_MagnEt();

          break_loop=FALSE; /* Keep on truckin' */
        }
      }   /* while (1) */
    }     /* if (setjmp()==0) */
  }       /* max_lines != 0 */


 /* Free everything, then return an ABORT code, since we only get this far *
  * if something craps out.                                                */

  Free_All();

  if (usr.bits2 & BITS2_CLS)
    Puts(CLS);
  else NoFF_CLS();

  ret=ABORT;

BackToCaller:

  MagnEt_SpellDone();

  in_msghibit--;

  Mdm_Flow_On();

  free(update_table);

  MciPopParseFlags();
  return ret;
}


static word near Process_Scan_Code(struct _replyp *pr)
{
  if (loc_peek()=='\x24')    /* Alt-J */
  {
    loc_getch();
    Shell_To_Dos();
    Fix_MagnEt();
  }
  else switch(Mdm_getcwcc())
  {
    case 59:
      MagnEt_Help();
      break;

    case 16:      /* Alt-Q: Quote popup */
      Quote_Popup(pr);
      Fix_MagnEt();
      break;

    case 31:      /* Exit/Save */
      return SAVE;

    case 45:      /* Exit/NoSave */
      return ABORT;

    case 46:      /* Alt-C: was Quote copy, now no-op */
      break;

     case 68:
      MagnEt_Menu();
      break;

    case 71:
      MagnEt_SpellClear();
      Cursor_BeginLine();
      break;

    case 72:
      MagnEt_SpellClear();
      Cursor_Up();
      break;

    case 73:
      MagnEt_SpellClear();
      Page_Up();
      break;

    case 75:
      MagnEt_SpellClear();
      Cursor_Left();
      break;

    case 77:
      MagnEt_SpellClear();
      Cursor_Right();
      break;

    case 79:
      MagnEt_SpellClear();
      Cursor_EndLine();
      break;

    case 80:
      MagnEt_SpellClear();
      Cursor_Down(TRUE);
      break;

    case 81:
      MagnEt_SpellClear();
      Page_Down();
      break;

    case 82:
      Toggle_Insert();
      break;

    case 83:
      MagnEt_SpellClear();
      Delete_Char();
      break;

    case 115:
      MagnEt_SpellClear();
      Word_Left();
      break;

    case 116:
      MagnEt_SpellClear();
      Word_Right();
      break;

    default:
      MagnEt_Bad_Keystroke();
      break;
  }

  return NOTHING;
}



static word near Process_Cursor_Key(struct _replyp *pr)
{
  int ch;

  if (Mdm_kpeek_tic(2) == -1)
  {
    switch (MagnEt_EscMenu(pr))
    {
      case MAGNET_ESC_SAVE:
        return SAVE;

      case MAGNET_ESC_ABORT:
        return ABORT;

      case MAGNET_ESC_HELP:
        MagnEt_Help();
        return NOTHING;

      case MAGNET_ESC_QUOTE:
        Quote_Popup(pr);
        Fix_MagnEt();
        return NOTHING;

      case MAGNET_ESC_COLOR:
        MagnEt_InsertColor();
        return NOTHING;

      case MAGNET_ESC_REDRAW:
        Fix_MagnEt();
        return NOTHING;
    }

    return NOTHING;
  }

  ch=Mdm_getcwcc();

  switch(ch)
  {
    case '\x1b':      /* Abort message */
      return ABORT;

    case '[':         /* Start of ANSI sequence */
    case 'O':
      switch (Mdm_getcwcc())
      {
        case 'A':     /* Cursor Up */
          MagnEt_SpellClear();
          Cursor_Up();
          break;

        case 'B':     /* Cursor Down */
          MagnEt_SpellClear();
          Cursor_Down(TRUE);
          break;

        case 'C':     /* Cursor Right */
          MagnEt_SpellClear();
          Cursor_Right();
          break;

        case 'D':     /* Cursor Left */
          MagnEt_SpellClear();
          Cursor_Left();
          break;
#ifdef UNIX
	case '1':
	  Mdm_getcwcc();
#else
        case 'H':     /* Home */
#endif	
          MagnEt_SpellClear();
          Cursor_BeginLine();
          break;
#ifdef UNIX
	case '4':
	  Mdm_getcwcc();
#else	            
        case 'K':     /* End */
#endif	
          MagnEt_SpellClear();
          Cursor_EndLine();
          break;

#ifdef UNIX
	case '5':
	  Mdm_getcwcc();
	  MagnEt_SpellClear();
	  Page_Up();
	  break;
	case '6':
	  Mdm_getcwcc();
	  MagnEt_SpellClear();
	  Page_Down();
	  break;
#endif    

        default:
          MagnEt_Bad_Keystroke();
          break;

      }
      break;

    default:
      MagnEt_Bad_Keystroke();
      break;
  }

  return NOTHING;
}


static word near Process_Control_K(struct _replyp *pr)
{
  int ret,
      ch;

  Goto(MAGNET_CONTEXT_ROW,usrwidth-3);
  Puts(YELONBLUE "^K");
  GOTO_TEXT(cursor_x,cursor_y);
  EMIT_MSG_TEXT_COL();
  vbuf_flush();

  ret=NOTHING;

  ch=Mdm_getcwcc();

  switch(toupper(ch))
  {
    case 0:         /* Scan code... */
      Mdm_getcw();  /* Throw it away */
      break;

    case 4:
    case 19:
    case 'D':     /* Exit/Save */
    case 'S':
      ret=SAVE;
      break;

    case '?':
      MagnEt_Help();
      break;

    case 8:
    case 'H':
      MagnEt_Menu();
      break;

#ifdef NEVER
    case 16:
    case 'P':
      Piggy();
      break;
#endif

    case 18:
    case 'R':
      Quote_Popup(pr);
      Fix_MagnEt();
      break;

    case 3:
    case 'C':
      MagnEt_InsertColor();
      break;


    case 17:
    case 'Q':     /* Exit/NoSave */
      ret=ABORT;
      break;

    default:
      MagnEt_Bad_Keystroke();
  }

  MagnEt_ClearContextLine();
  GOTO_TEXT(cursor_x,cursor_y);
  EMIT_MSG_TEXT_COL();
  vbuf_flush();

  return ret;
}






static void near Init_Vars(void)
{
  word line;

  offset=0;
  num_lines=offset;
  cursor_x=cursor_y=1;
  pos_to_be_updated=skip_update=FALSE;
  insert=update_table[max_lines]=TRUE;

  usrwidth=min((byte)LINELEN, TermWidth());

  /* Make sure that the user's screen is never larger than the Sysop's,    *
   * to avoid problems.                                                    */

  header_height=MagnEt_CalcHeaderHeight();
  text_start_row=(byte)(header_height ? header_height+1 : 1);

  usrlen=(byte)(TermLength() > header_height+3 ? TermLength()-header_height-3 : 1);

  for (line=0; line < max_lines; line++)
    update_table[line]=FALSE;
}


void EdMemOvfl(void)
{
  logit(mem_nmsgb);
  PutsForce(mem_nmsgb+1);

  Press_ENTER();

  longjmp(jumpto,-1);       /* I know, I know, I'm ashamed about it too... */
                            /* In here, we can return anything but zero.   */
}


