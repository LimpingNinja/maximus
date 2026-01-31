/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * area_toml.h - Message and file area TOML loader/saver
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#ifndef AREA_TOML_H
#define AREA_TOML_H

#include <stdbool.h>
#include "treeview.h"
#include "libmaxcfg.h"

/* Load message areas from TOML into tree structure
 * Returns array of root nodes and sets *count
 */
TreeNode **load_msgarea_toml(MaxCfgToml *toml, int *count, char *err, size_t err_sz);

/* Save message areas from tree structure to TOML
 * Returns true on success
 */
bool save_msgarea_toml(MaxCfgToml *toml, const char *toml_path, TreeNode **roots, int count, char *err, size_t err_sz);

/* Load file areas from TOML into tree structure
 * Returns array of root nodes and sets *count
 */
TreeNode **load_filearea_toml(MaxCfgToml *toml, int *count, char *err, size_t err_sz);

/* Save file areas from tree structure to TOML
 * Returns true on success
 */
bool save_filearea_toml(MaxCfgToml *toml, const char *toml_path, TreeNode **roots, int count, char *err, size_t err_sz);

#endif /* AREA_TOML_H */
