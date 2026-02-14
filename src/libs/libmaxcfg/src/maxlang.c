/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * maxlang.c - Language file API for TOML-based language strings
 *
 * Loads language TOML files via the MaxCfgToml infrastructure, provides
 * string retrieval by dotted key, legacy numeric ID mapping, RIP alternate
 * resolution, and runtime string registration for MEX/extensions.
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "maxlang.h"

/* ========================================================================== */
/* Constants                                                                   */
/* ========================================================================== */

#define ML_MAX_PATH       1024
#define ML_MAX_KEY        256
#define ML_MAX_LEGACY_MAP 2048   /**< Maximum legacy ID → key mappings */
#define ML_MAX_RUNTIME_NS 32    /**< Maximum runtime namespaces */
#define ML_MAX_RT_STRINGS 256   /**< Maximum strings per runtime namespace */

/* ========================================================================== */
/* Internal structures                                                         */
/* ========================================================================== */

/** A single legacy ID → dotted key mapping */
typedef struct {
    int id;
    char *key;          /**< Dotted key: "heap.symbol" */
} MlLegacyEntry;

/** A single runtime-registered string */
typedef struct {
    char *key;
    char *value;
} MlRtString;

/** A runtime-registered namespace */
typedef struct {
    char *ns;                          /**< Namespace name */
    MlRtString strings[ML_MAX_RT_STRINGS];
    int count;
} MlRtNamespace;

/** Main language handle */
struct MaxLang {
    MaxCfgToml *toml;                  /**< Underlying TOML data store */
    bool owns_toml;                    /**< true if we allocated the toml handle */

    MlLegacyEntry legacy_map[ML_MAX_LEGACY_MAP];
    int legacy_count;

    MlRtNamespace rt_ns[ML_MAX_RUNTIME_NS];
    int rt_ns_count;

    bool use_rip;                      /**< Prefer RIP alternates when available */
};

/* ========================================================================== */
/* Helpers                                                                     */
/* ========================================================================== */

/**
 * @brief Parse the [_legacy_map] table from the loaded TOML into the
 *        fast-lookup legacy_map array.
 *
 * The TOML legacy map has entries like:
 *   "0x0000" = "global.left_x"
 */
static void ml_load_legacy_map(MaxLang *lang)
{
    lang->legacy_count = 0;

    /*
     * Iterate keys under _legacy_map.  The MaxCfgToml API doesn't expose
     * a table iterator directly, so we probe hex IDs sequentially until
     * we hit a gap.  This is a pragmatic approach for Round 1 — the
     * converter emits contiguous IDs starting from 0.
     */
    for (int id = 0; id < ML_MAX_LEGACY_MAP; id++) {
        char probe_key[ML_MAX_KEY];
        snprintf(probe_key, sizeof(probe_key), "_legacy_map.\"0x%04x\"", id);

        MaxCfgVar v;
        if (maxcfg_toml_get(lang->toml, probe_key, &v) != MAXCFG_OK)
            break;

        if (v.type != MAXCFG_VAR_STRING || v.v.s == NULL)
            continue;

        MlLegacyEntry *e = &lang->legacy_map[lang->legacy_count++];
        e->id = id;
        e->key = strdup(v.v.s);
    }
}

/**
 * @brief Retrieve a raw string value from the TOML store.
 *
 * Handles both simple string values and inline-table values where
 * the text is in the "text" sub-key.
 */
static const char *ml_get_raw(MaxLang *lang, const char *key)
{
    if (!lang || !lang->toml || !key)
        return NULL;

    MaxCfgVar v;
    if (maxcfg_toml_get(lang->toml, key, &v) != MAXCFG_OK)
        return NULL;

    /* Simple string: key = "value" */
    if (v.type == MAXCFG_VAR_STRING)
        return v.v.s;

    /* Inline table: key = { text = "value", ... } */
    if (v.type == MAXCFG_VAR_TABLE) {
        char text_key[ML_MAX_KEY];
        snprintf(text_key, sizeof(text_key), "%s.text", key);
        MaxCfgVar tv;
        if (maxcfg_toml_get(lang->toml, text_key, &tv) == MAXCFG_OK &&
            tv.type == MAXCFG_VAR_STRING) {
            return tv.v.s;
        }
    }

    return NULL;
}

/**
 * @brief Retrieve the RIP alternate from the TOML store.
 */
static const char *ml_get_rip_raw(MaxLang *lang, const char *key)
{
    if (!lang || !lang->toml || !key)
        return NULL;

    char rip_key[ML_MAX_KEY];
    snprintf(rip_key, sizeof(rip_key), "%s.rip", key);

    MaxCfgVar v;
    if (maxcfg_toml_get(lang->toml, rip_key, &v) == MAXCFG_OK &&
        v.type == MAXCFG_VAR_STRING) {
        return v.v.s;
    }

    return NULL;
}

/**
 * @brief Search runtime namespaces for a dotted key "ns.symbol".
 */
static const char *ml_get_runtime(MaxLang *lang, const char *key)
{
    if (!lang || !key)
        return NULL;

    /* Split key into namespace.symbol */
    const char *dot = strchr(key, '.');
    if (!dot)
        return NULL;

    size_t ns_len = (size_t)(dot - key);
    const char *symbol = dot + 1;

    for (int i = 0; i < lang->rt_ns_count; i++) {
        MlRtNamespace *ns = &lang->rt_ns[i];
        if (strlen(ns->ns) == ns_len && strncmp(ns->ns, key, ns_len) == 0) {
            for (int j = 0; j < ns->count; j++) {
                if (strcmp(ns->strings[j].key, symbol) == 0)
                    return ns->strings[j].value;
            }
            return NULL;
        }
    }
    return NULL;
}

/* ========================================================================== */
/* Public API: Lifecycle                                                        */
/* ========================================================================== */

MaxCfgStatus maxlang_open(const char *toml_path, MaxLang **out_lang)
{
    if (!toml_path || !out_lang)
        return MAXCFG_ERR_INVALID_ARGUMENT;

    *out_lang = NULL;

    /* Allocate language handle */
    MaxLang *lang = calloc(1, sizeof(MaxLang));
    if (!lang)
        return MAXCFG_ERR_OOM;

    /* Initialize TOML store and load the language file */
    MaxCfgStatus st = maxcfg_toml_init(&lang->toml);
    if (st != MAXCFG_OK) {
        free(lang);
        return st;
    }
    lang->owns_toml = true;

    st = maxcfg_toml_load_file(lang->toml, toml_path, "");
    if (st != MAXCFG_OK) {
        maxcfg_toml_free(lang->toml);
        free(lang);
        return st;
    }

    /* Build legacy map from [_legacy_map] table */
    ml_load_legacy_map(lang);

    *out_lang = lang;
    return MAXCFG_OK;
}

void maxlang_close(MaxLang *lang)
{
    if (!lang)
        return;

    /* Free legacy map keys */
    for (int i = 0; i < lang->legacy_count; i++)
        free(lang->legacy_map[i].key);

    /* Free runtime namespaces */
    for (int i = 0; i < lang->rt_ns_count; i++) {
        MlRtNamespace *ns = &lang->rt_ns[i];
        free(ns->ns);
        for (int j = 0; j < ns->count; j++) {
            free(ns->strings[j].key);
            free(ns->strings[j].value);
        }
    }

    if (lang->owns_toml && lang->toml)
        maxcfg_toml_free(lang->toml);

    free(lang);
}

/* ========================================================================== */
/* Public API: String retrieval                                                */
/* ========================================================================== */

const char *maxlang_get(MaxLang *lang, const char *key)
{
    if (!lang || !key)
        return "";

    /* If RIP mode, try RIP alternate first */
    if (lang->use_rip) {
        const char *rip = ml_get_rip_raw(lang, key);
        if (rip)
            return rip;
    }

    /* Try TOML store */
    const char *text = ml_get_raw(lang, key);
    if (text)
        return text;

    /* Try runtime registered strings */
    const char *rt = ml_get_runtime(lang, key);
    if (rt)
        return rt;

    return "";
}

const char *maxlang_get_rip(MaxLang *lang, const char *key)
{
    if (!lang || !key)
        return NULL;

    return ml_get_rip_raw(lang, key);
}

bool maxlang_has_flag(MaxLang *lang, const char *key, const char *flag)
{
    if (!lang || !lang->toml || !key || !flag)
        return false;

    char flags_key[ML_MAX_KEY];
    snprintf(flags_key, sizeof(flags_key), "%s.flags", key);

    MaxCfgVar v;
    if (maxcfg_toml_get(lang->toml, flags_key, &v) != MAXCFG_OK)
        return false;

    if (v.type == MAXCFG_VAR_STRING_ARRAY) {
        for (size_t i = 0; i < v.v.strv.count; i++) {
            if (v.v.strv.items[i] && strcasecmp(v.v.strv.items[i], flag) == 0)
                return true;
        }
    }

    return false;
}

/* ========================================================================== */
/* Public API: Legacy numeric access                                           */
/* ========================================================================== */

const char *maxlang_get_by_id(MaxLang *lang, int strn)
{
    if (!lang)
        return "";

    /* Binary search would be ideal, but the IDs are contiguous from 0,
     * so direct indexing works if the map is populated densely. */
    for (int i = 0; i < lang->legacy_count; i++) {
        if (lang->legacy_map[i].id == strn) {
            return maxlang_get(lang, lang->legacy_map[i].key);
        }
    }

    return "";
}

/* ========================================================================== */
/* Public API: RIP mode                                                        */
/* ========================================================================== */

void maxlang_set_use_rip(MaxLang *lang, bool use_rip)
{
    if (lang)
        lang->use_rip = use_rip;
}

/* ========================================================================== */
/* Public API: Runtime string registration                                     */
/* ========================================================================== */

MaxCfgStatus maxlang_register(MaxLang *lang, const char *ns,
                              const char **keys, const char **values,
                              int count)
{
    if (!lang || !ns || !keys || !values || count <= 0)
        return MAXCFG_ERR_INVALID_ARGUMENT;

    /* Find or create the namespace */
    MlRtNamespace *target = NULL;
    for (int i = 0; i < lang->rt_ns_count; i++) {
        if (strcmp(lang->rt_ns[i].ns, ns) == 0) {
            target = &lang->rt_ns[i];
            break;
        }
    }

    if (!target) {
        if (lang->rt_ns_count >= ML_MAX_RUNTIME_NS)
            return MAXCFG_ERR_OOM;

        target = &lang->rt_ns[lang->rt_ns_count++];
        memset(target, 0, sizeof(*target));
        target->ns = strdup(ns);
        if (!target->ns) {
            lang->rt_ns_count--;
            return MAXCFG_ERR_OOM;
        }
    }

    /* Register each key/value pair */
    for (int i = 0; i < count; i++) {
        if (!keys[i] || !values[i])
            continue;

        /* Check for existing key — update in place */
        bool found = false;
        for (int j = 0; j < target->count; j++) {
            if (strcmp(target->strings[j].key, keys[i]) == 0) {
                free(target->strings[j].value);
                target->strings[j].value = strdup(values[i]);
                found = true;
                break;
            }
        }

        if (!found) {
            if (target->count >= ML_MAX_RT_STRINGS)
                return MAXCFG_ERR_OOM;

            MlRtString *s = &target->strings[target->count++];
            s->key = strdup(keys[i]);
            s->value = strdup(values[i]);
            if (!s->key || !s->value)
                return MAXCFG_ERR_OOM;
        }
    }

    return MAXCFG_OK;
}

void maxlang_unregister(MaxLang *lang, const char *ns)
{
    if (!lang || !ns)
        return;

    for (int i = 0; i < lang->rt_ns_count; i++) {
        if (strcmp(lang->rt_ns[i].ns, ns) != 0)
            continue;

        /* Free the namespace */
        MlRtNamespace *target = &lang->rt_ns[i];
        free(target->ns);
        for (int j = 0; j < target->count; j++) {
            free(target->strings[j].key);
            free(target->strings[j].value);
        }

        /* Shift remaining namespaces down */
        if (i + 1 < lang->rt_ns_count) {
            memmove(&lang->rt_ns[i], &lang->rt_ns[i + 1],
                    (size_t)(lang->rt_ns_count - i - 1) * sizeof(MlRtNamespace));
        }
        lang->rt_ns_count--;
        return;
    }
}
