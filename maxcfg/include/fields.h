/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * fields.h - Field definitions for maxcfg forms
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#ifndef FIELDS_H
#define FIELDS_H

#include <stdbool.h>

/* Field types */
typedef enum {
    FIELD_TEXT,         /* Free-form text input */
    FIELD_NUMBER,       /* Numeric input */
    FIELD_TOGGLE,       /* Yes/No toggle */
    FIELD_PATH,         /* File path (with validation) */
    FIELD_SELECT,       /* Selection from list */
    FIELD_FILE,         /* File picker (F2 opens picker) */
    FIELD_MULTISELECT,  /* Multi-select checkbox picker (F2 opens picker) */
    FIELD_READONLY,     /* Display only */
    FIELD_SEPARATOR     /* Blank line separator between groups */
} FieldType;

/* Field definition structure */
typedef struct {
    const char *keyword;        /* CTL file keyword (e.g., "Name", "SysOp") */
    const char *label;          /* Display label */
    const char *help;           /* Help text for bottom area */
    FieldType type;             /* Field type */
    int max_length;             /* Max input length (for text) or option count (for toggle) */
    const char *default_value;  /* Default value if not set */
    const char **toggle_options; /* Options for FIELD_TOGGLE (NULL-terminated array) */
    const char *file_filter;     /* File extension filter for FIELD_FILE (e.g., "*.bbs") */
    const char *file_base_path;  /* Base path for FIELD_FILE (e.g., "etc/misc") */
    bool can_disable;            /* If true, field can be disabled with F3 (commented out) */
    bool supports_mex;           /* If true, prefix with : for MEX .vm file */
    bool pair_with_next;         /* If true, render on same line as next field (two-column) */
} FieldDef;

/* Common toggle option sets */
extern const char *toggle_yes_no[];
extern const char *toggle_enabled_disabled[];
extern const char *toggle_on_off[];

/* Form definition structure */
typedef struct {
    const char *title;          /* Form title */
    const FieldDef *fields;     /* Array of field definitions */
    int field_count;            /* Number of fields */
} FormDef;

/* ============================================================================
 * BBS and Sysop Information (max.ctl System Section)
 * ============================================================================ */

extern const FieldDef bbs_sysop_fields[];
extern const int bbs_sysop_field_count;

/* ============================================================================
 * System Paths (max.ctl System Section)
 * ============================================================================ */

extern const FieldDef system_paths_fields[];
extern const int system_paths_field_count;

/* ============================================================================
 * Logging Options (max.ctl System Section)
 * ============================================================================ */

extern const FieldDef logging_options_fields[];
extern const int logging_options_field_count;

/* ============================================================================
 * Global Toggles (max.ctl System Section)
 * ============================================================================ */

extern const FieldDef global_toggles_fields[];
extern const int global_toggles_field_count;

/* ============================================================================
 * Login Settings (max.ctl Session Section)
 * ============================================================================ */

extern const FieldDef login_settings_fields[];
extern const int login_settings_field_count;

/* ============================================================================
 * New User Defaults (max.ctl Session Section)
 * ============================================================================ */

extern const FieldDef new_user_defaults_fields[];
extern const int new_user_defaults_field_count;

/* ============================================================================
 * Display Files (max.ctl General Filenames Section)
 * ============================================================================ */

extern const FieldDef display_files_fields[];
extern const int display_files_field_count;

/* ============================================================================
 * Message Division (msgarea.ctl MsgDivisionBegin)
 * ============================================================================ */

extern const FieldDef msg_division_fields[];
extern const int msg_division_field_count;

/* Access level options - shared by divisions and areas */
extern const char *access_level_options[];

/* ============================================================================
 * Message Area (msgarea.ctl MsgArea)
 * ============================================================================ */

extern const FieldDef msg_area_fields[];
extern const int msg_area_field_count;

/* Select options for message area fields */
extern const char *msg_format_options[];
extern const char *msg_type_options[];
extern const char *msg_name_style_options[];
extern const char *msg_division_options[];

/* ============================================================================
 * File Division (filearea.ctl FileDivisionBegin)
 * ============================================================================ */

extern const FieldDef file_division_fields[];
extern const int file_division_field_count;

/* ============================================================================
 * File Area (filearea.ctl FileArea)
 * ============================================================================ */

extern const FieldDef file_area_fields[];
extern const int file_area_field_count;

/* Select options for file area fields */
extern const char *file_division_options[];

/* ============================================================================
 * Access/Security Level (access.ctl Access)
 * ============================================================================ */

extern const FieldDef access_level_fields[];
extern const int access_level_field_count;

#endif /* FIELDS_H */
