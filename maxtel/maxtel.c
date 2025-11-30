/*
 * maxtel - Multi-node Telnet Supervisor for Maximus BBS
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 * https://github.com/LimpingNinja
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * Features:
 *   - Spawns and manages multiple Maximus BBS nodes
 *   - Built-in TCP listener for telnet connections
 *   - ncurses status display showing all node activity
 *   - Kick, snoop, and message functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef DARWIN
#include <util.h>
#else
#include <pty.h>
#include <utmp.h>
#endif

#include "../comdll/telnet.h"

/* Maximus headers for struct definitions
 * Must come before ncurses.h because ncurses declares raw() as a function
 * which conflicts with Maximus 'raw' enum in option.h */
#include "prog.h"
#include "max.h"
#include "prmapi.h"

/* Rename ncurses raw() to avoid conflict with Maximus raw enum already defined */
#define raw ncurses_raw
#include <ncurses.h>
#undef raw

/* Configuration */
#define MAX_NODES       16
#define DEFAULT_PORT    2323
#define DEFAULT_NODES   4
#define SOCKET_PREFIX   "maxipc"
#define LOCK_SUFFIX     ".lck"
#define STATUS_PREFIX   "bbstat"
#define REFRESH_MS      100
#define MAX_VISIBLE_NODES 6   /* Max nodes visible before scrolling */
#define LASTUS_PREFIX   "lastus"
#define MAX_CALLER_HISTORY 10

/* Layout modes for different terminal sizes */
typedef enum {
    LAYOUT_COMPACT = 0,   /* 80x25 - tabbed bottom panel */
    LAYOUT_MEDIUM,        /* ~100x40 - all panels, condensed */
    LAYOUT_FULL           /* 132x60+ - full detail */
} layout_mode_t;

/* Layout configuration */
typedef struct {
    int min_cols, min_rows;       /* Minimum size for this layout */
    int expand_system;            /* 1 = show System + Stats side-by-side, 0 = tabbed */
    int nodes_full_cols;          /* Show Activity column in nodes */
    int callers_full_cols;        /* Show City column in callers */
} layout_config_t;

static const layout_config_t layouts[] = {
    [LAYOUT_COMPACT] = { 80, 20, 0, 0, 0 },   /* Compact: tabbed system, minimal columns */
    [LAYOUT_MEDIUM]  = { 100, 20, 1, 0, 1 },  /* Medium: expanded system (width-based), callers city */
    [LAYOUT_FULL]    = { 132, 20, 1, 1, 1 },  /* Full: all columns */
};
#define NUM_LAYOUTS 3

#define CALLERS_MAX_PRELOAD 20  /* Max callers to keep in memory */

/* Tabs for compact mode system panel (System Info / System Stats) */
typedef enum {
    TAB_SYSTEM_INFO = 0,
    TAB_SYSTEM_STATS,
    TAB_COUNT
} system_tab_t;

static const char *tab_names[] = { "Info", "Stats" };

/* Caller history entry */
typedef struct {
    char      name[36];
    time_t    login_time;
    int       node_num;
} caller_entry_t;

/* Node states */
typedef enum {
    NODE_INACTIVE = 0,
    NODE_STARTING,
    NODE_WFC,           /* Waiting for caller */
    NODE_CONNECTED,
    NODE_STOPPING
} node_state_t;

/* Node information */
typedef struct {
    int             node_num;
    node_state_t    state;
    pid_t           max_pid;        /* PID of max process */
    pid_t           bridge_pid;     /* PID of bridge process (if connected) */
    int             pty_master;     /* PTY master fd for max process */
    char            username[64];
    char            activity[32];
    time_t          connect_time;
    unsigned long   baud;
    char            socket_path[256];
    char            lock_path[256];
} node_info_t;

/* Global state */
static node_info_t  nodes[MAX_NODES];
static int          num_nodes = DEFAULT_NODES;
static int          listen_fd = -1;
static int          listen_port = DEFAULT_PORT;
static char         base_path[512] = ".";
static char         max_path[512] = "./bin/max";
static char         config_path[512] = "etc/max.prm";
static volatile int running = 1;
static volatile int need_refresh = 1;
static int          selected_node = 0;  /* Currently selected node (0-based) */
static int          scroll_offset = 0;   /* For scrolling node list */
static WINDOW      *status_win = NULL;
static WINDOW      *info_win = NULL;
static FILE        *debug_log = NULL;

/* Statistics tracking */
static struct _bbs_stats bbs_stats;      /* Global BBS stats (from node 0) */
static struct _usr  current_user;        /* Current user on selected node */
static int          current_user_valid = 0;
static struct callinfo callers[MAX_CALLER_HISTORY];  /* Recent callers from callers.bbs */
static int          callers_count = 0;
/* caller_scroll removed - not currently used */

/* System information from PRM file */
static HPRM         prm_handle = NULL;
static char         system_name[64] = "";
static char         sysop_name[64] = "";
static char         ftn_address[32] = "";
static int          user_count = 0;

/* Runtime statistics */
static time_t       start_time = 0;      /* When maxtel started */
static int          peak_online = 0;     /* Peak concurrent users */

/* Layout state */
static layout_mode_t current_layout = LAYOUT_FULL;
static system_tab_t  current_tab = TAB_SYSTEM_INFO;
static volatile int  need_resize = 0;    /* Set by SIGWINCH */
static int           requested_cols = 0; /* User-requested terminal size */
static int           requested_rows = 0;
static int           headless_mode = 0;   /* Run without ncurses UI */
static int           daemonize = 0;       /* Fork to background */

#define DEBUG(fmt, ...) do { if (debug_log) { fprintf(debug_log, fmt "\n", ##__VA_ARGS__); fflush(debug_log); } } while(0)

/* Forward declarations */
static void setup_signals(void);
static void signal_handler(int sig);
static void sigchld_handler(int sig);
static int  setup_listener(int port);
static int  spawn_node(int node_num);
static void kill_node(int node_num);
static void restart_node(int node_num);
static int  find_free_node(void);
static void handle_connection(int client_fd, struct sockaddr_in *addr);
static void bridge_connection(int client_fd, int node_num);
static void drain_pty(int node_num);
static void update_node_status(void);
static void draw_box(WINDOW *win, int height, int width, int y, int x, const char *title);
static void init_display(void);
static void update_display(void);
static void cleanup_display(void);
static void ensure_visible(void);
static void handle_input(int ch);
static void load_bbs_stats(void);
static void load_current_user(int node_num);
static void load_callers(void);
static void load_prm_info(void);
static void load_user_count(void);
static void cleanup(void);
static void detect_and_negotiate(int fd, int *telnet_mode, int *ansi_mode);
static void sigwinch_handler(int sig);
static void detect_layout(void);
static void handle_resize(void);
static void request_terminal_size(int cols, int rows);

/* Signal setup */
static void setup_signals(void)
{
    struct sigaction sa;
    
    /* SIGINT, SIGTERM - graceful shutdown */
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    
    /* SIGCHLD - child process management */
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
    
    /* Ignore SIGPIPE */
    signal(SIGPIPE, SIG_IGN);
    
    /* SIGWINCH - terminal resize */
    sa.sa_handler = sigwinch_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, NULL);
}

static void signal_handler(int sig)
{
    (void)sig;
    running = 0;
}

static void sigchld_handler(int sig)
{
    (void)sig;
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        /* Find which node this was */
        for (int i = 0; i < num_nodes; i++) {
            if (nodes[i].max_pid == pid) {
                nodes[i].max_pid = 0;
                nodes[i].state = NODE_INACTIVE;
                if (nodes[i].pty_master >= 0) {
                    close(nodes[i].pty_master);
                    nodes[i].pty_master = -1;
                }
                /* Remove socket and lock files */
                unlink(nodes[i].socket_path);
                unlink(nodes[i].lock_path);
                need_refresh = 1;
                break;
            }
            if (nodes[i].bridge_pid == pid) {
                nodes[i].bridge_pid = 0;
                nodes[i].state = NODE_WFC;
                nodes[i].username[0] = '\0';
                nodes[i].activity[0] = '\0';
                nodes[i].connect_time = 0;
                need_refresh = 1;
                break;
            }
        }
    }
}

/* SIGWINCH handler - terminal resized */
static void sigwinch_handler(int sig)
{
    (void)sig;
    need_resize = 1;
}

/* Detect appropriate layout based on terminal size */
static void detect_layout(void)
{
    layout_mode_t new_layout = LAYOUT_COMPACT;
    
    /* Find the best layout for current terminal size */
    for (int i = NUM_LAYOUTS - 1; i >= 0; i--) {
        if (COLS >= layouts[i].min_cols && LINES >= layouts[i].min_rows) {
            new_layout = (layout_mode_t)i;
            break;
        }
    }
    
    if (new_layout != current_layout) {
        current_layout = new_layout;
        DEBUG("Layout changed to %s (%dx%d)",
              new_layout == LAYOUT_FULL ? "FULL" :
              new_layout == LAYOUT_MEDIUM ? "MEDIUM" : "COMPACT",
              COLS, LINES);
    }
}

/* Handle terminal resize */
static void handle_resize(void)
{
    endwin();
    refresh();
    
    /* Recreate windows with new size */
    if (status_win) delwin(status_win);
    if (info_win) delwin(info_win);
    
    status_win = newwin(LINES - 1, COLS, 0, 0);
    info_win = newwin(1, COLS, LINES - 1, 0);
    wbkgd(info_win, COLOR_PAIR(9));
    
    detect_layout();
    need_refresh = 1;
    need_resize = 0;
}

/* Request terminal to resize (xterm-compatible) */
static void request_terminal_size(int cols, int rows)
{
    /* Use xterm escape sequence to resize */
    printf("\033[8;%d;%dt", rows, cols);
    fflush(stdout);
    
    /* Give terminal time to resize */
    usleep(100000);
    
    /* Reinitialize ncurses to pick up new size */
    endwin();
    refresh();
    
    DEBUG("Requested terminal resize to %dx%d", cols, rows);
}

/* TCP Listener */
static int setup_listener(int port)
{
    int fd;
    struct sockaddr_in addr;
    int opt = 1;
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }
    
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }
    
    if (listen(fd, 5) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }
    
    /* Non-blocking */
    fcntl(fd, F_SETFL, O_NONBLOCK);
    
    return fd;
}

/* Spawn a max node process */
static int spawn_node(int node_num)
{
    pid_t pid;
    int master_fd;
    char node_str[16];
    char port_str[16];
    
    if (node_num < 0 || node_num >= MAX_NODES)
        return -1;
    
    node_info_t *node = &nodes[node_num];
    
    if (node->state != NODE_INACTIVE)
        return -1;
    
    /* Build paths */
    snprintf(node->socket_path, sizeof(node->socket_path), 
             "%s/%s%d", base_path, SOCKET_PREFIX, node_num + 1);
    snprintf(node->lock_path, sizeof(node->lock_path),
             "%s/%s%d%s", base_path, SOCKET_PREFIX, node_num + 1, LOCK_SUFFIX);
    
    /* Remove stale socket/lock */
    unlink(node->socket_path);
    unlink(node->lock_path);
    
    /* Fork with PTY */
    pid = forkpty(&master_fd, NULL, NULL, NULL);
    
    if (pid < 0) {
        perror("forkpty");
        return -1;
    }
    
    if (pid == 0) {
        /* Child - exec max */
        char node_arg[16];
        char lib_path[1024];
        char mex_path[1024];
        char full_base[1024];
        
        snprintf(node_str, sizeof(node_str), "%d", node_num + 1);
        snprintf(port_str, sizeof(port_str), "-pt%d", node_num + 1);
        snprintf(node_arg, sizeof(node_arg), "-n%d", node_num + 1);
        
        /* Get absolute path for base */
        if (base_path[0] == '/') {
            strncpy(full_base, base_path, sizeof(full_base) - 1);
        } else {
            if (getcwd(full_base, sizeof(full_base) - strlen(base_path) - 2)) {
                strcat(full_base, "/");
                strcat(full_base, base_path);
            }
        }
        
        /* Set up environment variables */
        char maximus_env[1024];
        snprintf(lib_path, sizeof(lib_path), "%s/lib", full_base);
        snprintf(mex_path, sizeof(mex_path), "%s/m", full_base);
        snprintf(maximus_env, sizeof(maximus_env), "%s/%s", full_base, config_path);
        
#ifdef DARWIN
        setenv("DYLD_LIBRARY_PATH", lib_path, 1);
#else
        setenv("LD_LIBRARY_PATH", lib_path, 1);
#endif
        setenv("MEX_INCLUDE", mex_path, 1);
        setenv("MAX_INSTALL_PATH", full_base, 1);
        setenv("MAXIMUS", maximus_env, 1);
        setenv("SHELL", "/bin/sh", 0);  /* Don't override if set */
        
        /* Change to base directory */
        chdir(base_path);
        
        /* Match working command: ./bin/max -w -pt1 -n1 -b38400 etc/max.prm */
        execl(max_path, "max", "-w", port_str, node_arg, "-b38400", 
              config_path, NULL);
        
        perror("execl");
        _exit(1);
    }
    
    /* Parent */
    node->node_num = node_num + 1;
    node->max_pid = pid;
    node->pty_master = master_fd;
    node->state = NODE_STARTING;
    DEBUG("Spawned node %d with PID %d, PTY master fd %d", node_num + 1, pid, master_fd);
    DEBUG("Socket path: %s", node->socket_path);
    node->bridge_pid = 0;
    node->username[0] = '\0';
    node->activity[0] = '\0';
    node->connect_time = 0;
    node->baud = 0;
    
    /* Non-blocking PTY */
    fcntl(master_fd, F_SETFL, O_NONBLOCK);
    
    need_refresh = 1;
    return 0;
}

/* Kill a node */
static void kill_node(int node_num)
{
    if (node_num < 0 || node_num >= num_nodes)
        return;
    
    node_info_t *node = &nodes[node_num];
    
    DEBUG("Killing node %d (max_pid=%d, bridge_pid=%d)", 
          node_num + 1, node->max_pid, node->bridge_pid);
    
    if (node->bridge_pid > 0) {
        kill(node->bridge_pid, SIGTERM);
        kill(node->bridge_pid, SIGKILL);  /* Force kill */
        node->bridge_pid = 0;
    }
    
    if (node->max_pid > 0) {
        kill(node->max_pid, SIGTERM);
        usleep(100000);  /* Give it 100ms to die gracefully */
        kill(node->max_pid, SIGKILL);  /* Force kill */
    }
    
    /* Close PTY master */
    if (node->pty_master >= 0) {
        close(node->pty_master);
        node->pty_master = -1;
    }
    
    /* Clean up socket */
    unlink(node->socket_path);
    
    node->state = NODE_STOPPING;
    need_refresh = 1;
}

/* Restart a node */
static void restart_node(int node_num)
{
    if (node_num < 0 || node_num >= num_nodes)
        return;
    
    node_info_t *node = &nodes[node_num];
    
    /* If already inactive, just spawn */
    if (node->state == NODE_INACTIVE || node->max_pid == 0) {
        node->state = NODE_INACTIVE;
        spawn_node(node_num);
        return;
    }
    
    /* Otherwise kill and let SIGCHLD handler clean up */
    kill_node(node_num);
}

/* Find a free node for incoming connection */
static int find_free_node(void)
{
    for (int i = 0; i < num_nodes; i++) {
        if (nodes[i].state == NODE_WFC) {
            /* Verify socket exists */
            struct stat st;
            if (stat(nodes[i].socket_path, &st) == 0) {
                return i;
            }
        }
    }
    return -1;
}

/* Telnet detection and negotiation (from maxcomm) */
static void detect_and_negotiate(int fd, int *telnet_mode, int *ansi_mode)
{
    unsigned char probe[8];
    unsigned char buf[256];
    int buflen = 0;
    fd_set rfds;
    struct timeval tv;
    int n, i;
    int got_iac = 0;
    int got_ansi = 0;
    
    /* Print detection message */
    write(fd, "\r\nDetecting terminal... ", 24);
    
    /* Send Telnet probe: IAC DO SGA */
    probe[0] = 255;  /* IAC */
    probe[1] = 253;  /* DO */
    probe[2] = 3;    /* SGA */
    write(fd, probe, 3);
    
    /* Wait for telnet response */
    tv.tv_sec = 0;
    tv.tv_usec = 150000;
    
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    
    while (select(fd + 1, &rfds, NULL, NULL, &tv) > 0) {
        n = read(fd, buf + buflen, sizeof(buf) - buflen - 1);
        if (n <= 0) break;
        buflen += n;
        
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 50000;
    }
    
    /* Check for IAC response */
    for (i = 0; i < buflen; i++) {
        if (buf[i] == 255) {  /* IAC */
            got_iac = 1;
            break;
        }
    }
    
    /* If telnet, assume ANSI */
    if (got_iac) {
        got_ansi = 1;
    } else {
        /* Send ANSI probe */
        probe[0] = 0x1B;
        probe[1] = '[';
        probe[2] = '6';
        probe[3] = 'n';
        write(fd, probe, 4);
        
        buflen = 0;
        tv.tv_sec = 0;
        tv.tv_usec = 200000;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        
        while (select(fd + 1, &rfds, NULL, NULL, &tv) > 0) {
            n = read(fd, buf + buflen, sizeof(buf) - buflen - 1);
            if (n <= 0) break;
            buflen += n;
            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);
            tv.tv_sec = 0;
            tv.tv_usec = 50000;
        }
        
        for (i = 0; i < buflen; i++) {
            if (buf[i] == 0x1B && i + 1 < buflen && buf[i+1] == '[') {
                got_ansi = 1;
                break;
            }
        }
    }
    
    *telnet_mode = got_iac;
    *ansi_mode = got_ansi;
    
    /* Clear line and report */
    write(fd, "\x1B[2K\rDetecting terminal...", 26);
    
    if (got_iac && got_ansi)
        write(fd, " Telnet+ANSI\r\n", 14);
    else if (got_iac)
        write(fd, " Telnet\r\n", 9);
    else if (got_ansi)
        write(fd, " ANSI\r\n", 7);
    else
        write(fd, " Raw\r\n", 6);
    
    /* Send telnet negotiation if needed */
    if (got_iac) {
        unsigned char cmd[4];
        cmd[0] = 255; cmd[1] = 254; cmd[2] = 36;  /* DONT ENVIRON */
        write(fd, cmd, 3);
        cmd[0] = 255; cmd[1] = 251; cmd[2] = 1;   /* WILL ECHO */
        write(fd, cmd, 3);
        cmd[0] = 255; cmd[1] = 251; cmd[2] = 3;   /* WILL SGA */
        write(fd, cmd, 3);
        cmd[0] = 255; cmd[1] = 254; cmd[2] = 31;  /* DONT NAWS */
        write(fd, cmd, 3);
        
        /* Drain responses to negotiation commands */
        tv.tv_sec = 0;
        tv.tv_usec = 100000;  /* 100ms */
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        while (select(fd + 1, &rfds, NULL, NULL, &tv) > 0) {
            n = read(fd, buf, sizeof(buf));
            if (n <= 0) break;
            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);
            tv.tv_sec = 0;
            tv.tv_usec = 50000;
        }
    }
}

/* Handle incoming telnet connection */
static void handle_connection(int client_fd, struct sockaddr_in *addr)
{
    int node_idx;
    pid_t pid;
    
    node_idx = find_free_node();
    
    if (node_idx < 0) {
        const char *msg = "\r\nSorry, all nodes are busy. Please try again later.\r\n";
        write(client_fd, msg, strlen(msg));
        close(client_fd);
        return;
    }
    
    /* Fork bridge process */
    pid = fork();
    
    if (pid < 0) {
        perror("fork");
        close(client_fd);
        return;
    }
    
    if (pid == 0) {
        /* Child - bridge process */
        bridge_connection(client_fd, node_idx);
        _exit(0);
    }
    
    /* Parent */
    close(client_fd);
    nodes[node_idx].bridge_pid = pid;
    nodes[node_idx].state = NODE_CONNECTED;
    nodes[node_idx].connect_time = time(NULL);
    snprintf(nodes[node_idx].activity, sizeof(nodes[node_idx].activity),
             "Connected from %s", inet_ntoa(addr->sin_addr));
    need_refresh = 1;
}

/* Bridge client to max node (runs in child process) */
static void bridge_connection(int client_fd, int node_num)
{
    int sock_fd;
    struct sockaddr_un addr;
    fd_set rfds;
    int maxfd;
    char buf[4096];
    int n;
    int telnet_mode = 0, ansi_mode = 0;
    
    /* Detect terminal type */
    detect_and_negotiate(client_fd, &telnet_mode, &ansi_mode);
    
    /* Connect to max's socket */
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        _exit(1);
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, nodes[node_num].socket_path, sizeof(addr.sun_path) - 1);
    
    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock_fd);
        _exit(1);
    }
    
    /* Bridge loop */
    maxfd = (client_fd > sock_fd) ? client_fd : sock_fd;
    
    while (1) {
        FD_ZERO(&rfds);
        FD_SET(client_fd, &rfds);
        FD_SET(sock_fd, &rfds);
        
        if (select(maxfd + 1, &rfds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            break;
        }
        
        /* Data from client -> max */
        if (FD_ISSET(client_fd, &rfds)) {
            n = read(client_fd, buf, sizeof(buf));
            if (n <= 0) break;
            /* TODO: telnet interpretation if telnet_mode */
            write(sock_fd, buf, n);
        }
        
        /* Data from max -> client */
        if (FD_ISSET(sock_fd, &rfds)) {
            n = read(sock_fd, buf, sizeof(buf));
            if (n <= 0) break;
            /* TODO: IAC escaping if telnet_mode */
            write(client_fd, buf, n);
        }
    }
    
    close(sock_fd);
    close(client_fd);
}

/* Drain PTY output to prevent blocking */
static void drain_pty(int node_num)
{
    node_info_t *node = &nodes[node_num];
    char buf[1024];
    int n;
    
    if (node->pty_master < 0)
        return;
    
    /* Non-blocking read to drain any output */
    while ((n = read(node->pty_master, buf, sizeof(buf))) > 0) {
        /* Discard output - could log it for debugging */
    }
}

/* Update node status from bbstat files */
static void update_node_status(void)
{
    for (int i = 0; i < num_nodes; i++) {
        node_info_t *node = &nodes[i];
        
        /* Drain PTY output to prevent child from blocking */
        drain_pty(i);
        
        /* Check if socket exists -> node is ready */
        if (node->state == NODE_STARTING) {
            struct stat st;
            if (stat(node->socket_path, &st) == 0) {
                DEBUG("Node %d socket found: %s", i + 1, node->socket_path);
                node->state = NODE_WFC;
                need_refresh = 1;
            }
        }
        
        /* Read lastus file for current user - written at login */
        if (node->state == NODE_CONNECTED) {
            char lastus_path[256];
            char username[36];
            struct stat st;
            snprintf(lastus_path, sizeof(lastus_path), "%s/%s%02d.bbs", 
                     base_path, LASTUS_PREFIX, i + 1);
            
            /* Only read if file was modified after connection started */
            if (stat(lastus_path, &st) == 0 && st.st_mtime >= node->connect_time) {
                int fd = open(lastus_path, O_RDONLY);
                if (fd >= 0) {
                    /* Read first 36 bytes = user name */
                    if (read(fd, username, 36) == 36 && username[0]) {
                        username[35] = '\0';  /* Ensure null termination */
                        if (strncmp(node->username, username, sizeof(node->username) - 1) != 0) {
                            strncpy(node->username, username, sizeof(node->username) - 1);
                            node->username[sizeof(node->username) - 1] = '\0';
                            need_refresh = 1;
                        }
                    }
                    close(fd);
                }
            }
        } else if (node->state == NODE_WFC && node->username[0]) {
            /* Clear username when back to WFC */
            node->username[0] = '\0';
            need_refresh = 1;
        }
    }
    
    /* Load global stats, current user, callers, and user count */
    load_bbs_stats();
    load_current_user(selected_node);
    load_callers();
    load_user_count();
    
    update_display();
}

/* Load BBS statistics from bbstat00.bbs or first available */
static void load_bbs_stats(void)
{
    char path[256];
    int fd;
    
    snprintf(path, sizeof(path), "%s/%s00.bbs", base_path, STATUS_PREFIX);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        snprintf(path, sizeof(path), "%s/%s01.bbs", base_path, STATUS_PREFIX);
        fd = open(path, O_RDONLY);
    }
    
    if (fd >= 0) {
        read(fd, &bbs_stats, sizeof(bbs_stats));
        close(fd);
    }
}

/* Load current user info from selected node's lastus file */
static void load_current_user(int node_num)
{
    char path[256];
    int fd;
    node_info_t *node = &nodes[node_num];
    
    current_user_valid = 0;
    
    if (node->state != NODE_CONNECTED || !node->username[0])
        return;
    
    snprintf(path, sizeof(path), "%s/%s%02d.bbs", base_path, LASTUS_PREFIX, node_num + 1);
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        if (read(fd, &current_user, sizeof(current_user)) == sizeof(current_user)) {
            current_user_valid = 1;
        }
        close(fd);
    }
}

/* Load recent callers from callers.bbs (read last N entries) */
static void load_callers(void)
{
    char path[256];
    struct stat st;
    int fd;
    
    snprintf(path, sizeof(path), "%s/etc/callers.bbs", base_path);
    if (stat(path, &st) < 0 || st.st_size < (off_t)sizeof(struct callinfo))
        return;
    
    fd = open(path, O_RDONLY);
    if (fd < 0)
        return;
    
    /* Calculate number of records and how many to read */
    int total_records = st.st_size / sizeof(struct callinfo);
    int to_read = (total_records < MAX_CALLER_HISTORY) ? total_records : MAX_CALLER_HISTORY;
    
    /* Seek to last N records */
    off_t offset = (total_records - to_read) * sizeof(struct callinfo);
    lseek(fd, offset, SEEK_SET);
    
    /* Read records into array (newest last in file, we want newest first) */
    struct callinfo temp[MAX_CALLER_HISTORY];
    callers_count = 0;
    for (int i = 0; i < to_read; i++) {
        if (read(fd, &temp[i], sizeof(struct callinfo)) == sizeof(struct callinfo)) {
            callers_count++;
        }
    }
    close(fd);
    
    /* Reverse order so newest is first */
    for (int i = 0; i < callers_count; i++) {
        callers[i] = temp[callers_count - 1 - i];
    }
}

/* Load system information from PRM file */
static void load_prm_info(void)
{
    char prm_path[512];
    
    snprintf(prm_path, sizeof(prm_path), "%s/%s", base_path, config_path);
    
    /* Try to open the PRM file */
    prm_handle = PrmFileOpen(prm_path, 0);
    if (!prm_handle)
        return;
    
    /* Get system name */
    const char *name = PrmFileString(prm_handle, system_name);
    if (name && *name)
        strncpy(system_name, name, sizeof(system_name) - 1);
    
    /* Get sysop name */
    const char *sysop = PrmFileString(prm_handle, sysop);
    if (sysop && *sysop)
        strncpy(sysop_name, sysop, sizeof(sysop_name) - 1);
    
    /* Get FidoNet address */
    NETADDR *addr = &prm_handle->mp.address[0];
    if (addr->zone || addr->net || addr->node) {
        if (addr->point)
            snprintf(ftn_address, sizeof(ftn_address), "%d:%d/%d.%d", 
                     addr->zone, addr->net, addr->node, addr->point);
        else
            snprintf(ftn_address, sizeof(ftn_address), "%d:%d/%d", 
                     addr->zone, addr->net, addr->node);
    }
}

/* Load user count from user.bbs file */
static void load_user_count(void)
{
    char path[512];
    struct stat st;
    
    /* Try standard user file locations */
    snprintf(path, sizeof(path), "%s/etc/user.bbs", base_path);
    if (stat(path, &st) == 0) {
        user_count = st.st_size / sizeof(struct _usr);
        return;
    }
    
    snprintf(path, sizeof(path), "%s/user.bbs", base_path);
    if (stat(path, &st) == 0) {
        user_count = st.st_size / sizeof(struct _usr);
        return;
    }
    
    user_count = 0;
}

/* Draw a box with title */
static void draw_box(WINDOW *win, int height, int width, int y, int x, const char *title)
{
    /* Draw border */
    mvwhline(win, y, x + 1, ACS_HLINE, width - 2);
    mvwhline(win, y + height - 1, x + 1, ACS_HLINE, width - 2);
    mvwvline(win, y + 1, x, ACS_VLINE, height - 2);
    mvwvline(win, y + 1, x + width - 1, ACS_VLINE, height - 2);
    mvwaddch(win, y, x, ACS_ULCORNER);
    mvwaddch(win, y, x + width - 1, ACS_URCORNER);
    mvwaddch(win, y + height - 1, x, ACS_LLCORNER);
    mvwaddch(win, y + height - 1, x + width - 1, ACS_LRCORNER);
    
    /* Title */
    if (title) {
        int tlen = strlen(title);
        int tpos = x + (width - tlen - 2) / 2;
        mvwprintw(win, y, tpos, " %s ", title);
    }
}

/* Draw User Stats content */
static void draw_user_stats_content(int y, int x, int width, int height)
{
    (void)width; (void)height;
    if (current_user_valid) {
        wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y, x, "Name  : ");
        wattron(status_win, COLOR_PAIR(16)); mvwprintw(status_win, y, x + 8, "%.18s", current_user.name);
        wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y + 1, x, "City  : ");
        wattron(status_win, COLOR_PAIR(16)); mvwprintw(status_win, y + 1, x + 8, "%.18s", current_user.city);
        wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y + 2, x, "Calls : ");
        wattron(status_win, COLOR_PAIR(16)); mvwprintw(status_win, y + 2, x + 8, "%u", current_user.times);
        /* blank line */
        wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y + 4, x, "Msgs  : ");
        wattron(status_win, COLOR_PAIR(16)); mvwprintw(status_win, y + 4, x + 8, "%u/%u", current_user.msgs_posted, current_user.msgs_read);
        wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y + 5, x, "Up/Dn : ");
        wattron(status_win, COLOR_PAIR(16)); mvwprintw(status_win, y + 5, x + 8, "%uK/%uK", current_user.up, current_user.down);
        wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y + 6, x, "Files : ");
        wattron(status_win, COLOR_PAIR(16)); mvwprintw(status_win, y + 6, x + 8, "%u/%u", current_user.nup, current_user.ndown);
    } else {
        wattron(status_win, COLOR_PAIR(14));
        mvwprintw(status_win, y + 2, x, "(No user online)");
    }
    wattroff(status_win, COLOR_PAIR(16));
}

/* Draw System Info content (BBS name, Sysop, FTN, Time, etc) */
static void draw_system_info_content(int y, int x, int width, int height)
{
    (void)height;
    int val_w = width - 10;
    if (val_w < 8) val_w = 8;
    /* No max cap - expand with available space */
    
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    char time_buf[16];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_now);
    
    int active = 0, waiting = 0;
    for (int i = 0; i < num_nodes; i++) {
        if (nodes[i].state == NODE_CONNECTED) active++;
        else if (nodes[i].state == NODE_WFC) waiting++;
    }
    if (active > peak_online) peak_online = active;
    
    wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y, x, "BBS     : ");
    wattron(status_win, COLOR_PAIR(19)); mvwprintw(status_win, y, x + 10, "%.*s", val_w, system_name[0] ? system_name : "-");
    wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y + 1, x, "Sysop   : ");
    wattron(status_win, COLOR_PAIR(19)); mvwprintw(status_win, y + 1, x + 10, "%.*s", val_w, sysop_name[0] ? sysop_name : "-");
    wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y + 2, x, "FTN     : ");
    wattron(status_win, COLOR_PAIR(19)); mvwprintw(status_win, y + 2, x + 10, "%.*s", val_w, ftn_address[0] ? ftn_address : "-");
    wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y + 3, x, "Time    : ");
    wattron(status_win, COLOR_PAIR(16)); mvwprintw(status_win, y + 3, x + 10, "%s", time_buf);
    wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y + 4, x, "Nodes   : ");
    wattron(status_win, COLOR_PAIR(16)); mvwprintw(status_win, y + 4, x + 10, "%d", num_nodes);
    wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y + 5, x, "Online  : ");
    wattron(status_win, COLOR_PAIR(6));  mvwprintw(status_win, y + 5, x + 10, "%d", active);
    wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y + 6, x, "Waiting : ");
    wattron(status_win, COLOR_PAIR(5));  mvwprintw(status_win, y + 6, x + 10, "%d", waiting);
    wattroff(status_win, COLOR_PAIR(5));
}

/* Draw Callers content - show last N callers, expand columns with width */
static void draw_callers_content(int y, int x, int width, int height)
{
    /* Column layout based on width:
     * Compact (<44):  Node Calls Name(14)
     * Medium (44-55): Node Calls Name(20) Date/Time
     * Full (56+):     Node Calls Name(20) Date/Time City(...)
     */
    int show_datetime = (width >= 44);
    int show_city = (width >= 56);
    int city_width = width - 56;  /* Remaining space for city */
    if (city_width < 8) city_width = 8;
    if (city_width > 20) city_width = 20;
    
    /* Header line */
    wattron(status_win, COLOR_PAIR(14));
    if (show_city) {
        mvwprintw(status_win, y, x, "Node Calls Name               Date/Time      City");
    } else if (show_datetime) {
        mvwprintw(status_win, y, x, "Node Calls Name               Date/Time");
    } else {
        mvwprintw(status_win, y, x, "Node Calls Name");
    }
    wattroff(status_win, COLOR_PAIR(14));
    
    int max_rows = height - 2;  /* Header + content */
    if (max_rows < 1) max_rows = 1;
    if (max_rows > CALLERS_MAX_PRELOAD) max_rows = CALLERS_MAX_PRELOAD;
    
    int row = 0;
    for (int i = 0; i < callers_count && row < max_rows; i++) {
        /* Filter: CALL_LOGON flag only */
        if (!(callers[i].flags & 0x8000)) continue;  /* Not a logon */
        
        wattron(status_win, COLOR_PAIR(17));  /* Magenta for node */
        mvwprintw(status_win, y + 1 + row, x, "%-4d", callers[i].task);
        wattron(status_win, COLOR_PAIR(7));   /* Red for calls count */
        mvwprintw(status_win, y + 1 + row, x + 5, "%-5d", callers[i].calls);
        wattron(status_win, COLOR_PAIR(18));  /* Green for name */
        
        if (show_datetime) {
            mvwprintw(status_win, y + 1 + row, x + 11, "%-18.18s", callers[i].name);
            /* Format date/time from DOS timestamp */
            wattron(status_win, COLOR_PAIR(16));  /* Yellow for date */
            mvwprintw(status_win, y + 1 + row, x + 30, "%d/%d/%02d %02d:%02d",
                      callers[i].login.msg_st.date.mo,
                      callers[i].login.msg_st.date.da,
                      (callers[i].login.msg_st.date.yr + 80) % 100,  /* DOS year starts 1980 */
                      callers[i].login.msg_st.date.hh,
                      callers[i].login.msg_st.date.mm);
            if (show_city) {
                wattron(status_win, COLOR_PAIR(14));  /* Cyan for city */
                mvwprintw(status_win, y + 1 + row, x + 45, "%.*s", city_width, callers[i].city);
            }
        } else {
            mvwprintw(status_win, y + 1 + row, x + 11, "%.14s", callers[i].name);
        }
        row++;
    }
    if (row == 0) {
        wattron(status_win, COLOR_PAIR(14));
        mvwprintw(status_win, y + 1, x, "(No callers)");
    }
    wattroff(status_win, COLOR_PAIR(14));
}

/* Draw System Stats content */
static void draw_system_stats_content(int y, int x, int width, int height)
{
    (void)width; (void)height;
    time_t now = time(NULL);
    time_t uptime_secs = now - start_time;
    int up_days = uptime_secs / 86400;
    int up_hours = (uptime_secs % 86400) / 3600;
    int up_mins = (uptime_secs % 3600) / 60;
    char uptime_str[32];
    if (up_days > 0)
        snprintf(uptime_str, sizeof(uptime_str), "%dd %02d:%02d", up_days, up_hours, up_mins);
    else
        snprintf(uptime_str, sizeof(uptime_str), "%02d:%02d", up_hours, up_mins);
    
    struct tm *tm_start = localtime(&start_time);
    char started_str[32];
    strftime(started_str, sizeof(started_str), "%H:%M %d-%b", tm_start);
    
    wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y, x, "Started     : ");
    wattron(status_win, COLOR_PAIR(16)); mvwprintw(status_win, y, x + 14, "%s", started_str);
    wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y + 1, x, "Uptime      : ");
    wattron(status_win, COLOR_PAIR(16)); mvwprintw(status_win, y + 1, x + 14, "%s", uptime_str);
    wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y + 2, x, "Peak Online : ");
    wattron(status_win, COLOR_PAIR(16)); mvwprintw(status_win, y + 2, x + 14, "%d", peak_online);
    wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y + 3, x, "Users       : ");
    wattron(status_win, COLOR_PAIR(16)); mvwprintw(status_win, y + 3, x + 14, "%d", user_count);
    wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y + 4, x, "Messages    : ");
    wattron(status_win, COLOR_PAIR(16)); mvwprintw(status_win, y + 4, x + 14, "%lu", (unsigned long)bbs_stats.msgs_written);
    wattron(status_win, COLOR_PAIR(15)); mvwprintw(status_win, y + 5, x, "Downloads   : ");
    wattron(status_win, COLOR_PAIR(16)); mvwprintw(status_win, y + 5, x + 14, "%lu", (unsigned long)bbs_stats.total_dl);
    wattroff(status_win, COLOR_PAIR(16));
}

/* ncurses display */
static void init_display(void)
{
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);
    
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLACK);    /* Shaded background */
        init_pair(2, COLOR_CYAN, COLOR_BLACK);    /* Box borders */
        init_pair(3, COLOR_WHITE, COLOR_BLACK);   /* Titles/headers - WHITE */
        init_pair(4, COLOR_WHITE, COLOR_BLACK);   /* Normal text */
        init_pair(5, COLOR_GREEN, COLOR_BLACK);   /* WFC status */
        init_pair(6, COLOR_YELLOW, COLOR_BLACK);  /* Connected/Online */
        init_pair(7, COLOR_RED, COLOR_BLACK);     /* Inactive/Stopping */
        init_pair(8, COLOR_BLACK, COLOR_CYAN);    /* Header bar */
        init_pair(9, COLOR_BLACK, COLOR_CYAN);    /* Status bar */
        init_pair(10, COLOR_BLACK, COLOR_WHITE);  /* Selected normal */
        init_pair(11, COLOR_BLACK, COLOR_RED);    /* Selected stopping */
        init_pair(12, COLOR_BLACK, COLOR_YELLOW); /* Selected starting */
        init_pair(13, COLOR_BLACK, COLOR_GREEN);  /* Selected WFC */
        init_pair(14, COLOR_CYAN, COLOR_BLACK);   /* Column headers - CYAN */
        init_pair(15, COLOR_RED, COLOR_BLACK);    /* Labels */
        init_pair(16, COLOR_YELLOW, COLOR_BLACK); /* Values */
        init_pair(17, COLOR_MAGENTA, COLOR_BLACK);/* Callers node */
        init_pair(18, COLOR_GREEN, COLOR_BLACK);  /* Callers name */
        init_pair(19, COLOR_GREEN, COLOR_BLACK);  /* System info values (BBS name, sysop, FTN) */
        init_pair(20, COLOR_BLACK, COLOR_WHITE);  /* Active tab */
        init_pair(21, COLOR_WHITE, COLOR_BLUE);   /* Inactive tab */
    }
    
    /* Request terminal resize if user specified */
    if (requested_cols > 0 && requested_rows > 0) {
        request_terminal_size(requested_cols, requested_rows);
    }
    
    /* Detect appropriate layout for current size */
    detect_layout();
    
    /* Main window - full screen */
    status_win = newwin(LINES - 1, COLS, 0, 0);
    
    /* Status bar at bottom */
    info_win = newwin(1, COLS, LINES - 1, 0);
    wbkgd(info_win, COLOR_PAIR(9));
}

static void update_display(void)
{
    time_t now = time(NULL);
    const layout_config_t *layout = &layouts[current_layout];
    
    werase(status_win);
    
    /* Fill background with shaded pattern */
    wattron(status_win, COLOR_PAIR(1));
    for (int y = 1; y < LINES - 1; y++) {
        for (int x = 0; x < COLS; x++) {
            mvwaddch(status_win, y, x, ACS_CKBOARD);
        }
    }
    wattroff(status_win, COLOR_PAIR(1));
    
    /* Header bar */
    wattron(status_win, COLOR_PAIR(8));
    mvwhline(status_win, 0, 0, ' ', COLS);
    mvwprintw(status_win, 0, 2, "MAXTEL v1.0");
    mvwprintw(status_win, 0, COLS/2 - 12, "Maximus Telnet Supervisor");
    mvwprintw(status_win, 0, COLS - 12, "Port: %d", listen_port);
    wattroff(status_win, COLOR_PAIR(8));
    
    /* =====================================================
     * TOP ROW: [User Stats] | [System Info/Stats]
     * Fixed height of 9 lines (7 content + 2 border)
     * ===================================================== */
    int top_height = 9;
    int user_width = 30;  /* Fixed width for user stats */
    int sys_width = COLS - user_width - 3;  /* Rest for system */
    
    /* USER STATS BOX (top left) - starts at row 2 for gap after header */
    wattron(status_win, COLOR_PAIR(4));
    for (int row = 3; row < 3 + top_height - 1; row++) {
        mvwhline(status_win, row, 2, ' ', user_width - 2);
    }
    wattroff(status_win, COLOR_PAIR(4));
    wattron(status_win, COLOR_PAIR(2));
    draw_box(status_win, top_height, user_width, 2, 1, NULL);
    wattroff(status_win, COLOR_PAIR(2));
    wattron(status_win, COLOR_PAIR(3));
    mvwprintw(status_win, 2, 3, " User Stats ");
    wattroff(status_win, COLOR_PAIR(3));
    draw_user_stats_content(3, 3, user_width - 4, top_height - 2);
    
    /* SYSTEM BOX (top right) - tabbed or expanded based on width */
    int sys_x = user_width + 2;
    wattron(status_win, COLOR_PAIR(4));
    for (int row = 3; row < 3 + top_height - 1; row++) {
        mvwhline(status_win, row, sys_x + 1, ' ', sys_width - 2);
    }
    wattroff(status_win, COLOR_PAIR(4));
    wattron(status_win, COLOR_PAIR(2));
    draw_box(status_win, top_height, sys_width, 2, sys_x, NULL);
    wattroff(status_win, COLOR_PAIR(2));
    
    if (layout->expand_system) {
        /* Expanded: System Info left, System Stats right */
        int half_w = (sys_width - 2) / 2;
        wattron(status_win, COLOR_PAIR(3));
        mvwprintw(status_win, 2, sys_x + 2, " System ");
        wattroff(status_win, COLOR_PAIR(3));
        draw_system_info_content(3, sys_x + 2, half_w - 2, top_height - 2);
        
        /* Divider line */
        wattron(status_win, COLOR_PAIR(2));
        mvwvline(status_win, 3, sys_x + half_w, ACS_VLINE, top_height - 3);
        wattroff(status_win, COLOR_PAIR(2));
        
        wattron(status_win, COLOR_PAIR(3));
        mvwprintw(status_win, 2, sys_x + half_w + 2, " Stats ");
        wattroff(status_win, COLOR_PAIR(3));
        draw_system_stats_content(3, sys_x + half_w + 2, half_w - 2, top_height - 2);
    } else {
        /* Compact: Tabbed Info/Stats */
        int tab_x = sys_x + 2;
        for (int t = 0; t < TAB_COUNT; t++) {
            if (t == current_tab) {
                wattron(status_win, COLOR_PAIR(20) | A_BOLD);
            } else {
                wattron(status_win, COLOR_PAIR(14));
            }
            mvwprintw(status_win, 2, tab_x, " %s ", tab_names[t]);
            tab_x += strlen(tab_names[t]) + 3;
            wattroff(status_win, COLOR_PAIR(20) | COLOR_PAIR(14) | A_BOLD);
        }
        wattron(status_win, COLOR_PAIR(14));
        mvwprintw(status_win, 2, sys_x + sys_width - 8, "<Tab>");
        wattroff(status_win, COLOR_PAIR(14));
        
        if (current_tab == TAB_SYSTEM_INFO) {
            draw_system_info_content(3, sys_x + 2, sys_width - 4, top_height - 2);
        } else {
            draw_system_stats_content(3, sys_x + 2, sys_width - 4, top_height - 2);
        }
    }
    
    /* =====================================================
     * BOTTOM ROW: [Nodes] | [Callers]
     * Fills remaining space
     * ===================================================== */
    int bottom_y = 2 + top_height + 1;  /* After top row (shifted down 1) + 1 gap */
    int bottom_height = LINES - bottom_y - 2;  /* Leave room for status bar, -1 for spacing */
    if (bottom_height < 6) bottom_height = 6;
    
    /* Callers panel width - narrower to give Nodes more space */
    int callers_width = layout->callers_full_cols ? 48 : 30;
    int nodes_width = COLS - callers_width - 3;
    
    /* Calculate visible nodes */
    int max_vis_nodes = bottom_height - 4;  /* Height - header - borders */
    if (max_vis_nodes < 2) max_vis_nodes = 2;
    int visible_nodes = (num_nodes < max_vis_nodes) ? num_nodes : max_vis_nodes;
    int can_scroll = (num_nodes > max_vis_nodes);
    
    /* NODES BOX (bottom left) */
    wattron(status_win, COLOR_PAIR(4));
    for (int row = bottom_y + 1; row < bottom_y + bottom_height - 1; row++) {
        mvwhline(status_win, row, 2, ' ', nodes_width - 2);
    }
    wattroff(status_win, COLOR_PAIR(4));
    wattron(status_win, COLOR_PAIR(2));
    draw_box(status_win, bottom_height, nodes_width, bottom_y, 1, NULL);
    wattroff(status_win, COLOR_PAIR(2));
    wattron(status_win, COLOR_PAIR(3));
    mvwprintw(status_win, bottom_y, 3, " Nodes ");
    wattroff(status_win, COLOR_PAIR(3));
    
    /* Nodes header */
    wattron(status_win, COLOR_PAIR(14));
    if (layout->nodes_full_cols) {
        mvwprintw(status_win, bottom_y + 1, 3, "Node  Status      User                 Activity              Time");
    } else {
        mvwprintw(status_win, bottom_y + 1, 3, "Node  Status    User              Time");
    }
    wattroff(status_win, COLOR_PAIR(14));
    
    if (can_scroll) {
        wattron(status_win, COLOR_PAIR(3));
        if (scroll_offset > 0)
            mvwaddch(status_win, bottom_y, nodes_width - 4, ACS_UARROW);
        if (scroll_offset + visible_nodes < num_nodes)
            mvwaddch(status_win, bottom_y + bottom_height - 1, nodes_width - 4, ACS_DARROW);
        mvwprintw(status_win, bottom_y, nodes_width - 12, " %d-%d/%d ",
                  scroll_offset + 1, scroll_offset + visible_nodes, num_nodes);
        wattroff(status_win, COLOR_PAIR(3));
    }
    
    /* Node rows */
    for (int vi = 0; vi < visible_nodes; vi++) {
        int i = scroll_offset + vi;
        node_info_t *node = &nodes[i];
        const char *status;
        const char *user_display;
        int status_color, lightbar_color = 10;
        char time_str[16] = "--:--";
        
        switch (node->state) {
            case NODE_INACTIVE:  status = "Inactive"; status_color = 7; lightbar_color = 11; break;
            case NODE_STARTING:  status = "Starting"; status_color = 6; lightbar_color = 12; break;
            case NODE_WFC:       status = "WFC";      status_color = 5; lightbar_color = 13; break;
            case NODE_CONNECTED:
                status = "Online"; status_color = 6; lightbar_color = 12;
                if (node->connect_time > 0) {
                    int mins = (now - node->connect_time) / 60;
                    int secs = (now - node->connect_time) % 60;
                    snprintf(time_str, sizeof(time_str), "%02d:%02d", mins, secs);
                }
                break;
            case NODE_STOPPING:  status = "Stopping"; status_color = 7; lightbar_color = 11; break;
            default:             status = "Unknown";  status_color = 7; lightbar_color = 10;
        }
        
        if (node->state == NODE_WFC) user_display = "<waiting>";
        else if (node->state == NODE_CONNECTED && !node->username[0]) user_display = "Log-on";
        else if (node->username[0]) user_display = node->username;
        else user_display = "";
        
        int row = bottom_y + 2 + vi;
        if (i == selected_node) {
            wattron(status_win, COLOR_PAIR(lightbar_color));
            mvwhline(status_win, row, 2, ' ', nodes_width - 2);
            if (layout->nodes_full_cols) {
                mvwprintw(status_win, row, 3, "%4d  %-10s  %-20s %-20s  %s",
                          node->node_num, status, user_display,
                          node->activity[0] ? node->activity : "", time_str);
            } else {
                mvwprintw(status_win, row, 3, "%4d  %-8s  %-16s  %s",
                          node->node_num, status, user_display, time_str);
            }
            wattroff(status_win, COLOR_PAIR(lightbar_color));
        } else {
            wattron(status_win, COLOR_PAIR(4));
            mvwprintw(status_win, row, 3, "%4d  ", node->node_num);
            wattroff(status_win, COLOR_PAIR(4));
            wattron(status_win, COLOR_PAIR(status_color));
            if (layout->nodes_full_cols) {
                mvwprintw(status_win, row, 9, "%-10s", status);
            } else {
                mvwprintw(status_win, row, 9, "%-8s", status);
            }
            wattroff(status_win, COLOR_PAIR(status_color));
            wattron(status_win, COLOR_PAIR(4));
            if (layout->nodes_full_cols) {
                mvwprintw(status_win, row, 21, "%-20s %-20s  %s",
                          user_display, node->activity[0] ? node->activity : "", time_str);
            } else {
                mvwprintw(status_win, row, 19, "%-16s  %s", user_display, time_str);
            }
            wattroff(status_win, COLOR_PAIR(4));
        }
    }
    
    /* CALLERS BOX (bottom right) */
    int callers_x = nodes_width + 2;
    wattron(status_win, COLOR_PAIR(4));
    for (int row = bottom_y + 1; row < bottom_y + bottom_height - 1; row++) {
        mvwhline(status_win, row, callers_x + 1, ' ', callers_width - 2);
    }
    wattroff(status_win, COLOR_PAIR(4));
    wattron(status_win, COLOR_PAIR(2));
    draw_box(status_win, bottom_height, callers_width, bottom_y, callers_x, NULL);
    wattroff(status_win, COLOR_PAIR(2));
    /* Callers expand to fill available height */
    int callers_avail = bottom_height - 4;  /* Height minus borders and header */
    if (callers_avail > CALLERS_MAX_PRELOAD) callers_avail = CALLERS_MAX_PRELOAD;
    if (callers_avail < 1) callers_avail = 1;
    
    wattron(status_win, COLOR_PAIR(3));
    mvwprintw(status_win, bottom_y, callers_x + 2, " Callers (Last %d) ", callers_avail);
    wattroff(status_win, COLOR_PAIR(3));
    
    /* Today count on bottom border */
    wattron(status_win, COLOR_PAIR(14));
    mvwprintw(status_win, bottom_y + bottom_height - 1, callers_x + 2, " Today: %d ", bbs_stats.today_callers);
    wattroff(status_win, COLOR_PAIR(14));
    
    draw_callers_content(bottom_y + 1, callers_x + 2, callers_width - 4, bottom_height - 2);
    
    wrefresh(status_win);
    
    /* Status bar */
    werase(info_win);
    wattron(info_win, COLOR_PAIR(9));
    if (!layout->expand_system) {
        mvwprintw(info_win, 0, 1, "1-%d:Node  K:Kick  R:Restart  Tab:System  Q:Quit", num_nodes);
    } else {
        mvwprintw(info_win, 0, 1, "1-%d:Node  K:Kick  R:Restart  S:Snoop  Q:Quit", num_nodes);
    }
    const char *mode_str = current_layout == LAYOUT_FULL ? "Full" :
                           current_layout == LAYOUT_MEDIUM ? "Med" : "Cmp";
    mvwprintw(info_win, 0, COLS - 30, "%dx%d [%s]", COLS, LINES, mode_str);
    if (selected_node >= 0 && selected_node < num_nodes) {
        mvwprintw(info_win, 0, COLS - 15, "Node %d", selected_node + 1);
    }
    wattroff(info_win, COLOR_PAIR(9));
    wrefresh(info_win);
}

static void cleanup_display(void)
{
    if (status_win) delwin(status_win);
    if (info_win) delwin(info_win);
    endwin();
}

/* Ensure selected node is visible, adjust scroll if needed */
static void ensure_visible(void)
{
    /* Calculate visible nodes based on current layout */
    int top_height = 9;
    int bottom_y = 2 + top_height + 1;
    int bottom_height = LINES - bottom_y - 2;
    if (bottom_height < 6) bottom_height = 6;
    int max_vis = bottom_height - 4;  /* Height - header - borders */
    if (max_vis < 2) max_vis = 2;
    int visible_nodes = (num_nodes < max_vis) ? num_nodes : max_vis;
    
    if (selected_node < scroll_offset) {
        scroll_offset = selected_node;
    } else if (selected_node >= scroll_offset + visible_nodes) {
        scroll_offset = selected_node - visible_nodes + 1;
    }
    
    /* Clamp scroll_offset */
    if (scroll_offset < 0) scroll_offset = 0;
    if (scroll_offset > num_nodes - visible_nodes) {
        scroll_offset = num_nodes - visible_nodes;
        if (scroll_offset < 0) scroll_offset = 0;
    }
}

static void handle_input(int ch)
{
    if (ch >= '1' && ch <= '9') {
        int n = ch - '1';
        if (n < num_nodes) {
            selected_node = n;
            ensure_visible();
            need_refresh = 1;
        }
    }
    
    switch (ch) {
        case 'q':
        case 'Q':
            running = 0;
            break;
            
        case 'k':
        case 'K':
            if (selected_node >= 0 && selected_node < num_nodes) {
                kill_node(selected_node);
                need_refresh = 1;
            }
            break;
            
        case 'r':
        case 'R':
            if (selected_node >= 0 && selected_node < num_nodes) {
                restart_node(selected_node);
                need_refresh = 1;
            }
            break;
            
        case 's':
        case 'S':
            /* TODO: Snoop mode */
            break;
            
        case KEY_UP:
            if (selected_node > 0) {
                selected_node--;
                ensure_visible();
                need_refresh = 1;
            }
            break;
            
        case KEY_DOWN:
            if (selected_node < num_nodes - 1) {
                selected_node++;
                ensure_visible();
                need_refresh = 1;
            }
            break;
            
        case KEY_LEFT:
        case KEY_RIGHT:
        case '\t':  /* Tab key cycles system Info/Stats in compact mode */
            if (!layouts[current_layout].expand_system) {
                current_tab = (current_tab + 1) % TAB_COUNT;
                need_refresh = 1;
            }
            break;
    }
}

static void cleanup(void)
{
    DEBUG("Cleanup starting");
    
    /* Kill all nodes forcefully */
    for (int i = 0; i < num_nodes; i++) {
        if (nodes[i].bridge_pid > 0) {
            kill(nodes[i].bridge_pid, SIGKILL);
            nodes[i].bridge_pid = 0;
        }
        if (nodes[i].max_pid > 0) {
            kill(nodes[i].max_pid, SIGKILL);
            nodes[i].max_pid = 0;
        }
        if (nodes[i].pty_master >= 0) {
            close(nodes[i].pty_master);
            nodes[i].pty_master = -1;
        }
        unlink(nodes[i].socket_path);
    }
    
    /* Close listener */
    if (listen_fd >= 0) {
        close(listen_fd);
        listen_fd = -1;
    }
    
    /* Non-blocking wait for children */
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0);
    
    if (!headless_mode) {
        cleanup_display();
    }
    
    /* Close PRM handle */
    if (prm_handle) {
        PrmFileClose(prm_handle);
        prm_handle = NULL;
    }
    
    if (debug_log) {
        DEBUG("maxtel shutdown complete");
        fclose(debug_log);
        debug_log = NULL;
    }
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -p PORT    Telnet port (default: %d)\n", DEFAULT_PORT);
    fprintf(stderr, "  -n NODES   Number of nodes (default: %d)\n", DEFAULT_NODES);
    fprintf(stderr, "  -d PATH    Base directory (default: current)\n");
    fprintf(stderr, "  -m PATH    Max binary path (default: ./bin/max)\n");
    fprintf(stderr, "  -c PATH    Config path (default: etc/max)\n");
    fprintf(stderr, "  -s SIZE    Request terminal size (e.g., 80x25, 132x60)\n");
    fprintf(stderr, "  -H         Headless mode (no UI, for scripts/daemons)\n");
    fprintf(stderr, "  -D         Daemonize (implies -H, fork to background)\n");
    fprintf(stderr, "  -h         Show this help\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    int opt;
    fd_set rfds;
    struct timeval tv;
    int ch;
    
    /* Parse arguments */
    while ((opt = getopt(argc, argv, "p:n:d:m:c:s:HDh")) != -1) {
        switch (opt) {
            case 'p':
                listen_port = atoi(optarg);
                break;
            case 'n':
                num_nodes = atoi(optarg);
                if (num_nodes > MAX_NODES) num_nodes = MAX_NODES;
                if (num_nodes < 1) num_nodes = 1;
                break;
            case 'd':
                strncpy(base_path, optarg, sizeof(base_path) - 1);
                break;
            case 'm':
                strncpy(max_path, optarg, sizeof(max_path) - 1);
                break;
            case 'c':
                strncpy(config_path, optarg, sizeof(config_path) - 1);
                break;
            case 's':
                /* Parse size as COLSxROWS (e.g., 80x25, 132x60) */
                if (sscanf(optarg, "%dx%d", &requested_cols, &requested_rows) != 2) {
                    fprintf(stderr, "Invalid size format. Use COLSxROWS (e.g., 80x25)\n");
                    exit(1);
                }
                break;
            case 'H':
                headless_mode = 1;
                break;
            case 'D':
                daemonize = 1;
                headless_mode = 1;  /* Daemon implies headless */
                break;
            case 'h':
            default:
                usage(argv[0]);
        }
    }
    
    /* Initialize */
    memset(nodes, 0, sizeof(nodes));
    for (int i = 0; i < MAX_NODES; i++) {
        nodes[i].pty_master = -1;
    }
    
    /* Open debug log */
    debug_log = fopen("maxtel.log", "w");
    DEBUG("maxtel starting, base_path=%s, max_path=%s, config_path=%s", 
          base_path, max_path, config_path);
    
    /* Initialize runtime tracking */
    start_time = time(NULL);
    
    /* Load system information from PRM file */
    load_prm_info();
    load_user_count();
    
    setup_signals();
    
    /* Daemonize if requested */
    if (daemonize) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return 1;
        }
        if (pid > 0) {
            /* Parent exits */
            printf("maxtel daemon started (PID %d), port %d\n", pid, listen_port);
            return 0;
        }
        /* Child continues */
        setsid();  /* New session */
        /* Redirect stdio to /dev/null */
        freopen("/dev/null", "r", stdin);
        freopen("/dev/null", "w", stdout);
        /* Keep stderr for errors, or redirect to log */
    }
    
    /* Set up TCP listener */
    listen_fd = setup_listener(listen_port);
    if (listen_fd < 0) {
        fprintf(stderr, "Failed to bind to port %d\n", listen_port);
        return 1;
    }
    
    /* Initialize display (skip in headless mode) */
    if (!headless_mode) {
        init_display();
    } else {
        fprintf(stderr, "maxtel running in headless mode on port %d with %d nodes\n", 
                listen_port, num_nodes);
    }
    
    /* Spawn initial nodes */
    for (int i = 0; i < num_nodes; i++) {
        spawn_node(i);
        usleep(100000);  /* Stagger startup */
    }
    
    /* Main loop */
    while (running) {
        /* Handle terminal resize (UI mode only) */
        if (!headless_mode && need_resize) {
            handle_resize();
        }
        
        /* Check for incoming connections */
        FD_ZERO(&rfds);
        FD_SET(listen_fd, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = REFRESH_MS * 1000;
        
        if (select(listen_fd + 1, &rfds, NULL, NULL, &tv) > 0) {
            if (FD_ISSET(listen_fd, &rfds)) {
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                int client_fd = accept(listen_fd, 
                                       (struct sockaddr *)&client_addr, 
                                       &addr_len);
                if (client_fd >= 0) {
                    handle_connection(client_fd, &client_addr);
                }
            }
        }
        
        /* Handle keyboard input (UI mode only) */
        if (!headless_mode) {
            while ((ch = getch()) != ERR) {
                handle_input(ch);
            }
        }
        
        /* Update status */
        update_node_status();
        
        /* Restart any inactive or stale nodes */
        for (int i = 0; i < num_nodes; i++) {
            /* Inactive nodes get restarted */
            if (nodes[i].state == NODE_INACTIVE && nodes[i].max_pid == 0) {
                spawn_node(i);
            }
            /* Stopping nodes with no PID should become inactive */
            else if (nodes[i].state == NODE_STOPPING && nodes[i].max_pid == 0) {
                nodes[i].state = NODE_INACTIVE;
                need_refresh = 1;
            }
            /* Starting nodes that have been starting too long - check if process died */
            else if (nodes[i].state == NODE_STARTING && nodes[i].max_pid > 0) {
                if (kill(nodes[i].max_pid, 0) < 0 && errno == ESRCH) {
                    /* Process doesn't exist anymore */
                    nodes[i].max_pid = 0;
                    nodes[i].state = NODE_INACTIVE;
                    need_refresh = 1;
                }
            }
        }
        
        /* Refresh display (UI mode only) */
        if (!headless_mode && need_refresh) {
            update_display();
            need_refresh = 0;
        }
    }
    
    cleanup();
    if (!daemonize) {
        printf("maxtel shutdown complete.\n");
    }
    return 0;
}
