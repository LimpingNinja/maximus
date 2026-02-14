/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * lang_browse.h - TOML language file string browser for maxcfg
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#ifndef LANG_BROWSE_H
#define LANG_BROWSE_H

/**
 * @brief Browse language strings from a TOML language file.
 *
 * Shows a filterable list of all "heap.key = value" entries found in the
 * specified TOML language file.  Pressing Enter on an entry shows the
 * full string value in a detail dialog.
 *
 * @param toml_path  Full path to the .toml language file to browse.
 */
void lang_browse_strings(const char *toml_path);

/**
 * @brief Action callback for the Language Settings form.
 *
 * Resolves the language path from the current TOML config and opens
 * a picker to select a .toml file, then browses its strings.
 *
 * @param unused  Ignored (FIELD_ACTION signature).
 */
void action_browse_lang_strings(void *unused);

#endif /* LANG_BROWSE_H */
