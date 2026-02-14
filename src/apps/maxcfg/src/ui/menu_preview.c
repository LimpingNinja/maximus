#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <wchar.h>

#include "maxcfg.h"
#include "ui.h"
#include "menu_preview.h"
#include "mci_preview.h"

static int ansi_color_to_dos_color(int ansi)
{
    /*
     * ANSI SGR colors (30-37 / 40-47) are ordered:
     *   0=Black,1=Red,2=Green,3=Yellow,4=Blue,5=Magenta,6=Cyan,7=White
     *
     * DOS palette indices used by Maximus attrs are ordered:
     *   0=Black,1=Blue,2=Green,3=Cyan,4=Red,5=Magenta,6=Brown,7=Gray
     */
    static const int map[8] = {
        0, /* black */
        4, /* red */
        2, /* green */
        6, /* yellow -> brown */
        1, /* blue */
        5, /* magenta */
        3, /* cyan */
        7  /* white -> light gray */
    };

    if (ansi < 0) ansi = 0;
    if (ansi > 7) ansi = 7;
    return map[ansi];
}

wchar_t cp437_to_unicode(unsigned char b)
{
    /* Map common CP437 graphics used by ANSI art to Unicode equivalents. */
    switch (b) {
        case 0xB0: return 0x2591; /* light shade */
        case 0xB1: return 0x2592; /* medium shade */
        case 0xB2: return 0x2593; /* dark shade */
        case 0xDB: return 0x2588; /* full block */
        case 0xDC: return 0x2584; /* lower half block */
        case 0xDD: return 0x258C; /* left half block */
        case 0xDE: return 0x2590; /* right half block */
        case 0xDF: return 0x2580; /* upper half block */

        /* Single-line box drawing */
        case 0xB3: return 0x2502; /* │ */
        case 0xC4: return 0x2500; /* ─ */
        case 0xDA: return 0x250C; /* ┌ */
        case 0xBF: return 0x2510; /* ┐ */
        case 0xC0: return 0x2514; /* └ */
        case 0xD9: return 0x2518; /* ┘ */
        case 0xC3: return 0x251C; /* ├ */
        case 0xB4: return 0x2524; /* ┤ */
        case 0xC2: return 0x252C; /* ┬ */
        case 0xC1: return 0x2534; /* ┴ */
        case 0xC5: return 0x253C; /* ┼ */

        /* Double-line box drawing (common in ANSI art) */
        case 0xCD: return 0x2550; /* ═ */
        case 0xBA: return 0x2551; /* ║ */
        case 0xC9: return 0x2554; /* ╔ */
        case 0xBB: return 0x2557; /* ╗ */
        case 0xC8: return 0x255A; /* ╚ */
        case 0xBC: return 0x255D; /* ╝ */
        case 0xCC: return 0x2560; /* ╠ */
        case 0xB9: return 0x2563; /* ╣ */
        case 0xCB: return 0x2566; /* ╦ */
        case 0xCA: return 0x2569; /* ╩ */
        case 0xCE: return 0x256C; /* ╬ */
        default:
            /* Best-effort: treat as Latin-1 codepoint */
            return (wchar_t)b;
    }
}

static void vs_clear(MenuPreviewVScreen *s, char fill)
{
    if (!s) return;
    for (int y = 0; y < MENU_PREVIEW_ROWS; y++) {
        for (int x = 0; x < MENU_PREVIEW_COLS; x++) {
            s->ch[y][x] = fill;
            s->attr[y][x] = 0x07;
        }
    }
}

static void vs_put(MenuPreviewVScreen *s, int x, int y, char c)
{
    if (!s) return;
    if (x < 0 || y < 0 || x >= MENU_PREVIEW_COLS || y >= MENU_PREVIEW_ROWS) return;
    s->ch[y][x] = c;
}

static void vs_put_attr(MenuPreviewVScreen *s, int x, int y, char c, uint8_t attr)
{
    if (!s) return;
    if (x < 0 || y < 0 || x >= MENU_PREVIEW_COLS || y >= MENU_PREVIEW_ROWS) return;
    s->ch[y][x] = c;
    s->attr[y][x] = attr;
}

static void vs_putn(MenuPreviewVScreen *s, int x, int y, const char *str, int n)
{
    if (!s || !str) return;
    if (y < 0 || y >= MENU_PREVIEW_ROWS) return;
    if (x < 0) {
        int skip = -x;
        if (skip >= n) return;
        str += skip;
        n -= skip;
        x = 0;
    }
    if (x >= MENU_PREVIEW_COLS) return;
    if (x + n > MENU_PREVIEW_COLS) n = MENU_PREVIEW_COLS - x;
    for (int i = 0; i < n; i++) {
        char c = str[i];
        if (c == '\0') break;
        s->ch[y][x + i] = c;
    }
}

static void vs_putn_attr(MenuPreviewVScreen *s, int x, int y, const char *str, int n, uint8_t attr)
{
    if (!s || !str) return;
    if (y < 0 || y >= MENU_PREVIEW_ROWS) return;
    if (x < 0) {
        int skip = -x;
        if (skip >= n) return;
        str += skip;
        n -= skip;
        x = 0;
    }
    if (x >= MENU_PREVIEW_COLS) return;
    if (x + n > MENU_PREVIEW_COLS) n = MENU_PREVIEW_COLS - x;
    for (int i = 0; i < n; i++) {
        char c = str[i];
        if (c == '\0') break;
        s->ch[y][x + i] = c;
        s->attr[y][x + i] = attr;
    }
}

static void vs_puts(MenuPreviewVScreen *s, int x, int y, const char *str)
{
    if (!s || !str) return;
    vs_putn(s, x, y, str, (int)strlen(str));
}

static int color_name_to_value(const char *name)
{
    if (!name || !*name) return -1;
    if (strcasecmp(name, "Black") == 0) return 0;
    if (strcasecmp(name, "Blue") == 0) return 1;
    if (strcasecmp(name, "Green") == 0) return 2;
    if (strcasecmp(name, "Cyan") == 0) return 3;
    if (strcasecmp(name, "Red") == 0) return 4;
    if (strcasecmp(name, "Magenta") == 0) return 5;
    if (strcasecmp(name, "Brown") == 0) return 6;
    if (strcasecmp(name, "Gray") == 0 || strcasecmp(name, "Grey") == 0) return 7;
    if (strcasecmp(name, "Light Gray") == 0 || strcasecmp(name, "Light Grey") == 0) return 7;
    if (strcasecmp(name, "DarkGray") == 0 || strcasecmp(name, "DarkGrey") == 0) return 8;
    if (strcasecmp(name, "Dark Gray") == 0 || strcasecmp(name, "Dark Grey") == 0) return 8;
    if (strcasecmp(name, "LightGray") == 0 || strcasecmp(name, "LightGrey") == 0) return 7;
    if (strcasecmp(name, "LightBlue") == 0 || strcasecmp(name, "Light Blue") == 0) return 9;
    if (strcasecmp(name, "LightGreen") == 0 || strcasecmp(name, "Light Green") == 0) return 10;
    if (strcasecmp(name, "LightCyan") == 0 || strcasecmp(name, "Light Cyan") == 0) return 11;
    if (strcasecmp(name, "LightRed") == 0 || strcasecmp(name, "Light Red") == 0) return 12;
    if (strcasecmp(name, "LightMagenta") == 0 || strcasecmp(name, "Light Magenta") == 0) return 13;
    if (strcasecmp(name, "Yellow") == 0) return 14;
    if (strcasecmp(name, "White") == 0) return 15;
    return -1;
}

static uint8_t make_dos_attr(const char *fg_name, const char *bg_name)
{
    int fg = color_name_to_value(fg_name);
    int bg = color_name_to_value(bg_name);
    if (fg < 0) fg = 7;
    if (bg < 0) bg = 0;
    if (bg > 7) bg = 0;
    return (uint8_t)((bg << 4) | (fg & 0x0f));
}

/**
 * @brief Load and render an ANSI file into the virtual screen buffer.
 *
 * Parses basic ANSI escape sequences (SGR color codes, cursor positioning)
 * and writes characters with DOS-style attributes into the virtual screen.
 *
 * @param vs Virtual screen buffer to render into
 * @param filepath Absolute path to .ans file
 */
static void ansi_load_file(MenuPreviewVScreen *vs, const char *filepath)
{
    if (!vs || !filepath || !*filepath) return;

    FILE *fp = fopen(filepath, "rb");
    if (!fp) return;

    int cx = 0, cy = 0;
    uint8_t fg = 7, bg = 0;
    int bright = 0;
    uint8_t current_attr = 0x07;

    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        /* DOS/CP/M-style EOF marker seen in some art/files. */
        if (ch == 0x1a) {
            break;
        }
        if (ch == 0x1b) {
            /* ANSI escape sequence */
            int next = fgetc(fp);
            if (next == '[') {
                /* CSI sequence */
                char params[64];
                int pi = 0;
                while (pi < 63) {
                    int c = fgetc(fp);
                    if (c == EOF) break;
                    if (c >= 0x40 && c <= 0x7e) {
                        params[pi] = '\0';
                        /* Parse CSI command */
                        if (c == 'm') {
                            /* SGR - Set Graphics Rendition */
                            if (pi == 0 || params[0] == '\0') {
                                /* Reset */
                                fg = 7; bg = 0; bright = 0;
                            } else {
                                char *tok = strtok(params, ";");
                                while (tok) {
                                    int code = atoi(tok);
                                    if (code == 0) { 
                                        fg = 7; bg = 0; bright = 0; 
                                    }
                                    else if (code == 1) {
                                        bright = 1;
                                        if (fg < 8) {
                                            fg = (uint8_t)(fg | 8);
                                        }
                                    }
                                    else if (code == 22) {
                                        /* Normal intensity (undo bright/bold) */
                                        bright = 0;
                                        if (fg >= 8) {
                                            fg = (uint8_t)(fg & 7);
                                        }
                                    }
                                    else if (code >= 30 && code <= 37) {
                                        fg = (uint8_t)ansi_color_to_dos_color(code - 30);
                                        if (bright) fg = (uint8_t)(fg | 8);
                                    }
                                    else if (code >= 40 && code <= 47) {
                                        bg = (uint8_t)ansi_color_to_dos_color(code - 40);
                                        if (bg > 7) bg = 0;
                                    }
                                    else if (code == 39) {
                                        fg = 7;
                                        if (bright) fg = (uint8_t)(fg | 8);
                                    }
                                    else if (code == 49) {
                                        bg = 0;
                                    }
                                    tok = strtok(NULL, ";");
                                }
                            }
                            current_attr = (uint8_t)((bg << 4) | (fg & 0x0f));
                        } else if (c == 'H' || c == 'f') {
                            /* Cursor position */
                            int row = 1, col = 1;
                            if (pi > 0 && params[0]) {
                                if (sscanf(params, "%d;%d", &row, &col) < 1) row = 1;
                            }
                            cy = row - 1;
                            cx = col - 1;
                            if (cy < 0) cy = 0;
                            if (cx < 0) cx = 0;
                        } else if (c == 'J') {
                            /* Erase display - ignore for now */
                        } else if (c == 'K') {
                            /* Erase line - ignore for now */
                        }
                        break;
                    }
                    params[pi++] = (char)c;
                }
            }
        } else if (ch == '\r') {
            cx = 0;
        } else if (ch == '\n') {
            cy++;
            cx = 0;
        } else if (ch == '\t') {
            cx = (cx + 8) & ~7;
            while (cx >= MENU_PREVIEW_COLS) {
                cx -= MENU_PREVIEW_COLS;
                cy++;
            }
        } else if (ch >= 0x20 || (ch >= 0x80 && ch <= 0xFF)) {
            /* Printable ASCII (0x20-0x7F) or CP437 extended (0x80-0xFF) */

            /* Auto-wrap at 80 columns (common in ANSI art). */
            if (cx >= MENU_PREVIEW_COLS) {
                cx = 0;
                cy++;
            }

            if (cy >= 0 && cy < MENU_PREVIEW_ROWS && cx >= 0 && cx < MENU_PREVIEW_COLS) {
                vs->ch[cy][cx] = (char)ch;
                vs->attr[cy][cx] = current_attr;
            }
            cx++;

            /* Wrap after printing last column. */
            if (cx >= MENU_PREVIEW_COLS) {
                cx = 0;
                cy++;
            }
        }

        if (cy >= MENU_PREVIEW_ROWS) {
            break;
        }
    }

    fclose(fp);
}

static bool layout_alloc(MenuPreviewLayout *layout, int count)
{
    if (!layout) return false;
    menu_preview_layout_free(layout);

    if (count <= 0) {
        layout->items = NULL;
        layout->count = 0;
        layout->cols = 0;
        return true;
    }

    layout->items = (MenuPreviewItem *)calloc((size_t)count, sizeof(MenuPreviewItem));
    if (!layout->items) {
        layout->count = 0;
        layout->cols = 0;
        return false;
    }

    layout->count = count;
    layout->cols = 0;
    return true;
}

void menu_preview_layout_free(MenuPreviewLayout *layout)
{
    if (!layout) return;
    free(layout->items);
    layout->items = NULL;
    layout->count = 0;
    layout->cols = 0;
}

bool menu_preview_hotkey_to_index(const MenuPreviewLayout *layout, int hotkey, int *out_index)
{
    if (out_index) *out_index = -1;
    if (!layout || !layout->items || layout->count <= 0) return false;

    int hk = tolower((unsigned char)hotkey);
    for (int i = 0; i < layout->count; i++) {
        if (layout->items[i].hotkey == hk) {
            if (out_index) *out_index = i;
            return true;
        }
    }
    return false;
}

static void add_item(MenuPreviewLayout *layout, int idx, int x, int y, int w, int hotkey, const char *desc)
{
    if (!layout || !layout->items) return;
    if (idx < 0 || idx >= layout->count) return;
    layout->items[idx].x = x;
    layout->items[idx].y = y;
    layout->items[idx].w = w;
    layout->items[idx].hotkey = tolower((unsigned char)hotkey);
    layout->items[idx].desc = desc;
}

static void render_option_cell(const MenuDefinition *menu,
                               MenuPreviewVScreen *vs,
                               int px,
                               int py,
                               int cell_w,
                               int margin,
                               const char *desc)
{
    if (!vs || !desc || !desc[0]) return;

    char hk = desc[0];
    const char *txt = desc + 1;

    int core_w = cell_w - (margin * 2);
    if (core_w < 0) core_w = 0;

    /* Runtime: field_w = opt_width + nontty - 3
     * ANSI/AVATAR (nontty=1): field_w = opt_width - 2 (hotkey + text, no ")")
     * TTY (nontty=0): field_w = opt_width - 3 (hotkey + ")" + text)
     * Preview uses ANSI style */
    int field_w = core_w - 2;
    if (field_w < 0) field_w = 0;

    int txt_len = 0;
    while (txt_len < field_w && txt[txt_len]) txt_len++;

    int pad_l = 0;
    if (menu && menu->cm_enabled) {
        if (menu->cm_option_justify == 1) pad_l = (field_w - txt_len) / 2;
        else if (menu->cm_option_justify == 2) pad_l = (field_w - txt_len);
    }
    if (pad_l < 0) pad_l = 0;
    int pad_r = field_w - pad_l - txt_len;
    if (pad_r < 0) pad_r = 0;

    int x0 = px - 1;
    int y0 = py - 1;

    /* Get colors from cm_lb_* fields or use runtime defaults
     * Runtime defaults from colors.lh:
     *   COL_MNU_OPTION = Gray (0x07) - normal option text
     *   COL_MNU_HILITE = Yellow (0x0e) - hotkey highlight
     */
    uint8_t normal_attr = 0x07;  /* Gray on Black - menu_opt_col default */
    uint8_t high_attr = 0x0e;    /* Yellow on Black - menu_high_col default */
    
    if (menu && menu->cm_enabled && menu->cm_lightbar) {
        /* Use custom lightbar colors if set, otherwise keep defaults */
        if (menu->cm_lb_normal_fg && menu->cm_lb_normal_fg[0]) {
            normal_attr = make_dos_attr(menu->cm_lb_normal_fg, menu->cm_lb_normal_bg);
        }
        if (menu->cm_lb_high_fg && menu->cm_lb_high_fg[0]) {
            high_attr = make_dos_attr(menu->cm_lb_high_fg, menu->cm_lb_high_bg);
        }
    }

    /* left margin */
    for (int i = 0; i < margin; i++) {
        vs_put_attr(vs, x0 + i, y0, ' ', normal_attr);
    }

    int cx = x0 + margin;
    
    /* left padding */
    for (int p = 0; p < pad_l; p++) {
        vs_put_attr(vs, cx + p, y0, ' ', normal_attr);
    }
    
    /* hotkey (highlight color) + text (normal color) */
    vs_put_attr(vs, cx + pad_l, y0, hk, high_attr);
    vs_putn_attr(vs, cx + pad_l + 1, y0, txt, txt_len, normal_attr);
    
    /* right padding */
    for (int p = 0; p < pad_r; p++) {
        vs_put_attr(vs, cx + pad_l + 1 + txt_len + p, y0, ' ', normal_attr);
    }

    /* right margin */
    for (int i = 0; i < margin; i++) {
        vs_put_attr(vs, x0 + (cell_w - margin) + i, y0, ' ', normal_attr);
    }
}

void menu_preview_render(const MenuDefinition *menu, MenuPreviewVScreen *vs, MenuPreviewLayout *layout, int selected_index)
{
    if (!menu || !vs) return;

    vs_clear(vs, ' ');

    /* Render header_file if present */
    if (menu->header_file && menu->header_file[0]) {
        char header_path[512];
        snprintf(header_path, sizeof(header_path), "%s.ans", menu->header_file);
        ansi_load_file(vs, header_path);
    }

    /* Render menu_file if present */
    if (menu->menu_file && menu->menu_file[0]) {
        char menu_path[512];
        snprintf(menu_path, sizeof(menu_path), "%s.ans", menu->menu_file);
        ansi_load_file(vs, menu_path);
    }

    (void)selected_index;
    if (layout) {
        menu_preview_layout_free(layout);
    }

    /* compute bounds (1-based inclusive, matching Goto semantics) */
    int x1 = 1;
    int y1 = 1;
    int x2 = MENU_PREVIEW_COLS;
    int y2 = MENU_PREVIEW_ROWS;

    if (menu->cm_enabled && menu->cm_top_row > 0 && menu->cm_top_col > 0 &&
        menu->cm_bottom_row > 0 && menu->cm_bottom_col > 0) {
        x1 = menu->cm_top_col;
        y1 = menu->cm_top_row;
        x2 = menu->cm_bottom_col;
        y2 = menu->cm_bottom_row;
        if (x1 < 1) x1 = 1;
        if (y1 < 1) y1 = 1;
        if (x2 > MENU_PREVIEW_COLS) x2 = MENU_PREVIEW_COLS;
        if (y2 > MENU_PREVIEW_ROWS) y2 = MENU_PREVIEW_ROWS;
        if (x2 < x1) x2 = x1;
        if (y2 < y1) y2 = y1;
    }

    /* title */
    int title_x = x1;
    int title_y = y1;
    if (menu->cm_enabled && menu->cm_title_row > 0 && menu->cm_title_col > 0) {
        title_y = menu->cm_title_row;
        title_x = menu->cm_title_col;
    }
    if (title_x < 1) title_x = 1;
    if (title_y < 1) title_y = 1;
    if (title_x > MENU_PREVIEW_COLS) title_x = MENU_PREVIEW_COLS;
    if (title_y > MENU_PREVIEW_ROWS) title_y = MENU_PREVIEW_ROWS;

    if (!menu->cm_enabled || menu->cm_show_title) {
        const char *t = (menu->title && menu->title[0]) ? menu->title : (menu->name ? menu->name : "");
        if (t && t[0]) {
            /* Render title through MCI interpreter for pipe color codes + %t */
            MciVScreen mvs;
            mvs.ch   = &vs->ch[0][0];
            mvs.attr = &vs->attr[0][0];
            mvs.cols = MENU_PREVIEW_COLS;
            mvs.rows = MENU_PREVIEW_ROWS;

            MciMockData mock;
            mci_mock_load(&mock);

            MciState mst;
            mci_state_init(&mst);
            mst.cx = title_x - 1;
            mst.cy = title_y - 1;
            mst.ca = 0x0e; /* Default title color: Yellow (menu_name_col) */

            mci_preview_expand(&mvs, &mst, &mock, t);
        }
    }

    int opt_width = (menu->opt_width > 0) ? menu->opt_width : 20;
    if (opt_width < 4) opt_width = 4;
    if (opt_width > MENU_PREVIEW_COLS) opt_width = MENU_PREVIEW_COLS;

    bool use_lightbar = (menu->cm_enabled && menu->cm_lightbar);
    int margin = use_lightbar ? menu->cm_lightbar_margin : 0;
    if (margin < 0) margin = 0;

    /* Lightbar cell uses option width + margins; classic grid uses opt_width */
    int cell_w = use_lightbar ? (opt_width + (margin * 2)) : opt_width;
    if (cell_w < 1) cell_w = 1;
    if (cell_w > MENU_PREVIEW_COLS) cell_w = MENU_PREVIEW_COLS;

    int opts_count = 0;
    for (int i = 0; i < menu->option_count; i++) {
        MenuOption *opt = menu->options[i];
        if (!opt) continue;
        if (opt->flags & OFLAG_NODSP) continue;
        if (!opt->description || !opt->description[0]) continue;
        opts_count++;
    }

    if (layout) {
        if (!layout_alloc(layout, opts_count)) {
            /* keep rendering even without layout data */
            menu_preview_layout_free(layout);
        }
    }

    int bounds_w = x2 - x1 + 1;
    int opt_start_y = y1;
    if ((!menu->cm_enabled || menu->cm_show_title) && title_y == y1) {
        opt_start_y = y1 + 1;
    }
    if (opt_start_y < 1) opt_start_y = 1;
    if (opt_start_y > y2) opt_start_y = y2;

    int bounds_h = y2 - opt_start_y + 1;
    if (bounds_w < 1) bounds_w = 1;
    if (bounds_h < 1) bounds_h = 1;

    int opts_per_line = bounds_w / cell_w;
    if (opts_per_line < 1) opts_per_line = 1;
    if (layout) layout->cols = opts_per_line;

    int rows = (opts_count + opts_per_line - 1) / opts_per_line;
    if (rows < 0) rows = 0;

    int row_spacing = (menu->cm_enabled && menu->cm_option_spacing) ? 1 : 0;
    int row_step = 1 + row_spacing;
    int max_rows = (bounds_h + row_step - 1) / row_step;

    /* Boundary layout modes: 0=grid, 1=tight, 2=spread, 3=spread_width, 4=spread_height */
    int boundary_layout = menu->cm_enabled ? menu->cm_boundary_layout : 0;
    int spread_w = (boundary_layout == 2 || boundary_layout == 3) ? 1 : 0;
    int spread_h = (boundary_layout == 2 || boundary_layout == 4) ? 1 : 0;

    /* For tight/spread we need total_rows and last_row_cols */
    int total_rows = rows;
    int last_row_cols = (opts_count % opts_per_line);
    if (last_row_cols == 0 && opts_count > 0) last_row_cols = opts_per_line;

    /* Vertical spread pre-compute */
    int spread_gap_y = 0;
    int spread_off_y = 0;
    int vjust_off_y = 0;

    if (menu->cm_enabled && spread_h && total_rows > 0) {
        if (total_rows <= 1) {
            int span_y = bounds_h - 1;
            if (span_y < 0) span_y = 0;
            if (menu->cm_boundary_vjustify == 1) spread_off_y = span_y / 2;
            else if (menu->cm_boundary_vjustify == 2) spread_off_y = span_y;
            spread_gap_y = 0;
        } else {
            int base_row_gap = row_spacing;
            int content_h = total_rows + (total_rows - 1) * base_row_gap;
            int span_y = bounds_h - content_h;
            int gaps = total_rows - 1;
            if (span_y < 0) span_y = 0;
            if (row_spacing) spread_gap_y = span_y / gaps;
            else spread_gap_y = (span_y >= gaps) ? 1 : 0;
            int leftover_y = span_y - (spread_gap_y * gaps);
            if (leftover_y < 0) leftover_y = 0;
            int offset_y = 0;
            if (menu->cm_boundary_vjustify == 1) offset_y = leftover_y / 2;
            else if (menu->cm_boundary_vjustify == 2) offset_y = leftover_y;
            spread_off_y = offset_y;
        }
    } else if (menu->cm_enabled && !spread_h && total_rows > 0) {
        /* Non-spread vertical justification */
        int Rdisp = (total_rows > max_rows) ? max_rows : total_rows;
        int content_h = (Rdisp > 1) ? (Rdisp + (Rdisp - 1) * row_spacing) : ((Rdisp == 1) ? 1 : 0);
        int span_y = bounds_h - content_h;
        if (span_y < 0) span_y = 0;
        if (menu->cm_boundary_vjustify == 1) vjust_off_y = span_y / 2;
        else if (menu->cm_boundary_vjustify == 2) vjust_off_y = span_y;
    }

    int base_x = x1;
    int base_x_inited = 0;

    int out_i = 0;
    for (int i = 0; i < menu->option_count; i++) {
        MenuOption *opt = menu->options[i];
        if (!opt) continue;
        if (opt->flags & OFLAG_NODSP) continue;
        const char *desc = opt->description;
        if (!desc || !desc[0]) continue;

        int row = out_i / opts_per_line;
        int col = out_i % opts_per_line;
        int cols_in_row = opts_per_line;

        if (menu->cm_enabled && (boundary_layout == 1 || spread_w) && total_rows > 0) {
            if (row == total_rows - 1) cols_in_row = last_row_cols;
        }

        int px;
        if (menu->cm_enabled && spread_w) {
            /* Spread width: distribute horizontal gaps */
            int span = bounds_w - (cols_in_row * cell_w);
            if (span <= 0) {
                px = x1 + (col * cell_w);
            } else if (cols_in_row <= 1) {
                int offset = 0;
                if (menu->cm_boundary_justify == 1) offset = span / 2;
                else if (menu->cm_boundary_justify == 2) offset = span;
                px = x1 + offset;
            } else {
                int gaps = cols_in_row - 1;
                int gap = span / gaps;
                int leftover = span - (gap * gaps);
                int offset = 0;
                if (menu->cm_boundary_justify == 1) offset = leftover / 2;
                else if (menu->cm_boundary_justify == 2) offset = leftover;
                px = x1 + offset + (col * (cell_w + gap));
            }
        } else {
            /* Grid or tight: recompute base_x per row for tight */
            if (!base_x_inited || (menu->cm_enabled && boundary_layout == 1)) {
                int grid_w = (menu->cm_enabled && boundary_layout != 1) ? (opts_per_line * cell_w) : (cols_in_row * cell_w);
                if (grid_w >= bounds_w) base_x = x1;
                else if (menu->cm_enabled && menu->cm_boundary_justify == 1) base_x = x1 + (bounds_w - grid_w) / 2;
                else if (menu->cm_enabled && menu->cm_boundary_justify == 2) base_x = x2 - grid_w + 1;
                else base_x = x1;
                base_x_inited = 1;
            }
            px = base_x + (col * cell_w);
        }

        int py;
        if (row >= max_rows) continue;
        if (menu->cm_enabled && spread_h) {
            py = opt_start_y + spread_off_y + (row * (1 + row_spacing + spread_gap_y));
        } else {
            py = opt_start_y + vjust_off_y + row + (row * row_spacing);
        }

        if (layout && layout->items && out_i < layout->count) {
            add_item(layout, out_i, px, py, cell_w, desc[0], desc);
        }

        render_option_cell(menu, vs, px, py, cell_w, margin, desc);
        out_i++;
    }

    /* prompt - only shown in non-lightbar mode (runtime behavior) */
    if (!use_lightbar) {
        int prompt_x = x1;
        int prompt_y = y2;
        if (menu->cm_enabled && menu->cm_prompt_row > 0 && menu->cm_prompt_col > 0) {
            prompt_y = menu->cm_prompt_row;
            prompt_x = menu->cm_prompt_col;
        }
        if (prompt_x >= 1 && prompt_y >= 1 && prompt_x <= MENU_PREVIEW_COLS && prompt_y <= MENU_PREVIEW_ROWS) {
            /* Prompt uses White (0x0f) - CWHITE in runtime */
            vs_putn_attr(vs, prompt_x - 1, prompt_y - 1, "Select: ", 8, 0x0f);
        }
    }
}

static void draw_selected_item(int x, int y, int w)
{
    if (w < 1) return;
    if (x < 0 || y < 0 || x >= COLS || y >= LINES) return;

    if (x + w > COLS) w = COLS - x;
    if (w < 1) return;

    attron(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
    mvhline(y, x, ' ', w);
    attroff(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
}

int dos_color_to_ncurses(int dos_color)
{
    /* DOS colors: 0=Black, 1=Blue, 2=Green, 3=Cyan, 4=Red, 5=Magenta, 6=Brown, 7=Gray
     * 8-15 are bright versions */
    static const int map[] = {
        COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
        COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE
    };
    return map[dos_color & 0x07];
}

/*
 * Pair pool state shared by dos_pair_for_fg_bg() and menu_preview_pairs_reset().
 * See the Doxygen comment on menu_preview_pairs_reset() below for rationale.
 */
static int g_menu_preview_pair_for_combo[16][8];
static int g_menu_preview_pool_start;
static int g_menu_preview_pool_end;
static int g_menu_preview_next_pair;
static int g_menu_preview_pool_inited;

/**
 * @brief Reset the preview's dynamic ncurses color-pair cache.
 *
 * maxcfg uses ncurses color pairs in multiple places (screen theme, color picker,
 * color preview fields). Those areas call `init_pair()` dynamically and can
 * overwrite pair numbers that the menu preview previously initialized.
 *
 * If we then reuse a stale cached pair number without reinitializing it, the
 * preview can "bleed" colors into unrelated screen regions (e.g. cyan fills),
 * especially after opening the preview and then changing colors.
 *
 * To avoid collisions, the menu preview lazily allocates its own pairs from a
 * dedicated high-number pool and resets that mapping for each blit.
 */
void menu_preview_pairs_reset(void);

/**
 * @brief Get an ncurses pair number for a DOS fg/bg combination.
 *
 * This allocates from a dedicated pair-number pool (above the color picker's
 * dynamic range) to avoid collisions. Allocation is lazy: only combinations
 * encountered during a blit are assigned a pair.
 */
int dos_pair_for_fg_bg(int fg, int bg)
{
    /*
     * Pair pool state. This is reset each blit via menu_preview_pairs_reset().
     *
     * Note: We intentionally do not attempt to reserve a contiguous 128-pair
     * block because some terminals expose only ~256 pairs and the color picker
     * already consumes many of the lower indices.
     */
    int safe_fg = fg & 0x0f;
    int safe_bg = bg & 0x07;

    if (!has_colors()) {
        return CP_DROPDOWN;
    }

    if (!g_menu_preview_pool_inited) {
        /*
         * Color picker uses CP_PICKER_BASE=30 and allocates:
         *   30..45  (16 fg-on-black)
         *   46..173 (8*16 fg/bg grid)
         * Color form uses CP_PREVIEW_BASE=50 + idx.
         * Keep well above those to avoid runtime overwrites.
         */
        g_menu_preview_pool_start = 180;
        g_menu_preview_pool_end = COLOR_PAIRS - 1;
        if (g_menu_preview_pool_start < 1) g_menu_preview_pool_start = 1;
        if (g_menu_preview_pool_start > g_menu_preview_pool_end) {
            return CP_DROPDOWN;
        }
        g_menu_preview_next_pair = g_menu_preview_pool_start;
        memset(g_menu_preview_pair_for_combo, 0, sizeof(g_menu_preview_pair_for_combo));
        g_menu_preview_pool_inited = 1;
    }

    if (g_menu_preview_pair_for_combo[safe_fg][safe_bg] != 0) {
        return g_menu_preview_pair_for_combo[safe_fg][safe_bg];
    }

    if (g_menu_preview_next_pair > g_menu_preview_pool_end) {
        /* Out of pairs; fall back to a reasonable default. */
        return CP_DROPDOWN;
    }

    int pair = g_menu_preview_next_pair++;
    int nc_fg = dos_color_to_ncurses(safe_fg);
    int nc_bg = dos_color_to_ncurses(safe_bg);
    init_pair((short)pair, (short)nc_fg, (short)nc_bg);
    g_menu_preview_pair_for_combo[safe_fg][safe_bg] = pair;
    return pair;
}

void menu_preview_pairs_reset(void)
{
    /* See dos_pair_for_fg_bg() for why this exists. */
    if (!has_colors()) {
        return;
    }

    if (!g_menu_preview_pool_inited) {
        g_menu_preview_pool_start = 180;
        g_menu_preview_pool_end = COLOR_PAIRS - 1;
        if (g_menu_preview_pool_start < 1) g_menu_preview_pool_start = 1;
        if (g_menu_preview_pool_start > g_menu_preview_pool_end) {
            return;
        }
        g_menu_preview_pool_inited = 1;
    }

    memset(g_menu_preview_pair_for_combo, 0, sizeof(g_menu_preview_pair_for_combo));
    g_menu_preview_next_pair = g_menu_preview_pool_start;
}

void menu_preview_blit(const MenuDefinition *menu,
                       const MenuPreviewVScreen *vs,
                       const MenuPreviewLayout *layout,
                       int selected_index,
                       int x,
                       int y)
{
    if (!vs) return;

    /* Reset preview pair mapping so live color changes can't reuse stale pairs. */
    menu_preview_pairs_reset();

    /* Get selected colors from TOML */
    uint8_t sel_attr = 0x1f;      /* White on Blue - default selected */
    uint8_t sel_high_attr = 0x1e; /* Yellow on Blue - default high selected */
    
    if (menu && menu->cm_enabled && menu->cm_lightbar) {
        if (menu->cm_lb_selected_fg && menu->cm_lb_selected_fg[0]) {
            sel_attr = make_dos_attr(menu->cm_lb_selected_fg, menu->cm_lb_selected_bg);
        }
        if (menu->cm_lb_high_sel_fg && menu->cm_lb_high_sel_fg[0]) {
            sel_high_attr = make_dos_attr(menu->cm_lb_high_sel_fg, menu->cm_lb_high_sel_bg);
        }
    }

    /* Determine selected cell bounds and hotkey position */
    int sel_row = -1, sel_col_start = -1, sel_col_end = -1;
    int hotkey_col = -1;

    /* Determine what attribute we used for the hotkey in the unselected state */
    uint8_t base_high_attr = 0x0e; /* runtime default menu_high_col */
    if (menu && menu->cm_enabled && menu->cm_lightbar) {
        if (menu->cm_lb_high_fg && menu->cm_lb_high_fg[0]) {
            base_high_attr = make_dos_attr(menu->cm_lb_high_fg, menu->cm_lb_high_bg);
        }
    }
    
    if (layout && layout->items && selected_index >= 0 && selected_index < layout->count) {
        const MenuPreviewItem *it = &layout->items[selected_index];
        sel_row = it->y - 1;
        sel_col_start = it->x - 1;
        sel_col_end = sel_col_start + it->w - 1;
        
        /*
         * Calculate hotkey position:
         * runtime highlights exactly one character (the hotkey) using the menu
         * highlight attribute.
         */
        for (int c = sel_col_start; c <= sel_col_end; c++) {
            if (sel_row >= 0 && sel_row < MENU_PREVIEW_ROWS && c >= 0 && c < MENU_PREVIEW_COLS) {
                uint8_t orig_attr = vs->attr[sel_row][c];
                if (orig_attr == base_high_attr) {
                    hotkey_col = c;
                    break;
                }
            }
        }
    }

    /* Render each cell with its DOS attribute mapped to ncurses colors */
    for (int row = 0; row < MENU_PREVIEW_ROWS; row++) {
        for (int col = 0; col < MENU_PREVIEW_COLS; col++) {
            char ch = vs->ch[row][col];
            uint8_t attr = vs->attr[row][col];
            
            /* Override with selected colors if this cell is in the selected item */
            if (row == sel_row && col >= sel_col_start && col <= sel_col_end) {
                /* Hotkey stays with its highlight color, everything else gets selected color */
                if (col == hotkey_col) {
                    /* Keep hotkey as-is (it's already yellow from render), just change bg to blue */
                    attr = sel_high_attr;
                } else {
                    /* Non-hotkey characters get selected color (white on blue) */
                    attr = sel_attr;
                }
            }
            
            int fg = attr & 0x0f;
            int bg = (attr >> 4) & 0x07;

            int attrs = 0;
            if (fg == 8) {
                attrs |= A_DIM;
                fg = 7;
            } else if (fg >= 9) {
                attrs |= A_BOLD;
                fg -= 8;
            }

            int pair = dos_pair_for_fg_bg(fg, bg);

            /*
             * menu_preview runs in a UTF-8 locale (setlocale(LC_ALL, "")).
             * The ANSI art is CP437 bytes; convert common graphics glyphs to
             * Unicode so modern terminals render the shapes correctly.
             */
#if defined(HAVE_WIDE_CURSES)
            {
                wchar_t wch = cp437_to_unicode((unsigned char)ch);
                wchar_t wstr[2] = { wch, 0 };
                cchar_t cc;
                setcchar(&cc, wstr, attrs, (short)pair, NULL);
                mvadd_wch(y + row, x + col, &cc);
            }
#else
            attron(COLOR_PAIR(pair) | attrs);
            mvaddch(y + row, x + col, (unsigned char)ch);
            attroff(COLOR_PAIR(pair) | attrs);
#endif
        }
    }
}
