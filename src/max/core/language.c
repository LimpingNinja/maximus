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
static char rcs_id[]="$Id: language.c,v 1.5 2004/01/27 21:00:30 paltas Exp $";
#pragma on(unreferenced)
#endif

#ifdef INTERNAL_LANGUAGES

  #define INITIALIZE_LANGUAGE

#define MAX_LANG_global
#define MAX_LANG_m_area
#define MAX_LANG_max_chat
#define MAX_LANG_max_chng
  #include "prog.h"
  #include "mm.h"

  void Chg_Language(void) {}

#else /* external languages */

#define MAX_LANG_global
#define MAX_LANG_max_chat
#define MAX_LANG_max_chng
#define MAX_LANG_end
#include <string.h>
#include <stdlib.h>
#include "libmaxcfg.h"
#include "maxlang.h"
#include "alc.h"
#include "prog.h"
#include "mm.h"
#include "language.h"
#include "mci.h"

static int using_alternate=0;

/** @brief Static storage for the loaded theme color table. */
static MaxCfgThemeColors s_theme_colors;

/**
 * @brief Load theme colors from colors.toml (via ng_cfg) and wire up g_mci_theme.
 *
 * Called once during Initialize_Languages().  If the [theme.colors] section
 * is missing from ng_cfg, the built-in defaults are used instead.
 */
static void near load_theme_colors(void)
{
  if (!ng_cfg)
    return;

  MaxCfgStatus st = maxcfg_theme_load_from_toml(&s_theme_colors, ng_cfg,
                                                 "general.colors");
  if (st == MAXCFG_OK)
  {
    g_mci_theme = &s_theme_colors;
    logit(">Theme colors loaded: %s", s_theme_colors.name);
  }
  else
  {
    /* Fall back to compiled-in defaults */
    maxcfg_theme_init(&s_theme_colors);
    g_mci_theme = &s_theme_colors;
    logit("!Theme color load failed (st=%d), using defaults", (int)st);
  }
}

/**
 * @brief Global TOML-based language handle.
 *
 * All string retrieval (s_ret, s_reth, english.h macros) resolves through
 * this handle via the maxlang API.  The legacy .ltf binary heap system has
 * been removed.
 *
 * Modifications Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 */
MaxLang *g_current_lang = NULL;

/** @brief Look up a language file name from the TOML config by index. */
static const char *near ngcfg_lang_file_name(byte idx)
{
  MaxCfgVar v;
  MaxCfgVar it;
  size_t cnt = 0;

  if (ng_cfg && maxcfg_toml_get(ng_cfg, "general.language.lang_file", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING_ARRAY)
  {
    if (maxcfg_var_count(&v, &cnt) == MAXCFG_OK && idx < cnt)
    {
      if (maxcfg_toml_array_get(&v, (size_t)idx, &it) == MAXCFG_OK && it.type == MAXCFG_VAR_STRING && it.v.s && *it.v.s)
        return it.v.s;
    }
  }

  return "";
}


/* Update global variables containing often-used strings so that we don't
 * have to go through the language file to get them.
 */

static void near UpdateStaticStrings(void)
{
  cYes=*Yes;
  cNo=*No;
  cNonStop=*M_nonstop;

  if (szHeyDude)
    free(szHeyDude);

  if (szPageMsg)
    free(szPageMsg);

  szHeyDude = strdup(ch_hey_dude_msg);
  szPageMsg = strdup(ch_page_msg);
}

 /* Sets/unset indicator for use of alternate strings in the current heap
  * This current scheme allows for only 1 alternate string set for any
  * heap, but can be expanded later if needed, in which case 'usealt'
  * becomes and index rather than 1 (alternate) or 0 (standard) as it is now
  */

void Set_Lang_Alternate(int usealt)
{
  using_alternate=(usealt && !local) ? TRUE : FALSE;

  /* Mirror RIP alternate state into the TOML language handle */
  if (g_current_lang)
    maxlang_set_use_rip(g_current_lang, using_alternate ? true : false);
}

#ifndef ORACLE
  void LanguageCleanup(void)
  {
    if (g_current_lang)
    {
      maxlang_close(g_current_lang);
      g_current_lang = NULL;
    }
  }
#endif

/** @brief Open the TOML language file for the given language name. */
static void near open_toml_lang(const char *lang_name)
{
  if (g_current_lang)
  {
    maxlang_close(g_current_lang);
    g_current_lang = NULL;
  }

  char full[PATHLEN];
  const char *lang_dir = ngcfg_get_path("maximus.lang_path");
  snprintf(full, sizeof(full), "%s/%s.toml", lang_dir, lang_name);
  MaxCfgStatus mlst = maxlang_open(full, &g_current_lang);
  if (mlst == MAXCFG_OK)
    logit(">TOML language loaded: %s", full);
  else
  {
    logit("!TOML language load FAILED: path='%s' status=%d", full, (int)mlst);
    g_current_lang = NULL;
  }
}

/* Initialize the language system (TOML only) */

void Initialize_Languages(void)
{
  if (usr.lang > ngcfg_get_int("general.language.max_lang"))
    usr.lang=0;

  const char *user_lang = ngcfg_lang_file_name(usr.lang);
  if (!*user_lang)
  {
    user_lang = ngcfg_lang_file_name(usr.lang = 0);
    if (!*user_lang)
    {
      logit("!No language files configured");
      quit(ERROR_CRITICAL);
    }
  }

  open_toml_lang(user_lang);

  if (!g_current_lang)
  {
    logit("!Failed to load TOML language '%s'", user_lang);
    quit(ERROR_CRITICAL);
  }

  /* Bootstrap theme colors from colors.toml (via ng_cfg) */
  load_theme_colors();

  UpdateStaticStrings();
}


/* Retrieve a given string number from a named heap */

char *s_reth(char *hname, word strn)
{
  if (g_current_lang)
  {
    const char *ts = maxlang_get_by_heap_id(g_current_lang, hname, (int)strn);
    if (ts && *ts)
      return (char *)ts;
  }
  return "";
}


/* Retrieve a given string number from one of the three main heaps */

char *s_ret(word strn)
{
  if (g_current_lang)
  {
    const char *ts = maxlang_get_by_id(g_current_lang, (int)strn);
    if (ts && *ts)
      return (char *)ts;
  }
  return "";
}

/* Change the user's language to a new one, and reload lang heaps */

void Switch_To_Language(void)
{
  const char *user_lang = ngcfg_lang_file_name(usr.lang);
  if (!*user_lang)
  {
    user_lang = ngcfg_lang_file_name(usr.lang = 0);
    if (!*user_lang)
    {
      logit("!No language files configured for switch");
      quit(ERROR_CRITICAL);
    }
  }

  open_toml_lang(user_lang);

  if (!g_current_lang)
    logit("!TOML language reload FAILED for '%s'", user_lang);

  UpdateStaticStrings();
}


/* Prompt the user to change to a new language */

int Get_Language(void)
{
  char temp[PATHLEN];
  byte lng;

  do
  {
    if (! *linebuf)
    {
      Puts(select_lang);

      for (lng=0; lng < MAX_LANG; lng++)
      {
        const char *lname = ngcfg_lang_file_name(lng);
        if (*lname)
        {
          /* Open the TOML to get the display name */
          char full[PATHLEN];
          const char *lang_dir = ngcfg_get_path("maximus.lang_path");
          snprintf(full, sizeof(full), "%s/%s.toml", lang_dir, lname);

          MaxLang *probe = NULL;
          if (maxlang_open(full, &probe) == MAXCFG_OK && probe)
          {
            const char *dname = maxlang_get_name(probe);
            if (dname && *dname)
            {
              char _tb[16];
              snprintf(_tb, sizeof(_tb), "%d", lng+1);
              LangPrintf(list_option, _tb, (char *)dname);
            }
            maxlang_close(probe);
          }
        }
      }
    }

    WhiteN();

    InputGets(temp, select_p);

    lng=(byte)atoi(temp);

    if (!lng)
      return -1;

    lng--;
  }
  while (lng >= MAX_LANG || *ngcfg_lang_file_name(lng)=='\0');

  return lng;
}


/* Set the user's default language */

void Chg_Language(void)
{
  int lang;

  if ((lang=Get_Language())==-1)
    return;

  usr.lang=lang;

  Switch_To_Language();

  if (*language_change)
  {
    Puts(language_change);
    Press_ENTER();
  }
}


#ifndef ORACLE
  /* No-op: heap save/restore was .ltf-specific */
  int Language_Save_Heap(void) { return 0; }
  void Language_Restore_Heap(int h) { (void)h; }
#endif

#endif  /* !INTERNAL_LANGUAGES */


