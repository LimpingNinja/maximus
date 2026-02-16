/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * mci_helper.h - MCI code reference helper dialog for the language editor
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#ifndef MCI_HELPER_H
#define MCI_HELPER_H

/**
 * @brief Show the MCI code helper popup.
 *
 * Displays a tabbed dialog with all MCI code categories (Colors,
 * Info Codes, Terminal Control, Format Operators, Positional Params,
 * Cursor Control).  Color codes show inline color samples.
 *
 * @return Pointer to the selected MCI code string (static), or NULL
 *         if the user cancelled.  Caller should NOT free the pointer.
 */
const char *mci_helper_show(void);

#endif /* MCI_HELPER_H */
