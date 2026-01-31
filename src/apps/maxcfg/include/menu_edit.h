#ifndef MENU_EDIT_H
#define MENU_EDIT_H

#include <stdbool.h>
#include "menu_data.h"

typedef struct {
    const char *sys_path;
    MenuDefinition **menus;
    int menu_count;
    MenuDefinition *current_menu;
    bool *options_modified;
} MenuEditContext;

void menu_load_properties_form(MenuDefinition *menu, char **values);
bool menu_save_properties_form(MenuDefinition *menu, char **values);

void menu_load_option_form(MenuOption *opt, char **values);
bool menu_save_option_form(MenuOption *opt, char **values);

void menu_free_values(char **values, int count);

#endif
