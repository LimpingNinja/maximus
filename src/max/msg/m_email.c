/*
 * m_email.c — Dedicated EMAIL area functions
 *
 * Copyright 2026 by Kevin Morgan.  All rights reserved.
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

/*# name=Dedicated EMAIL area functions
 *
 * Provides the singleton EMAIL area concept: a dedicated private mailbox
 * that is separate from the normal message area hierarchy. The EMAIL area
 * is excluded from normal area listings and browse scans, and is accessed
 * exclusively through these functions.
 */

#define MAX_LANG_global
#define MAX_LANG_m_area
#define MAX_LANG_m_browse
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <mem.h>
#include "prog.h"
#include "max_msg.h"
#include "m_browse.h"
#include "debug_log.h"


/**
 * @brief Check the dedicated EMAIL area for unread messages to this user.
 *
 * Pushes into g_email_area, runs a browse scan for unread messages
 * addressed to usr.name or usr.alias, presents them for reading,
 * and pops back to the caller's original area.
 *
 * This is the primary email experience, typically called at login
 * before the general Msg_Checkmail scan.
 *
 * @param menuname  Optional menu name context (may be NULL)
 */
void Msg_CheckEmail(char *menuname)
{
  SEARCH first, *s;
  char saved_in_echeck;

  /* If no EMAIL area is configured, silently return */
  if (g_email_area[0] == '\0')
    return;

  if (debuglog)
    debug_log("Msg_CheckEmail: pushing to email area '%s'", g_email_area);

  /* Save and push into the EMAIL area */
  if (!PushMsgArea(g_email_area, NULL))
  {
    logit("!Msg_CheckEmail: cannot push to EMAIL area '%s'", g_email_area);
    return;
  }

  /* Build the search: unread messages addressed to us */
  Init_Search(&first);
  first.txt = strdup(usr.name);
  first.attr = MSGREAD;
  first.flag = SF_NOT_ATTR | SF_OR;
  first.where = WHERE_TO;
  first.next = NULL;

  /* Also search by alias if the user has one */
  if (*usr.alias && (first.next = s = malloc(sizeof(SEARCH))) != NULL)
  {
    Init_Search(s);
    s->txt = strdup(usr.alias);
    s->attr = MSGREAD;
    s->flag = SF_NOT_ATTR | SF_OR;
    s->where = WHERE_TO;
  }

  /* Use BROWSE_ACUR to scan only the EMAIL area, with READ mode
   * to present messages for reading, and EXACT for name matching */
  saved_in_echeck = in_echeck;
  in_echeck = TRUE;

  Msg_Browse(BROWSE_ACUR | BROWSE_EXACT | BROWSE_READ,
             &first, menuname);

  in_echeck = saved_in_echeck;

  /* Pop back to the original area */
  PopMsgArea();

  if (debuglog)
    debug_log("Msg_CheckEmail: done, popped back");
}


/**
 * @brief Compose a new email message in the EMAIL area.
 *
 * Pushes into the EMAIL area, runs the standard Msg_Enter flow
 * (header prompts → editor → save), and pops back when done.
 * Messages in the EMAIL area are always private (MA_PVT is implied
 * by the Email style).
 *
 * @param to  Currently unused (reserved for future pre-fill).
 */
void Email_Compose(char *to)
{
  int aborted, rc;
  XMSG msg;

  NW(to);

  if (g_email_area[0] == '\0')
    return;

  if (debuglog)
    debug_log("Email_Compose: pushing to email area '%s'", g_email_area);

  if (!PushMsgArea(g_email_area, NULL))
  {
    logit("!Email_Compose: cannot push to EMAIL area '%s'", g_email_area);
    return;
  }

  /* Follow the same flow as Msg_Enter(): header → editor → save */
  isareply = isachange = FALSE;

  Blank_Msg(&msg);
  *netnode = *orig_msgid = '\0';

  if (GetMsgAttr(&msg, &mah, usr.msg, 0L, -1L) == -1)
    aborted = TRUE;
  else if ((rc = Editor(&msg, NULL, 0L, NULL, NULL)) == ABORT)
    aborted = TRUE;
  else if (rc == LOCAL_EDIT)
    aborted = FALSE;
  else
    aborted = SaveMsg(&msg, NULL, FALSE, 0L, FALSE,
                      &mah, usr.msg, sq, NULL, NULL, FALSE);

  if (aborted)
    Puts(msg_aborted);

  PopMsgArea();

  if (debuglog)
    debug_log("Email_Compose: done, popped back");
}


/**
 * @brief List all messages in the EMAIL area addressed to this user.
 *
 * Shows both read and unread email in a list format. Unlike
 * Msg_CheckEmail (which only shows unread), this shows everything.
 *
 * @param menuname  Optional menu name context (may be NULL)
 */
void Email_Inbox(char *menuname)
{
  SEARCH first, *s;

  if (g_email_area[0] == '\0')
    return;

  if (debuglog)
    debug_log("Email_Inbox: pushing to email area '%s'", g_email_area);

  if (!PushMsgArea(g_email_area, NULL))
  {
    logit("!Email_Inbox: cannot push to EMAIL area '%s'", g_email_area);
    return;
  }

  /* Build search: all messages addressed to us (read + unread) */
  Init_Search(&first);
  first.txt = strdup(usr.name);
  first.flag = SF_OR;
  first.where = WHERE_TO;
  first.next = NULL;

  if (*usr.alias && (first.next = s = malloc(sizeof(SEARCH))) != NULL)
  {
    Init_Search(s);
    s->txt = strdup(usr.alias);
    s->flag = SF_OR;
    s->where = WHERE_TO;
  }

  /* BROWSE_ACUR = this area only, BROWSE_FROM = start from msg 1,
   * BROWSE_LIST = list mode, BROWSE_EXACT = exact name match */
  Msg_Browse(BROWSE_ACUR | BROWSE_FROM | BROWSE_EXACT | BROWSE_LIST,
             &first, menuname);

  PopMsgArea();

  if (debuglog)
    debug_log("Email_Inbox: done, popped back");
}


/**
 * @brief Return the name of the system's EMAIL area.
 *
 * @return Pointer to g_email_area (never NULL, but may be empty
 *         string if called before startup validation completes).
 */
const char *Email_AreaName(void)
{
  return g_email_area;
}


/**
 * @brief Test whether the given area name is the EMAIL area.
 *
 * @param aname  Area name to test
 * @return TRUE if aname matches g_email_area, FALSE otherwise
 */
int IsEmailArea(const char *aname)
{
  if (g_email_area[0] == '\0' || aname == NULL)
    return FALSE;
  return eqstri((char *)aname, g_email_area);
}
