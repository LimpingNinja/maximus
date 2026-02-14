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

#define MAX_LANG_global
#define MAX_LANG_m_area
#define MAX_LANG_sysop
#include <ctype.h>
#include "mm.h"
#include "max_msg.h"
#include "max_file.h"
#include "max_menu.h"

/* Scan the menu for this area, and make sure that it's okay to allow       *
 * the user to access this command from this menu.                          */

int CanAccessMsgCommand(PMAH pmah, option opt, char letter)
{
  char *menuname;
  char *barricade;
  struct _opt *popt, *eopt;
  struct _amenu am;

  /* Be defensive: these strings are offsets into the area heap. If the
   * heap is missing/corrupt, fall back to defaults instead of crashing.
   */

  menuname="";
  barricade="";

  if (pmah && pmah->heap)
  {
    if (pmah->ma.cbHeap > 0 && pmah->ma.menuname < pmah->ma.cbHeap)
      menuname=PMAS(pmah, menuname);
    if (pmah->ma.cbHeap > 0 && pmah->ma.barricade < pmah->ma.cbHeap)
      barricade=PMAS(pmah, barricade);
  }

  if (! *menuname)
    menuname=mnu_msg;

  Initialize_Menu(&am);

  if (Read_Menu(&am, menuname))
  {
    logit(cantread, menuname);
    return FALSE;
  }

  letter=toupper(letter);

  /* Scan all of the menu options to find a match for opt (and            *
   * possibly letter)                                                     */

  for (popt=am.opt, eopt=popt+am.m.num_options;
       popt < eopt;
       popt++)
  {
    if (popt->type==opt &&
        (!letter || toupper(am.menuheap[popt->name])==letter))
    {
      if (OptionOkay(&am, popt, FALSE, barricade, pmah,
                     &fah, menuname))
        break;
    }
  }

  Free_Menu(&am);

  /* Return TRUE if the option was okay */

  return popt != eopt;
}

int CanAccessFileCommand(PFAH pfah, option opt, char letter, BARINFO *pbi)
{
  BARINFO biSave;
  char *menuname;
  char *barricade;
  struct _opt *popt, *eopt;
  struct _amenu am;

  menuname="";
  barricade="";

  if (pfah && pfah->heap)
  {
    if (pfah->fa.cbHeap > 0 && pfah->fa.menuname < pfah->fa.cbHeap)
      menuname=PFAS(pfah, menuname);
    if (pfah->fa.cbHeap > 0 && pfah->fa.barricade < pfah->fa.cbHeap)
      barricade=PFAS(pfah, barricade);
  }

  if (! *menuname)
    menuname=mnu_file;

  Initialize_Menu(&am);

  if (Read_Menu(&am, menuname))
  {
    logit(cantread, menuname);
    return FALSE;
  }

  letter=toupper(letter);

  if (pbi && pbi->use_barpriv)
  {
    biSave.priv=usr.priv;
    biSave.keys=usr.xkeys;

    usr.priv=pbi->priv;
    usr.xkeys=pbi->keys;
  }

  /* Scan all of the menu options to find a match for opt (and            *
   * possibly letter)                                                     */

  for (popt=am.opt, eopt=popt+am.m.num_options;
       popt < eopt;
       popt++)
  {
    if (popt->type==opt &&
        (!letter || toupper(am.menuheap[popt->name])==letter))
    {
      if (OptionOkay(&am, popt, FALSE, barricade, &mah,
                     pfah, menuname))
        break;
    }
  }

  if (pbi && pbi->use_barpriv)
  {
    usr.priv=biSave.priv;
    usr.xkeys=biSave.keys;
  }

  Free_Menu(&am);

  /* Return TRUE if the option was okay */

  return popt != eopt;
}


