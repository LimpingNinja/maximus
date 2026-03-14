/*
 * theme.c — Theme registry implementation
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

#define MAX_LANG_m_area
#include <string.h>
#include "libmaxcfg.h"
#include "prog.h"
#include "mm.h"
#include "theme.h"

/** @brief Static theme registry — populated once at startup. */
static theme_registry_t s_registry;

/** @brief Whether the registry has been initialized. */
static int s_registry_loaded = 0;

/**
 * @brief Safe string copy with NUL termination.
 *
 * Copies at most dst_sz-1 bytes from src into dst, always NUL-terminates.
 */
static void near theme_strncpy(char *dst, const char *src, size_t dst_sz)
{
  if (dst_sz == 0)
    return;
  strncpy(dst, src, dst_sz - 1);
  dst[dst_sz - 1] = '\0';
}

/**
 * @brief Find a theme entry in the registry by index.
 *
 * Scans the loaded themes for a matching index value.
 *
 * @param index  Theme index to find
 * @return Pointer to theme_entry_t, or NULL if not found
 */
static const theme_entry_t *near theme_find_by_index(int index)
{
  int i;

  if (!s_registry_loaded)
    return NULL;

  for (i = 0; i < s_registry.count; i++)
  {
    if (s_registry.themes[i].index == index)
      return &s_registry.themes[i];
  }
  return NULL;
}

/**
 * @brief Find a theme entry by short_name.
 *
 * @param short_name  Theme short_name to look up
 * @return Pointer to theme_entry_t, or NULL if not found
 */
static const theme_entry_t *near theme_find_by_name(const char *short_name)
{
  int i;

  if (!s_registry_loaded || !short_name)
    return NULL;

  for (i = 0; i < s_registry.count; i++)
  {
    if (stricmp(s_registry.themes[i].short_name, short_name) == 0)
      return &s_registry.themes[i];
  }
  return NULL;
}

/**
 * @brief Get the default theme entry.
 *
 * Falls back to the first theme if default_theme name is not found.
 *
 * @return Pointer to the default theme, or first theme, or NULL if empty
 */
static const theme_entry_t *near theme_get_default(void)
{
  const theme_entry_t *e;

  if (!s_registry_loaded || s_registry.count == 0)
    return NULL;

  e = theme_find_by_name(s_registry.default_theme);
  if (e)
    return e;

  /* Fallback: first registered theme */
  return &s_registry.themes[0];
}


int theme_registry_init(void)
{
  MaxCfgVar themes_arr, item, v;
  size_t count;
  size_t i;
  int loaded = 0;

  memset(&s_registry, 0, sizeof(s_registry));
  s_registry_loaded = 0;

  if (!ng_cfg)
  {
    logit("!theme_registry_init: ng_cfg is NULL");
    return -1;
  }

  /* Read [general] scalars */
  if (maxcfg_toml_get(ng_cfg, "general.theme.general.default_theme", &v) == MAXCFG_OK
      && v.type == MAXCFG_VAR_STRING && v.v.s)
  {
    theme_strncpy(s_registry.default_theme, v.v.s, sizeof(s_registry.default_theme));
  }
  else
  {
    theme_strncpy(s_registry.default_theme, "maxng", sizeof(s_registry.default_theme));
  }

  if (maxcfg_toml_get(ng_cfg, "general.theme.general.default_lang", &v) == MAXCFG_OK
      && v.type == MAXCFG_VAR_STRING && v.v.s)
  {
    theme_strncpy(s_registry.default_lang, v.v.s, sizeof(s_registry.default_lang));
  }
  else
  {
    theme_strncpy(s_registry.default_lang, "english", sizeof(s_registry.default_lang));
  }

  /* Read [[theme]] table array */
  if (maxcfg_toml_get(ng_cfg, "general.theme.theme", &themes_arr) != MAXCFG_OK
      || themes_arr.type != MAXCFG_VAR_TABLE_ARRAY)
  {
    /* No [[theme]] entries — synthesize a default theme */
    logit(":theme_registry_init: no [[theme]] entries, synthesizing default");
    s_registry.themes[0].index = 0;
    theme_strncpy(s_registry.themes[0].short_name, s_registry.default_theme,
                  sizeof(s_registry.themes[0].short_name));
    theme_strncpy(s_registry.themes[0].name, "Default",
                  sizeof(s_registry.themes[0].name));
    s_registry.themes[0].lang[0] = '\0';
    s_registry.count = 1;
    s_registry_loaded = 1;
    return 0;
  }

  if (maxcfg_var_count(&themes_arr, &count) != MAXCFG_OK || count == 0)
  {
    logit("!theme_registry_init: empty [[theme]] array");
    return -1;
  }

  /* Cap at MAX_THEMES */
  if (count > MAX_THEMES)
  {
    logit("!theme_registry_init: %d themes exceeds MAX_THEMES=%d, truncating",
          (int)count, MAX_THEMES);
    count = MAX_THEMES;
  }

  for (i = 0; i < count; i++)
  {
    theme_entry_t *te;
    int idx = -1;

    if (maxcfg_toml_array_get(&themes_arr, i, &item) != MAXCFG_OK
        || item.type != MAXCFG_VAR_TABLE)
    {
      logit("!theme_registry_init: failed to read [[theme]] entry %d", (int)i);
      continue;
    }

    /* Read index (required, must be 1..MAX_THEMES-1; 0 is reserved for "unset") */
    if (maxcfg_toml_table_get(&item, "index", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
      idx = v.v.i;

    if (idx <= 0 || idx >= MAX_THEMES)
    {
      logit("!theme_registry_init: entry %d has invalid index %d (must be 1-%d), skipping",
            (int)i, idx, MAX_THEMES - 1);
      continue;
    }

    te = &s_registry.themes[loaded];
    te->index = idx;

    /* Read short_name (required) */
    if (maxcfg_toml_table_get(&item, "short_name", &v) == MAXCFG_OK
        && v.type == MAXCFG_VAR_STRING && v.v.s)
    {
      theme_strncpy(te->short_name, v.v.s, sizeof(te->short_name));
    }
    else
    {
      logit("!theme_registry_init: entry %d missing short_name, skipping", (int)i);
      continue;
    }

    /* Read name (display name) */
    if (maxcfg_toml_table_get(&item, "name", &v) == MAXCFG_OK
        && v.type == MAXCFG_VAR_STRING && v.v.s)
    {
      theme_strncpy(te->name, v.v.s, sizeof(te->name));
    }
    else
    {
      theme_strncpy(te->name, te->short_name, sizeof(te->name));
    }

    /* Read lang (optional, "" = inherit default_lang) */
    if (maxcfg_toml_table_get(&item, "lang", &v) == MAXCFG_OK
        && v.type == MAXCFG_VAR_STRING && v.v.s)
    {
      theme_strncpy(te->lang, v.v.s, sizeof(te->lang));
    }
    else
    {
      te->lang[0] = '\0';
    }

    loaded++;
  }

  s_registry.count = loaded;
  s_registry_loaded = 1;

  logit("+Theme registry loaded: %d theme(s), default='%s', default_lang='%s'",
        loaded, s_registry.default_theme, s_registry.default_lang);

  return 0;
}


const char *theme_get_shortname(int index)
{
  const theme_entry_t *e = theme_find_by_index(index);
  if (e)
    return e->short_name;

  /* Invalid index — return default theme's short_name */
  e = theme_get_default();
  return e ? e->short_name : "";
}


const char *theme_get_current_shortname(void)
{
  int idx = (int)usr.theme;
  return theme_get_shortname(idx);
}


int theme_get_index(const char *short_name)
{
  const theme_entry_t *e = theme_find_by_name(short_name);
  return e ? e->index : 0;
}


const char *theme_get_lang(int index)
{
  const theme_entry_t *e = theme_find_by_index(index);

  /* If entry not found, try default theme */
  if (!e)
    e = theme_get_default();

  if (!e)
    return s_registry.default_lang;

  /* Empty lang string means inherit default_lang */
  if (e->lang[0] == '\0')
    return s_registry.default_lang;

  return e->lang;
}


int theme_get_count(void)
{
  return s_registry.count;
}


const char *theme_get_name(int index)
{
  const theme_entry_t *e = theme_find_by_index(index);

  if (!e)
    e = theme_get_default();

  return e ? e->name : "";
}


const char *theme_get_default_shortname(void)
{
  return s_registry.default_theme;
}


int theme_get_entry_index(int pos)
{
  if (!s_registry_loaded || pos < 0 || pos >= s_registry.count)
    return 0;

  return s_registry.themes[pos].index;
}
