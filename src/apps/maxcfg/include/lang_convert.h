/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * lang_convert.h - Legacy .MAD language file to TOML converter
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#ifndef LANG_CONVERT_H
#define LANG_CONVERT_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Convert a single .MAD language file to TOML format.
 *
 * Reads the .MAD file, resolves #include and #define directives,
 * parses heap sections and string definitions, converts AVATAR
 * color/cursor sequences to MCI codes, and writes a .toml file
 * in the same directory.
 *
 * @param mad_path   Full path to the .MAD input file.
 * @param out_dir    Directory to write the .toml output (NULL = same dir as input).
 * @param err        Buffer to receive error message on failure.
 * @param err_len    Size of the error buffer.
 * @return true on success.
 */
bool lang_convert_mad_to_toml(const char *mad_path,
                              const char *out_dir,
                              char *err,
                              size_t err_len);

/**
 * @brief Convert all .MAD files found in a directory to TOML.
 *
 * Scans lang_dir for *.mad files and converts each one.
 * Output .toml files are written to out_dir (or lang_dir if NULL).
 *
 * @param lang_dir   Directory containing .MAD files.
 * @param out_dir    Output directory (NULL = same as lang_dir).
 * @param err        Buffer to receive error message on failure.
 * @param err_len    Size of the error buffer.
 * @return Number of files successfully converted, or -1 on fatal error.
 */
int lang_convert_all_mad(const char *lang_dir,
                         const char *out_dir,
                         char *err,
                         size_t err_len);

#endif /* LANG_CONVERT_H */
