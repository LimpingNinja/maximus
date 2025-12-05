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
#include <sys/stat.h>
#include "maxcfg.h"
#include "ui.h"
#include "prm_data.h"

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
    .current_menu = 0,
    .menu_open = false
};

static void print_usage(const char *progname)
{
    fprintf(stderr, "Usage: %s [options] [config_path]\n", progname);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  -h, --help     Show this help message\n");
    fprintf(stderr, "  -v, --version  Show version information\n");
    fprintf(stderr, "\nIf config_path is not specified, defaults to %s\n",
            DEFAULT_CONFIG_PATH);
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
        else if (argv[i][0] != '-') {
            /* Assume it's the config path */
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

/* Check if config file exists (adds .prm extension) */
static int check_config_file(void)
{
    char prm_path[MAX_PATH_LEN + 8];
    struct stat st;
    
    snprintf(prm_path, sizeof(prm_path), "%s.prm", g_state.config_path);
    
    if (stat(prm_path, &st) != 0) {
        fprintf(stderr, "Error: Configuration file not found: %s\n", prm_path);
        return 0;
    }
    
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Error: Not a regular file: %s\n", prm_path);
        return 0;
    }
    
    return 1;
}

/* Load the PRM configuration file */
static int load_config(void)
{
    char prm_path[MAX_PATH_LEN + 8];
    
    snprintf(prm_path, sizeof(prm_path), "%s.prm", g_state.config_path);
    
    if (!prm_load(prm_path)) {
        fprintf(stderr, "Error: Failed to load configuration: %s\n", prm_path);
        return 0;
    }
    
    return 1;
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
                if (dialog_confirm("Exit", "Are you sure you want to exit?")) {
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
                    if (dialog_confirm("Exit", "Are you sure you want to exit?")) {
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

    /* Check if config file exists */
    if (!check_config_file()) {
        exit(1);
    }

    /* Load the PRM configuration */
    if (!load_config()) {
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
    prm_close();

    return 0;
}
