/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * texteditor.h - Full-screen text editor for display files
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <stdbool.h>

/* Editor result codes */
typedef enum {
    EDITOR_SAVED = 0,
    EDITOR_CANCELLED = 1,
    EDITOR_ERROR = 2
} EditorResult;

/* File type detection for syntax highlighting */
typedef enum {
    FILETYPE_UNKNOWN = 0,
    FILETYPE_MECCA,      /* .mec files */
    FILETYPE_MEX,        /* .mex files */
    FILETYPE_TEXT        /* Plain text/display files */
} FileType;

/*
 * Launch full-screen text editor for a file
 * 
 * Parameters:
 *   filepath - Absolute path to file to edit
 *   
 * Returns:
 *   EDITOR_SAVED if file was saved
 *   EDITOR_CANCELLED if user exited without saving
 *   EDITOR_ERROR on error
 */
EditorResult text_editor_edit(const char *filepath);

#endif /* TEXTEDITOR_H */
