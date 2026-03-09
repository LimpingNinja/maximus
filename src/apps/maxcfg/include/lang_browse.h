/*
 * lang_browse.h — Language string browser header
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
