/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ctl_parse.h - CTL file parser for reading configuration values
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#ifndef CTL_PARSE_H
#define CTL_PARSE_H

#include <stdbool.h>
#include <stddef.h>

/* Parse a keyword value from a CTL file (path-driven, no g_prm dependency)
 * Returns true if keyword found, false otherwise
 * ctl_path: explicit path to the CTL file to parse
 * keyword: the keyword to search for (e.g., "Name", "SysOp", "Path System")
 * value_buf: buffer to store the value
 * value_sz: size of value buffer
 */
bool ctl_parse_keyword_from_file(const char *ctl_path, const char *keyword, char *value_buf, size_t value_sz);

/* Parse a boolean keyword from a CTL file (path-driven)
 * Returns true if keyword found, false otherwise
 * ctl_path: explicit path to the CTL file to parse
 * keyword: the keyword to search for (e.g., "Snoop", "StatusLine", "Alias System")
 * value: output boolean value
 */
bool ctl_parse_boolean_from_file(const char *ctl_path, const char *keyword, bool *value);

#endif
