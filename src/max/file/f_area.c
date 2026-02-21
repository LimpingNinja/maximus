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
static char rcs_id[]="$Id: f_area.c,v 1.3 2004/01/27 21:00:27 paltas Exp $";
#pragma on(unreferenced)
#endif

/*# name=File Section: A)rea Change command and listing of file areas
*/

#define INITIALIZE_FILE    /* Intialize message-area variables */

#define MAX_LANG_f_area
#define MAX_LANG_global
#define MAX_LANG_m_area
#define MAX_LANG_sysop
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include "prog.h"
#include "max_file.h"
#include "max_menu.h"
#include "debug_log.h"
#include "ui_lightbar.h"

/* Search for the next or prior file area */

static int near SearchArea(int search, char *input, PFAH pfahDest, BARINFO *pbi, int *piDidValid)
{
  FAH fa;
  HAFF haff;

  memset(&fa, 0, sizeof fa);
  *piDidValid=FALSE;
  strcpy(linebuf, input+1);

  /* Try to find the current file area */

  if ((haff=AreaFileFindOpen(haf, usr.files, 0))==NULL)
    return TRUE;

  /* Peform the first search to make sure that usr.files exists */

  if (AreaFileFindNext(haff, &fa, FALSE) != 0)
  {
    AreaFileFindClose(haff);
    return TRUE;
  }

  /* Change the search parameters to find the next area */

  AreaFileFindChange(haff, NULL, 0);

  /* Search for the prior or next area, as appropriate */

  while ((search==-1 ? AreaFileFindPrior : AreaFileFindNext)(haff, &fa, TRUE)==0)
  {
    if ((fa.fa.attribs & FA_HIDDN)==0 &&
        ValidFileArea(NULL, &fa, VA_VAL | VA_PWD | VA_EXTONLY, pbi))
    {
      *piDidValid=TRUE;
      search=0;
      SetAreaName(usr.files, FAS(fa, name));
      CopyFileArea(pfahDest, &fa);
      break;
    }
  }

  AreaFileFindClose(haff);
  DisposeFah(&fa);

  /* If it was found, get out. */

  return (search==0);
}


/* Change to a named message area */

static int near ChangeToArea(char *group, char *input, int first, PFAH pfahDest)
{
  FAH fa;
  char temp[PATHLEN];
  HAFF haff;

  memset(&fa, 0, sizeof fa);

  if (! *input)
  {
    if (first)
    {
      char sel[MAX_ALEN] = {0};
      int ret = ListFileAreas(group, !!*group, sel);
      if (ret > 0 && *sel)
      {
        SetAreaName(usr.files, sel);
        return TRUE;
      }
      else if (ret < 0)
        return TRUE;
    }
    else return TRUE;
  }
  else if ((haff=AreaFileFindOpen(haf, input, AFFO_DIV)) != NULL)
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
    rc=AreaFileFindNext(haff, &fa, FALSE);

    if (debuglog)
      debug_log("ChangeToArea: input='%s' group='%s' qualified='%s' rc1=%d", input, group, temp, rc);

    if (rc==0)  /* got it as a qualified area name */
      strcpy(input, temp);
    else
    {
      /* Try to find it as a fully-qualified area name */

      AreaFileFindReset(haff);
      AreaFileFindChange(haff, input, AFFO_DIV);

      rc=AreaFileFindNext(haff, &fa, FALSE);

      if (debuglog)
        debug_log("ChangeToArea: fully-qualified lookup input='%s' rc2=%d", input, rc);
    }

    if (rc==0 && (fa.fa.attribs & FA_DIVBEGIN))
    {
      if (debuglog)
        debug_log("ChangeToArea: matched division begin name='%s'", FAS(fa, name));
      strcpy(group, FAS(fa, name));
      AreaFileFindClose(haff);
      DisposeFah(&fa);
      {
        char sel[MAX_ALEN] = {0};
        int ret = ListFileAreas(group, !!*group, sel);
        if (ret > 0 && *sel)
        {
          SetAreaName(usr.files, sel);
          return TRUE;
        }
        else if (ret < 0)
          return TRUE;
      }
      return FALSE;
    }
    else if (rc==0)
    {
      if (debuglog)
        debug_log("ChangeToArea: selecting area input='%s' rc=%d attribs=0x%x downpath='%s'", input, rc,
                  (unsigned)fa.fa.attribs, fa.heap ? FAS(fa, downpath) : "(null)");
      SetAreaName(usr.files, input);
      CopyFileArea(pfahDest, &fa);
      AreaFileFindClose(haff);
      DisposeFah(&fa);
      return TRUE;
    }

    AreaFileFindClose(haff);
  }

  DisposeFah(&fa);
  return FALSE;
}


static int near FileAreaMenu(PFAH pfah, char *group, BARINFO *pbi)
{
  char input[PATHLEN];
  unsigned first=TRUE;    /* Display the area list 1st time <enter> is hit */
  int did_valid=FALSE;
  const char *achg;

  WhiteN();

  for (;;)
  {
    int search=0;

    achg = ngcfg_get_string_raw("general.session.area_change_keys");
    if (achg == NULL || strlen(achg) < 3)
      achg = "-+?";

    Puts(WHITE);
    
    { char _k0[2]={achg[0],0}, _k1[2]={achg[1],0}, _k2[2]={achg[2],0};
      /* Use line-mode input here so names like "BBS Files.DOORS" are
       * accepted as one token for this prompt only. */
      InputGetsL(input, PATHLEN-1, file_prmpt, _k0, _k1, _k2); }
    cstrupr(input);

    /* See if the user wishes to search for something */

    if (*input==achg[1] || *input==']' || *input=='>' || *input=='+')
      search=1;
    else if (*input==achg[0] || *input=='[' || *input=='<' || *input=='-')
      search=-1;



    if (search) /* Search for a specific area */
    {
      if (SearchArea(search, input, pfah, pbi, &did_valid))
      {
        /* Update group to reflect the division of the new area so that
         * subsequent '?' lists show the correct division context. */
        FileSection(usr.files, group);
        return did_valid;
      }
    }
    else if (*input=='\'' || *input=='`' || *input=='"')
      Display_File(0, NULL, ss, (char *)ngcfg_get_string_raw("maximus.display_path"), quotes_misunderstood);
    else if (*input=='#')              /* Maybe the user misunderstood? */
      Display_File(0, NULL, ss, (char *)ngcfg_get_string_raw("maximus.display_path"), numsign_misunderstood);
    else if (*input=='/' || *input=='\\')
    {
      *group=0;
      strcpy(linebuf, input+1);

      if (! *linebuf)
      {
        char sel[MAX_ALEN] = {0};
        int ret = ListFileAreas(group, !!*group, sel);
        if (ret > 0 && *sel)
        {
          SetAreaName(usr.files, sel);
          CopyFileArea(pfah, &fah);
          return did_valid;
        }
        else if (ret < 0)
          return did_valid;
      }
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
      {
        char sel[MAX_ALEN] = {0};
        int ret = ListFileAreas(group, !!*group, sel);
        if (ret > 0 && *sel)
        {
          SetAreaName(usr.files, sel);
          CopyFileArea(pfah, &fah);
          return did_valid;
        }
        else if (ret < 0)
          return did_valid;
      }
    }
    else if (*input==achg[2] || *input=='?')
    {
      strcpy(linebuf, input+1);
      {
        char sel[MAX_ALEN] = {0};
        int ret = ListFileAreas(group, !!*group, sel);
        if (ret > 0 && *sel)
        {
          SetAreaName(usr.files, sel);
          CopyFileArea(pfah, &fah);
          return did_valid;
        }
        else if (ret < 0)
          return did_valid;
      }
    }
    else if (*input=='=')
      ListFileAreas(NULL, FALSE, NULL);
    else if (! *input ||
             (*input >= '0' && *input <= '9') ||
             (*input >= 'A' && *input <= 'Z'))
    {
      if (ChangeToArea(group, input, first, pfah))
        return did_valid;
    }
    else { char _cb[2] = { *input, '\0' };
           LangPrintf(dontunderstand, _cb); }

    first=FALSE;
  }
}


int File_Area(void)
{
  FAH fa;
  BARINFO bi;
  char group[PATHLEN];
  char savearea[MAX_ALEN];
  int ok=FALSE;
  int did_valid;

  memset(&fa, 0, sizeof fa);
  strcpy(savearea, usr.files);
  FileSection(usr.files, group);

  do
  {
    /* Re-extract division context from current area each iteration so that
     * [/] navigation across divisions keeps group in sync. */
    FileSection(usr.files, group);

    CopyFileArea(&fa, &fah);
    did_valid=FileAreaMenu(&fa, group, &bi);

    if (debuglog)
      debug_log("File_Area: after menu did_valid=%d usr.files='%s' fa.heap=%p", did_valid, usr.files, (void *)fa.heap);

    if (!fa.heap || !(did_valid || ValidFileArea(NULL, &fa, VA_VAL | VA_PWD, &bi)))
    {
      if (debuglog)
        debug_log("File_Area: invalid selection did_valid=%d heap=%p name='%s' downpath='%s'", did_valid,
                  (void *)fa.heap, fa.heap ? FAS(fa, name) : "(null)",
                  fa.heap ? FAS(fa, downpath) : "(null)");
      logit(denied_access, deny_file, usr.files);

      strcpy(usr.files, savearea);

      {
        const char *area_not_exist = ngcfg_get_path("general.display_files.area_not_exist");

        if (area_not_exist && *area_not_exist)
          Display_File(0, NULL, area_not_exist);
        else
          Puts(areadoesntexist);
      }

      continue;
    }

    if (!PopPushFileAreaSt(&fa, &bi))
    {
      if (debuglog)
        debug_log("File_Area: PopPushFileAreaSt failed name='%s' downpath='%s'", FAS(fa, name), FAS(fa, downpath));
      Puts(areadoesntexist);
    }
    else ok=TRUE;
  }
  while (!ok);

  logit(log_farea, usr.files);
  DisposeFah(&fa);

  return 0;
}


static int near FoundOurFileDivision(HAFF haff, char *division, PFAH pfah)
{
  if (!division || *division==0)
    return TRUE;

  return (AreaFileFindNext(haff, pfah, FALSE)==0 &&
          (pfah->fa.attribs & FA_DIVBEGIN) &&
          eqstri(PFAS(pfah, name), division));
}


/* ============================================================================
 * Lightbar file-area list helpers
 * ============================================================================ */

#define LB_FAREA_MAX 200

/* Uncomment the next line to fill the lightbar with synthetic test data
 * instead of real file areas.  Rebuild and run — no areas needed. */
#define LB_FAREA_TEST

/** @brief One entry in the collected lightbar list. */
typedef struct {
  char name[MAX_ALEN];      /**< Full qualified area name */
  char display[PATHLEN];    /**< Formatted display string (no trailing newline) */
  int  is_div;              /**< Non-zero if this is a FA_DIVBEGIN entry */
} lb_farea_entry_t;

/** @brief Context passed to the lightbar get_item callback. */
typedef struct {
  lb_farea_entry_t *entries;
  int count;
  int *selected_index_ptr;
  int highlight_mode;
  char selected_attr_code[8];
  char normal_attr_code[8];
} lb_farea_ctx_t;

enum {
  LB_HILITE_ROW = 0,
  LB_HILITE_FULL = 1,
  LB_HILITE_NAME = 2
};

/**
 * @brief Parse a color nibble (0..15) from a color name or numeric token.
 */
static int lb_parse_color_nibble(const char *s, int *out_nibble)
{
  char *end = NULL;
  long v;

  if (!s || !*s || !out_nibble)
    return 0;

  if (*s == '|')
    s++;

  if (eqstri((char *)s, "black"))      { *out_nibble = 0; return 1; }
  if (eqstri((char *)s, "blue"))       { *out_nibble = 1; return 1; }
  if (eqstri((char *)s, "green"))      { *out_nibble = 2; return 1; }
  if (eqstri((char *)s, "cyan"))       { *out_nibble = 3; return 1; }
  if (eqstri((char *)s, "red"))        { *out_nibble = 4; return 1; }
  if (eqstri((char *)s, "magenta"))    { *out_nibble = 5; return 1; }
  if (eqstri((char *)s, "brown"))      { *out_nibble = 6; return 1; }
  if (eqstri((char *)s, "gray") || eqstri((char *)s, "grey"))
                                          { *out_nibble = 7; return 1; }
  if (eqstri((char *)s, "darkgray") || eqstri((char *)s, "darkgrey"))
                                          { *out_nibble = 8; return 1; }
  if (eqstri((char *)s, "lightblue"))   { *out_nibble = 9; return 1; }
  if (eqstri((char *)s, "lightgreen"))  { *out_nibble = 10; return 1; }
  if (eqstri((char *)s, "lightcyan"))   { *out_nibble = 11; return 1; }
  if (eqstri((char *)s, "lightred"))    { *out_nibble = 12; return 1; }
  if (eqstri((char *)s, "lightmagenta")){ *out_nibble = 13; return 1; }
  if (eqstri((char *)s, "yellow"))      { *out_nibble = 14; return 1; }
  if (eqstri((char *)s, "white"))       { *out_nibble = 15; return 1; }

  v = strtol(s, &end, 16);
  if (end && *end == '\0' && v >= 0 && v <= 15)
  {
    *out_nibble = (int)v;
    return 1;
  }

  return 0;
}

/**
 * @brief Resolve configured highlight mode for file-area lightbar selection.
 */
static int lb_get_highlight_mode(void)
{
  const char *mode = ngcfg_get_string_raw("general.display.file_areas.lightbar_what");

  if (mode && *mode)
  {
    if (eqstri((char *)mode, "full"))
      return LB_HILITE_FULL;
    if (eqstri((char *)mode, "name"))
      return LB_HILITE_NAME;
  }

  return LB_HILITE_ROW;
}

/**
 * @brief Build lightbar attrs with configurable foreground/background overrides.
 *
 * Defaults:
 * - Normal row: theme text fallback (0x07)
 * - Selected row background: theme lightbar background fallback (|17)
 * - Selected row foreground: inherited from normal row unless overridden
 */
static void lb_get_lightbar_attrs(byte *normal_attr, byte *selected_attr)
{
  byte normal = Mci2Attr("|tx", 0x07);
  byte bg_default = Mci2Attr("|17", 0x17);
  byte selected;
  const char *fore = ngcfg_get_string_raw("general.display.file_areas.lightbar_fore");
  const char *back = ngcfg_get_string_raw("general.display.file_areas.lightbar_back");
  int fore_nibble = -1;
  int back_nibble = -1;

  if (fore && *fore)
    lb_parse_color_nibble(fore, &fore_nibble);
  if (back && *back)
    lb_parse_color_nibble(back, &back_nibble);

  selected = (byte)((normal & 0x0f) | (bg_default & 0x70));

  if (fore_nibble >= 0)
    selected = (byte)((selected & 0xf0) | (byte)fore_nibble);
  if (back_nibble >= 0)
    selected = (byte)((selected & 0x0f) | (byte)(back_nibble << 4));

  *normal_attr = normal;
  *selected_attr = selected;
}

/**
 * @brief Apply name-only highlight mode by wrapping selected name with attrs.
 */
static void lb_apply_name_highlight(const lb_farea_ctx_t *c,
                                    const lb_farea_entry_t *e,
                                    char *out,
                                    size_t out_sz)
{
  const char *p;
  size_t prefix_len;
  size_t name_len;

  if (!c || !e || !out || out_sz == 0)
    return;

  if (c->highlight_mode != LB_HILITE_NAME || !c->selected_index_ptr)
    return;

  p = strstr(out, e->name);
  if (!p)
    return;

  prefix_len = (size_t)(p - out);
  name_len = strlen(e->name);

  {
    char tmp[PATHLEN];
    size_t used = 0;

    tmp[0] = '\0';

    if (prefix_len >= sizeof(tmp))
      return;

    memcpy(tmp, out, prefix_len);
    used = prefix_len;
    tmp[used] = '\0';

    used += (size_t)snprintf(tmp + used, sizeof(tmp) - used, "%s", c->selected_attr_code);
    used += (size_t)snprintf(tmp + used, sizeof(tmp) - used, "%s", e->name);
    used += (size_t)snprintf(tmp + used, sizeof(tmp) - used, "%s", c->normal_attr_code);
    used += (size_t)snprintf(tmp + used, sizeof(tmp) - used, "%s", p + name_len);

    if (used < sizeof(tmp))
    {
      strncpy(out, tmp, out_sz - 1);
      out[out_sz - 1] = '\0';
    }
  }
}

/**
 * @brief Lightbar get_item callback — copies pre-formatted display string.
 */
static int lb_farea_get_item(void *ctx, int index, char *out, size_t out_sz)
{
  lb_farea_ctx_t *c = (lb_farea_ctx_t *)ctx;

  if (!c || index < 0 || index >= c->count)
    return -1;

  strncpy(out, c->entries[index].display, out_sz - 1);
  out[out_sz - 1] = '\0';

  if (c->selected_index_ptr && index == *c->selected_index_ptr)
    lb_apply_name_highlight(c, &c->entries[index], out, out_sz);

  return 0;
}

#ifdef LB_FAREA_TEST
/**
 * @brief Fill entries with synthetic test data for UI development.
 *
 * Generates a mix of fake divisions and areas so the lightbar display,
 * paging, footer, and drill-in can be tested without real file areas.
 * Division drill-in is supported one level deep ("TestDiv.child").
 */
static int lb_generate_test_entries(char *div_name, lb_farea_entry_t *entries, int max_entries)
{
  int i, count = 0;

#define LB_ADD_DIV(_name, _desc)                                                \
  do {                                                                           \
    if (count < max_entries) {                                                   \
      lb_farea_entry_t *e = &entries[count++];                                   \
      strncpy(e->name, (_name), MAX_ALEN - 1);                                   \
      e->name[MAX_ALEN - 1] = '\0';                                              \
      snprintf(e->display, PATHLEN,                                               \
               "|tx[|hddiv |tx]|pr %-20s |tx... %s|cd",                          \
               (_name), (_desc));                                                 \
      e->is_div = 1;                                                              \
    }                                                                             \
  } while (0)

#define LB_ADD_AREA(_name, _desc)                                               \
  do {                                                                           \
    if (count < max_entries) {                                                   \
      lb_farea_entry_t *e = &entries[count++];                                   \
      strncpy(e->name, (_name), MAX_ALEN - 1);                                   \
      e->name[MAX_ALEN - 1] = '\0';                                              \
      snprintf(e->display, PATHLEN,                                               \
               "|tx[|hdarea|tx]|pr %-20s |tx... %s|cd",                          \
               (_name), (_desc));                                                 \
      e->is_div = 0;                                                              \
    }                                                                             \
  } while (0)

  if (div_name && *div_name)
  {
    /*
     * Explicit hierarchy map for testing deep lightbar drill-in paths.
     * Includes 3-4 nested levels such as:
     *   Programming -> Programming.Languages -> Programming.Languages.C
     *              -> Programming.Languages.C.Compilers
     */
    if (eqstri(div_name, "Programming"))
    {
      LB_ADD_DIV("Programming.Languages", "Language categories");
      LB_ADD_DIV("Programming.Tools", "Build and debug tools");
      LB_ADD_AREA("Programming.Docs", "Programming manuals and references");
      return count;
    }

    if (eqstri(div_name, "Programming.Languages"))
    {
      LB_ADD_DIV("Programming.Languages.C", "C language ecosystem");
      LB_ADD_DIV("Programming.Languages.Pascal", "Pascal language ecosystem");
      LB_ADD_DIV("Programming.Languages.Rust", "Rust language ecosystem");
      LB_ADD_AREA("Programming.Languages.Misc", "Other language resources");
      return count;
    }

    if (eqstri(div_name, "Programming.Languages.C"))
    {
      LB_ADD_DIV("Programming.Languages.C.Compilers", "C compilers");
      LB_ADD_DIV("Programming.Languages.C.Libraries", "C library packs");
      LB_ADD_AREA("Programming.Languages.C.Tutorials", "C tutorials");
      return count;
    }

    if (eqstri(div_name, "Programming.Languages.C.Compilers"))
    {
      LB_ADD_AREA("Programming.Languages.C.Compilers.OpenWatcom", "Open Watcom toolchain");
      LB_ADD_AREA("Programming.Languages.C.Compilers.DJGPP", "DJGPP toolchain");
      LB_ADD_AREA("Programming.Languages.C.Compilers.GCC", "GCC cross-builds");
      LB_ADD_AREA("Programming.Languages.C.Compilers.TurboC", "Turbo C archives");
      return count;
    }

    if (eqstri(div_name, "Retro"))
    {
      LB_ADD_DIV("Retro.DOS", "DOS classics");
      LB_ADD_DIV("Retro.Amiga", "Amiga scene files");
      LB_ADD_DIV("Retro.C64", "Commodore 64 archives");
      LB_ADD_AREA("Retro.BBSHistory", "Historic BBS artifacts");
      return count;
    }

    if (eqstri(div_name, "Retro.DOS"))
    {
      LB_ADD_DIV("Retro.DOS.BBS", "DOS BBS software");
      LB_ADD_DIV("Retro.DOS.Games", "DOS games");
      LB_ADD_AREA("Retro.DOS.Utils", "DOS utilities");
      return count;
    }

    if (eqstri(div_name, "Retro.DOS.BBS"))
    {
      LB_ADD_DIV("Retro.DOS.BBS.Doors", "BBS door games and apps");
      LB_ADD_AREA("Retro.DOS.BBS.Mailers", "FTN mailers");
      LB_ADD_AREA("Retro.DOS.BBS.MessageBases", "Message base tools");
      return count;
    }

    if (eqstri(div_name, "Retro.DOS.BBS.Doors"))
    {
      LB_ADD_AREA("Retro.DOS.BBS.Doors.Trivia", "Trivia door packs");
      LB_ADD_AREA("Retro.DOS.BBS.Doors.RPG", "RPG door packs");
      LB_ADD_AREA("Retro.DOS.BBS.Doors.Classics", "Classic door collections");
      return count;
    }

    if (eqstri(div_name, "BBS Files"))
    {
      LB_ADD_DIV("BBS Files.Menus", "Menu templates");
      LB_ADD_DIV("BBS Files.Themes", "Theme packs");
      LB_ADD_AREA("BBS Files.Logos", "ANSI/RIP logos");
      return count;
    }

    /* Generic fallback for any unknown synthetic division. */
    for (i = 0; i < 8 && count < max_entries; i++)
    {
      char child_name[MAX_ALEN];
      char child_desc[64];
      snprintf(child_name, sizeof(child_name), "%s.child%d", div_name, i + 1);
      snprintf(child_desc, sizeof(child_desc), "Fallback child area %d", i + 1);
      LB_ADD_AREA(child_name, child_desc);
    }
    return count;
  }

  /* Root-level test set: more divisions + more plain areas for paging tests. */
  LB_ADD_DIV("BBS Files", "Top-level BBS file collections");
  LB_ADD_DIV("Programming", "Top-level programming collections");
  LB_ADD_DIV("Retro", "Top-level retro computing collections");
  LB_ADD_DIV("Linux", "Linux software and distros");
  LB_ADD_DIV("Networking", "Network utilities and protocols");
  LB_ADD_DIV("Uploads", "User upload staging");

  for (i = 0; i < 20 && count < max_entries; i++)
  {
    char area_name[MAX_ALEN];
    char area_desc[64];
    snprintf(area_name, sizeof(area_name), "area_root_%02d", i + 1);
    snprintf(area_desc, sizeof(area_desc), "Root test file area %d", i + 1);
    LB_ADD_AREA(area_name, area_desc);
  }

#undef LB_ADD_DIV
#undef LB_ADD_AREA

  return count;
}
#endif /* LB_FAREA_TEST */

/**
 * @brief Collect visible file-area entries for the given division context.
 *
 * Uses the same filter logic as the legacy scroll-based listing:
 * root view shows top-level divisions + top-level areas,
 * division view shows immediate children only.
 *
 * @return Number of entries collected.
 */
static int lb_collect_file_areas(char *div_name, lb_farea_entry_t *entries, int max_entries)
{
  BARINFO bi;
  FAH fa;
  HAFF haff;
  int count = 0;

  memset(&fa, 0, sizeof fa);

  if ((haff = AreaFileFindOpen(haf, div_name, AFFO_DIV)) == NULL)
    return 0;

  if (!FoundOurFileDivision(haff, div_name, &fa))
  {
    AreaFileFindReset(haff);
    div_name = NULL;
  }

  AreaFileFindChange(haff, NULL, AFFO_DIV);

  while (count < max_entries && AreaFileFindNext(haff, &fa, FALSE) == 0)
  {
    const char *rec_name = FAS(fa, name);
    int show = FALSE;

    if (fa.fa.attribs & FA_DIVEND)
      continue;

    if (!div_name)
    {
      if (fa.fa.attribs & FA_DIVBEGIN)
        show = PrivOK(FAS(fa, acs), TRUE);
      else if (strchr(rec_name, '.') == NULL)
        show = ValidFileArea(NULL, &fa, VA_NOVAL, &bi);
    }
    else
    {
      size_t dlen = strlen(div_name);

      if (strnicmp(rec_name, div_name, dlen) == 0 && rec_name[dlen] == '.')
      {
        const char *child = rec_name + dlen + 1;

        if (*child && strchr(child, '.') == NULL)
        {
          if (fa.fa.attribs & FA_DIVBEGIN)
            show = PrivOK(FAS(fa, acs), TRUE);
          else
            show = ValidFileArea(NULL, &fa, VA_NOVAL, &bi);
        }
      }
    }

    if (show && (fa.fa.attribs & FA_HIDDN) == 0)
    {
      lb_farea_entry_t *e = &entries[count];
      char raw[PATHLEN];

      strncpy(e->name, rec_name, MAX_ALEN - 1);
      e->name[MAX_ALEN - 1] = '\0';
      e->is_div = !!(fa.fa.attribs & FA_DIVBEGIN);

      ParseCustomFileAreaList(&fa, div_name,
        (char *)ngcfg_get_string_raw("general.display_files.file_format"),
        raw, FALSE);

      /* Strip trailing newline/CR for lightbar row display */
      { size_t len = strlen(raw);
        while (len > 0 && (raw[len-1] == '\n' || raw[len-1] == '\r' ||
               raw[len-1] == '\x0a'))
          raw[--len] = '\0';
      }
      strncpy(e->display, raw, PATHLEN - 1);
      e->display[PATHLEN - 1] = '\0';
      count++;
    }
  }

  AreaFileFindClose(haff);
  DisposeFah(&fa);
  return count;
}

/**
 * @brief Resolve lightbar list boundaries from config with fallback rules.
 *
 * Per lightbar_areas_proposal.md:
 * - top_boundary overrides start_row/start_col
 * - bottom_boundary overrides screen_rows - reduce_area
 * - If both top and bottom are valid, reduce_area is ignored
 * - bottom_col defaults to screen width (span from top_col to screen width)
 */
static void lb_resolve_boundaries(int *out_x, int *out_y, int *out_w, int *out_h,
                                  int start_row, int start_col)
{
  int screen_rows = (int)usr.len;
  int screen_cols = (int)usr.width;
  int reduce_raw  = ngcfg_get_int("general.display.file_areas.reduce_area");
  int reduce      = reduce_raw > 0 ? reduce_raw : 8;
  int top_row = 0, top_col = 0, bot_row = 0, bot_col = 0;

  ngcfg_get_int_array_2("general.display.file_areas.top_boundary", &top_row, &top_col);
  ngcfg_get_int_array_2("general.display.file_areas.bottom_boundary", &bot_row, &bot_col);

  /* Top boundary fallback */
  if (top_row <= 0 || top_col <= 0)
  {
    top_row = start_row > 0 ? start_row : 3;
    top_col = start_col > 0 ? start_col : 1;
  }

  /* Bottom boundary fallback */
  if (bot_row <= 0 || bot_col <= 0)
  {
    bot_row = screen_rows - reduce;
    bot_col = screen_cols;
  }

  /* Safety clamping */
  if (top_row < 1) top_row = 1;
  if (top_col < 1) top_col = 1;
  if (bot_row > screen_rows) bot_row = screen_rows;
  if (bot_col > screen_cols) bot_col = screen_cols;
  if (bot_row < top_row) bot_row = top_row;
  if (bot_col < top_col) bot_col = top_col;

  *out_x = top_col;
  *out_y = top_row;
  *out_w = (bot_col - top_col) + 1;
  *out_h = (bot_row - top_row) + 1;
}

/**
 * @brief Run the lightbar file-area selection loop.
 *
 * Handles division drill-in (Enter on division → rebuild list) and
 * area selection (Enter on area → return name).
 * ESC in a division goes up one level; ESC at root returns 0.
 *
 * @param div_name  Current division context (NULL for root).
 * @param selected_out  Buffer (MAX_ALEN) to receive selected area name.
 * @return 1 if an area was selected, 0 if cancelled.
 */
static int lb_file_area_interact(char *div_name, char *selected_out)
{
  lb_farea_entry_t *entries = NULL;
  lb_farea_ctx_t ctx;
  ui_lightbar_list_t list;
  const char *custom_screen = ngcfg_get_string_raw("general.display.file_areas.custom_screen");
  char headfoot[PATHLEN];
  char current_div[MAX_ALEN] = {0};
  int result;
  int selected_index = 0;
  int last_key = 0;
  int header_row = 0, header_col = 0;
  int footer_row = 0, footer_col = 0;
  int show_header = 0;
  int show_footer = 0;
  byte normal_attr;
  byte selected_attr;

  lb_get_lightbar_attrs(&normal_attr, &selected_attr);

  memset(&ctx, 0, sizeof(ctx));
  ctx.highlight_mode = lb_get_highlight_mode();
  ctx.selected_index_ptr = &selected_index;
  snprintf(ctx.selected_attr_code, sizeof(ctx.selected_attr_code), "|%02x", selected_attr);
  snprintf(ctx.normal_attr_code, sizeof(ctx.normal_attr_code), "|%02x", normal_attr);

  ngcfg_get_int_array_2("general.display.file_areas.header_location", &header_row, &header_col);
  ngcfg_get_int_array_2("general.display.file_areas.footer_location", &footer_row, &footer_col);
  show_header = (header_row > 0 && header_col > 0);
  show_footer = (footer_row > 0 && footer_col > 0);

  if (div_name && *div_name)
    strncpy(current_div, div_name, MAX_ALEN - 1);

  entries = (lb_farea_entry_t *)malloc(LB_FAREA_MAX * sizeof(lb_farea_entry_t));
  if (!entries)
    return 0;

  for (;;)
  {
    char *cdiv = *current_div ? current_div : NULL;
    int did_show_custom_screen = 0;
    int lx, ly, lw, lh;

    /* Collect entries for the current division context */
    ctx.entries = entries;
#ifdef LB_FAREA_TEST
    ctx.count = lb_generate_test_entries(cdiv, entries, LB_FAREA_MAX);
#else
    ctx.count = lb_collect_file_areas(cdiv, entries, LB_FAREA_MAX);
#endif

    if (ctx.count == 0)
    {
      /* No entries to show — go back up or bail */
      if (cdiv)
      {
        char *p = strrchr(current_div, '.');
        if (p) *p = '\0';
        else *current_div = '\0';
        continue;
      }
      break;
    }

    /* Display: clear screen, then optional custom screen/header/footer */
    Puts(CLS);
    display_line = display_col = 1;

    if (custom_screen && *custom_screen)
    {
      const char *dp = ngcfg_get_path("maximus.display_path");
      int df_ret;

      if (debuglog)
        debug_log("lb_file_area: custom_screen='%s' display_path='%s'",
                  custom_screen, dp ? dp : "(null)");

      df_ret = Display_File(0, NULL, "%s%s", (char *)dp, (char *)custom_screen);

      if (debuglog)
        debug_log("lb_file_area: Display_File returned %d", df_ret);

      did_show_custom_screen = (df_ret == 0);
    }

    if (!did_show_custom_screen)
    {
      ParseCustomFileAreaList(NULL, cdiv,
        (char *)ngcfg_get_string_raw("general.display_files.file_header"),
        headfoot, TRUE);

      if (show_header)
      {
        ui_goto(header_row, header_col);
        Puts(headfoot);
      }
      else
      {
        Puts(headfoot);
      }
    }
    vbuf_flush();

    /* Resolve list boundaries (start_row = current display_line) */
    lb_resolve_boundaries(&lx, &ly, &lw, &lh, display_line, 1);

    /* Position cursor below the lightbar region before rendering
     * footer and help so the list doesn't overwrite them. */
    if (!did_show_custom_screen)
    {
      if (!show_footer)
        ui_goto(ly + lh, 1);

      ParseCustomFileAreaList(NULL, cdiv,
        (char *)ngcfg_get_string_raw("general.display_files.file_footer"),
        headfoot, FALSE);

      if (show_footer)
      {
        ui_goto(footer_row, footer_col);
        Puts(headfoot);
      }
      else
      {
        Puts(headfoot);
      }
    }

    /* Suppress built-in help when a custom screen is displayed. */
    if (!did_show_custom_screen)
      Puts(achg_lb_help);
    vbuf_flush();

    /* Configure and run lightbar */
    memset(&list, 0, sizeof(list));
    list.x = lx;
    list.y = ly;
    list.width = lw;
    list.height = lh;
    list.count = ctx.count;
    list.initial_index = selected_index;
    list.selected_index_ptr = &selected_index;
    list.normal_attr = normal_attr;
    list.selected_attr = (ctx.highlight_mode == LB_HILITE_NAME) ? normal_attr : selected_attr;
    list.wrap = 0;
    list.get_item = lb_farea_get_item;
    list.ctx = &ctx;
    list.out_key = &last_key;

    result = ui_lightbar_list_run(&list);

    /* '/' — jump to root level */
    if (result == LB_LIST_KEY_PASSTHROUGH && (last_key == '/' || last_key == '\\'))
    {
      *current_div = '\0';
      selected_index = 0;
      continue;
    }

    /* '.' — go up one division level (same as ESC inside a division) */
    if (result == LB_LIST_KEY_PASSTHROUGH && last_key == '.')
    {
      if (cdiv)
      {
        char *p = strrchr(current_div, '.');
        if (p) *p = '\0';
        else *current_div = '\0';
        selected_index = 0;
      }
      continue;
    }

    /* 'Q'/'q' — quit the lightbar immediately */
    if (result == LB_LIST_KEY_PASSTHROUGH && (last_key == 'q' || last_key == 'Q'))
      break;

    if (result >= 0 && result < ctx.count)
    {
      lb_farea_entry_t *sel = &entries[result];

      if (sel->is_div)
      {
        /* Drill into division */
        strncpy(current_div, sel->name, MAX_ALEN - 1);
        current_div[MAX_ALEN - 1] = '\0';
        continue;
      }
      else
      {
        /* Area selected — park cursor below list region and return */
        ui_goto(ly + lh, 1);
        ui_set_attr(Mci2Attr("|tx", 0x07));
        Puts("\n");
        vbuf_flush();
        strncpy(selected_out, sel->name, MAX_ALEN - 1);
        selected_out[MAX_ALEN - 1] = '\0';
        free(entries);
        return 1;
      }
    }
    else
    {
      /* ESC pressed — go up one division level or exit */
      if (cdiv)
      {
        char *p = strrchr(current_div, '.');
        if (p) *p = '\0';
        else *current_div = '\0';
        continue;
      }
      break;
    }
  }

  /* Park cursor at bottom of screen and reset attribute before returning.
   * Return -1 to signal the caller that the user cancelled (ESC at root),
   * as opposed to legacy-scroll returning 0 (list shown, re-prompt). */
  ui_goto((int)usr.len, 1);
  ui_set_attr(Mci2Attr("|tx", 0x07));
  Puts("\n");
  vbuf_flush();

  free(entries);
  return -1;
}

/* ============================================================================
 * ListFileAreas — main entry point (legacy scroll + lightbar dispatch)
 * ============================================================================ */

int ListFileAreas(char *div_name, int show_help, char *selected_out)
{
  BARINFO bi;
  FAH fa;
  HAFF haff=0;
  char nonstop=FALSE;
  int shown_from_file=FALSE;
  char headfoot[PATHLEN];
  char *file;

  /* Normalize empty division name to NULL so callers passing "" get a
   * flat area list instead of an empty division-filtered one. */
  if (div_name && !*div_name)
    div_name = NULL;

  if (debuglog)
    debug_log("ListFileAreas: entry div_name='%s' show_help=%d haf=%p",
              div_name ? div_name : "(null)", (int)show_help, (void *)haf);

  /* Lightbar mode: if enabled and caller can accept a selection, run the
   * interactive lightbar list instead of the legacy scroll-based listing. */
  if (selected_out && ngcfg_get_bool("general.display.file_areas.lightbar_area"))
    return lb_file_area_interact(div_name, selected_out);

  memset(&fa, 0, sizeof fa);

  {
    const char *file_area_list = ngcfg_get_path("general.display_files.file_area_list");

    if (debuglog)
      debug_log("ListFileAreas: file_area_list='%s' (ptr=%p, empty=%d)",
                file_area_list ? file_area_list : "(null)",
                (void *)file_area_list,
                file_area_list ? (*file_area_list == '\0') : -1);

    if (file_area_list && *file_area_list)
    {
      if (!div_name || *div_name==0 ||
          (haff=AreaFileFindOpen(haf, div_name, AFFO_DIV))==NULL ||
          !FoundOurFileDivision(haff, div_name, &fa) ||
          eqstri(FAS(fa, filesbbs), dot))
      {
        file=(char *)file_area_list;
      }
      else
      {
        file=FAS(fa, filesbbs);
      }

      if (debuglog)
        debug_log("ListFileAreas: attempting Display_File file='%s'", file);

      /* If the configured area-list file cannot be shown, fall back to
       * dynamic list generation so '?' always displays something. */
      shown_from_file=(Display_File(0, NULL, file)==0);

      if (debuglog)
        debug_log("ListFileAreas: Display_File returned shown_from_file=%d", shown_from_file);
    }

    if (!shown_from_file)
    {
    Puts(CLS);

    display_line=display_col=1;


    ParseCustomFileAreaList(NULL, div_name, (char *)ngcfg_get_string_raw("general.display_files.file_header"), headfoot, TRUE);
    Puts(headfoot);

    if ((haff=AreaFileFindOpen(haf, div_name, AFFO_DIV))==NULL)
    {
      if (debuglog)
        debug_log("ListFileAreas: AreaFileFindOpen FAILED div_name='%s' haf=%p",
                  div_name ? div_name : "(null)", (void *)haf);
      return 0;
    }

    /* Ensure that we have found the beginning of our division */

    if (!FoundOurFileDivision(haff, div_name, &fa))
    {
      if (debuglog)
        debug_log("ListFileAreas: FoundOurFileDivision failed div_name='%s' -> reset to flat",
                  div_name ? div_name : "(null)");
      AreaFileFindReset(haff);
      div_name = NULL;
    }

    {
      /* Now find anything after the current division */

      AreaFileFindChange(haff, NULL, AFFO_DIV);

      { int iter = 0, printed = 0;
      while (AreaFileFindNext(haff, &fa, FALSE)==0)
      {
        const char *rec_name = FAS(fa, name);
        int show = FALSE;
        iter++;
        if (debuglog && iter <= 200)
          debug_log("ListFileAreas: rec name='%s' attribs=0x%x division=%d this_div=%d div_name='%s'",
                    FAS(fa, name),
                    (unsigned)fa.fa.attribs,
                    (int)fa.fa.division,
                    -1,
                    div_name ? div_name : "(null)");

        if (fa.fa.attribs & FA_DIVEND)
          continue;

        /* Root view: show only top-level divisions and top-level areas. */
        if (!div_name)
        {
          if (fa.fa.attribs & FA_DIVBEGIN)
            show = PrivOK(FAS(fa, acs), TRUE);
          else if (strchr(rec_name, '.') == NULL)
            show = ValidFileArea(NULL, &fa, VA_NOVAL, &bi);
        }
        else
        {
          size_t dlen = strlen(div_name);

          /* Division view: show only immediate children of this division. */
          if (strnicmp(rec_name, div_name, dlen) == 0 && rec_name[dlen] == '.')
          {
            const char *child = rec_name + dlen + 1;

            if (*child && strchr(child, '.') == NULL)
            {
              if (fa.fa.attribs & FA_DIVBEGIN)
                show = PrivOK(FAS(fa, acs), TRUE);
              else
                show = ValidFileArea(NULL, &fa, VA_NOVAL, &bi);
            }
          }
        }

        if (show && (fa.fa.attribs & FA_HIDDN)==0)
        {
          printed++;
          ParseCustomFileAreaList(&fa, div_name, (char *)ngcfg_get_string_raw("general.display_files.file_format"), headfoot, FALSE);

          Puts(headfoot);
          vbuf_flush();
        }

        if (halt() || (printed > 0 && MoreYnBreak(&nonstop, CYAN)))
          break;
      }
      if (debuglog)
        debug_log("ListFileAreas: done iter=%d printed=%d div_name='%s' this_div=%d",
                  iter, printed, div_name ? div_name : "(null)", -1);
      }
    }


    ParseCustomFileAreaList(NULL, div_name, (char *)ngcfg_get_string_raw("general.display_files.file_footer"), headfoot, FALSE);
    Puts(headfoot);

    Putc('\n');

    /* If necessary, display help for changing areas */

    if (show_help)
      Puts(achg_help);

    vbuf_flush();
    }
  }
  if (haff)
    AreaFileFindClose(haff);

  DisposeFah(&fa);
  return 0;
}


char *FileSection(char *current, char *szDest)
{
  char *p;

  strcpy(szDest, current);

  if ((p=strrchr(szDest, '.')) != NULL)
    *p=0;
  else
    *szDest=0;

  return szDest;
}


