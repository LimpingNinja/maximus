/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * prm_data.h - PRM binary file data access for maxcfg
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 *
 * Provides read/write access to the compiled max.prm binary file.
 */

#ifndef PRM_DATA_H
#define PRM_DATA_H

#include <stdbool.h>

/* Include the actual Maximus headers for struct m_pointers */
#include "prog.h"
#include "prm.h"

/*
 * PRM data context - holds loaded PRM and heap
 */
typedef struct {
    struct m_pointers prm;  /* The PRM structure (from prm.h) */
    char *heap;             /* String heap (called 'offsets' in Maximus) */
    size_t heap_size;       /* Size of heap in bytes */
    char *filepath;         /* Path to loaded .prm file */
    bool modified;          /* True if data has been modified */
} PrmData;

/* Global PRM data instance */
extern PrmData *g_prm;

/* Macro to access strings, same as Maximus uses */
#define PRMSTR(s) (g_prm->heap + g_prm->prm.s)

/* Load a PRM file from disk */
bool prm_load(const char *filepath);

/* Save the current PRM data back to disk */
bool prm_save(void);

/* Save the current PRM data to a new file */
bool prm_save_as(const char *filepath);

/* Close and free PRM data */
void prm_close(void);

/* Get a string from the PRM heap by offset */
const char *prm_string(word offset);

/* Set a string in the PRM heap */
word prm_set_string(word *offset_field, const char *value);

/* Check/set flags in prm.flags */
bool prm_flag_get(word flag);
void prm_flag_set(word flag, bool value);

/* Check/set flags in prm.flags2 */
bool prm_flag2_get(word flag);
void prm_flag2_set(word flag, bool value);

/* Auto-detect and load the default PRM file */
bool prm_load_default(void);

/* Debug print */
void prm_debug_print(void);

#endif /* PRM_DATA_H */
