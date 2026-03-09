/*
 * privilege_picker.c — Privilege level picker dialog
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

/** @brief Static table of Maximus privilege levels with descriptions. */
static const PickerOption privilege_options[] = {
    { "Twit", "Lowest access level. User has minimal privileges and restricted access to system features.", NULL },
    { "Disgrace", "Disgraced user. Limited access, typically used as punishment or probation status.", NULL },
    { "Limited", "Limited access user. More privileges than Twit but still restricted in many areas.", NULL },
    { "Normal", "Standard user access level. Default for regular callers with typical BBS privileges.", NULL },
    { "Worthy", "Above-average user. Trusted caller with additional privileges beyond normal users.", NULL },
    { "Privil", "Privileged user. Enhanced access to system features and areas.", NULL },
    { "Favored", "Favored user. High-trust level with extensive privileges and access.", NULL },
    { "Extra", "Extra privileges. Near-staff level access with special permissions.", NULL },
    { "Clerk", "Clerk level. Staff member with administrative duties and elevated access.", NULL },
    { "AsstSysop", "Assistant System Operator. Co-sysop with near-full system control.", NULL },
    { "Sysop", "System Operator. Full system access and control over all BBS operations.", NULL },
    { "Hidden", "Hidden access level. Special privilege level for testing or hidden features.", NULL },
    { NULL, NULL, NULL }
};

/**
 * @brief Display the privilege level picker dialog and return the selected index.
 *
 * @param current_idx Currently selected privilege index, or -1 for none.
 * @return Selected privilege index, or -1 if cancelled.
 */
int privilege_picker_show(int current_idx)
{
    int num_options = 0;
    while (privilege_options[num_options].name) {
        num_options++;
    }
    
    return picker_with_help_show("Select Privilege Level", privilege_options, num_options, current_idx);
}

/**
 * @brief Get the privilege level name string for a given option index.
 *
 * @param index Zero-based index into the privilege options table.
 * @return Privilege level name string, or NULL if index is out of range.
 */
const char *privilege_picker_get_name(int index)
{
    int count = 0;
    while (privilege_options[count].name) {
        if (count == index) {
            return privilege_options[count].name;
        }
        count++;
    }
    return NULL;
}
