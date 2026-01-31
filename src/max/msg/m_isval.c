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
static char rcs_id[]="$Id: m_isval.c,v 1.4 2004/01/28 06:38:10 paltas Exp $";
#pragma on(unreferenced)
#endif

/*# name=Routine to validate message areas
*/

#define MAX_LANG_max_main
#include <string.h>
#include "prog.h"
#include "max_msg.h"
#include "debug_log.h"

static int near _ResolveMsgAreaName(char *name)
{
  HAFF haff;
  MAH ma;
  char found[MAX_ALEN];
  int got_one = FALSE;

  if (name == NULL || *name == '\0')
    return FALSE;

  /* Already qualified */
  if (strchr(name, '.') != NULL)
    return FALSE;

  haff = AreaFileFindOpen(ham, NULL, 0);
  if (haff == NULL)
    return FALSE;

  memset(&ma, 0, sizeof ma);
  memset(found, 0, sizeof found);

  while (AreaFileFindNext(haff, &ma, FALSE) == 0)
  {
    char *full = MAS(ma, name);
    char *p = strrchr(full, '.');
    const char *leaf = p ? p + 1 : full;

    if (eqstri(leaf, name))
    {
      if (!got_one)
      {
        SetAreaName(found, full);
        got_one = TRUE;
      }
      else if (!eqstri(found, full))
      {
        /* Ambiguous leaf name across multiple divisions */
        got_one = FALSE;
        break;
      }
    }

    DisposeMah(&ma);
    memset(&ma, 0, sizeof ma);
  }

  DisposeMah(&ma);
  AreaFileFindClose(haff);

  if (!got_one)
    return FALSE;

  SetAreaName(name, found);
  return TRUE;
}

static int near _ValidMsgArea(PMAH pmah, unsigned flags, BARINFO *pbi)
{
  char *bar;

  if ((flags & VA_OVRPRIV)==0 && !PrivOK(PMAS(pmah, acs), TRUE))
  {
    if (debuglog)
      debug_log("ValidMsgArea: reject ACS name='%s' acs='%s' flags=0x%x", PMAS(pmah, name), PMAS(pmah, acs), (unsigned)flags);
    return FALSE;
  }

    /* Make sure there's a message area here... */
      
  if (isblstr(PMAS(pmah, path)))
  {
    if (debuglog)
      debug_log("ValidMsgArea: reject blank path name='%s' flags=0x%x", PMAS(pmah, name), (unsigned)flags);
    return FALSE;
  }

  if ((flags & VA_VAL) && !MsgValidate(pmah->ma.type, PMAS(pmah, path)))
  {
    if (debuglog)
      debug_log("ValidMsgArea: reject MsgValidate name='%s' type=0x%x path='%s' flags=0x%x",
                PMAS(pmah, name), (unsigned)pmah->ma.type, PMAS(pmah, path), (unsigned)flags);
    return FALSE;
  }

  if ((flags & VA_OVRPRIV)==0 &&
      *(bar=PMAS(pmah, barricade)) && (flags & VA_PWD))
  {
    if (!GetBarPriv(bar, TRUE, pmah, NULL, pbi, !!(flags & VA_EXTONLY)))
    {
      if (debuglog)
        debug_log("ValidMsgArea: reject GetBarPriv name='%s' barricade='%s' flags=0x%x", PMAS(pmah, name), bar, (unsigned)flags);
      return FALSE;
    }
  }

  return TRUE;
}


int ValidMsgArea(char *name, MAH *pmah, unsigned flags, BARINFO *pbi)
{
  MAH ma;
  int rc;

  memset(&ma, 0, sizeof ma);

  pbi->use_barpriv=FALSE;

  /* Use the current area, if supplied */

  if (pmah)
    ma=*pmah;
  else
  {
    if (debuglog)
      debug_log("ValidMsgArea: ReadMsgArea attempt name='%s' flags=0x%x ham=%p", name ? name : "(null)", (unsigned)flags, (void *)ham);

    if (!ReadMsgArea(ham, name, &ma))
    {
      if (debuglog)
        debug_log("ValidMsgArea: ReadMsgArea failed name='%s' -> trying resolve", name ? name : "(null)");

      if (!_ResolveMsgAreaName(name))
      {
        if (debuglog)
          debug_log("ValidMsgArea: resolve FAILED original='%s'", name ? name : "(null)");
        return FALSE;
      }

      if (debuglog)
        debug_log("ValidMsgArea: resolved to '%s' -> retry ReadMsgArea", name ? name : "(null)");

      if (!ReadMsgArea(ham, name, &ma))
      {
        if (debuglog)
          debug_log("ValidMsgArea: ReadMsgArea still failed name='%s'", name ? name : "(null)");
        return FALSE;
      }
    }
  }

  if (debuglog)
    debug_log("ValidMsgArea: loaded name='%s' rec_name='%s' division=%d attribs=0x%x type=0x%x path='%s' acs='%s'",
              name ? name : "(null)",
              MAS(ma, name),
              (int)ma.ma.division,
              (unsigned)ma.ma.attribs,
              (unsigned)ma.ma.type,
              MAS(ma, path),
              MAS(ma, acs));

  rc=_ValidMsgArea(&ma, flags, pbi);

  if (debuglog)
    debug_log("ValidMsgArea: result=%d name='%s' rec_name='%s' flags=0x%x", rc, name ? name : "(null)", MAS(ma, name), (unsigned)flags);

  if (!pmah)
    DisposeMah(&ma);

  return rc;
}

void ForceGetMsgArea(void)
{
  BARINFO bi;
  int bad=FALSE;    /* To ensure that the begin_msgarea is valid */
  int first;

  do
  {
    /* Enable the VA_PWD check only on the first time through this loop.
     * On second and subsequent times, the password checking will be done
     * within Msg_Area itself, but we need to prompt for the password
     * at least once.
     */

    first=TRUE;

    while (!ValidMsgArea(usr.msg, NULL, VA_VAL | (first ? VA_PWD : 0), &bi) ||
           bad)
    {
      first=FALSE;

      if (debuglog)
        debug_log("ForceGetMsgArea: invalid current usr.msg='%s' bad=%d", usr.msg ? (char *)usr.msg : "(null)", bad);

      Puts(inval_cur_msg);
      Msg_Area();
      return;       /* Msg_Area will have pushed a valid area on the stack */
    }

    bad=TRUE;
  }
  while (!PushMsgArea(usr.msg, &bi));
}




