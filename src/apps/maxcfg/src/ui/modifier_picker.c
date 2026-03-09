/*
 * modifier_picker.c — Privilege modifier picker
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

#include <string.h>
#include "maxcfg.h"
#include "ui.h"

/** @brief Static table of menu option modifier flags with descriptions. */
static const PickerOption modifier_options[] = {
    { "(None)", "No modifier. Option displays normally for all users.", NULL },
    { "NoDsp", "Don't display on menu (hidden). Useful for creating hotkey bindings without showing the option to users.", NULL },
    { "Ctl", "Produce .CTL file for external command. Used with some external menu actions.", NULL },
    { "NoCLS", "Don't clear screen before displaying menu. Only used with Display_Menu command.", NULL },
    { "RIP", "Display this option only to users with RIPscrip graphics enabled.", NULL },
    { "NoRIP", "Display this option only to users without RIPscrip graphics.", NULL },
    { "Then", "Execute only if the last IF condition was true.", NULL },
    { "Else", "Execute only if the last IF condition was false.", NULL },
    { "Stay", "Don't perform menu cleanup; remain in current menu context.", NULL },
    { "UsrLocal", "Display this option only when user is logged in at the local console (SysOp).", NULL },
    { "UsrRemote", "Display this option only when user is logged in remotely.", NULL },
    { "ReRead", "Re-read LASTUSER.BBS after running external command. Used with Xtern_Dos and Xtern_Run.", NULL },
    { "Local", "Display this option only in areas with Style Local attribute.", NULL },
    { "Matrix", "Display this option only in areas with Style Net attribute.", NULL },
    { "Echo", "Display this option only in areas with Style Echo attribute.", NULL },
    { "Conf", "Display this option only in areas with Style Conf attribute.", NULL },
    { NULL, NULL, NULL }
};

/**
 * @brief Display the modifier picker dialog and return the selected index.
 *
 * @param current_idx Currently selected modifier index, or -1 for none.
 * @return Selected modifier index, or -1 if cancelled.
 */
int modifier_picker_show(int current_idx)
{
    int num_options = 0;
    while (modifier_options[num_options].name) {
        num_options++;
    }
    
    return picker_with_help_show("Select Modifier", modifier_options, num_options, current_idx);
}

/**
 * @brief Get the modifier name string for a given option index.
 *
 * @param index Zero-based index into the modifier options table.
 * @return Modifier name string, or NULL if index is out of range.
 */
const char *modifier_picker_get_name(int index)
{
    int count = 0;
    while (modifier_options[count].name) {
        if (count == index) {
            return modifier_options[count].name;
        }
        count++;
    }
    return NULL;
}
