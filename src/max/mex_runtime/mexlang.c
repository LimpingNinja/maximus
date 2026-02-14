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
 *
 * Modifications Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 * https://github.com/LimpingNinja
 */

#define MAX_LANG_global
#define MAX_LANG_m_area
#include "mexall.h"

/* Forward declarations from maxlang API — we avoid #include "maxlang.h" here
 * because mexall.h pulls in english.lth macros that collide with libmaxcfg.h
 * struct field names (e.g. file_offline → s_ret(...)). */
struct MaxLang;
extern const char *maxlang_get(struct MaxLang *lang, const char *key);
extern const char *maxlang_get_rip(struct MaxLang *lang, const char *key);

#ifdef MEX

#ifndef INTERNAL_LANGUAGES

  /**
   * @brief MEX intrinsic: lstr(int index) -> string
   *
   * @deprecated Uses legacy numeric IDs via s_ret() which fall back to
   * the .ltf binary heap.  MEX scripts should migrate to lang_get()
   * with dotted TOML keys (e.g. lang_get("global.located")).
   */
  word EXPENTRY intrin_lang_string(void)
  {
    MA ma;
    int stringnum;

    MexArgBegin(&ma);
    stringnum=MexArgGetWord(&ma);

    MexReturnString(s_ret(stringnum));

    return MexArgEnd(&ma);
  }

  /**
   * @brief MEX intrinsic: hstr(string heapname, int index) -> string
   *
   * @deprecated Uses legacy heap-based numeric IDs via s_reth() which
   * fall back to the .ltf binary heap.  MEX scripts should migrate to
   * lang_get() with dotted TOML keys.
   */
  word EXPENTRY intrin_lang_heap_string(void)
  {
    MA ma;
    char *psz;
    int stringnum;

    MexArgBegin(&ma);
    psz=MexArgGetString(&ma, TRUE);
    stringnum=MexArgGetWord(&ma);
    if (!psz)
      MexReturnString("");
    else
    {
      MexReturnString(s_reth(psz,stringnum));
      free(psz);
    }

    return MexArgEnd(&ma);
  }

  /**
   * @brief MEX intrinsic: lang_get(string key) -> string
   *
   * Retrieves a language string by dotted key (e.g. "global.located")
   * from the TOML language file via the maxlang API.
   * Falls back to empty string if no TOML language is loaded.
   */
  word EXPENTRY intrin_lang_get(void)
  {
    MA ma;
    char *key;

    MexArgBegin(&ma);
    key = MexArgGetString(&ma, TRUE);

    if (key && g_current_lang)
    {
      const char *val = maxlang_get(g_current_lang, key);
      MexReturnString((char *)(val ? val : ""));
    }
    else
    {
      MexReturnString("");
    }

    if (key)
      free(key);

    return MexArgEnd(&ma);
  }

  /**
   * @brief MEX intrinsic: lang_get_rip(string key) -> string
   *
   * Retrieves the RIP alternate for a language string.
   * Returns empty string if no RIP alternate exists.
   */
  word EXPENTRY intrin_lang_get_rip(void)
  {
    MA ma;
    char *key;

    MexArgBegin(&ma);
    key = MexArgGetString(&ma, TRUE);

    if (key && g_current_lang)
    {
      const char *val = maxlang_get_rip(g_current_lang, key);
      MexReturnString((char *)(val ? val : ""));
    }
    else
    {
      MexReturnString("");
    }

    if (key)
      free(key);

    return MexArgEnd(&ma);
  }

#endif /* !INTERNAL_LANGUAGES */

#endif /* MEX */
