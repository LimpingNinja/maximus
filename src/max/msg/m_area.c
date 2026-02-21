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
#include "ui_lightbar.h"


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
    {
      char sel[MAX_ALEN] = {0};
      int ret = ListMsgAreas(group, FALSE, !!*group, sel);
      if (ret > 0 && *sel)
      {
        SetAreaName(usr.msg, sel);
        return TRUE;
      }
      else if (ret < 0)
        return TRUE;
    }
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
      DisposeMah(&ma);
      {
        char sel[MAX_ALEN] = {0};
        int ret = ListMsgAreas(group, FALSE, !!*group, sel);
        if (ret > 0 && *sel)
        {
          SetAreaName(usr.msg, sel);
          return TRUE;
        }
        else if (ret < 0)
          return TRUE;
      }
      return FALSE;
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
        int ret = ListMsgAreas(group, FALSE, !!*group, sel);
        if (ret > 0 && *sel)
        {
          SetAreaName(usr.msg, sel);
          CopyMsgArea(pmah, &mah);
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
        int ret = ListMsgAreas(group, FALSE, !!*group, sel);
        if (ret > 0 && *sel)
        {
          SetAreaName(usr.msg, sel);
          CopyMsgArea(pmah, &mah);
          return did_valid;
        }
        else if (ret < 0)
          return did_valid;
      }
    }
    else if (*input==keys[2] || *input=='?')
    {
      strcpy(linebuf, input+1);
      {
        char sel[MAX_ALEN] = {0};
        int ret = ListMsgAreas(group, FALSE, !!*group, sel);
        if (ret > 0 && *sel)
        {
          SetAreaName(usr.msg, sel);
          CopyMsgArea(pmah, &mah);
          return did_valid;
        }
        else if (ret < 0)
          return did_valid;
      }
    }
    else if (*input=='=')
    {
      char sel[MAX_ALEN] = {0};
      int ret = ListMsgAreas(NULL, FALSE, FALSE, sel);
      if (ret > 0 && *sel)
      {
        SetAreaName(usr.msg, sel);
        CopyMsgArea(pmah, &mah);
        return did_valid;
      }
      else if (ret < 0)
        return did_valid;
    }
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


/* ============================================================================
 * Lightbar message-area list helpers
 * ============================================================================ */

#define LB_MAREA_MAX 200

/* Uncomment the next line to fill the lightbar with synthetic test data
 * instead of real message areas.  Rebuild and run — no areas needed. */
#define LB_MAREA_TEST

/** @brief One entry in the collected lightbar list. */
typedef struct {
  char name[MAX_ALEN];      /**< Full qualified area name */
  char display[PATHLEN];    /**< Formatted display string (no trailing newline) */
  int  is_div;              /**< Non-zero if this is a MA_DIVBEGIN entry */
  int  tag_ch;              /**< Tag character for %* token: '*', '@', or ' ' */
} lb_marea_entry_t;

/** @brief Context passed to the lightbar get_item callback. */
typedef struct {
  lb_marea_entry_t *entries;
  int count;
  int *selected_index_ptr;
  int highlight_mode;
  char selected_attr_code[8];
  char normal_attr_code[8];
} lb_marea_ctx_t;

enum {
  LB_MA_HILITE_ROW  = 0,
  LB_MA_HILITE_FULL = 1,
  LB_MA_HILITE_NAME = 2
};

/**
 * @brief Parse a color nibble (0..15) from a color name or numeric token.
 */
static int lb_ma_parse_color_nibble(const char *s, int *out_nibble)
{
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

  char *end = NULL;
  long v = strtol(s, &end, 16);
  if (end && *end == '\0' && v >= 0 && v <= 15)
  {
    *out_nibble = (int)v;
    return 1;
  }

  return 0;
}

/**
 * @brief Resolve configured highlight mode for message-area lightbar selection.
 */
static int lb_ma_get_highlight_mode(void)
{
  const char *mode = ngcfg_get_string_raw("general.display.msg_areas.lightbar_what");

  if (mode && *mode)
  {
    if (eqstri((char *)mode, "full"))
      return LB_MA_HILITE_FULL;
    if (eqstri((char *)mode, "name"))
      return LB_MA_HILITE_NAME;
  }

  return LB_MA_HILITE_ROW;
}

/**
 * @brief Build lightbar attrs with configurable foreground/background overrides.
 *
 * Defaults:
 * - Normal row: theme text fallback (0x07)
 * - Selected row background: theme lightbar background fallback (|17)
 * - Selected row foreground: inherited from normal row unless overridden
 */
static void lb_ma_get_lightbar_attrs(byte *normal_attr, byte *selected_attr)
{
  byte normal = Mci2Attr("|tx", 0x07);
  byte bg_default = Mci2Attr("|17", 0x17);
  const char *fore = ngcfg_get_string_raw("general.display.msg_areas.lightbar_fore");
  const char *back = ngcfg_get_string_raw("general.display.msg_areas.lightbar_back");
  int fore_nibble = -1;
  int back_nibble = -1;

  if (fore && *fore)
    lb_ma_parse_color_nibble(fore, &fore_nibble);
  if (back && *back)
    lb_ma_parse_color_nibble(back, &back_nibble);

  byte selected = (byte)((normal & 0x0f) | (bg_default & 0x70));

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
static void lb_ma_apply_name_highlight(const lb_marea_ctx_t *c,
                                       const lb_marea_entry_t *e,
                                       char *out,
                                       size_t out_sz)
{
  if (!c || !e || !out || out_sz == 0)
    return;

  if (c->highlight_mode != LB_MA_HILITE_NAME || !c->selected_index_ptr)
    return;

  const char *p = strstr(out, e->name);
  if (!p)
    return;

  size_t prefix_len = (size_t)(p - out);
  size_t name_len = strlen(e->name);

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

/**
 * @brief Lightbar get_item callback — copies pre-formatted display string.
 */
static int lb_marea_get_item(void *ctx, int index, char *out, size_t out_sz)
{
  lb_marea_ctx_t *c = (lb_marea_ctx_t *)ctx;

  if (!c || index < 0 || index >= c->count)
    return -1;

  strncpy(out, c->entries[index].display, out_sz - 1);
  out[out_sz - 1] = '\0';

  if (c->selected_index_ptr && index == *c->selected_index_ptr)
    lb_ma_apply_name_highlight(c, &c->entries[index], out, out_sz);

  return 0;
}

#ifdef LB_MAREA_TEST
/**
 * @brief Fill entries with synthetic test data for message-area lightbar UI.
 *
 * Generates a mix of fake divisions and message areas so the lightbar
 * display, paging, footer, tagging, and drill-in can be tested without
 * real message areas configured.  Division drill-in is supported
 * multiple levels deep.
 */
static int lb_generate_msg_test_entries(char *div_name, int do_tag,
                                        lb_marea_entry_t *entries,
                                        int max_entries)
{
  int count = 0;

#define LB_MADD_DIV(_name, _desc)                                              \
  do {                                                                          \
    if (count < max_entries) {                                                  \
      lb_marea_entry_t *e = &entries[count++];                                  \
      strncpy(e->name, (_name), MAX_ALEN - 1);                                  \
      e->name[MAX_ALEN - 1] = '\0';                                             \
      snprintf(e->display, PATHLEN,                                              \
               "|tx |tx[|hddiv |tx]|pr %-20s |tx... %s|cd",                    \
               (_name), (_desc));                                                \
      e->is_div = 1;                                                             \
      e->tag_ch = ' ';                                                           \
    }                                                                            \
  } while (0)

#define LB_MADD_AREA(_name, _desc, _tagged)                                    \
  do {                                                                          \
    if (count < max_entries) {                                                  \
      lb_marea_entry_t *e = &entries[count++];                                  \
      strncpy(e->name, (_name), MAX_ALEN - 1);                                  \
      e->name[MAX_ALEN - 1] = '\0';                                             \
      e->tag_ch = do_tag ? ((_tagged) ? '@' : ' ') : '*';                       \
      snprintf(e->display, PATHLEN,                                              \
               "|tx%c|tx[|hdarea|tx]|pr %-20s |tx... %s|cd",                   \
               e->tag_ch, (_name), (_desc));                                    \
      e->is_div = 0;                                                             \
    }                                                                            \
  } while (0)

  if (div_name && *div_name)
  {
    /* --- Echomail sub-areas --- */
    if (eqstri(div_name, "Echomail"))
    {
      LB_MADD_DIV("Echomail.FidoNet", "FidoNet echomail conferences");
      LB_MADD_DIV("Echomail.RetroNet", "RetroNet echomail conferences");
      LB_MADD_AREA("Echomail.Announce", "Network announcements", 1);
      return count;
    }

    if (eqstri(div_name, "Echomail.FidoNet"))
    {
      LB_MADD_DIV("Echomail.FidoNet.Tech", "FidoNet technical echoes");
      LB_MADD_AREA("Echomail.FidoNet.Chat", "FidoNet general chat", 1);
      LB_MADD_AREA("Echomail.FidoNet.BBS", "FidoNet BBS discussion", 0);
      LB_MADD_AREA("Echomail.FidoNet.Sysop", "FidoNet sysop echo", 0);
      return count;
    }

    if (eqstri(div_name, "Echomail.FidoNet.Tech"))
    {
      LB_MADD_AREA("Echomail.FidoNet.Tech.C_Echo", "C programming echo", 1);
      LB_MADD_AREA("Echomail.FidoNet.Tech.Pascal", "Pascal programming echo", 0);
      LB_MADD_AREA("Echomail.FidoNet.Tech.Unix", "Unix echo", 0);
      LB_MADD_AREA("Echomail.FidoNet.Tech.HAM", "Ham radio echo", 0);
      return count;
    }

    if (eqstri(div_name, "Echomail.RetroNet"))
    {
      LB_MADD_AREA("Echomail.RetroNet.General", "RetroNet general", 1);
      LB_MADD_AREA("Echomail.RetroNet.DOS", "RetroNet DOS discussion", 0);
      LB_MADD_AREA("Echomail.RetroNet.Coding", "RetroNet coding", 0);
      return count;
    }

    /* --- Local sub-areas --- */
    if (eqstri(div_name, "Local"))
    {
      LB_MADD_DIV("Local.General", "General local discussion");
      LB_MADD_DIV("Local.Trading", "Buy/sell/trade");
      LB_MADD_AREA("Local.Sysop", "Sysop-only local area", 0);
      LB_MADD_AREA("Local.Feedback", "User feedback", 1);
      return count;
    }

    if (eqstri(div_name, "Local.General"))
    {
      LB_MADD_AREA("Local.General.Chat", "General chat area", 1);
      LB_MADD_AREA("Local.General.Intro", "New user introductions", 0);
      LB_MADD_AREA("Local.General.Off-Topic", "Off-topic discussion", 0);
      return count;
    }

    /* --- Netmail sub-areas --- */
    if (eqstri(div_name, "Netmail"))
    {
      LB_MADD_AREA("Netmail.FidoNet", "FidoNet netmail", 0);
      LB_MADD_AREA("Netmail.RetroNet", "RetroNet netmail", 0);
      return count;
    }

    /* Generic fallback for unknown divisions */
    for (int i = 0; i < 6 && count < max_entries; i++)
    {
      char child_name[MAX_ALEN];
      char child_desc[64];
      snprintf(child_name, sizeof(child_name), "%s.child%d", div_name, i + 1);
      snprintf(child_desc, sizeof(child_desc), "Fallback msg area %d", i + 1);
      LB_MADD_AREA(child_name, child_desc, (i % 3 == 0));
    }
    return count;
  }

  /* Root-level test set */
  LB_MADD_DIV("Echomail", "Echomail conferences");
  LB_MADD_DIV("Local", "Local message areas");
  LB_MADD_DIV("Netmail", "Private netmail");

  for (int i = 0; i < 15 && count < max_entries; i++)
  {
    char area_name[MAX_ALEN];
    char area_desc[64];
    snprintf(area_name, sizeof(area_name), "msg_root_%02d", i + 1);
    snprintf(area_desc, sizeof(area_desc), "Root test msg area %d", i + 1);
    LB_MADD_AREA(area_name, area_desc, (i % 4 == 0));
  }

#undef LB_MADD_DIV
#undef LB_MADD_AREA

  return count;
}
#endif /* LB_MAREA_TEST */

/**
 * @brief Collect visible message-area entries for the given division context.
 *
 * Uses name-prefix filtering (consistent with file-area lightbar) rather
 * than the legacy numeric division-level approach.
 *
 * @return Number of entries collected.
 */
static int lb_collect_msg_areas(char *div_name, int do_tag,
                                lb_marea_entry_t *entries, int max_entries)
{
  BARINFO bi;
  MAH ma;
  HAFF haff;
  int count = 0;

  memset(&ma, 0, sizeof ma);

  if ((haff = AreaFileFindOpen(ham, div_name, AFFO_DIV)) == NULL)
    return 0;

  if (!FoundOurMsgDivision(haff, div_name, &ma))
  {
    AreaFileFindReset(haff);
    div_name = NULL;
  }

  AreaFileFindChange(haff, NULL, AFFO_DIV);

  while (count < max_entries && AreaFileFindNext(haff, &ma, FALSE) == 0)
  {
    const char *rec_name = MAS(ma, name);
    int show = FALSE;

    if (ma.ma.attribs & MA_DIVEND)
      continue;

    /* Name-prefix filtering (consistent with file-area lightbar) */
    if (!div_name)
    {
      if (ma.ma.attribs & MA_DIVBEGIN)
        show = PrivOK(MAS(ma, acs), TRUE);
      else if (strchr(rec_name, '.') == NULL)
        show = ValidMsgArea(NULL, &ma, VA_NOVAL, &bi);
    }
    else
    {
      size_t dlen = strlen(div_name);

      if (strnicmp(rec_name, div_name, dlen) == 0 && rec_name[dlen] == '.')
      {
        const char *child = rec_name + dlen + 1;

        if (*child && strchr(child, '.') == NULL)
        {
          if (ma.ma.attribs & MA_DIVBEGIN)
            show = PrivOK(MAS(ma, acs), TRUE);
          else
            show = ValidMsgArea(NULL, &ma, VA_NOVAL, &bi);
        }
      }
    }

    if (show && (ma.ma.attribs & MA_HIDDN) == 0)
    {
      lb_marea_entry_t *e = &entries[count];
      char raw[PATHLEN];

      strncpy(e->name, rec_name, MAX_ALEN - 1);
      e->name[MAX_ALEN - 1] = '\0';
      e->is_div = !!(ma.ma.attribs & MA_DIVBEGIN);

      /* Resolve tag character */
      int ch = !do_tag ? '*' : TagQueryTagList(&mtm, MAS(ma, name)) ? '@' : ' ';
      e->tag_ch = ch;

      ParseCustomMsgAreaList(&ma, div_name,
        (char *)ngcfg_get_string_raw("general.display_files.msg_format"),
        raw, FALSE, ch);

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
  DisposeMah(&ma);
  return count;
}

/**
 * @brief Resolve lightbar list boundaries from config with fallback rules.
 *
 * Reads from general.display.msg_areas.* config keys.
 */
static void lb_ma_resolve_boundaries(int *out_x, int *out_y, int *out_w, int *out_h,
                                     int start_row, int start_col)
{
  int screen_rows = (int)usr.len;
  int screen_cols = (int)usr.width;
  int reduce_raw  = ngcfg_get_int("general.display.msg_areas.reduce_area");
  int reduce      = reduce_raw > 0 ? reduce_raw : 8;
  int top_row = 0, top_col = 0, bot_row = 0, bot_col = 0;

  ngcfg_get_int_array_2("general.display.msg_areas.top_boundary", &top_row, &top_col);
  ngcfg_get_int_array_2("general.display.msg_areas.bottom_boundary", &bot_row, &bot_col);

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
 * @brief Run the lightbar message-area selection loop.
 *
 * Handles division drill-in (Enter on division → rebuild list) and
 * area selection (Enter on area → return name).
 * ESC in a division goes up one level; ESC at root returns 0.
 *
 * @param div_name     Current division context (NULL for root).
 * @param do_tag       Non-zero to show tag indicators.
 * @param selected_out Buffer (MAX_ALEN) to receive selected area name.
 * @return 1 if an area was selected, 0 if cancelled.
 */
static int lb_msg_area_interact(char *div_name, int do_tag, char *selected_out)
{
  lb_marea_entry_t *entries = NULL;
  lb_marea_ctx_t ctx;
  ui_lightbar_list_t list;
  const char *custom_screen = ngcfg_get_string_raw("general.display.msg_areas.custom_screen");
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

  lb_ma_get_lightbar_attrs(&normal_attr, &selected_attr);

  memset(&ctx, 0, sizeof(ctx));
  ctx.highlight_mode = lb_ma_get_highlight_mode();
  ctx.selected_index_ptr = &selected_index;
  snprintf(ctx.selected_attr_code, sizeof(ctx.selected_attr_code), "|%02x", selected_attr);
  snprintf(ctx.normal_attr_code, sizeof(ctx.normal_attr_code), "|%02x", normal_attr);

  ngcfg_get_int_array_2("general.display.msg_areas.header_location", &header_row, &header_col);
  ngcfg_get_int_array_2("general.display.msg_areas.footer_location", &footer_row, &footer_col);
  show_header = (header_row > 0 && header_col > 0);
  show_footer = (footer_row > 0 && footer_col > 0);

  if (div_name && *div_name)
    strncpy(current_div, div_name, MAX_ALEN - 1);

  entries = (lb_marea_entry_t *)malloc(LB_MAREA_MAX * sizeof(lb_marea_entry_t));
  if (!entries)
    return 0;

  for (;;)
  {
    char *cdiv = *current_div ? current_div : NULL;
    int did_show_custom_screen = 0;
    int lx, ly, lw, lh;

    /* Collect entries for the current division context */
    ctx.entries = entries;
#ifdef LB_MAREA_TEST
    ctx.count = lb_generate_msg_test_entries(cdiv, do_tag, entries, LB_MAREA_MAX);
#else
    ctx.count = lb_collect_msg_areas(cdiv, do_tag, entries, LB_MAREA_MAX);
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
        debug_log("lb_msg_area: custom_screen='%s' display_path='%s'",
                  custom_screen, dp ? dp : "(null)");

      df_ret = Display_File(0, NULL, "%s%s", (char *)dp, (char *)custom_screen);

      if (debuglog)
        debug_log("lb_msg_area: Display_File returned %d", df_ret);

      did_show_custom_screen = (df_ret == 0);
    }

    if (!did_show_custom_screen)
    {
      ParseCustomMsgAreaList(NULL, cdiv,
        (char *)ngcfg_get_string_raw("general.display_files.msg_header"),
        headfoot, TRUE, '*');

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
    lb_ma_resolve_boundaries(&lx, &ly, &lw, &lh, display_line, 1);

    /* Position cursor below the lightbar region before rendering
     * footer and help so the list doesn't overwrite them. */
    if (!did_show_custom_screen)
    {
      if (!show_footer)
        ui_goto(ly + lh, 1);

      ParseCustomMsgAreaList(NULL, cdiv,
        (char *)ngcfg_get_string_raw("general.display_files.msg_footer"),
        headfoot, FALSE, '*');

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
    list.selected_attr = (ctx.highlight_mode == LB_MA_HILITE_NAME) ? normal_attr : selected_attr;
    list.wrap = 0;
    list.get_item = lb_marea_get_item;
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
      lb_marea_entry_t *sel = &entries[result];

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
 * ListMsgAreas — main entry point (legacy scroll + lightbar dispatch)
 * ============================================================================ */

int ListMsgAreas(char *div_name, int do_tag, int show_help, char *selected_out)
{
  BARINFO bi;
  MAH ma;
  HAFF haff=0;
  char nonstop=FALSE;
  char headfoot[PATHLEN];
  char *file;
  int ch;
  const char *msg_area_list;

  /* Normalize empty division name to NULL */
  if (div_name && !*div_name)
    div_name = NULL;

  if (debuglog)
    debug_log("ListMsgAreas: entry div_name='%s' do_tag=%d show_help=%d ham=%p usr.msg='%s'",
              div_name ? div_name : "(null)",
              (int)do_tag,
              (int)show_help,
              (void *)ham,
              usr.msg ? (char *)usr.msg : "(null)");

  /* Lightbar mode: if enabled and caller can accept a selection, run the
   * interactive lightbar list instead of the legacy scroll-based listing. */
  if (selected_out && ngcfg_get_bool("general.display.msg_areas.lightbar_area"))
    return lb_msg_area_interact(div_name, do_tag, selected_out);

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
      return 0;
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
  return 0;
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


