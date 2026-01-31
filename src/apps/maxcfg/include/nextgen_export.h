/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * nextgen_export.h - Next-gen config export (MAXCFG)
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#ifndef NEXTGEN_EXPORT_H
#define NEXTGEN_EXPORT_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    NG_EXPORT_SYSTEM     = 1u << 0,
    NG_EXPORT_MENUS      = 1u << 1,
    NG_EXPORT_MSG_AREAS  = 1u << 2,
    NG_EXPORT_FILE_AREAS = 1u << 3,

    NG_EXPORT_ALL = NG_EXPORT_SYSTEM | NG_EXPORT_MENUS | NG_EXPORT_MSG_AREAS | NG_EXPORT_FILE_AREAS
} NextGenExportFlags;

bool nextgen_export_config(const char *sys_path,
                           const char *config_dir,
                           unsigned int flags,
                           char *err,
                           size_t err_len);

bool nextgen_export_config_from_maxctl(const char *maxctl_path,
                                       const char *config_dir,
                                       unsigned int flags,
                                       char *err,
                                       size_t err_len);

#endif
