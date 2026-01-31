/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ctl_to_ng.h - CTL file to MaxCfgNg* struct population (zero PRM dependency)
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#ifndef CTL_TO_NG_H
#define CTL_TO_NG_H

#include <stdbool.h>
#include <stddef.h>
#include "libmaxcfg.h"

/* Parse a keyword value from a CTL file */
bool ctl_to_ng_parse_keyword(const char *ctl_path, const char *keyword, char *value_buf, size_t value_sz);

/* Parse a boolean keyword from a CTL file */
bool ctl_to_ng_parse_boolean(const char *ctl_path, const char *keyword, bool *value);

/* Parse an integer keyword from a CTL file */
bool ctl_to_ng_parse_int(const char *ctl_path, const char *keyword, int *value);

/* Populate MaxCfgNg* structs from CTL files */
bool ctl_to_ng_populate_system(const char *maxctl_path, const char *sys_path, const char *config_dir, MaxCfgNgSystem *sys);
bool ctl_to_ng_populate_session(const char *maxctl_path, MaxCfgNgGeneralSession *session);
bool ctl_to_ng_populate_equipment(const char *maxctl_path, MaxCfgNgEquipment *equip);
bool ctl_to_ng_populate_reader(const char *sys_path, MaxCfgNgReader *reader);
bool ctl_to_ng_populate_display_files(const char *maxctl_path, MaxCfgNgGeneralDisplayFiles *files);
bool ctl_to_ng_populate_language(const char *sys_path, MaxCfgNgLanguage *lang);

#endif /* CTL_TO_NG_H */
