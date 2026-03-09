/*
 * maxlang.h — Language system public API header
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

#ifndef MAXLANG_H
#define MAXLANG_H

#include <stdbool.h>
#include <stddef.h>
#include "libmaxcfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque handle for a loaded language file */
typedef struct MaxLang MaxLang;

/** Parameter binding for positional substitution (|!1..|!F) */
typedef struct {
    const char *values[15];  /**< Pre-formatted string values for |!1..|!F */
    int count;               /**< Number of bound parameters */
} MaxLangParams;

/* ========================================================================== */
/* Lifecycle                                                                   */
/* ========================================================================== */

/**
 * @brief Load a language file from TOML.
 *
 * @param toml_path  Full path to the language .toml file.
 * @param out_lang   Receives the loaded language handle.
 * @return MAXCFG_OK on success.
 */
MaxCfgStatus maxlang_open(const char *toml_path, MaxLang **out_lang);

/**
 * @brief Free a loaded language handle and all associated memory.
 */
void maxlang_close(MaxLang *lang);

/* ========================================================================== */
/* String retrieval                                                            */
/* ========================================================================== */

/**
 * @brief Get a language string by heap and symbol name.
 *
 * @param lang  Language handle.
 * @param key   Dotted key: "heap.symbol" (e.g. "global.located").
 * @return Pointer to the string, or "" if not found.
 *         Valid until maxlang_close().
 */
const char *maxlang_get(MaxLang *lang, const char *key);

/**
 * @brief Get the RIP alternate for a string, if one exists.
 *
 * @param lang  Language handle.
 * @param key   Dotted key.
 * @return RIP string, or NULL if no alternate defined.
 */
const char *maxlang_get_rip(MaxLang *lang, const char *key);

/**
 * @brief Check whether a string has a specific flag (e.g. "mex").
 *
 * @param lang  Language handle.
 * @param key   Dotted key.
 * @param flag  Flag name to check.
 * @return true if the flag is present.
 */
bool maxlang_has_flag(MaxLang *lang, const char *key, const char *flag);

/* ========================================================================== */
/* Backward-compatible numeric access                                          */
/* ========================================================================== */

/**
 * @brief Resolve a legacy numeric string ID to a TOML string.
 *
 * Uses the [_legacy_map] table embedded in the language file.
 *
 * @param lang  Language handle.
 * @param strn  Legacy string number (as used by s_ret).
 * @return The string, or "" if not mapped.
 */
const char *maxlang_get_by_id(MaxLang *lang, int strn);

/**
 * @brief Resolve a legacy heap-relative string ID to a TOML string.
 *
 * Scans the [_legacy_map] to find the base ID for the named heap,
 * then delegates to maxlang_get_by_id(lang, base + strn).
 *
 * @param lang       Language handle.
 * @param heap_name  Heap name (e.g. "m_area", "mexbank").
 * @param strn       Relative string index within the heap.
 * @return The string, or "" if not mapped.
 */
const char *maxlang_get_by_heap_id(MaxLang *lang, const char *heap_name, int strn);

/**
 * @brief Get the language display name from the TOML [meta] section.
 *
 * @param lang  Language handle.
 * @return The name string, or "" if not available.
 */
const char *maxlang_get_name(MaxLang *lang);

/* ========================================================================== */
/* RIP alternate mode                                                          */
/* ========================================================================== */

/**
 * @brief Enable or disable RIP alternate string resolution.
 *
 * When enabled, maxlang_get() returns the RIP alternate if one exists,
 * falling back to the primary text otherwise.
 */
void maxlang_set_use_rip(MaxLang *lang, bool use_rip);

/* ========================================================================== */
/* Extension language file loading                                              */
/* ========================================================================== */

/**
 * @brief Load an extension language TOML file into the language handle.
 *
 * All heaps (top-level tables) in the extension file become accessible
 * via maxlang_get() using "heap.key" dotted notation, just like the
 * primary language file.
 *
 * @param lang  Language handle.
 * @param path  Full path to the extension TOML file.
 * @return MAXCFG_OK on success, MAXCFG_ERR_DUPLICATE if any heap name
 *         in the extension file conflicts with an existing heap,
 *         MAXCFG_ERR_NOT_FOUND if the file does not exist.
 */
MaxCfgStatus maxlang_load_extension(MaxLang *lang, const char *path);

/* ========================================================================== */
/* Runtime string registration (MEX / extensions)                              */
/* ========================================================================== */

/**
 * @brief Register a runtime language string set.
 *
 * @param lang    Language handle.
 * @param ns      Namespace (e.g. "mex_bank").
 * @param keys    Array of key names.
 * @param values  Array of string values (same length as keys).
 * @param count   Number of entries.
 * @return MAXCFG_OK on success.
 *
 * Registered strings are accessed as "<ns>.<key>" via maxlang_get().
 */
MaxCfgStatus maxlang_register(MaxLang *lang, const char *ns,
                              const char **keys, const char **values,
                              int count);

/**
 * @brief Unregister a previously registered namespace.
 */
void maxlang_unregister(MaxLang *lang, const char *ns);

#ifdef __cplusplus
}
#endif

#endif /* MAXLANG_H */
