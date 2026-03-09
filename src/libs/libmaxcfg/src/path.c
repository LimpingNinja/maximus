/*
 * path.c — Path resolution utilities
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

#include "libmaxcfg.h"

#include <string.h>

/**
 * @brief Join a relative path to the config base directory.
 *
 * Concatenates the base directory from the MaxCfg handle with the given
 * relative path, inserting a slash separator if needed.
 *
 * @param cfg            Config handle providing the base directory.
 * @param relative_path  Relative path to append.
 * @param out_path       Output buffer for the joined path.
 * @param out_path_size  Size of the output buffer.
 * @return MAXCFG_OK on success, or an error status.
 */
MaxCfgStatus maxcfg_join_path(const MaxCfg *cfg,
                             const char *relative_path,
                             char *out_path,
                             size_t out_path_size)
{
    const char *base;
    size_t base_len;
    size_t rel_len;
    int need_slash;
    size_t total_len;

    if (cfg == NULL || relative_path == NULL || out_path == NULL || out_path_size == 0) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    base = maxcfg_base_dir(cfg);
    if (base == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    base_len = strlen(base);
    rel_len = strlen(relative_path);

    need_slash = (base_len > 0 && base[base_len - 1] != '/');

    total_len = base_len + (need_slash ? 1u : 0u) + rel_len;
    if (total_len + 1u > out_path_size) {
        return MAXCFG_ERR_PATH_TOO_LONG;
    }

    memcpy(out_path, base, base_len);

    if (need_slash) {
        out_path[base_len] = '/';
        memcpy(out_path + base_len + 1u, relative_path, rel_len);
        out_path[base_len + 1u + rel_len] = '\0';
    } else {
        memcpy(out_path + base_len, relative_path, rel_len);
        out_path[base_len + rel_len] = '\0';
    }

    return MAXCFG_OK;
}
