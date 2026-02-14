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

void menu_preview_layout_free(MenuPreviewLayout *layout);

void menu_preview_render(const MenuDefinition *menu,
                         MenuPreviewVScreen *vs,
                         MenuPreviewLayout *layout,
                         int selected_index);

void menu_preview_blit(const MenuDefinition *menu,
                       const MenuPreviewVScreen *vs,
                       const MenuPreviewLayout *layout,
                       int selected_index,
                       int x,
                       int y);

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
