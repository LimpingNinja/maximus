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
 * @brief Controls which delta tiers are applied during delta overlay.
 *
 * - LANG_DELTA_FULL:       Apply both Tier 1 (@merge params) and Tier 2
 *                          ([maximusng-*] theme overrides).
 * - LANG_DELTA_MERGE_ONLY: Apply only Tier 1 (@merge param metadata).
 *                          Preserves user color choices in migrated files.
 * - LANG_DELTA_NG_ONLY:    Apply only Tier 2 ([maximusng-*] theme colors).
 *                          For adding theme to an already-enriched file.
 */
typedef enum {
    LANG_DELTA_FULL,        /**< Apply all delta tiers (default) */
    LANG_DELTA_MERGE_ONLY,  /**< Tier 1 only: @merge param metadata */
    LANG_DELTA_NG_ONLY      /**< Tier 2 only: [maximusng-*] theme overrides */
} LangDeltaMode;

/**
 * @brief Convert a single .MAD language file to TOML format.
 *
 * Reads the .MAD file, resolves #include and #define directives,
 * parses heap sections and string definitions, converts AVATAR
 * color/cursor sequences to MCI codes, and writes a .toml file
 * in the same directory. Applies delta overlay according to mode.
 *
 * @param mad_path   Full path to the .MAD input file.
 * @param out_dir    Directory to write the .toml output (NULL = same dir as input).
 * @param mode       Delta application mode (LANG_DELTA_FULL if no preference).
 * @param err        Buffer to receive error message on failure.
 * @param err_len    Size of the error buffer.
 * @return true on success.
 */
bool lang_convert_mad_to_toml(const char *mad_path,
                              const char *out_dir,
                              LangDeltaMode mode,
                              char *err,
                              size_t err_len);

/**
 * @brief Apply a delta overlay to an existing TOML language file.
 *
 * Reads the base .toml file and applies changes from the delta file
 * according to the specified mode. The delta file is located automatically
 * as delta_<basename>.toml in the same directory, or can be specified
 * explicitly via delta_path.
 *
 * Tier 1 (@merge) entries add param metadata via shallow field merge.
 * Tier 2 ([maximusng-*]) entries replace full string values for theme colors.
 *
 * @param toml_path   Full path to the base .toml file to modify in-place.
 * @param delta_path  Full path to the delta .toml file (NULL = auto-detect).
 * @param mode        Which tiers to apply.
 * @param err         Buffer to receive error message on failure.
 * @param err_len     Size of the error buffer.
 * @return true on success, false on error.
 */
bool lang_apply_delta(const char *toml_path,
                      const char *delta_path,
                      LangDeltaMode mode,
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
 * @param mode       Delta application mode for each converted file.
 * @param err        Buffer to receive error message on failure.
 * @param err_len    Size of the error buffer.
 * @return Number of files successfully converted, or -1 on fatal error.
 */
int lang_convert_all_mad(const char *lang_dir,
                         const char *out_dir,
                         LangDeltaMode mode,
                         char *err,
                         size_t err_len);

#endif /* LANG_CONVERT_H */
