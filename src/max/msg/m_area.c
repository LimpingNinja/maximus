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
static char rcs_id[]="$Id: m_area.c,v 1.4 2004/01/27 21:00:30 paltas Exp $";
#pragma on(unreferenced)
#endif

/*# name=Message Section: A)rea Change command and listing of message areas
*/

#define INITIALIZE_MSG    /* Intialize message-area variables */

#define MAX_LANG_global
#define MAX_LANG_m_area
#define MAX_LANG_sysop
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include "prog.h"
#include "max_msg.h"
#include "max_menu.h"
#include "debug_log.h"


/* Search for the next or prior message area */

static int near SearchArea(int search, char *input, PMAH pmahDest, BARINFO *pbi, int *piDidValid)
{
  MAH ma;
  HAFF haff;

  memset(&ma, 0, sizeof ma);

  *piDidValid=FALSE;
  strcpy(linebuf, input+1);

  /* Try to find the current message area */

  if ((haff=AreaFileFindOpen(ham, usr.msg, 0))==NULL)
    return TRUE;

  /* Peform the first search to make sure that usr.msg exists */

  if (AreaFileFindNext(haff, &ma, FALSE) != 0)
  {
    AreaFileFindClose(haff);
    return TRUE;
  }

  /* Change the search parameters to find the next area */

  AreaFileFindChange(haff, NULL, 0);

  /* Search for the prior or next area, as appropriate */

  while ((search==-1 ? AreaFileFindPrior : AreaFileFindNext)(haff, &ma, TRUE)==0)
  {
    if ((ma.ma.attribs & MA_HIDDN)==0 &&
        ValidMsgArea(NULL, &ma, VA_VAL | VA_PWD | VA_EXTONLY, pbi))
    {
      *piDidValid=TRUE;
      search=0;
      SetAreaName(usr.msg, MAS(ma, name));
      CopyMsgArea(pmahDest, &ma);
      break;
    }
  }

  AreaFileFindClose(haff);
  DisposeMah(&ma);

  /* If it was found, get out. */

  return (search==0);
}


/* Change to a named message area */

static int near ChangeToArea(char *group, char *input, int first, PMAH pmahDest)
{
  MAH ma;
  char temp[PATHLEN];
  HAFF haff;

  memset(&ma, 0, sizeof ma);

  if (! *input)
  {
    if (first)
      ListMsgAreas(group, FALSE, !!*group);
    else return TRUE;
  }
  else if ((haff=AreaFileFindOpen(ham, input, AFFO_DIV)) != NULL)
  {
    int rc;

    /* Try to find this area relative to the current division */

    strcpy(temp, group);

    /* If we have a non-blank group, add a dot */

    if (*temp)
      strcat(temp, dot);

    /* Add the specified area */

    strcat(temp, input);

    AreaFileFindChange(haff, temp, AFFO_DIV);
    rc=AreaFileFindNext(haff, &ma, FALSE);

    if (rc==0)  /* got it as a qualified area name */
      strcpy(input, temp);
    else
    {
      /* Try to find it as a fully-qualified area name */

      AreaFileFindReset(haff);
      AreaFileFindChange(haff, input, AFFO_DIV);

      rc=AreaFileFindNext(haff, &ma, FALSE);
    }

    if (rc==0 && (ma.ma.attribs & MA_DIVBEGIN))
    {
      strcpy(group, MAS(ma, name));
      AreaFileFindClose(haff);
      ListMsgAreas(group, FALSE, !!*group);
    }
    else
    {
      SetAreaName(usr.msg, input);
      CopyMsgArea(pmahDest, &ma);
      AreaFileFindClose(haff);
      DisposeMah(&ma);
      return TRUE;
    }
  }

  DisposeMah(&ma);
  return FALSE;
}


static int near MsgAreaMenu(PMAH pmah, BARINFO *pbi, char *group)
{
  char input[PATHLEN];
  unsigned first=TRUE;    /* Display the area list 1st time <enter> is hit */
  int did_valid=FALSE;
  const char *keys;

  WhiteN();

  {
    keys = ngcfg_get_string_raw("general.session.area_change_keys");
    if (keys == NULL || strlen(keys) < 3)
      keys = "-+?";
  }

  for (;;)
  {
    int search=0;

    Puts(WHITE);
    
    { char _k0[2]={keys[0],0}, _k1[2]={keys[1],0}, _k2[2]={keys[2],0};
      InputGets(input, msg_prmpt, _k0, _k1, _k2); }
    cstrupr(input);

    /* See if the user wishes to search for something */

    if (*input==keys[1] || *input==']' || *input=='>' || *input=='+')
      search=1;
    else if (*input==keys[0] || *input=='[' || *input=='<' || *input=='-')
      search=-1;



    if (search) /* Search for a specific area */
    {
      if (SearchArea(search, input, pmah, pbi, &did_valid))
        return did_valid;
    }
    else if (*input=='\'' || *input=='`' || *input=='"')
      Display_File(0, NULL, ss, (char *)ngcfg_get_string_raw("maximus.misc_path"), quotes_misunderstood);
    else if (*input=='#')              /* Maybe the user misunderstood? */
      Display_File(0, NULL, ss, (char *)ngcfg_get_string_raw("maximus.misc_path"), numsign_misunderstood);
    else if (*input=='/' || *input=='\\')
    {
      *group=0;
      strcpy(linebuf, input+1);

      if (! *linebuf)
        ListMsgAreas(group, FALSE, !!*group);
    }
    else if (*input=='.')   /* go up one or more levels */
    {
      char *p=input;
      int up_level=0;

      /* Count the number of dots */

      while (*++p=='.')
        up_level++;

      /* Add any area names which may come after this */

      if (*p)
        strcpy(linebuf, p);

      /* Now go up the specified number of levels */

      while (up_level--)
        if ((p=strrchr(group, '.')) != NULL)
          *p=0;
        else *group=0;

      if (! *linebuf)
        ListMsgAreas(group, FALSE, !!*group);
    }
    else if (*input==keys[2] || *input=='?')
    {
      strcpy(linebuf, input+1);
      ListMsgAreas(group, FALSE, !!*group);
    }
    else if (*input=='=')
      ListMsgAreas(NULL, FALSE, FALSE);
    else if (! *input ||
             (*input >= '0' && *input <= '9') ||
             (*input >= 'A' && *input <= 'Z'))
    {
      if (ChangeToArea(group, input, first, pmah))
        return did_valid;
    }
    else { char _cb[2] = { *input, '\0' };
           LangPrintf(dontunderstand, _cb); }

    first=FALSE;
  }
}

int Msg_Area(void)
{
  MAH ma;
  BARINFO bi;
  char group[PATHLEN];
  char savearea[MAX_ALEN];
  int ok=FALSE, did_valid;

  memset(&ma, 0, sizeof ma);

  strcpy(savearea, usr.msg);
  MessageSection(usr.msg, group);

  do
  {
    CopyMsgArea(&ma, &mah);
    did_valid=MsgAreaMenu(&ma, &bi, group);

    if (!ma.heap ||
        !(did_valid || ValidMsgArea(NULL, &ma, VA_VAL | VA_PWD, &bi)))
    {
      logit(denied_access, msg_abbr, usr.msg);

      strcpy(usr.msg, savearea);

      {
        const char *area_not_exist = ngcfg_get_path("general.display_files.area_not_exist");

        if (area_not_exist && *area_not_exist)
          Display_File(0, NULL, area_not_exist);
        else
          Puts(areadoesntexist);
      }

      continue;
    }

    if (!PopPushMsgAreaSt(&ma, &bi))
      AreaError(msgapierr);
    else ok=TRUE;
  }
  while (!ok);

  logit(log_msga, usr.msg ? (char *)usr.msg : "(null)");
  DisposeMah(&ma);

  return 0;
}


/* See if we can find the record for our current division */

static int near FoundOurMsgDivision(HAFF haff, char *division, PMAH pmah)
{
  if (!division || *division==0)
    return TRUE;

  return (AreaFileFindNext(haff, pmah, FALSE)==0 &&
          (pmah->ma.attribs & MA_DIVBEGIN) &&
          eqstri(PMAS(pmah, name), division));
}



void ListMsgAreas(char *div_name, int do_tag, int show_help)
{
  BARINFO bi;
  MAH ma;
  HAFF haff=0;
  char nonstop=FALSE;
  char headfoot[PATHLEN];
  char *file;
  int ch;
  const char *msg_area_list;

  if (debuglog)
    debug_log("ListMsgAreas: entry div_name='%s' do_tag=%d show_help=%d ham=%p usr.msg='%s'",
              div_name ? div_name : "(null)",
              (int)do_tag,
              (int)show_help,
              (void *)ham,
              usr.msg ? (char *)usr.msg : "(null)");

  memset(&ma, 0, sizeof ma);

  msg_area_list = ngcfg_get_path("general.display_files.msg_area_list");

  if (msg_area_list && *msg_area_list && !do_tag)
  {
    /* Display different files depending on the current message
     * division.
     */

    if (!div_name || *div_name==0 ||
        (haff=AreaFileFindOpen(ham, div_name, AFFO_DIV))==NULL ||
        !FoundOurMsgDivision(haff, div_name, &ma) ||
        eqstri(MAS(ma, path), dot))
    {
      if (debuglog)
        debug_log("ListMsgAreas: using default msg_area_list file='%s' (div_name='%s' haff=%p found_div=%d div_path='%s')",
                  msg_area_list ? msg_area_list : "(null)",
                  div_name ? div_name : "(null)",
                  (void *)haff,
                  (int)(haff && FoundOurMsgDivision(haff, div_name, &ma)),
                  ma.heap ? MAS(ma, path) : "(no-ma)");
      file=(char *)msg_area_list;
    }
    else
    {
      if (debuglog)
        debug_log("ListMsgAreas: using division display file='%s' div_name='%s'",
                  MAS(ma, path),
                  div_name ? div_name : "(null)");
      file=MAS(ma, path);
    }

    Display_File(0, NULL, file);
  }
  else
  {
    Puts(CLS);

    display_line=display_col=1;

    ParseCustomMsgAreaList(NULL, div_name,
                           (char *)ngcfg_get_string_raw("general.display_files.msg_header"),
                           headfoot, TRUE, '*');
    Puts(headfoot);

    if ((haff=AreaFileFindOpen(ham, div_name, AFFO_DIV))==NULL)
    {
      if (debuglog)
        debug_log("ListMsgAreas: AreaFileFindOpen failed div_name='%s' ham=%p",
                  div_name ? div_name : "(null)", (void *)ham);
      return;
    }

    /* Ensure that we have found the beginning of our division */

    if (!FoundOurMsgDivision(haff, div_name, &ma))
    {
      if (debuglog)
        debug_log("ListMsgAreas: FoundOurMsgDivision failed div_name='%s' -> reset to flat list",
                  div_name ? div_name : "(null)");
      AreaFileFindReset(haff);
      div_name = "";
    }

    {
      sword this_div=div_name && *div_name ? ma.ma.division : -1;
      int printed = 0;
      int iter = 0;

      /* Now find anything after the current division */

      AreaFileFindChange(haff, NULL, AFFO_DIV);

      while (AreaFileFindNext(haff, &ma, FALSE)==0)
      {
        int in_div = (!div_name || ma.ma.division==this_div+1);
        int not_hidden = ((ma.ma.attribs & MA_HIDDN)==0);
        int div_ok = ((ma.ma.attribs & MA_DIVBEGIN) && PrivOK(MAS(ma, acs), TRUE));
        int area_ok = ValidMsgArea(NULL, &ma, VA_NOVAL, &bi);

        iter++;
        if (debuglog && iter <= 200)
          debug_log("ListMsgAreas: rec name='%s' attribs=0x%x division=%d this_div=%d in_div=%d hidden=%d divbegin=%d divend=%d path='%s' acs='%s' div_ok=%d area_ok=%d",
                    MAS(ma, name),
                    (unsigned)ma.ma.attribs,
                    (int)ma.ma.division,
                    (int)this_div,
                    in_div,
                    !not_hidden,
                    (int)!!(ma.ma.attribs & MA_DIVBEGIN),
                    (int)!!(ma.ma.attribs & MA_DIVEND),
                    MAS(ma, path),
                    MAS(ma, acs),
                    div_ok,
                    area_ok);

        /* If we're just doing a flat area list, don't display              *
         * division names.                                                  */

        if (!div_name && (ma.ma.attribs & MA_DIVBEGIN))
          continue;

        /* If we have reached the end of our division, break out of the     *
         * loop.                                                            */

        if (ma.ma.attribs & MA_DIVEND)
        {
          if (div_name && ma.ma.division==this_div)
            break;
          else continue;
        }

        /* If we're in the right division and the area is valid, display    *
         * its name.                                                        */

        if ((!div_name || ma.ma.division==this_div+1) &&
            (ma.ma.attribs & MA_HIDDN)==0 &&
            (((ma.ma.attribs & MA_DIVBEGIN) && PrivOK(MAS(ma, acs), TRUE)) ||
             ValidMsgArea(NULL, &ma, VA_NOVAL, &bi)))
        {
          printed++;
          ch=!do_tag ? '*' : TagQueryTagList(&mtm, MAS(ma, name)) ? '@' : ' ';

          ParseCustomMsgAreaList(&ma, div_name,
                                 (char *)ngcfg_get_string_raw("general.display_files.msg_format"),
                                 headfoot, FALSE, ch);

          Puts(headfoot);
          vbuf_flush();
        }

        if (halt())
          break;

        if ((!do_tag && MoreYnBreak(&nonstop, CYAN)) ||
            (do_tag && TagMoreBreak(&nonstop)))
        {
          break;
        }
      }

      if (debuglog)
        debug_log("ListMsgAreas: done iter=%d printed=%d div_name='%s' this_div=%d",
                  iter, printed, div_name ? div_name : "(null)", (int)this_div);
    }

    ParseCustomMsgAreaList(NULL, div_name,
                           (char *)ngcfg_get_string_raw("general.display_files.msg_footer"),
                           headfoot, FALSE, '*');
    Puts(headfoot);

    Putc('\n');

    /* If necessary, display help for changing areas */

    if (show_help)
      Puts(achg_help);

    vbuf_flush();
  }

  if (haff)
    AreaFileFindClose(haff);

  DisposeMah(&ma);
}


char *MessageSection(char *current, char *szDest)
{
  char *p;

  strcpy(szDest, current);

  if ((p=strrchr(szDest, '.')) != NULL)
    *p=0;
  else
    *szDest=0;

  return szDest;
}


