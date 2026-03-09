/*
 * mci_helper.h — MCI code helper dialog header
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
