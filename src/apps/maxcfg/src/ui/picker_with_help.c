#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "maxcfg.h"
#include "ui.h"

typedef struct {
    const char *name;
    int *indices;
    int count;
    int capacity;
} Category;

int picker_with_help_show(const char *title, const PickerOption *options, int num_options, int current_idx)
{
    if (!options || num_options == 0) return -1;
    
    Category *categories = NULL;
    int num_categories = 0;
    int current_category = 0;
    bool has_categories = false;
    
    if (options[0].category) {
        has_categories = true;
        categories = calloc(20, sizeof(Category));
        if (!categories) return -1;
        
        for (int i = 0; i < num_options; i++) {
            const char *cat = options[i].category ? options[i].category : "Other";
            int found = -1;
            for (int j = 0; j < num_categories; j++) {
                if (strcmp(categories[j].name, cat) == 0) {
                    found = j;
                    break;
                }
            }
            if (found == -1) {
                categories[num_categories].name = cat;
                categories[num_categories].indices = calloc(num_options, sizeof(int));
                if (!categories[num_categories].indices) {
                    for (int k = 0; k < num_categories; k++) {
                        free(categories[k].indices);
                    }
                    free(categories);
                    return -1;
                }
                categories[num_categories].capacity = num_options;
                categories[num_categories].indices[0] = i;
                categories[num_categories].count = 1;
                num_categories++;
            } else {
                categories[found].indices[categories[found].count] = i;
                categories[found].count++;
            }
        }
        
        if (current_idx >= 0) {
            for (int i = 0; i < num_categories; i++) {
                for (int j = 0; j < categories[i].count; j++) {
                    if (categories[i].indices[j] == current_idx) {
                        current_category = i;
                        break;
                    }
                }
            }
        }
    }
    
    int max_width = title ? strlen(title) : 0;
    for (int i = 0; i < num_options; i++) {
        int len = strlen(options[i].name);
        if (len > max_width) max_width = len;
    }
    
    int width = max_width + 6;
    if (width < 40) width = 40;
    
    int help_height = 4;
    int list_height = num_options;
    int max_list_visible = LINES - help_height - 8;
    if (list_height > max_list_visible) list_height = max_list_visible;
    
    int height = list_height + help_height + 4;
    
    int x = (COLS - width) / 2;
    int y = (LINES - height) / 2;
    
    int selected = current_idx >= 0 && current_idx < num_options ? current_idx : 0;
    int scroll_offset = 0;
    
    if (has_categories && num_categories > 0) {
        if (current_idx >= 0 && current_idx < num_options) {
            int found_in_category = 0;
            for (int i = 0; i < categories[current_category].count; i++) {
                if (categories[current_category].indices[i] == current_idx) {
                    found_in_category = 1;
                    if (i < list_height) {
                        scroll_offset = 0;
                    } else if (i >= categories[current_category].count - list_height) {
                        scroll_offset = categories[current_category].count - list_height;
                        if (scroll_offset < 0) scroll_offset = 0;
                    } else {
                        scroll_offset = i - list_height / 2;
                        if (scroll_offset < 0) scroll_offset = 0;
                    }
                    break;
                }
            }
            if (!found_in_category) {
                selected = categories[current_category].indices[0];
                scroll_offset = 0;
            }
        } else {
            selected = categories[current_category].indices[0];
            scroll_offset = 0;
        }
    } else {
        if (selected >= list_height) {
            scroll_offset = selected - list_height + 1;
        }
    }
    
    bool done = false;
    int result = -1;
    
    curs_set(0);
    
    while (!done) {
        int display_count = has_categories ? categories[current_category].count : num_options;
        
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        
        for (int row = 0; row < height; row++) {
            mvhline(y + row, x, ' ', width);
        }
        
        mvaddch(y, x, ACS_ULCORNER);
        for (int i = 1; i < width - 1; i++) {
            addch(ACS_HLINE);
        }
        addch(ACS_URCORNER);
        
        if (title) {
            int title_len = strlen(title);
            int title_x = x + (width - title_len) / 2;
            mvaddch(y, title_x - 1, ' ');
            attron(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);
            mvprintw(y, title_x, "%s", title);
            attroff(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            addch(' ');
        }
        
        if (has_categories && num_categories > 1) {
            int tab_y = y + 1;
            int center_x = x + width / 2;
            
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvhline(tab_y, x + 1, ' ', width - 2);
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
            
            int current_tab_len = strlen(categories[current_category].name) + 3;
            int current_tab_start = center_x - current_tab_len / 2;
            
            attron(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
            mvprintw(tab_y, current_tab_start, " %s ", categories[current_category].name);
            attroff(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
            
            int left_x = current_tab_start - 1;
            for (int i = current_category - 1; i >= 0 && left_x > x + 2; i--) {
                int tab_len = strlen(categories[i].name) + 3;
                if (left_x - tab_len < x + 2) break;
                left_x -= tab_len;
                attron(COLOR_PAIR(CP_MENU_BAR));
                mvprintw(tab_y, left_x, " %s ", categories[i].name);
                attroff(COLOR_PAIR(CP_MENU_BAR));
            }
            
            int right_x = current_tab_start + current_tab_len;
            for (int i = current_category + 1; i < num_categories && right_x < x + width - 2; i++) {
                int tab_len = strlen(categories[i].name) + 3;
                if (right_x + tab_len > x + width - 2) break;
                attron(COLOR_PAIR(CP_MENU_BAR));
                mvprintw(tab_y, right_x, " %s ", categories[i].name);
                attroff(COLOR_PAIR(CP_MENU_BAR));
                right_x += tab_len;
            }
            
            if (current_category > 0) {
                attron(COLOR_PAIR(CP_DIALOG_BORDER));
                mvaddch(tab_y, x + 1, ACS_LARROW);
                attroff(COLOR_PAIR(CP_DIALOG_BORDER));
            }
            if (current_category < num_categories - 1) {
                attron(COLOR_PAIR(CP_DIALOG_BORDER));
                mvaddch(tab_y, x + width - 2, ACS_RARROW);
                attroff(COLOR_PAIR(CP_DIALOG_BORDER));
            }
        }
        
        int content_y_offset = has_categories && num_categories > 1 ? 2 : 1;
        
        for (int i = 1; i < height - 1; i++) {
            mvaddch(y + i, x, ACS_VLINE);
            mvaddch(y + i, x + width - 1, ACS_VLINE);
        }
        
        for (int i = 0; i < list_height && (i + scroll_offset) < display_count; i++) {
            int opt_idx = has_categories ? categories[current_category].indices[i + scroll_offset] : (i + scroll_offset);
            int row = y + content_y_offset + i;
            
            if (opt_idx == selected) {
                attron(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
                mvprintw(row, x + 2, "%-*s", width - 4, options[opt_idx].name);
                attroff(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
            } else {
                attron(COLOR_PAIR(CP_DIALOG_TEXT));
                mvprintw(row, x + 2, "%-*s", width - 4, options[opt_idx].name);
                attroff(COLOR_PAIR(CP_DIALOG_TEXT));
            }
        }
        
        if (scroll_offset > 0) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvaddch(y + content_y_offset, x + width - 2, ACS_UARROW);
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }
        if (scroll_offset + list_height < display_count) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvaddch(y + content_y_offset + list_height - 1, x + width - 2, ACS_DARROW);
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }
        
        int separator_y = y + content_y_offset + list_height;
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        mvaddch(separator_y, x, ACS_LTEE);
        for (int i = 1; i < width - 1; i++) {
            addch(ACS_HLINE);
        }
        mvaddch(separator_y, x + width - 1, ACS_RTEE);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        
        const char *help_text = options[selected].help;
        int help_y = separator_y + 1;
        int help_x = x + 2;
        int help_width = width - 4;
        
        attron(COLOR_PAIR(CP_MENU_BAR));
        for (int line = 0; line < help_height - 1; line++) {
            mvhline(help_y + line, help_x, ' ', help_width);
        }
        
        if (help_text) {
            int cur_y = help_y;
            int cur_x = 0;
            const char *p = help_text;
            char word[80];
            int word_len;
            
            while (*p && cur_y < help_y + help_height - 1) {
                word_len = 0;
                while (*p && !isspace(*p) && word_len < 79) {
                    word[word_len++] = *p++;
                }
                word[word_len] = '\0';
                
                if (cur_x + word_len >= help_width && cur_x > 0) {
                    cur_y++;
                    cur_x = 0;
                }
                
                if (cur_y < help_y + help_height - 1 && word_len > 0) {
                    mvprintw(cur_y, help_x + cur_x, "%s", word);
                    cur_x += word_len;
                }
                
                while (*p && isspace(*p)) {
                    if (*p == '\n') {
                        cur_y++;
                        cur_x = 0;
                    } else if (cur_x < help_width) {
                        cur_x++;
                    }
                    p++;
                }
            }
        }
        attroff(COLOR_PAIR(CP_MENU_BAR));
        
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        mvaddch(y + height - 1, x, ACS_LLCORNER);
        addch(ACS_HLINE);
        addch(' ');
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        
        attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
        printw("ENTER");
        attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
        attron(COLOR_PAIR(CP_MENU_BAR));
        printw("=Sel");
        attroff(COLOR_PAIR(CP_MENU_BAR));
        
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        printw(" ");
        addch(ACS_HLINE);
        printw(" ");
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        
        attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
        printw("ESC");
        attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
        attron(COLOR_PAIR(CP_MENU_BAR));
        printw("=Cancel");
        attroff(COLOR_PAIR(CP_MENU_BAR));
        
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        printw(" ");
        int cur_x = getcurx(stdscr);
        for (int i = cur_x; i < x + width - 1; i++) {
            addch(ACS_HLINE);
        }
        mvaddch(y + height - 1, x + width - 1, ACS_LRCORNER);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        
        refresh();
        
        int ch = getch();
        switch (ch) {
            case KEY_LEFT:
                if (has_categories && num_categories > 1 && current_category > 0) {
                    current_category--;
                    selected = categories[current_category].indices[0];
                    scroll_offset = 0;
                }
                break;
                
            case KEY_RIGHT:
                if (has_categories && num_categories > 1 && current_category < num_categories - 1) {
                    current_category++;
                    selected = categories[current_category].indices[0];
                    scroll_offset = 0;
                }
                break;
                
            case KEY_UP:
            case 'k':
                if (has_categories) {
                    int cur_pos = -1;
                    for (int i = 0; i < display_count; i++) {
                        if (categories[current_category].indices[i] == selected) {
                            cur_pos = i;
                            break;
                        }
                    }
                    if (cur_pos > 0) {
                        selected = categories[current_category].indices[cur_pos - 1];
                        if (cur_pos - 1 < scroll_offset) {
                            scroll_offset = cur_pos - 1;
                        }
                    }
                } else {
                    if (selected > 0) {
                        selected--;
                        if (selected < scroll_offset) {
                            scroll_offset = selected;
                        }
                    }
                }
                break;
                
            case KEY_DOWN:
            case 'j':
                if (has_categories) {
                    int cur_pos = -1;
                    for (int i = 0; i < display_count; i++) {
                        if (categories[current_category].indices[i] == selected) {
                            cur_pos = i;
                            break;
                        }
                    }
                    if (cur_pos >= 0 && cur_pos < display_count - 1) {
                        selected = categories[current_category].indices[cur_pos + 1];
                        if (cur_pos + 1 >= scroll_offset + list_height) {
                            scroll_offset = cur_pos + 1 - list_height + 1;
                        }
                    }
                } else {
                    if (selected < num_options - 1) {
                        selected++;
                        if (selected >= scroll_offset + list_height) {
                            scroll_offset = selected - list_height + 1;
                        }
                    }
                }
                break;
                
            case KEY_HOME:
                if (has_categories) {
                    selected = categories[current_category].indices[0];
                } else {
                    selected = 0;
                }
                scroll_offset = 0;
                break;
                
            case KEY_END:
                if (has_categories) {
                    selected = categories[current_category].indices[display_count - 1];
                    if (display_count > list_height) {
                        scroll_offset = display_count - list_height;
                    }
                } else {
                    selected = num_options - 1;
                    if (selected >= list_height) {
                        scroll_offset = selected - list_height + 1;
                    }
                }
                break;
                
            case '\n':
            case '\r':
                result = selected;
                done = true;
                break;
                
            case 27:
                result = -1;
                done = true;
                break;
        }
    }
    
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    
    if (categories) {
        for (int i = 0; i < num_categories; i++) {
            free(categories[i].indices);
        }
        free(categories);
    }
    
    return result;
}
