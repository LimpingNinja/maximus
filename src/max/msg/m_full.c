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
static char rcs_id[]="$Id: m_full.c,v 1.5 2004/01/28 06:38:10 paltas Exp $";
#pragma on(unreferenced)
#endif

#define MAX_LANG_m_area
#define MAX_LANG_m_browse
#include <stdio.h>
#include <string.h>
#include "prog.h"
#include "max_msg.h"
#include "m_full.h"


static char * near Show_Attributes(long attr,char *str);
static void near DisplayMessageFrom(XMSG *msg);
static void near DisplayMessageTo(XMSG *msg);

void DrawReaderScreen(MAH *pmah, int inbrowse)
{
  int use_umsgids = ngcfg_get_bool("general.session.use_umsgids");
  { char _ib[16]; snprintf(_ib, sizeof(_ib), "%02d",
      (int)(TermWidth() - strlen(MAS(*pmah, descript)) - strlen(MAS(*pmah, name)) - 5));
    LangPrintf(inbrowse ? browse_rbox_top : reader_box_top,
               PMAS(pmah, name), PMAS(pmah, descript), _ib); }
  LangPrintf(reader_box_mid, use_umsgids ? reader_box_highest : reader_box_of);
  Printf(reader_box_from);
  Printf(reader_box_to);
  Printf(reader_box_subj);
  { char _ib[16]; snprintf(_ib, sizeof(_ib), "%02d", TermWidth());
    LangPrintf(reader_box_bottom, _ib); }
}


void DisplayMessageHeader(XMSG *msg, word *msgoffset, long msgnum, long highmsg, MAH *pmah)
{
  DisplayMessageNumber(msg, msgnum, highmsg);
  DisplayMessageAttributes(msg, pmah);
  DisplayMessageFrom(msg);
  DisplayMessageTo(msg);
  DisplayMessageSubj(msg, pmah);
  Puts(reader_msg_init);
  Puts("|cd");             /* Reset to default color after header chrome */

  if (msgoffset)
    *msgoffset=(hasRIP()) ? 1 : 7;

}




void DisplayMessageNumber(XMSG *msg, long msgnum, long highmsg)
{
  int  i;
  long tlong;
  char tmp[64];

  { char _ib[32]; snprintf(_ib, sizeof(_ib), "%ld", UIDnum(msgnum ? msgnum : MsgGetHighMsg(sq)) + !msgnum);
    LangPrintf(rbox_msgn, _ib); }
  { char _ib[32]; snprintf(_ib, sizeof(_ib), "%ld", UIDnum(highmsg));
    LangPrintf(rbox_high, _ib); }

  i=0;
  if (msg->replyto)
  {
    int use_umsgids = ngcfg_get_bool("general.session.use_umsgids");
    tlong=use_umsgids ? msg->replyto :
           MsgUidToMsgn(sq, msg->replyto, UID_EXACT);

    if (tlong)
    { char _tl[16]; snprintf(_tl, sizeof(_tl), "%ld", tlong);
      i+=LangSprintf(tmp, sizeof(tmp), rbox_replyto, _tl); }
  }

  if (msg->replies[0])
  {
    int use_umsgids = ngcfg_get_bool("general.session.use_umsgids");
    tlong=use_umsgids ? msg->replies[0] :
          MsgUidToMsgn(sq, msg->replies[0], UID_EXACT);

    if (tlong)
    { char _tl[16]; snprintf(_tl, sizeof(_tl), "%ld", tlong);
      i+=LangSprintf(tmp+i, sizeof(tmp)-(size_t)i, rbox_replies, _tl); }
  }

  if (i)
    LangPrintf(rbox_links, tmp);
}


void DisplayMessageAttributes(XMSG *msg, MAH *pmah)
{
  char temp[PATHLEN];
  long amask;
  
  /* Display the message attributes - everything EXCEPT for local */

  amask=~0 & ~MSGLOCAL;

  /* ... and strip off KILL if in an echo area, too. */

  if (pmah->ma.attribs & MA_SHARED)
    amask &= ~MSGKILL;

  LangPrintf(rbox_attrs, Show_Attributes(msg->attr & amask, temp));
}


static char * near Show_Attributes(long attr, char *str)
{
  int i;
  long acomp;

  /* Now catenate the message atttributes... */
                     
  for (i=0, acomp=1L, *str='\0'; i < 16; acomp <<= 1, i++)
    if (attr & acomp)
    {
      char akey[32];
      snprintf(akey, sizeof(akey), "m_area.attribs%d", i);
      strcat(str, maxlang_get(g_current_lang, akey));
      strcat(str, " ");
    }

  Strip_Trailing(str,' ');
  return str;
}




/**************************************************************************** 
                             The TO/FROM fields field
 ****************************************************************************/

static void near DisplayMessageFrom(XMSG *msg)
{
  DisplayShowName(rbox_sho_fname, msg->from);
  DisplayShowDate(rbox_sho_fdate, (union stamp_combo *)&msg->date_written);
  DisplayShowAddress(rbox_sho_faddr, &msg->orig, &mah);
}


static void near DisplayMessageTo(XMSG *msg)
{
  DisplayShowName(rbox_sho_tname, msg->to);
  DisplayShowDate(rbox_sho_tdate, (union stamp_combo *)&msg->date_arrived);
  DisplayShowAddress(rbox_sho_taddr, &msg->dest, &mah);
}


void DisplayShowName(char *sho_name, char *who)
{
  LangPrintf(sho_name, Strip_Ansi(who, NULL, 0L));
}

void DisplayShowDate(char *sho_date, union stamp_combo *sc)
{
  char temp[PATHLEN];
  
  LangPrintf(sho_date, MsgDte(sc, temp));
}

void DisplayShowAddress(char *sho_addr, NETADDR *n, MAH *pmah)
{
  LangPrintf(sho_addr, (pmah->ma.attribs & MA_NET) ? (char *)Address(n) : (char *)blank_str);
}

void DisplayMessageSubj(XMSG *msg, PMAH pmah)
{
  /* Show just "files attached" if it is a composite local file attach */

  if (!mailflag(CFLAGM_ATTRANY) && (pmah->ma.attribs & MA_NET) != 0 &&
     (msg->attr & MSGFILE) != 0 && AllowAttribute(pmah, MSGKEY_LATTACH))
    Puts(rbox_files_att);
  else
  {
    char * subjline;
    if ((pmah->ma.attribs & MA_NET) && (msg->attr & (MSGFILE | MSGFRQ | MSGURQ)))
      subjline=reader_box_file;
    else
      subjline=reader_box_subj;
    Puts(subjline);
    LangPrintf(rbox_sho_subj, Strip_Ansi(msg->subj, NULL, 0L));
  }
}


