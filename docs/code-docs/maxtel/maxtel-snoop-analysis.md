# Maxtel Snoop/Chat/Takeover Feature Analysis

## User Request

> When using MAXTEL, can the SysOp "snoop" on user sessions, answer pages or break into chat, take control of the keyboard and navigate menus to help a user, and do all of the same with doors? When users were on my board, I could see a status bar and an sysop overlay that were invisible to the user in both Maximus and door games. Is there a way to still have this functionality?

## Current Architecture

### Maxtel Connection Flow

```
Telnet Client ──► Listener (TCP) ──► fork() bridge process
                                         │
                                         ├──► Unix Socket ──► Max process (PTY slave)
                                         │
                                         └──► PTY Master (for WFC display)
```

### Key Files

| File | Purpose |
|------|---------|
| `src/apps/maxtel/maxtel.c` | Multi-node supervisor, ncurses status display |
| `src/max/max_chat.c` | IPC system (`IPCxx.BBS`), chat/page handling |
| `src/max/max_cmod.c` | Sysop command mode (Alt-C chat, local keyboard) |
| `src/max/log.c` | MAXSNOOP pipe system (OS/2 legacy) |
| `src/max/max_locl.c` | Local keyboard parsing (F-keys, Alt-keys) |
| `src/max/max_main.c` | Yell/page handling |

### Current Data Flow

```c
// src/apps/maxtel/maxtel.c - bridge_connection()
while (1) {
    /* Data from telnet client -> max socket */
    if (FD_ISSET(client_fd, &rfds)) {
        n = read(client_fd, buf, sizeof(buf));
        write(sock_fd, buf, n);  // Direct passthrough
    }
    
    /* Data from max socket -> telnet client */
    if (FD_ISSET(sock_fd, &rfds)) {
        n = read(sock_fd, buf, sizeof(buf));
        write(client_fd, buf, n);  // Direct passthrough
    }
}
```

### Existing Infrastructure

1. **PTY Master** (`nodes[i].pty_master`) - Direct access to max output
2. **Unix Socket** (`nodes[i].socket_path`) - Bidirectional comm with max
3. **IPC Files** (`IPCxx.BBS`) - Inter-node messaging, chat status
4. **Status Files** (`bbstatXX.bbs`, `lastusXX.bbs`) - Node state, user info
5. **`snoop` global variable** - Controls local display echo in max
6. **`keyboard` global variable** - Controls local keyboard injection

### _cstat Structure (IPC Header)

```c
struct _cstat {
    word avail;           // Chat availability flag
    byte username[36];    // Current username
    byte status[80];      // Activity status string
    word msgs_waiting;    // IPC messages waiting
    dword next_msgofs;    // Next message offset
    dword new_msgofs;     // New message offset
};
```

## Feature Implementation Analysis

### 1. Snoop (View Session)

**Feasibility: HIGH** ✓

The bridge process already has access to both data streams. Implementation:

```c
// In bridge_connection(), add snoop_fd for supervisor
static int snoop_pipe[2] = {-1, -1};  // Pipe to supervisor

/* Data from max -> client (and snoop) */
if (FD_ISSET(sock_fd, &rfds)) {
    n = read(sock_fd, buf, sizeof(buf));
    write(client_fd, buf, n);
    
    // Copy to snoop window if enabled
    if (snoop_pipe[1] >= 0 && snoop_enabled) {
        write(snoop_pipe[1], buf, n);
    }
}
```

Supervisor reads snoop data and displays in dedicated ncurses window.

**Alternative**: Use PTY master directly when in WFC mode:
```c
// Already exists in drain_pty() - just need to display instead of discard
while ((n = read(node->pty_master, buf, sizeof(buf))) > 0) {
    display_in_snoop_window(buf, n);  // Instead of discarding
}
```

### 2. Answer Pages / Break Into Chat

**Feasibility: HIGH** ✓

Maximus already has page detection via `chatreq` flag and yell events:

```c
// src/max/max_main.c
chatreq = TRUE;
ci_paged();  // Callinfo update - "User is paging"
```

**Implementation in maxtel**:
1. Read IPC files to detect page status
2. Display "PAGING" indicator in node status
3. Send chat initiation signal to max

```c
// Read IPCxx.BBS header
struct _cstat cstat;
read(ipc_fd, &cstat, sizeof(cstat));
if (cstat.status contains "Paging" || chatreq indicator) {
    nodes[i].paging = 1;
    // Flash/highlight in display
}
```

For chat initiation, can inject the equivalent of Alt-C:
```c
// Inject keyboard command to initiate chat
write(sock_fd, "\x03", 1);  // Ctrl-C or specific escape sequence
```

### 3. Keyboard Takeover

**Feasibility: MEDIUM-HIGH** ✓

The bridge already passes data bidirectionally. Add sysop injection:

```c
// Extended bridge with sysop keyboard injection
static int sysop_input_pipe[2] = {-1, -1};

while (1) {
    FD_SET(client_fd, &rfds);
    FD_SET(sock_fd, &rfds);
    FD_SET(sysop_input_pipe[0], &rfds);  // Add sysop input
    
    /* Sysop keyboard -> max (priority) */
    if (FD_ISSET(sysop_input_pipe[0], &rfds)) {
        n = read(sysop_input_pipe[0], buf, sizeof(buf));
        write(sock_fd, buf, n);  // Inject to max
    }
    
    /* Client keyboard -> max (can be disabled during takeover) */
    if (FD_ISSET(client_fd, &rfds) && !takeover_mode) {
        // ... normal passthrough
    }
}
```

### 4. Door Support

**Feasibility: MEDIUM** ✓

Doors run as child processes of max, inheriting the PTY. Snoop works automatically since we're capturing PTY output. Keyboard injection works the same way.

**Consideration**: Some doors may use direct video writes (DOS legacy), but modern Unix doors use stdout which goes through the PTY.

### 5. Status Bar / Sysop Overlay

**Feasibility: HIGH for maxtel, MEDIUM for in-session**

**Maxtel side**: Already has ncurses status display. Can add snoop window.

**In-session overlay** (like DOS Maximus): More complex because:
- Would need to inject ANSI sequences that only go to local display
- Maximus's `snoop` flag controls this, but we don't have separate local display

**Alternative approach**: Split-screen in maxtel supervisor:
```
┌─────────────────────────────────────────┐
│ Node 1: John Smith - Reading Messages   │
│ Node 2: Jane Doe - File Area            │
├─────────────────────────────────────────┤
│ [SNOOP: Node 2]                         │
│ ┌─────────────────────────────────────┐ │
│ │ User's session output appears here  │ │
│ │ ...                                 │ │
│ └─────────────────────────────────────┘ │
│ [Sysop Input: _______________]          │
└─────────────────────────────────────────┘
```

## Proposed Implementation

### Phase 1: Snoop Display

1. Add snoop window to ncurses layout
2. Create pipe from bridge to supervisor
3. Display captured output in real-time
4. Toggle with 'S' key (already stubbed)

### Phase 2: Page Detection

1. Parse IPC files for chat status
2. Add visual indicator (flashing, color change)
3. Sound/notification option

### Phase 3: Chat/Keyboard Injection

1. Add sysop input pipe to bridge
2. Create input mode in supervisor (Enter to start typing)
3. Inject keystrokes to selected node
4. Add takeover toggle (block user input)

### Phase 4: Chat Mode

1. Implement proper Alt-C equivalent signal
2. Split-screen chat display
3. Chat escape sequences

## Code Modifications Required

### src/apps/maxtel/maxtel.c

```c
// Add to node_info_t
typedef struct {
    // ... existing fields ...
    int snoop_pipe[2];     // Pipe for snoop output
    int sysop_pipe[2];     // Pipe for sysop input
    int snoop_enabled;     // Snoop active flag
    int takeover_mode;     // Sysop has keyboard control
    int paging;            // User is paging
} node_info_t;

// Add snoop window
static WINDOW *snoop_win = NULL;
static int snoop_node = -1;  // Which node we're snooping

// Modify bridge_connection() to support snoop and injection
// Modify handle_input() to handle snoop toggle and input mode
// Add snoop display update function
```

### New IPC Monitor

```c
// Check IPC files for page status
static void check_page_status(void) {
    for (int i = 0; i < num_nodes; i++) {
        char ipc_path[256];
        snprintf(ipc_path, sizeof(ipc_path), "%s/ipc%02d.bbs", 
                 base_path, i + 1);
        
        int fd = open(ipc_path, O_RDONLY);
        if (fd >= 0) {
            struct _cstat cs;
            if (read(fd, &cs, sizeof(cs)) == sizeof(cs)) {
                // Check for page indicator
                if (strstr(cs.status, "Page") || chatreq_flag) {
                    nodes[i].paging = 1;
                }
            }
            close(fd);
        }
    }
}
```

## Complexity Assessment

| Feature | Effort | Lines | Risk |
|---------|--------|-------|------|
| Snoop display | Medium | ~150 | Low |
| Page detection | Low | ~50 | Low |
| Keyboard injection | Medium | ~100 | Low |
| Takeover mode | Medium | ~80 | Low |
| Chat integration | Medium | ~150 | Medium |
| **Total** | | **~530** | |

## Implementation Path

### Phase 1: Simple Snoop (MVP)

**Goal**: Basic snoop functionality with zero user impact

**Controls**:
- Arrow Up/Down: Select node in supervisor
- `S`: Enter snoop mode on selected node
- ESC: Exit snoop, return to supervisor

**Implementation**:
1. Add `enter_snoop_mode(int node_num)` function
2. Hook into existing `handle_input()` for 'S' key
3. Read from `nodes[selected_node].pty_master`
4. Write to stdout (sysop sees session)
5. Read from stdin, write to PTY (keyboard injection)
6. ESC detection exits loop, returns to ncurses

**No status bar in Phase 1** - just raw session view.

### Phase 2: Status Bar

- Add scroll region (`\033[1;23r`) if `FLAG_statusline` set
- Draw status on line 24 with user info, time, CHAT indicator
- Update every second during snoop

### Phase 3: Page Detection

- Parse IPC files for chat request flag
- Show indicator in supervisor node list (flashing/color)
- Optional: audio notification

### Phase 4: Chat Integration

- Inject Alt-C equivalent to break into chat
- Consider split-screen chat mode

## Simplified Approach: Direct PTY Passthrough ("Easy Snoop")

Since each max process runs in a PTY via `forkpty()` and we keep `nodes[i].pty_master`, we can implement snoop much more simply:

### Current State
- `drain_pty()` currently **discards** PTY output to prevent blocking
- Each node's PTY master fd is always available

### Simple Implementation

```c
// Press 'S' to enter snoop mode
void enter_snoop_mode(int node_num) {
    node_info_t *node = &nodes[node_num];
    
    // Exit ncurses temporarily
    endwin();
    
    // Reset terminal to raw mode
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    
    // Show status header
    printf("\033[H\033[7m[SNOOP: Node %d - %s - Press ESC to exit]\033[0m\n",
           node_num + 1, node->username);
    
    int snoop_active = 1;
    while (snoop_active) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(node->pty_master, &rfds);
        
        struct timeval tv = {0, 100000};  // 100ms
        if (select(node->pty_master + 1, &rfds, NULL, NULL, &tv) > 0) {
            char buf[4096];
            
            // PTY output -> display (snoop)
            if (FD_ISSET(node->pty_master, &rfds)) {
                int n = read(node->pty_master, buf, sizeof(buf));
                if (n > 0) write(STDOUT_FILENO, buf, n);
            }
            
            // Sysop keyboard -> PTY (injection)
            if (FD_ISSET(STDIN_FILENO, &rfds)) {
                int n = read(STDIN_FILENO, buf, sizeof(buf));
                if (n > 0) {
                    if (buf[0] == 27) {  // ESC
                        snoop_active = 0;
                    } else {
                        write(node->pty_master, buf, n);
                    }
                }
            }
        }
    }
    
    // Restore ncurses
    refresh();
    need_refresh = 1;
}
```

### Advantages
- **~50 lines of code** vs complex pipe architecture
- Full-screen session view - exactly what user sees
- Keyboard injection works automatically
- Works with doors (same PTY)
- No bridge modification needed

### Built-in Sysop Status Line

Maximus has `Draw_StatusLine()` in `max_misc.c` that displays:
```
[Username      ] [Alias         ] [Phone        ] [Priv/Keys] [Time] C F K
                                                                     ^ ^ ^
                                                    Chat request ────┘ │ │
                                                    Flow control ──────┘ │
                                                    Keyboard mode ───────┘
```

**Limitation**: Uses `VIDEO_IBM` mode with direct video writes (`WinOpen()`), not stdout. Won't appear through PTY.

### Respecting FLAG_statusline

```c
// From src/max/prm.h
#define FLAG_statusline  0x0400 /* If SysOp wants a status line on screen */
```

If sysop has this enabled, we should show status bar during snoop. Check via:
```c
if (prm.flags & FLAG_statusline) {
    // Show status bar in snoop mode
}
```

### Cleanest Solution: Scroll Region + ANSI Status (Zero User Impact)

**Critical requirement**: Snooping must NEVER affect the user's session. No PTY resize, no SIGWINCH, nothing that touches max or doors.

Solution: Use ANSI scroll region on the **sysop's terminal only**:

```c
void enter_snoop_mode(int node_num) {
    node_info_t *node = &nodes[node_num];
    int show_status = (prm.flags & FLAG_statusline);
    
    // Exit ncurses
    endwin();
    
    // Raw terminal mode for sysop's terminal
    struct termios raw, saved;
    tcgetattr(STDIN_FILENO, &saved);
    raw = saved;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    
    if (show_status) {
        // Set scroll region to lines 1-23 (sysop terminal only)
        // This does NOT affect the PTY or user's session
        printf("\033[1;23r");         // Scroll region: lines 1-23
        printf("\033[24;1H\033[7m");  // Position line 24, reverse video
        printf("%-79s", "");          // Clear status line
        printf("\033[H");             // Home cursor to top
    }
    
    int snoop_active = 1;
    time_t last_status = 0;
    
    while (snoop_active) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(node->pty_master, &rfds);
        
        struct timeval tv = {0, 100000};  // 100ms
        if (select(node->pty_master + 1, &rfds, NULL, NULL, &tv) > 0) {
            char buf[4096];
            
            // PTY output -> sysop display (snoop)
            // User's session is completely unaffected
            if (FD_ISSET(node->pty_master, &rfds)) {
                int n = read(node->pty_master, buf, sizeof(buf));
                if (n > 0) write(STDOUT_FILENO, buf, n);
            }
            
            // Sysop keyboard -> PTY (injection to session)
            if (FD_ISSET(STDIN_FILENO, &rfds)) {
                int n = read(STDIN_FILENO, buf, sizeof(buf));
                if (n > 0) {
                    if (buf[0] == 27) {  // ESC to exit
                        snoop_active = 0;
                    } else {
                        write(node->pty_master, buf, n);
                    }
                }
            }
        }
        
        // Update status bar every second if enabled
        if (show_status && time(NULL) != last_status) {
            last_status = time(NULL);
            update_snoop_status(node_num);
        }
    }
    
    if (show_status) {
        // Reset scroll region to full screen
        printf("\033[r");  // Reset scroll region to default
    }
    
    // Restore terminal and ncurses
    tcsetattr(STDIN_FILENO, TCSANOW, &saved);
    refresh();
    need_refresh = 1;
}

void update_snoop_status(int node_num) {
    // Save cursor, jump to line 24, draw status, restore cursor
    printf("\033[s\033[24;1H\033[7m");
    printf("[Node %d: %-16s | %s | %4ld:%02ld | %s%s]",
           node_num + 1,
           nodes[node_num].username,
           current_user_valid ? privstr(current_user.priv) : "---",
           time_remaining / 60, time_remaining % 60,
           chatreq ? "CHAT " : "",
           keyboard ? "KBD" : "");
    printf("\033[K\033[0m\033[u");  // Clear EOL, normal, restore cursor
    fflush(stdout);
}
```

### Why This Works

1. **Zero user impact** - No PTY resize, no SIGWINCH, nothing touches max or doors
2. **Scroll region is local** - Only affects sysop's terminal display
3. **Session output constrained** - Scrolls within lines 1-23 on sysop's screen
4. **Line 24 protected** - Status bar stays fixed at bottom
5. **Respects FLAG_statusline** - Only show if sysop wants it
6. **Works with doors** - Door output passes through unchanged

### Important Notes

- The `\033[1;23r` sequence sets scroll region on the **sysop's terminal**
- PTY output is read and written to sysop's stdout - user never knows
- If src/max/door sends its own scroll region commands, they'll affect sysop's display (acceptable)
- User's actual session continues at full 24/25 rows

## Summary

**All requested features are feasible** with the current architecture:

| Feature | Feasibility | Notes |
|---------|-------------|-------|
| Snoop on sessions | ✓ HIGH | PTY/socket access exists |
| Answer pages | ✓ HIGH | IPC files have status |
| Break into chat | ✓ HIGH | Inject Alt-C equivalent |
| Keyboard takeover | ✓ HIGH | Bridge modification |
| Door support | ✓ MEDIUM | Inherits PTY, works naturally |
| Status overlay | ✓ MEDIUM | Via maxtel ncurses, not in-session |

The key insight is that maxtel's bridge architecture already has the data paths needed - we just need to tap into them for the supervisor display and add reverse paths for sysop input injection.
