/*
 * menu_preview.h — Menu preview renderer header
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

#ifndef MENU_PREVIEW_H
#define MENU_PREVIEW_H

#include <stdbool.h>
#include <stdint.h>
#include "menu_data.h"

#define MENU_PREVIEW_COLS 80
#define MENU_PREVIEW_ROWS 25

typedef struct {
    char ch[MENU_PREVIEW_ROWS][MENU_PREVIEW_COLS];
    uint8_t attr[MENU_PREVIEW_ROWS][MENU_PREVIEW_COLS];
} MenuPreviewVScreen;

typedef struct {
    int x;
    int y;
    int w;
    int hotkey;
    const char *desc;
} MenuPreviewItem;

typedef struct {
    MenuPreviewItem *items;
    int count;
    int cols;
} MenuPreviewLayout;

/**
 * @brief Free a previously computed menu preview layout.
 *
 * @param layout  Layout to free (items array and metadata).
 */
void menu_preview_layout_free(MenuPreviewLayout *layout);

/**
 * @brief Render a menu definition into a virtual screen and compute layout.
 *
 * Expands MCI codes in the menu's header/body/footer, positions option
 * items, and fills the virtual screen with rendered characters and DOS
 * color attributes.
 *
 * @param menu            Menu definition to render.
 * @param sys_path        System path for resolving display files.
 * @param vs              Output virtual screen (cleared and filled).
 * @param layout          Output layout describing item positions.
 * @param selected_index  Index of the currently selected option (-1 for none).
 */
void menu_preview_render(const MenuDefinition *menu,
                         const char *sys_path,
                         MenuPreviewVScreen *vs,
                         MenuPreviewLayout *layout,
                         int selected_index);

/**
 * @brief Blit a rendered virtual screen onto the ncurses display.
 *
 * Converts DOS attributes to ncurses color pairs and draws the preview
 * at the specified screen coordinates.
 *
 * @param menu            Menu definition (used for lightbar highlighting).
 * @param vs              Virtual screen to display.
 * @param layout          Layout for option highlighting.
 * @param selected_index  Currently selected option index.
 * @param x               Screen column for top-left corner.
 * @param y               Screen row for top-left corner.
 */
void menu_preview_blit(const MenuDefinition *menu,
                       const MenuPreviewVScreen *vs,
                       const MenuPreviewLayout *layout,
                       int selected_index,
                       int x,
                       int y);

/**
 * @brief Map a hotkey character to the corresponding menu option index.
 *
 * @param layout     Layout containing option hotkey mappings.
 * @param hotkey     Hotkey character to look up.
 * @param out_index  Receives the option index on success.
 * @return true if a matching hotkey was found.
 */
bool menu_preview_hotkey_to_index(const MenuPreviewLayout *layout, int hotkey, int *out_index);

/** @brief Map a DOS color index (0–7) to an ncurses COLOR_* constant. */
int dos_color_to_ncurses(int dos_color);

/** @brief Get/allocate an ncurses pair for a DOS fg/bg combination. */
int dos_pair_for_fg_bg(int fg, int bg);

/** @brief Reset the preview color-pair pool (call before each blit). */
void menu_preview_pairs_reset(void);

/** @brief Convert a CP437 byte to its Unicode equivalent (wide curses). */
wchar_t cp437_to_unicode(unsigned char b);

#endif
