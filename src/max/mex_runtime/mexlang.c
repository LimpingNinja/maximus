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
 * because mexall.h pulls in english.h macros that collide with libmaxcfg.h
 * struct field names (e.g. file_offline → s_ret(...)). */
struct MaxLang;
extern const char *maxlang_get(struct MaxLang *lang, const char *key);
extern const char *maxlang_get_rip(struct MaxLang *lang, const char *key);
extern int maxlang_register(struct MaxLang *lang, const char *ns,
                            const char **keys, const char **values,
                            int count);
extern void maxlang_unregister(struct MaxLang *lang, const char *ns);
extern int maxlang_load_extension(struct MaxLang *lang, const char *path);

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
    key = MexArgGetString(&ma, FALSE);

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
    key = MexArgGetString(&ma, FALSE);

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

  /**
   * @brief MEX intrinsic: lang_register(string ns, string key, string value) -> int
   *
   * Registers a single language string under a runtime namespace.
   * The string becomes accessible as "<ns>.<key>" via lang_get().
   * Returns 1 on success, 0 on failure.
   */
  word EXPENTRY intrin_lang_register(void)
  {
    MA ma;
    char *ns, *key, *value;

    MexArgBegin(&ma);
    ns    = MexArgGetString(&ma, TRUE);
    key   = MexArgGetString(&ma, TRUE);
    value = MexArgGetString(&ma, TRUE);

    regs_2[0] = 0;

    if (ns && key && value && g_current_lang)
    {
      const char *k = key;
      const char *v = value;
      /* maxlang_register returns MAXCFG_OK (0) on success */
      if (maxlang_register(g_current_lang, ns, &k, &v, 1) == 0)
        regs_2[0] = 1;
    }

    if (ns)    free(ns);
    if (key)   free(key);
    if (value) free(value);

    return MexArgEnd(&ma);
  }

  /**
   * @brief MEX intrinsic: lang_unregister(string ns) -> void
   *
   * Removes all runtime-registered strings under the given namespace.
   */
  word EXPENTRY intrin_lang_unregister(void)
  {
    MA ma;
    char *ns;

    MexArgBegin(&ma);
    ns = MexArgGetString(&ma, TRUE);

    if (ns && g_current_lang)
      maxlang_unregister(g_current_lang, ns);

    if (ns)
      free(ns);

    return MexArgEnd(&ma);
  }

  /**
   * @brief MEX intrinsic: lang_load_extension(string path) -> int
   *
   * Loads an extension language TOML file.  All heaps in the file become
   * accessible via lang_get() as "heap.key".
   * If the path is not absolute, it is resolved relative to the configured
   * language directory (config/lang under the system prefix, or as set by lang_path).
   * Returns 1 on success, 0 on failure (file not found, heap conflict, etc.).
   */
  word EXPENTRY intrin_lang_load_extension(void)
  {
    MA ma;
    char *path;

    MexArgBegin(&ma);
    path = MexArgGetString(&ma, TRUE);

    regs_2[0] = 0;

    if (path && g_current_lang)
    {
      char full_path[1024];

      if (path[0] == '/' || path[0] == '\\') {
        /* Absolute path — use as-is */
        snprintf(full_path, sizeof(full_path), "%s", path);
      } else {
        /* Relative path — resolve against lang directory */
        snprintf(full_path, sizeof(full_path), "%s/%s",
                 ngcfg_get_path("maximus.lang_path"), path);
      }

      /* maxlang_load_extension returns MAXCFG_OK (0) on success */
      if (maxlang_load_extension(g_current_lang, full_path) == 0)
        regs_2[0] = 1;
    }

    if (path)
      free(path);

    return MexArgEnd(&ma);
  }

#endif /* !INTERNAL_LANGUAGES */

#endif /* MEX */
