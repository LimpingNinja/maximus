/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * prm.c - PRM binary file data access for maxcfg
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 *
 * Provides read/write access to the compiled max.prm binary file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "prm_data.h"

/* Global PRM data instance */
PrmData *g_prm = NULL;

/* Internal heap management */
static char *new_heap = NULL;
static size_t new_heap_size = 0;
static size_t new_heap_used = 0;

/*
 * Load a PRM file from disk.
 */
bool prm_load(const char *filepath)
{
    FILE *fp;
    struct stat st;
    size_t heap_size;

    /* Close any existing data */
    prm_close();

    /* Get file size */
    if (stat(filepath, &st) != 0) {
        return false;
    }

    /* Open file */
    fp = fopen(filepath, "rb");
    if (!fp) {
        return false;
    }

    /* Allocate context */
    g_prm = calloc(1, sizeof(PrmData));
    if (!g_prm) {
        fclose(fp);
        return false;
    }

    /* Read just the header first to get heap_offset */
    if (fread(&g_prm->prm, 4, 1, fp) != 1) {
        fclose(fp);
        prm_close();
        return false;
    }

    /* Validate header */
    if (g_prm->prm.id != 'M') {
        fclose(fp);
        prm_close();
        return false;
    }

    /* Read the structure up to heap_offset (use file's heap_offset, not sizeof) */
    fseek(fp, 0, SEEK_SET);
    size_t read_size = g_prm->prm.heap_offset;
    if (read_size > sizeof(struct m_pointers)) {
        read_size = sizeof(struct m_pointers);
    }
    if (fread(&g_prm->prm, read_size, 1, fp) != 1) {
        fclose(fp);
        prm_close();
        return false;
    }

    /* Calculate heap size */
    heap_size = st.st_size - g_prm->prm.heap_offset;
    if (heap_size > 0 && heap_size < (size_t)(st.st_size)) {
        /* Seek to heap */
        fseek(fp, g_prm->prm.heap_offset, SEEK_SET);

        /* Allocate and read heap */
        g_prm->heap = malloc(heap_size + 1);
        if (!g_prm->heap) {
            fclose(fp);
            prm_close();
            return false;
        }

        if (fread(g_prm->heap, 1, heap_size, fp) != heap_size) {
            fclose(fp);
            prm_close();
            return false;
        }

        g_prm->heap[heap_size] = '\0';  /* Safety null terminator */
        g_prm->heap_size = heap_size;
    }

    fclose(fp);

    /* Store filepath */
    g_prm->filepath = strdup(filepath);
    g_prm->modified = false;

    /* Initialize new heap for modifications */
    new_heap_size = heap_size + 4096;  /* Extra space for growth */
    new_heap = malloc(new_heap_size);
    if (new_heap && g_prm->heap) {
        memcpy(new_heap, g_prm->heap, heap_size);
        new_heap_used = heap_size;
    } else if (new_heap) {
        new_heap_used = 0;
    }

    return true;
}

/*
 * Save the current PRM data back to disk.
 */
bool prm_save(void)
{
    if (!g_prm || !g_prm->filepath) {
        return false;
    }
    return prm_save_as(g_prm->filepath);
}

/*
 * Save the current PRM data to a new file.
 */
bool prm_save_as(const char *filepath)
{
    FILE *fp;
    struct m_pointers prm_copy;

    if (!g_prm) {
        return false;
    }

    fp = fopen(filepath, "wb");
    if (!fp) {
        return false;
    }

    /* Update heap offset if heap changed */
    prm_copy = g_prm->prm;
    prm_copy.heap_offset = sizeof(struct m_pointers);

    /* Write PRM structure */
    if (fwrite(&prm_copy, sizeof(struct m_pointers), 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    /* Write heap */
    if (new_heap && new_heap_used > 0) {
        if (fwrite(new_heap, 1, new_heap_used, fp) != new_heap_used) {
            fclose(fp);
            return false;
        }
    } else if (g_prm->heap && g_prm->heap_size > 0) {
        if (fwrite(g_prm->heap, 1, g_prm->heap_size, fp) != g_prm->heap_size) {
            fclose(fp);
            return false;
        }
    }

    fclose(fp);

    /* Update stored path if different */
    if (strcmp(filepath, g_prm->filepath) != 0) {
        free(g_prm->filepath);
        g_prm->filepath = strdup(filepath);
    }

    g_prm->modified = false;
    return true;
}

/*
 * Close and free PRM data.
 */
void prm_close(void)
{
    if (g_prm) {
        free(g_prm->heap);
        free(g_prm->filepath);
        free(g_prm);
        g_prm = NULL;
    }

    free(new_heap);
    new_heap = NULL;
    new_heap_size = 0;
    new_heap_used = 0;
}

/*
 * Get a string from the PRM heap by offset.
 */
const char *prm_string(word offset)
{
    if (!g_prm || offset == 0) {
        return "";
    }

    /* Use new heap if available, otherwise original */
    if (new_heap && offset < new_heap_used) {
        return new_heap + offset;
    }

    if (g_prm->heap && offset < g_prm->heap_size) {
        return g_prm->heap + offset;
    }

    return "";
}

/*
 * Set a string in the PRM heap.
 */
word prm_set_string(word *offset_field, const char *value)
{
    size_t len;
    word new_offset;

    if (!g_prm || !offset_field || !value) {
        return 0;
    }

    len = strlen(value) + 1;  /* Include null terminator */

    /* Ensure heap has space */
    if (new_heap_used + len > new_heap_size) {
        size_t new_size = new_heap_size + len + 4096;
        char *resized = realloc(new_heap, new_size);
        if (!resized) {
            return 0;
        }
        new_heap = resized;
        new_heap_size = new_size;
    }

    /* Add string to heap */
    new_offset = (word)new_heap_used;
    memcpy(new_heap + new_heap_used, value, len);
    new_heap_used += len;

    /* Update offset field */
    *offset_field = new_offset;
    g_prm->modified = true;

    return new_offset;
}

/*
 * Check if a flag is set in prm.flags
 */
bool prm_flag_get(word flag)
{
    if (!g_prm) {
        return false;
    }
    return (g_prm->prm.flags & flag) != 0;
}

/*
 * Set or clear a flag in prm.flags
 */
void prm_flag_set(word flag, bool value)
{
    if (!g_prm) {
        return;
    }
    if (value) {
        g_prm->prm.flags |= flag;
    } else {
        g_prm->prm.flags &= ~flag;
    }
    g_prm->modified = true;
}

/*
 * Check if a flag is set in prm.flags2
 */
bool prm_flag2_get(word flag)
{
    if (!g_prm) {
        return false;
    }
    return (g_prm->prm.flags2 & flag) != 0;
}

/*
 * Set or clear a flag in prm.flags2
 */
void prm_flag2_set(word flag, bool value)
{
    if (!g_prm) {
        return;
    }
    if (value) {
        g_prm->prm.flags2 |= flag;
    } else {
        g_prm->prm.flags2 &= ~flag;
    }
    g_prm->modified = true;
}

/*
 * Auto-detect and load the default PRM file.
 */
bool prm_load_default(void)
{
    const char *paths[] = {
        "etc/max.prm",              /* Relative to current dir */
        "../etc/max.prm",           /* One level up */
        "max.prm",                  /* Current directory */
        NULL
    };
    
    const char *maximus_env = getenv("MAXIMUS");
    char env_path[512];

    /* Try MAXIMUS environment variable first */
    if (maximus_env && *maximus_env) {
        snprintf(env_path, sizeof(env_path), "%s/etc/max.prm", maximus_env);
        if (prm_load(env_path)) {
            return true;
        }
        snprintf(env_path, sizeof(env_path), "%s/max.prm", maximus_env);
        if (prm_load(env_path)) {
            return true;
        }
    }

    /* Try common paths */
    for (int i = 0; paths[i]; i++) {
        if (prm_load(paths[i])) {
            return true;
        }
    }

    return false;
}

/*
 * Debug: Print PRM info
 */
void prm_debug_print(void)
{
    if (!g_prm) {
        printf("No PRM loaded\n");
        return;
    }

    printf("=== PRM File Info ===\n");
    printf("File: %s\n", g_prm->filepath ? g_prm->filepath : "(unknown)");
    printf("ID: '%c' (0x%02X)\n", g_prm->prm.id, g_prm->prm.id);
    printf("Version: %d\n", g_prm->prm.version);
    printf("Heap Offset: %u\n", g_prm->prm.heap_offset);
    printf("Heap Size: %zu bytes\n", g_prm->heap_size);
    printf("Modified: %s\n", g_prm->modified ? "yes" : "no");
    printf("\n");

    printf("=== System Info ===\n");
    printf("System Name: %s\n", prm_string(g_prm->prm.system_name));
    printf("SysOp: %s\n", prm_string(g_prm->prm.sysop));
    printf("Task: %d\n", g_prm->prm.task_num);
    printf("Log Mode: %d\n", g_prm->prm.log_mode);
    printf("\n");

    printf("=== Paths ===\n");
    printf("System Path: %s\n", prm_string(g_prm->prm.sys_path));
    printf("Misc Path: %s\n", prm_string(g_prm->prm.misc_path));
    printf("Language Path: %s\n", prm_string(g_prm->prm.lang_path));
    printf("Temp Path: %s\n", prm_string(g_prm->prm.temppath));
    printf("IPC Path: %s\n", prm_string(g_prm->prm.ipc_path));
    printf("User File: %s\n", prm_string(g_prm->prm.user_file));
    printf("Log File: %s\n", prm_string(g_prm->prm.log_name));
    printf("\n");

    printf("=== Flags ===\n");
    printf("Snoop: %s\n", prm_flag_get(FLAG_snoop) ? "Yes" : "No");
    printf("Watchdog: %s\n", prm_flag_get(FLAG_watchdog) ? "Yes" : "No");
    printf("Status Line: %s\n", prm_flag_get(FLAG_statusline) ? "Yes" : "No");
    printf("Ask Phone: %s\n", prm_flag_get(FLAG_ask_phone) ? "Yes" : "No");
    printf("Alias System: %s\n", prm_flag_get(FLAG_alias) ? "Yes" : "No");
    printf("Ask Name: %s\n", prm_flag_get(FLAG_ask_name) ? "Yes" : "No");
    printf("\n");

    printf("=== Flags2 ===\n");
    printf("Local Timeout: %s\n", prm_flag2_get(FLAG2_ltimeout) ? "Yes" : "No");
    printf("No Share: %s\n", prm_flag2_get(FLAG2_noshare) ? "Yes" : "No");
    printf("Swap Out: %s\n", prm_flag2_get(FLAG2_SWAPOUT) ? "Yes" : "No");
    printf("No Encrypt: %s\n", prm_flag2_get(FLAG2_NOENCRYPT) ? "Yes" : "No");
    printf("Check ANSI: %s\n", prm_flag2_get(FLAG2_CHKANSI) ? "Yes" : "No");
    printf("Check RIP: %s\n", prm_flag2_get(FLAG2_CHKRIP) ? "Yes" : "No");
    printf("Single Name: %s\n", prm_flag2_get(FLAG2_1NAME) ? "Yes" : "No");
    printf("\n");

    printf("=== Login Settings ===\n");
    printf("Logon Priv: %u\n", g_prm->prm.logon_priv);
    printf("Logon Time: %u\n", g_prm->prm.logon_time);
    printf("Min Baud: %u\n", g_prm->prm.min_baud);
    printf("Graphics Baud: %u\n", g_prm->prm.speed_graphics);
    printf("RIP Baud: %u\n", g_prm->prm.speed_rip);
    printf("Input Timeout: %u\n", g_prm->prm.input_timeout);
    printf("\n");

    printf("=== Display Files (sample) ===\n");
    printf("Logo: %s\n", prm_string(g_prm->prm.logo));
    printf("Welcome: %s\n", prm_string(g_prm->prm.welcome));
    printf("Byebye: %s\n", prm_string(g_prm->prm.byebye));
    printf("First Menu: %s\n", prm_string(g_prm->prm.first_menu));
    printf("Begin Msg Area: %s\n", prm_string(g_prm->prm.begin_msgarea));
    printf("Begin File Area: %s\n", prm_string(g_prm->prm.begin_filearea));
}
