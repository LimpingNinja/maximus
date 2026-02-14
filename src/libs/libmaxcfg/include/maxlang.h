/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * maxlang.h - Language file API for TOML-based language strings
 *
 * Provides loading, retrieval, legacy numeric access, RIP alternates,
 * and runtime string registration for MEX/extensions.
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
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
