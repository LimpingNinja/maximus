/*
 * Maximus Version 3.02
 * Copyright 1989, 2002 by Lanius Corporation.  All rights reserved.
 *
 * Modifications Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 * https://github.com/LimpingNinja
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

/*# name=Test command for paged lightbar list primitive
*/

#define MAX_LANG_global
#define MAX_LANG_m_area
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "prog.h"
#include "max_msg.h"
#include "ui_lightbar.h"

#define TEST_LIST_COUNT 200

/**
 * @brief Callback to format dummy test items
 */
static int test_list_get_item(void *ctx, int index, char *out, size_t out_sz)
{
  (void)ctx;
  snprintf(out, out_sz, "Item %3d - This is a test entry for the paged lightbar list", index + 1);
  return 0;
}

/**
 * @brief Test command for paged lightbar list primitive
 * 
 * Creates a dummy list of 200 items and displays it with the lightbar list helper.
 * This allows testing of paging behavior (Up/Down, PgUp/PgDn, Home/End) without
 * involving message data.
 */
void Msg_ListTest(void)
{
  ui_lightbar_list_t list;
  int result;
  
  /* Clear screen and show header */
  Puts(CLS);
  Printf("\n\x16\x01\x1f Paged Lightbar List Test \x16\x07\n\n");
  Printf("Testing with %d dummy items. Use:\n", TEST_LIST_COUNT);
  Printf("  Up/Down    - Navigate\n");
  Printf("  PgUp/PgDn  - Page by screen height\n");
  Printf("  Home/End   - Jump to first/last\n");
  Printf("  Enter      - Select item\n");
  Printf("  ESC        - Cancel\n\n");
  
  Press_ENTER();
  
  /* Configure the list */
  memset(&list, 0, sizeof(list));
  list.x = 1;
  list.y = 3;
  list.width = 78;
  list.height = 20;
  list.count = TEST_LIST_COUNT;
  list.initial_index = 0;
  list.normal_attr = 0x07;    /* White on black */
  list.selected_attr = 0x70;  /* Black on white (inverse) */
  list.wrap = 0;              /* No wrapping */
  list.get_item = test_list_get_item;
  list.ctx = NULL;
  
  /* Clear screen and draw header */
  Puts(CLS);
  Printf("\x16\x01\x1f Paged Lightbar List Test - %d items \x16\x07\n\n", TEST_LIST_COUNT);
  
  /* Run the list */
  result = ui_lightbar_list_run(&list);
  
  /* Show result */
  Puts(CLS);
  if (result >= 0)
  {
    Printf("\n\x16\x0e You selected item #%d\x16\x07\n\n", result + 1);
  }
  else
  {
    Printf("\n\x16\x0c Cancelled (ESC pressed)\x16\x07\n\n");
  }
  
  Press_ENTER();
}
