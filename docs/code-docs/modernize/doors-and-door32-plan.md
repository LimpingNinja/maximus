# Doors and Door32 Plan (Linux/macOS)

This document captures current findings about how Maximus runs external programs (“doors”) when invoked from menus, how Maxtel connects users to Maximus, and a two-phase plan for adding modern Linux/macOS door support.

## Scope and assumptions

- Target platforms: Linux and macOS only.
- Current runtime supervisor: `maxtel`.
- “Doors” refers to external programs invoked from Maximus menus.

## Current implementation: menu → external runner

### Menu dispatch

- `max/max_menu.c`
  - `ProcessMenuResponse()` locates a matching menu option and calls:
    - `RunOption(pam, popt, upper_ch, msg, &flag, menuname)`

### Option router

- `max/max_runo.c`
  - `RunOption()` routes to the correct execution block.
  - External program menu items end up here:
    - `Exec_Xtern(type, thisopt, arg, &result, menuname)`

### External execution entry point

- `max/max_xtrn.c`
  - `Exec_Xtern()` maps menu option types:
    - `xtern_run` → `OUTSIDE_RUN`
    - `xtern_chain` → `OUTSIDE_CHAIN`
    - `xtern_concur` → `OUTSIDE_CONCUR`
    - `xtern_dos` → `OUTSIDE_DOS`
    - `xtern_erlvl` → `OUTSIDE_ERRORLEVEL` (not applicable for Linux/macOS)
  - Then calls:
    - `Outside(PRM(out_leaving), PRM(out_return), method, Strip_Underscore(arg), TRUE, ctl, RESTART_MENU, menuname)`

### Where the current UNIX behavior is insufficient

- `max/max_xtrn.c` (UNIX path)
  - Uses `xxspawnvp(P_WAIT, args[0], args)` for `OUTSIDE_RUN/CHAIN/CONCUR`.
- `max/xspawn.c`
  - Current `xxspawnvp()` is not a real interactive spawn:
    - It builds a command string and uses `popen(tmp, "r")`.
    - It reads only stdout and prints via `Puts()`.
    - It does not provide a true interactive stdin stream to the child process.
  - This is incompatible with typical interactive “stdio doors” that expect a real terminal or a bidirectional stream.

## How Maxtel connects users to Maximus (and what FD Maximus sees)

### Maxtel process model

- `src/apps/maxtel/maxtel.c`
  - Spawns Maximus nodes ahead of time via `forkpty()`:
    - Node process runs `./bin/max -w -pt<N> -n<N> -b38400 etc/max.prm`
    - The PTY is for the node console / local supervision, not the remote user’s telnet socket.

### User connection path

- `src/apps/maxtel/maxtel.c`
  - Accepts telnet TCP socket: `client_fd`.
  - Forks a bridge process (`bridge_connection()`), which:
    - Connects to a per-node AF_UNIX socket at `./maxipc<N>`
    - Bridges bytes between:
      - `client_fd` (TCP) ↔ `sock_fd` (AF_UNIX)

### Maximus-side TCP/IP comm driver

- `comdll/ipcomm.c`
  - `IpComOpen()` creates and listens on the AF_UNIX socket `./maxipc<N>`.
  - `IpComIsOnline()` accepts the connection and stores the accepted fd into the comm handle:
    - `CommHandle_setFileHandle(hc->h, fd)`

### Extracting the active session’s UNIX fd inside Maximus

Inside Maximus runtime, the active communications handle is `hcModem`.

- `max/asyncnt.c`
  - `HCOMM hcModem = NULL;`

The raw UNIX fd used by the comm driver can be obtained via:

- `ComGetHandle(hcModem)` → `COMMHANDLE`
  - see `slib/ntcomm.h`
- `FileHandle_fromCommHandle(COMMHANDLE)` → `int fd`
  - implemented in `comdll/wincomm.c`

**Important:** For Maxtel telnet sessions, this fd is the AF_UNIX socket connected to Maxtel’s bridge process, not the original TCP socket.

## Lost connection handling today

- `max/max_fini.c`
  - `Lost_Carrier()` is Maximus’s “caller dropped” handler.

- `comdll/ipcomm.c`
  - `IpComIsOnline()` detects disconnect and toggles `hc->fDCD`.

## Door32.sys specification (reference)

A commonly-cited Door32.sys Revision 1 spec describes `door32.sys` as a text file with:

- Line 1: comm type (`0=local`, `1=serial`, `2=telnet`)
- Line 2: comm or socket handle
- Line 3: baud
- Line 4: bbsid
- Line 5: user record position (1-based)
- Line 6: real name
- Line 7: handle/alias
- Line 8: security level
- Line 9: time left (minutes)
- Line 10: emulation
- Line 11: node number

Reference used during research:
- https://raw.githubusercontent.com/NuSkooler/ansi-bbs/master/docs/dropfile_formats/door32_sys.txt

## Phase plan

### Phase 1: interactive stdio doors

Goal: make “normal” UNIX stdio doors work (interactive, bidirectional), runnable from menus.

#### Requirements

- Execute command lines via the system shell:
  - `/bin/sh -c <command>`
- Attach door process I/O to the active session fd:
  - `dup2(session_fd, 0)`
  - `dup2(session_fd, 1)`
  - `dup2(session_fd, 2)` (optional but recommended)
- Ensure clean teardown:
  - If the user disconnects, terminate the door.
  - If the door exits, return to Maximus menus cleanly.

#### Proposed implementation approach

- Replace the UNIX `xxspawnvp()` implementation used for externals with a real fork/exec runner (no `popen`).
  - Candidate file: `max/xspawn.c`
  - Likely new helper: `xxspawn_sh_with_session_io(...)`.

- Session fd to use:
  - `int session_fd = FileHandle_fromCommHandle(ComGetHandle(hcModem));`

- Optional tty/raw handling:
  - If `isatty(session_fd)`:
    - Save termios.
    - Put into “raw-ish” mode for doors that expect it.
    - Restore on return.

- Watchdog:
  - Parent process periodically checks `ComIsOnline(hcModem)`.
  - On loss, kill child (SIGTERM → SIGKILL) and trigger `Lost_Carrier()` or equivalent cleanup path.

#### Validation

- Use a known stdio test door:
  - OpenDoors test door (as specified for this project).

### Phase 2: Door32.sys generation + handle passing

Goal: provide Door32.sys dropfile generation for doors that require it.

#### Handle semantics (confirmed working)

Modern Door32 libraries (OpenDoors and derivatives) are **protocol-neutral** and work with any valid file descriptor:

- **AF_UNIX sockets work perfectly:** Door libraries use standard `read()`/`write()` (or `recv()`/`send()`) calls without validating socket family
- **No telnet protocol concerns:** Libraries treat the descriptor as a raw stream; telnet negotiation (if any) is handled by the BBS upstream
- **isatty() behavior:** 
  - Returns `false` for both AF_INET and AF_UNIX sockets
  - Door libraries proceed with remote I/O logic (not local console mode)
  - This is the expected behavior for network sessions

**For Maximus:**
- Network sessions: pass the AF_UNIX socket fd (to Maxtel bridge) as the handle
- Local sessions: pass the PTY fd (if `isatty()` returns true)
- Both scenarios work identically from the door's perspective

**Comparison with other BBSes:**
- Mystic/Synchronet: pass the actual TCP socket fd
- Maximus: pass the AF_UNIX socket fd (to Maxtel bridge)
- **Result:** Same behavior - door libraries don't care about socket family, only that I/O works

#### Door32.sys contents

Standard Door32.sys Revision 1 format:

- Line 1: comm type
  - `0` = local (PTY/console)
  - `2` = socket (network session via AF_UNIX socket to Maxtel)
- Line 2: comm or socket handle (raw fd number)
- Line 3: baud rate (38400 or configured rate)
- Line 4: BBS ID (Maximus version string)
- Line 5: user record position (1-based)
- Line 6: real name
- Line 7: handle/alias
- Line 8: security level
- Line 9: time left (minutes)
- Line 10: emulation (ANSI, etc.)
- Line 11: node number

#### Implementation approach

**Mecca integration (completed):**
- Enhanced existing `%P` control character to return socket handle on UNIX:
  - Windows/NT: returns `ComGetHandle(hcModem)` (HANDLE)
  - OS/2: returns `ComGetFH(hcModem)` (file handle)
  - UNIX: returns `FileHandle_fromCommHandle(ComGetHandle(hcModem))` (file descriptor)
  - This provides consistent behavior across platforms for Door32.sys
- Added `%#` control character for node-specific temp directory path:
  - Returns `<temppath>node<task_num_hex>` (e.g., `/tmp/node01` on UNIX, `C:\MAX\TEMP\node01` on Windows)
  - Uses `PRM(temppath)` from system configuration on all platforms
  - Used for isolating door files per node
- Created `door32.mec` template script in `build/etc/misc/door32.mec`
- Template handles both local (comm_type=0) and network (comm_type=2) sessions
- Template creates node-specific temp directory and writes door32.sys there
- Sysops can use standard Mecca `[write_file door32.sys door32.mec]` in menu scripts

**Menu integration:**
- Sysops can create menu items that:
  1. Write door32.sys using the Mecca template: `[write_file door32.sys door32.mec]`
  2. Execute the door program with node temp directory: `xtern_run /path/to/door %#`
  3. Door receives node temp path as argument (e.g., `/tmp/NODE1`)
  4. Door can find door32.sys at `<node_temp_path>/door32.sys`
  5. Optional cleanup on return: `[delete %#/door32.sys]` (or door cleans up itself)
- No new menu option type needed - uses existing Maximus menu scripting

**Example menu configuration:**
```
[write_file door32.sys door32.mec]
xtern_run /path/to/door %#
[delete %#/door32.sys]
```

The door program receives the node temp path (e.g., `/tmp/node01` or `C:\MAX\TEMP\node01`) as its first argument and can locate door32.sys there.

**Files modified:**
- `max/max_ocmd.c`: Enhanced `%P` to return file descriptor on UNIX; added `%#` for node temp directory

**Files created:**
- `build/etc/misc/door32.mec`: Door32.sys dropfile template

## Next code changes (high level)

1. Replace UNIX external execution to support true interactive stdio.
   - Primary files: `max/max_xtrn.c`, `max/xspawn.c`.
2. Add a watchdog tied to `ComIsOnline(hcModem)`.
3. Add an explicit Door32.sys generator and a distinct menu mode for Door32 doors.

## Implemented (January 2026): automatic dropfile generation + Door32_Run

This section documents the current, implemented behavior in this repo. The original plan sections above remain for historical context.

### Automatic dropfile generation in C (no Mecca dependency)

Dropfiles are now generated directly by Maximus in C on **every external program execution**.

- **Implementation:**
  - `max/dropfile.c`, `max/dropfile.h`
  - Called from `max/max_xtrn.c` (`Outside()`), before executing the external program.

#### Node-specific temp directory

All dropfiles are written into a node-specific directory derived from `PRM(temppath)` and the node number in hex:

- **Pattern:** `<temppath>node%02x`
- **Example (node 1):** `/tmp/node01`

This directory is created automatically if missing.

#### Dropfiles written

The following are written automatically into the node temp directory:

- **`Dorinfo1.Def`**
- **`Door.Sys`**
- **`Chain.Txt`**
- **`door32.sys`**

These are generated by:

- `Write_Dorinfo1()`
- `Write_DoorSys()`
- `Write_ChainTxt()`
- `Write_Door32Sys()`

and orchestrated by:

- `Write_All_Dropfiles()`

#### Cleanup on connect/disconnect

The node temp directory is cleared (files removed) at:

- **User connect/logon:** `max/callinfo.c` (`ci_login()`)
- **User disconnect/logoff:** `max/max_fini.c` (`FinishUp2()`)

Cleanup deletes files in the node directory but does not remove the directory itself.

### Menu argument and control-character notes

#### Underscores for spaces

Maximus menu args are space-hostile; use underscores in `menus.ctl` and Maximus will convert underscores back to spaces at runtime.

#### `%k` and `%#`

- **`%k`** expands to the node number.
- **`%#`** expands to the node temp directory path (`<temppath>node%02x`).

### Door32_Run vs Xtern_Run

Door32 doors still run through the same external execution entry points, but we now have a dedicated menu option so sysops can clearly distinguish Door32-style doors.

#### New menu option

- **Menu statement:** `Door32_Run`
- **Option type:** `xtern_door32`

This is now recognized by the menu compiler and runtime:

- `src/max/option.h`: adds `xtern_door32` and maps `door32_run`/`xtern_door32` strings.
- `src/utils/util/s_menu.c`: updated so SILT treats `xtern_door32` like other externals (expects an argument field).
- `src/max/max_runo.c`: updated to allow this option type.
- `src/max/max_xtrn.c`: `Exec_Xtern()` maps `xtern_door32` → `OUTSIDE_DOOR32`.
- `src/max/max.h`: defines `OUTSIDE_DOOR32`.

#### Runtime behavior

On UNIX builds, `OUTSIDE_DOOR32` is treated the same as `OUTSIDE_RUN` in `Outside()`. The important behavior is:

- Dropfiles (including `door32.sys`) are guaranteed to exist in the node temp directory before the door starts.
- The `door32.sys` “handle” line contains the active comm file descriptor (the AF_UNIX socket to Maxtel’s bridge) for network sessions.

#### When to use which

- **Use `Xtern_Run`**
  - For “stdio style” doors that only need a dropfile like `Door.Sys`/`Dorinfo1.Def` and do not use Door32.

- **Use `Door32_Run`**
  - For Door32-aware doors/libraries expecting a valid descriptor in `door32.sys`.
  - It documents intent at the menu level and ensures your door environment matches the Door32 expectations.

#### Example menu line

Example (passing node + node temp directory):

```
NoDsp Door32_Run etc/doors/testdoor.sh_%k_%# Normal "R"
```

Door script argument convention used in this repo:

- `$1` = node number
- `$2` = node temp directory path

The door should read dropfiles from `$2/`.

## Case Study: Shadow Dungeon Door Implementation (January 2025)

This section documents the implementation of a working interactive Python door (Shadow Dungeon) as a proof-of-concept for Phase 1 stdio door support. It captures the Maximus-side challenges encountered and solutions applied.

### Initial problem: `args` array initialization bug in `Outside()`

**File:** `max/max_xtrn.c`

**Symptom:** Double-free crash when running external doors on macOS.

**Root cause:** The `args` array in `Outside()` was declared but not initialized to NULL:
```c
char *args[MAX_EXTERNAL_ARGS];
```

When the cleanup loop attempted to free uninitialized pointers, it caused a malloc crash.

**Fix:** Initialize the array to NULL at declaration:
```c
char *args[MAX_EXTERNAL_ARGS] = {NULL};
```

**Impact:** This was a critical bug preventing any external door execution on UNIX platforms. The fix enabled basic door launching.

### Challenge: PTY-based door runner architecture

**Context:** The initial approach used a PTY wrapper to run doors, spawning the door process under a pseudo-terminal to provide better terminal emulation.

**File created:** `build/doors/shadowdungeon/run_door_pty.py`

**Architecture:**
- Maximus spawns `run_door_pty.py` with the session fd passed as an argument
- PTY runner creates a pseudo-terminal pair (master/slave)
- Door process runs on the PTY slave
- Bridge loop forwards bytes between:
  - Session fd (AF_UNIX socket to Maxtel) ↔ PTY master

**Key implementation details:**
- Used `pty.openpty()` for PTY pair creation
- Used `select.select()` for non-blocking bidirectional I/O
- Handled SIGCHLD to detect door process exit
- Cleaned up PTY and restored terminal state on exit

**Advantage:** Provides a real terminal environment for doors that expect PTY behavior (window size, terminal control sequences, etc.).

**Trade-off:** Adds complexity vs. direct fd redirection, but necessary for doors that check `isatty()` or need terminal capabilities.

### Challenge: Mouse tracking escape sequences causing flicker

**Symptom:** Massive screen flicker on the input line when moving the mouse on the client terminal emulator.

**Root cause:** The PTY terminal was intercepting mouse movement events and sending CSI mouse tracking sequences (e.g., `\x1b[<0;10;5M`) to the door's stdin.

**Solution (door-side):**
1. Explicitly disable mouse tracking when entering raw mode:
   ```python
   def _disable_mouse_reporting() -> None:
       sys.stdout.write("\x1b[?1000l\x1b[?1002l\x1b[?1003l\x1b[?1006l")
       sys.stdout.flush()
   ```
2. Filter mouse sequences in the input line editor (`safe_input()`)
3. Ignore mouse sequences in the non-blocking input decoder (`getch_nonblocking()`)

**Lesson for door developers:** Always disable mouse tracking explicitly if your door uses raw mode. The PTY may have mouse tracking enabled by default from previous sessions.

### Challenge: Raw mode and backspace handling

**Symptom:** Backspace key sent raw `^H` (0x08) characters to the input, appearing as literal text instead of deleting characters.

**Root cause:** The door was using Python's `input()` which relies on cooked mode line editing. In raw mode, backspace must be handled manually.

**Solution (door-side):**
Implemented a raw-mode line editor in `safe_input()`:
```python
def safe_input(prompt: str = "") -> str:
    # ... enter raw mode ...
    while True:
        b = os.read(fd, 1)
        if b == b'\x7f' or b == b'\x08':  # DEL or BS
            if buffer:
                buffer.pop()
                sys.stdout.write('\b \b')  # erase character
                sys.stdout.flush()
        elif b == b'\r' or b == b'\n':
            break
        # ... handle other input ...
```

**Lesson for door developers:** If using raw mode with `termios`, you must implement your own line editing for any input prompts. Standard library functions like `input()` will not work correctly.

### Challenge: Arrow key input decoding

**Symptom:** Arrow keys stopped working in the dungeon after switching to raw mode input handling.

**Root cause:** Arrow keys send multi-byte CSI sequences (e.g., `\x1b[A` for up arrow). The initial implementation used text-mode `sys.stdin.read()` which could not reliably decode these sequences when mixed with `select.select()`.

**Solution (door-side):**
1. Switched to byte-level `os.read()` for all input
2. Implemented persistent buffering for partial CSI sequences:
   ```python
   _input_buffer = bytearray()
   
   def getch_nonblocking() -> Optional[str]:
       # Read available bytes
       chunk = os.read(sys.stdin.fileno(), 1024)
       _input_buffer.extend(chunk)
       
       # Try to decode complete sequences
       if _input_buffer.startswith(b'\x1b['):
           # Parse full CSI sequence...
   ```
3. Full CSI parsing for arrow keys, function keys, and modifiers

**Lesson for door developers:** For reliable non-blocking input in raw mode:
- Use `os.read()` at the byte level, not text-mode reads
- Buffer partial sequences across reads
- Implement a state machine for CSI sequence parsing

### Challenge: Terminal state management

**Context:** Doors need to toggle between cooked mode (for BBS menus) and raw mode (for game input) without leaving the terminal in a broken state.

**Solution (door-side):**
1. Save original terminal attributes at startup:
   ```python
   class _TerminalState:
       def __init__(self, fd: int):
           self.fd = fd
           self.original = termios.tcgetattr(fd)
   ```
2. Register cleanup with `atexit`:
   ```python
   atexit.register(_TERM_STATE.restore)
   ```
3. Wrap raw mode toggling in a state tracker:
   ```python
   def set_raw_mode(enabled: bool):
       if enabled:
           tty.setraw(fd)
           _disable_mouse_reporting()
       else:
           termios.tcsetattr(fd, termios.TCSADRAIN, original)
   ```

**Lesson for door developers:** Always save and restore terminal state. Use `atexit` handlers to ensure cleanup even on exceptions or signals.

### Door development best practices (from this implementation)

1. **Terminal mode handling:**
   - Save original `termios` attributes at startup
   - Use `atexit` to guarantee restoration
   - Disable mouse tracking explicitly when entering raw mode
   - Test with both local PTY and network (AF_UNIX socket) sessions

2. **Input handling in raw mode:**
   - Use byte-level `os.read()` for reliable non-blocking I/O
   - Buffer partial escape sequences across reads
   - Implement CSI parsing for arrow keys and function keys
   - Handle both `\x7f` (DEL) and `\x08` (BS) for backspace

3. **Output handling:**
   - Use `\r\n` (CRLF) for line endings, not just `\n`
   - Flush output explicitly after writes
   - Consider CP437 encoding for box-drawing characters (BBS compatibility)
   - Provide runtime glyph mode toggles (Unicode/CP437/ASCII) for flexibility

4. **Rendering considerations:**
   - Use ANSI escape sequences for colors and cursor control
   - Provide a color toggle for monochrome terminals
   - Test with 80x25 terminal dimensions (standard BBS size)
   - Avoid assuming UTF-8 support (many BBS terminals use CP437)

5. **Session management:**
   - Check for broken pipe errors on write (user disconnect)
   - Handle SIGPIPE gracefully
   - Save game state frequently (users may disconnect unexpectedly)

### Files modified (Maximus side)

- `max/max_xtrn.c`: Fixed `args` array initialization bug in `Outside()`

### Files created (door side)

- `build/doors/shadowdungeon/shadowdungeon.py`: Main door implementation
- `build/doors/shadowdungeon/run_door_pty.py`: PTY wrapper for door execution

### Testing environment

- Platform: macOS (arm64)
- BBS: Maximus 3.03 (NotoriousPTY fork)
- Supervisor: Maxtel (telnet server with AF_UNIX socket bridge)
- Terminal: MuffinTerm (custom BBS terminal emulator)
- Door language: Python 3

### Validation results

✅ Door launches successfully from Maximus menus  
✅ Bidirectional I/O works (keyboard input, ANSI output)  
✅ Arrow keys, backspace, and special keys function correctly  
✅ Raw mode toggling works without terminal corruption  
✅ Mouse tracking disabled (no flicker)  
✅ Color ANSI rendering works  
✅ CP437 box-drawing characters render correctly  
✅ Runtime glyph mode switching (Unicode/CP437/ASCII)  
✅ Save/load game state persists across sessions  
✅ Clean exit returns to BBS menus

### Remaining Phase 1 work

The Shadow Dungeon implementation demonstrates that interactive stdio doors work with the current Maximus external execution path after fixing the `args` initialization bug. However, the following Phase 1 items remain:

1. **Watchdog for disconnect handling:**
   - Currently, if a user disconnects, the door process continues running
   - Need to monitor `ComIsOnline(hcModem)` and kill orphaned door processes
   - Should trigger `Lost_Carrier()` cleanup path

2. **Termios handling in the door runner:**
   - The PTY wrapper currently doesn't save/restore terminal state
   - Should handle raw mode setup/teardown on the Maximus side for doors that need it

3. **Standardized door runner:**
   - The PTY wrapper is door-specific
   - Should create a generic door runner that can be reused for any stdio door
   - Consider making it part of Maximus itself (C implementation in `max/xspawn.c`)

