/*
 * theme.h — Theme registry data structures and API
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

#ifndef __THEME_H_DEFINED
#define __THEME_H_DEFINED

/** @brief Maximum number of themes supported by the registry. */
#define MAX_THEMES 16

/**
 * @brief A single theme registry entry loaded from theme.toml.
 *
 * Index 0 is reserved to mean "no theme chosen / use default".
 * Valid theme indices are 1 through MAX_THEMES-1.
 */
typedef struct {
    int  index;              /**< Numeric index 1-15 (0 = unset/default)  */
    char short_name[32];     /**< Filesystem-safe identifier               */
    char name[64];           /**< Human-readable display name              */
    char lang[64];           /**< Language file basename, "" = inherit     */
} theme_entry_t;

/**
 * @brief Theme registry — holds all loaded themes from theme.toml.
 */
typedef struct {
    int  count;              /**< Number of loaded themes                  */
    char default_theme[32];  /**< Default theme short_name from [general]  */
    char default_lang[64];   /**< System default language basename         */
    theme_entry_t themes[MAX_THEMES]; /**< Theme entries indexed by slot   */
} theme_registry_t;

/**
 * @brief Initialize theme registry from loaded TOML config.
 *
 * Call after theme.toml has been loaded into ng_cfg with prefix
 * "general.theme".  Reads [general] scalars and iterates [[theme]]
 * table-array entries to populate the static registry.
 *
 * @return 0 on success, -1 on error
 */
int theme_registry_init(void);

/**
 * @brief Get the short_name for a theme by index.
 *
 * Index 0 means "unset" and resolves to the default theme's short_name.
 *
 * @param index  Theme index (1-15), or 0 for default
 * @return short_name string, or default theme's short_name if index is 0 or invalid
 */
const char *theme_get_shortname(int index);

/**
 * @brief Get the current user's theme short_name.
 *
 * Uses usr.theme as the theme index.
 *
 * @return short_name string for current user's theme
 */
const char *theme_get_current_shortname(void);

/**
 * @brief Get theme index by short_name.
 *
 * @param short_name  Theme short_name to look up
 * @return index (1-15), or 0 if not found (maps to default at lookup time)
 */
int theme_get_index(const char *short_name);

/**
 * @brief Get the language file associated with a theme.
 *
 * @param index  Theme index
 * @return Language basename, or default_lang if theme has lang=""
 */
const char *theme_get_lang(int index);

/**
 * @brief Get the number of loaded themes.
 *
 * @return Count of themes in registry
 */
int theme_get_count(void);

/**
 * @brief Get the display name for a theme by index.
 *
 * @param index  Theme index
 * @return Display name string
 */
const char *theme_get_name(int index);

/**
 * @brief Get the system default theme short_name.
 *
 * @return default_theme from [general] section
 */
const char *theme_get_default_shortname(void);

/**
 * @brief Get the theme index value for the N-th loaded entry.
 *
 * Iterates by load order (0-based) rather than by index value.
 * Used by Chg_Theme() to enumerate themes for user selection.
 *
 * @param pos  Array position (0 .. theme_get_count()-1)
 * @return     Theme index (1-15), or 0 if pos is out of range
 */
int theme_get_entry_index(int pos);

#endif /* __THEME_H_DEFINED */
