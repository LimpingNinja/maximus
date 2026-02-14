/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * main.c - Entry point for Maximus Configuration Editor
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <signal.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include "maxcfg.h"
#include "ui.h"
#include "nextgen_export.h"
#include "lang_convert.h"

/* Stub for Maximus runtime symbol */
unsigned short dosProc_exitCode = 0;

/* Resize flag set by SIGWINCH */
static volatile int need_resize = 0;

/* SIGWINCH handler */
static void sigwinch_handler(int sig)
{
    (void)sig;
    need_resize = 1;
}

static bool g_cli_export_nextgen = false;
static char g_cli_export_dir[MAX_PATH_LEN] = { 0 };
static bool g_cli_convert_lang = false;
static bool g_cli_convert_lang_all = false;
static char g_cli_convert_lang_path[MAX_PATH_LEN] = { 0 };

/* Request terminal to resize (xterm-compatible) */
static void request_terminal_size(int cols, int rows)
{
    /* Use xterm escape sequence to resize */
    printf("\033[8;%d;%dt", rows, cols);
    fflush(stdout);
}

/* Setup signal handlers */
static void setup_signals(void)
{
    struct sigaction sa;
    
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    
    /* SIGWINCH - terminal resize */
    sa.sa_handler = sigwinch_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, NULL);
}

/* Handle terminal resize */
static void handle_resize(void)
{
    endwin();
    refresh();
    need_resize = 0;
}

/* Global application state */
AppState g_state = {
    .config_path = DEFAULT_CONFIG_PATH,
    .dirty = false,
    .ctl_modified = false,
    .current_menu = 0,
    .menu_open = false
};

MaxCfg *g_maxcfg = NULL;
MaxCfgToml *g_maxcfg_toml = NULL;

static void maxcfg_toml_cleanup(void)
{
    if (g_maxcfg_toml) {
        maxcfg_toml_free(g_maxcfg_toml);
        g_maxcfg_toml = NULL;
    }
    if (g_maxcfg) {
        maxcfg_close(g_maxcfg);
        g_maxcfg = NULL;
    }
}

static int load_toml_config(const char *sys_path)
{
    if (sys_path == NULL || sys_path[0] == '\0') {
        fprintf(stderr, "Error: sys_path is not available\n");
        return 0;
    }

    if (maxcfg_open(&g_maxcfg, sys_path) != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to init libmaxcfg for base_dir: %s\n", sys_path);
        return 0;
    }

    if (maxcfg_toml_init(&g_maxcfg_toml) != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to init TOML config\n");
        maxcfg_toml_cleanup();
        return 0;
    }

    char maximus_path[MAX_PATH_LEN];
    char session_path[MAX_PATH_LEN];
    char display_files_path[MAX_PATH_LEN];
    char matrix_path[MAX_PATH_LEN];
    char reader_path[MAX_PATH_LEN];
    char equipment_path[MAX_PATH_LEN];
    char language_path[MAX_PATH_LEN];
    char protocol_path[MAX_PATH_LEN];

    if (maxcfg_join_path(g_maxcfg, "config/maximus.toml", maximus_path, sizeof(maximus_path)) != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to resolve config/maximus.toml under: %s\n", sys_path);
        maxcfg_toml_cleanup();
        return 0;
    }
    if (maxcfg_join_path(g_maxcfg, "config/general/session.toml", session_path, sizeof(session_path)) != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to resolve config/general/session.toml under: %s\n", sys_path);
        maxcfg_toml_cleanup();
        return 0;
    }
    if (maxcfg_join_path(g_maxcfg, "config/general/display_files.toml", display_files_path, sizeof(display_files_path)) != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to resolve config/general/display_files.toml under: %s\n", sys_path);
        maxcfg_toml_cleanup();
        return 0;
    }

    if (maxcfg_join_path(g_maxcfg, "config/matrix.toml", matrix_path, sizeof(matrix_path)) != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to resolve config/matrix.toml under: %s\n", sys_path);
        maxcfg_toml_cleanup();
        return 0;
    }
    if (maxcfg_join_path(g_maxcfg, "config/general/reader.toml", reader_path, sizeof(reader_path)) != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to resolve config/general/reader.toml under: %s\n", sys_path);
        maxcfg_toml_cleanup();
        return 0;
    }
    if (maxcfg_join_path(g_maxcfg, "config/general/equipment.toml", equipment_path, sizeof(equipment_path)) != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to resolve config/general/equipment.toml under: %s\n", sys_path);
        maxcfg_toml_cleanup();
        return 0;
    }
    if (maxcfg_join_path(g_maxcfg, "config/general/language.toml", language_path, sizeof(language_path)) != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to resolve config/general/language.toml under: %s\n", sys_path);
        maxcfg_toml_cleanup();
        return 0;
    }
    if (maxcfg_join_path(g_maxcfg, "config/general/protocol.toml", protocol_path, sizeof(protocol_path)) != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to resolve config/general/protocol.toml under: %s\n", sys_path);
        maxcfg_toml_cleanup();
        return 0;
    }

    MaxCfgStatus st;
    st = maxcfg_toml_load_file(g_maxcfg_toml, maximus_path, "maximus");
    if (st != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to load TOML: %s (%s)\n", maximus_path, maxcfg_status_string(st));
        maxcfg_toml_cleanup();
        return 0;
    }
    st = maxcfg_toml_load_file(g_maxcfg_toml, session_path, "general.session");
    if (st != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to load TOML: %s (%s)\n", session_path, maxcfg_status_string(st));
        maxcfg_toml_cleanup();
        return 0;
    }

    st = maxcfg_toml_load_file(g_maxcfg_toml, display_files_path, "general.display_files");
    if (st != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to load TOML: %s (%s)\n", display_files_path, maxcfg_status_string(st));
        maxcfg_toml_cleanup();
        return 0;
    }

    st = maxcfg_toml_load_file(g_maxcfg_toml, matrix_path, "matrix");
    if (st != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to load TOML: %s (%s)\n", matrix_path, maxcfg_status_string(st));
        maxcfg_toml_cleanup();
        return 0;
    }
    st = maxcfg_toml_load_file(g_maxcfg_toml, reader_path, "general.reader");
    if (st != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to load TOML: %s (%s)\n", reader_path, maxcfg_status_string(st));
        maxcfg_toml_cleanup();
        return 0;
    }
    st = maxcfg_toml_load_file(g_maxcfg_toml, equipment_path, "general.equipment");
    if (st != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to load TOML: %s (%s)\n", equipment_path, maxcfg_status_string(st));
        maxcfg_toml_cleanup();
        return 0;
    }
    st = maxcfg_toml_load_file(g_maxcfg_toml, language_path, "general.language");
    if (st != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to load TOML: %s (%s)\n", language_path, maxcfg_status_string(st));
        maxcfg_toml_cleanup();
        return 0;
    }
    st = maxcfg_toml_load_file(g_maxcfg_toml, protocol_path, "general.protocol");
    if (st != MAXCFG_OK) {
        fprintf(stderr, "Error: failed to load TOML: %s (%s)\n", protocol_path, maxcfg_status_string(st));
        maxcfg_toml_cleanup();
        return 0;
    }

    return 1;
}

static void print_usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [sys_path] [options]\n", prog);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  -h, --help     Show this help message\n");
    fprintf(stderr, "  -v, --version  Show version information\n");
    fprintf(stderr, "  --export-nextgen <path/to/max.ctl>  Export legacy CTL to next-gen TOML and exit\n");
    fprintf(stderr, "  --export-dir <path>  Override next-gen export directory (implies --export-nextgen)\n");
    fprintf(stderr, "  --convert-lang <file.mad>  Convert a single .MAD language file to TOML and exit\n");
    fprintf(stderr, "  --convert-lang-all         Convert all .MAD files in <sys_path>/etc/lang/ and exit\n");
    fprintf(stderr, "\nIf sys_path is not specified, it will be derived from argv[0] or the first positional argument.\n");
}

static void print_version(void)
{
    printf("MAXCFG - Maximus Configuration Editor\n");
    printf("Version %s\n", MAXCFG_VERSION);
    printf("Copyright (C) 2025 Kevin Morgan (Limping Ninja)\n");
    printf("License: GPL-2.0-or-later\n");
}

static void parse_args(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            exit(0);
        }
        else if (strcmp(argv[i], "--export-nextgen") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for %s\n", argv[i]);
                exit(1);
            }
            g_cli_export_nextgen = true;
            strncpy(g_state.config_path, argv[i + 1], MAX_PATH_LEN - 1);
            g_state.config_path[MAX_PATH_LEN - 1] = '\0';
            i++;
        }
        else if (strcmp(argv[i], "--export-dir") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for %s\n", argv[i]);
                exit(1);
            }
            g_cli_export_nextgen = true;
            strncpy(g_cli_export_dir, argv[i + 1], MAX_PATH_LEN - 1);
            g_cli_export_dir[MAX_PATH_LEN - 1] = '\0';
            i++;
        }
        else if (strcmp(argv[i], "--convert-lang") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for %s\n", argv[i]);
                exit(1);
            }
            g_cli_convert_lang = true;
            strncpy(g_cli_convert_lang_path, argv[i + 1], MAX_PATH_LEN - 1);
            g_cli_convert_lang_path[MAX_PATH_LEN - 1] = '\0';
            i++;
        }
        else if (strcmp(argv[i], "--convert-lang-all") == 0) {
            g_cli_convert_lang_all = true;
        }
        else if (argv[i][0] != '-') {
            /* Assume it's the sys_path */
            strncpy(g_state.config_path, argv[i], MAX_PATH_LEN - 1);
            g_state.config_path[MAX_PATH_LEN - 1] = '\0';
        }
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            exit(1);
        }
    }
}

static bool resolve_sys_path_from_argv0(const char *argv0, char *out, size_t out_sz)
{
    char exe_path[MAX_PATH_LEN];
    if (argv0 == NULL || argv0[0] == '\0') {
        return false;
    }

    if (realpath(argv0, exe_path) == NULL) {
        return false;
    }

    char tmp1[MAX_PATH_LEN];
    strncpy(tmp1, exe_path, sizeof(tmp1) - 1);
    tmp1[sizeof(tmp1) - 1] = '\0';

    const char *bin_dir = dirname(tmp1);
    if (bin_dir == NULL || bin_dir[0] == '\0') {
        return false;
    }

    char tmp2[MAX_PATH_LEN];
    strncpy(tmp2, bin_dir, sizeof(tmp2) - 1);
    tmp2[sizeof(tmp2) - 1] = '\0';

    const char *prefix_dir = dirname(tmp2);
    if (prefix_dir == NULL || prefix_dir[0] == '\0') {
        return false;
    }

    if (snprintf(out, out_sz, "%s", prefix_dir) >= (int)out_sz) {
        return false;
    }

    return true;
}

/* Check if we should offer to save before exiting */
static bool handle_exit_prompt(void)
{
    if (!dialog_confirm("Exit", "Are you sure you want to exit?")) {
        return false;
    }

    if (!g_state.dirty) {
        return true;
    }

    DialogResult r = dialog_save_prompt();
    if (r == DIALOG_RETURN || r == DIALOG_CANCEL) {
        return false;
    }
    if (r == DIALOG_ABORT) {
        if (g_maxcfg_toml != NULL) {
            maxcfg_toml_override_clear(g_maxcfg_toml);
        }
        return true;
    }

    if (g_maxcfg_toml == NULL) {
        dialog_message("Save Failed", "TOML configuration is not loaded.");
        return dialog_confirm("Exit Anyway", "Exit without saving?");
    }

    MaxCfgStatus st = maxcfg_toml_persist_overrides_and_save(g_maxcfg_toml);
    if (st != MAXCFG_OK) {
        dialog_message("Save Failed", maxcfg_status_string(st));
        return dialog_confirm("Exit Anyway", "Saving failed. Exit without saving?");
    }

    g_state.dirty = false;
    return true;
}

static void main_loop(void)
{
    int ch;
    bool running = true;

    while (running) {
        /* Handle terminal resize if needed */
        if (need_resize) {
            handle_resize();
        }

        /* Draw everything */
        draw_title_bar();
        draw_menubar();
        draw_work_area();
        draw_dropdown();
        draw_status_bar("F1=Help  ESC=Menu  Ctrl+Q=Quit");
        
        doupdate();

        /* Get input */
        ch = getch();

        /* Global keys */
        switch (ch) {
            case 17:  /* Ctrl+Q */
                if (handle_exit_prompt()) {
                    running = false;
                }
                break;

            case KEY_F(1):
                dialog_message("Help", "MAXCFG - Maximus Configuration Editor\n\n"
                              "Use arrow keys to navigate menus.\n"
                              "Press Enter to select.\n"
                              "Press ESC to go back.\n"
                              "Press Ctrl+Q to quit.");
                break;

            case 27:  /* ESC */
                if (dropdown_is_open()) {
                    /* Let dropdown handle it - will close appropriately */
                    dropdown_handle_key(ch);
                } else {
                    /* At top level with no menu open - prompt to exit */
                    if (handle_exit_prompt()) {
                        running = false;
                    }
                }
                break;

            default:
                /* Try dropdown first if open */
                if (dropdown_is_open()) {
                    if (!dropdown_handle_key(ch)) {
                        /* If dropdown didn't handle it, try menubar */
                        menubar_handle_key(ch);
                    }
                } else {
                    /* Try menubar */
                    menubar_handle_key(ch);
                }
                break;
        }
    }
}

int main(int argc, char *argv[])
{
    /* Set locale for proper character handling */
    setlocale(LC_ALL, "");

    /* Parse command line arguments */
    parse_args(argc, argv);

    /* Handle --convert-lang (single file, no TUI needed) */
    if (g_cli_convert_lang) {
        char err[512] = {0};
        if (lang_convert_mad_to_toml(g_cli_convert_lang_path, NULL, err, sizeof(err))) {
            printf("Converted: %s\n", g_cli_convert_lang_path);
            return 0;
        } else {
            fprintf(stderr, "Error: %s\n", err[0] ? err : "conversion failed");
            return 1;
        }
    }

    /* Handle --convert-lang-all (batch, needs sys_path for lang dir) */
    if (g_cli_convert_lang_all) {
        char sys_path_buf2[MAX_PATH_LEN];
        const char *sp = NULL;
        if (strcmp(g_state.config_path, DEFAULT_CONFIG_PATH) != 0) {
            sp = g_state.config_path;
        } else if (resolve_sys_path_from_argv0(argv[0], sys_path_buf2, sizeof(sys_path_buf2))) {
            sp = sys_path_buf2;
        } else {
            fprintf(stderr, "Error: unable to determine sys_path. Pass it as the first argument.\n");
            return 1;
        }

        char lang_dir[MAX_PATH_LEN];
        snprintf(lang_dir, sizeof(lang_dir), "%s/etc/lang", sp);

        char err[512] = {0};
        int count = lang_convert_all_mad(lang_dir, NULL, err, sizeof(err));
        if (count < 0) {
            fprintf(stderr, "Error: %s\n", err[0] ? err : "conversion failed");
            return 1;
        }
        printf("Converted %d .MAD file(s) in %s\n", count, lang_dir);
        if (err[0]) {
            fprintf(stderr, "Warning: %s\n", err);
        }
        return 0;
    }

    if (g_cli_export_nextgen) {
        const char *maxctl_path = g_state.config_path;
        if (maxctl_path == NULL || maxctl_path[0] == '\0') {
            fprintf(stderr, "Error: missing max.ctl path (use --export-nextgen <path/to/max.ctl>)\n");
            return 1;
        }

        char out_dir[MAX_PATH_LEN];
        if (g_cli_export_dir[0]) {
            strncpy(out_dir, g_cli_export_dir, sizeof(out_dir) - 1);
            out_dir[sizeof(out_dir) - 1] = '\0';
        } else {
            /* Default: sibling config dir of <sys_path>/etc/max.ctl */
            char tmp[MAX_PATH_LEN];
            strncpy(tmp, maxctl_path, sizeof(tmp) - 1);
            tmp[sizeof(tmp) - 1] = '\0';

            char *p = strrchr(tmp, '/');
            if (p == NULL) {
                fprintf(stderr, "Error: invalid max.ctl path\n");
                return 1;
            }
            *p = '\0';
            p = strrchr(tmp, '/');
            if (p == NULL) {
                fprintf(stderr, "Error: invalid max.ctl path\n");
                return 1;
            }
            *p = '\0';

            if (snprintf(out_dir, sizeof(out_dir), "%s/config", tmp) >= (int)sizeof(out_dir)) {
                fprintf(stderr, "Error: export dir too long\n");
                return 1;
            }
        }

        char err[512];
        err[0] = '\0';
        if (!nextgen_export_config_from_maxctl(maxctl_path, out_dir, NG_EXPORT_ALL, err, sizeof(err))) {
            fprintf(stderr, "Error: export failed: %s\n", err[0] ? err : "unknown error");
            return 1;
        }
        return 0;
    }

    char sys_path_buf[MAX_PATH_LEN];
    const char *sys_path = NULL;

    if (strcmp(g_state.config_path, DEFAULT_CONFIG_PATH) != 0) {
        sys_path = g_state.config_path;
    } else if (resolve_sys_path_from_argv0(argv[0], sys_path_buf, sizeof(sys_path_buf))) {
        sys_path = sys_path_buf;
    } else {
        fprintf(stderr, "Error: unable to determine sys_path. Pass it as the first argument.\n");
        exit(1);
    }

    if (!load_toml_config(sys_path)) {
        exit(1);
    }

    /* Setup signal handlers */
    setup_signals();

    /* Request 80x25 terminal size */
    request_terminal_size(80, 25);

    /* Initialize ncurses */
    screen_init();

    /* Initialize color picker */
    colorpicker_init();

    /* Initialize menu system */
    menubar_init();

    /* Main event loop */
    main_loop();

    /* Cleanup */
    screen_cleanup();
    maxcfg_toml_cleanup();

    return 0;
}
