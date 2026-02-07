# Linux Port Changes

This document details changes made to support Linux builds (November 2025) and their compatibility with macOS.

## Summary

All changes are designed to be cross-platform compatible. The codebase now builds on both macOS (Darwin) and Linux with the same source files.

---

## Changes by File

### `configuration-tests/endian.c`
**Change:** Added `#include <stdlib.h>` and fixed `%i` → `%zu` format specifiers for `sizeof()`.

**Reason:** Modern GCC requires explicit declaration of `exit()`. The `%zu` format is the correct specifier for `size_t`.

**macOS Impact:** ✅ Safe - standard C, improves correctness.

---

### `src/utils/squish/s_squash.c` (line 353)
**Change:** Changed `open()` to `sopen()`.

**Reason:** The original code called `open()` with 4 arguments (including `SH_DENYNO` share mode), but POSIX `open()` only accepts 2 or 3 arguments. Linux glibc's fortify source checking caught this error.

**macOS Impact:** ✅ Safe - `sopen()` is defined in `unix/include/io.h` for all Unix platforms.

---

### `src/utils/squish/sqfeat.h` (line 68)
**Change:** Removed variable definition from struct declaration (`} _feat_init;` → `};`).

**Reason:** The original code defined both a struct type AND a variable in the header. With GCC 10+ default `-fno-common`, this causes multiple definition errors when the header is included in multiple .c files.

**macOS Impact:** ✅ Safe - fixes a latent bug that macOS tolerated.

---

### `unix/dossem.c` (lines 28-32)
**Change:** Removed RedHat 5.2 pthread workaround that mapped `pthread_mutexattr_settype` to `pthread_mutexattr_setkind_np`.

**Reason:** The `_np` (non-portable) functions were deprecated and removed from modern glibc. Modern Linux has standard `pthread_mutexattr_settype` directly.

**macOS Impact:** ✅ Safe - macOS has standard POSIX pthreads.

---

### `slib/align.c` (after line 11)
**Change:** Added `#include <string.h>` and conditionally `#include <malloc.h>` for Linux.

**Reason:** 
- `string.h` needed for `memcpy()` 
- `malloc.h` provides `memalign()` on Linux (macOS defines its own via `NEED_MEMALIGN`)

**macOS Impact:** ✅ Safe - `malloc.h` include is wrapped in `#if defined(LINUX)`.

---

### `slib/compiler_unix.h` (lines 73-84)
**Change:** Replaced `_init`/`_fini` macros with `__attribute__((constructor))`/`__attribute__((destructor))`.

**Reason:** `_init` and `_fini` are reserved symbols in glibc and conflict with the C runtime initialization. The constructor/destructor attributes are the modern, portable way to run code at shared library load/unload.

**macOS Impact:** ✅ Safe - Clang fully supports `__attribute__((constructor))`.

**Old code:**
```c
# define __dll_initialize  _init
# define __dll_terminate   _fini
```

**New code:**
```c
# define __dll_initialize DLL_CONSTRUCTOR static void _maximus_dll_init_func
# define __dll_terminate  DLL_DESTRUCTOR static void _maximus_dll_fini_func
```

---

### `btree/dllc.c` (line 25)
**Change:** Updated function to match new macro signature (removed `int` return type).

**Reason:** Constructor functions don't return values - the attribute handles initialization.

**macOS Impact:** ✅ Safe - matches the new macro definition.

---

### `slib/ntcomm.h` (line 89)
**Change:** Made `CommApi` declaration `extern`.

**Reason:** The original code defined a global variable in a header file. With `-fno-common`, this causes multiple definition errors.

**macOS Impact:** ✅ Safe - proper C practice, should have always been extern.

---

### `comdll/common.c` (line 14)
**Change:** Added actual definition of `struct CommApi_ CommApi`.

**Reason:** With the header now declaring `extern`, the actual storage must be defined in exactly one .c file.

**macOS Impact:** ✅ Safe - just moves the definition from header to source file.

---

### `Makefile` (lines 32, 34)
**Change:** Reordered library directories: `unix` now comes before `slib`.

**Reason:** `slib/libmax.so` depends on `unix/libcompat.so`. The dependency order must be respected.

**macOS Impact:** ✅ Safe - correct order works on all platforms. macOS previously tolerated wrong order due to `-undefined dynamic_lookup` linker flag.

---

## Build System Files

### `vars_linux.mk`
**Changes:**
- Added `-lncurses` to `OS_LIBS`
- Added rpath setting: `-Wl,-rpath,'$$ORIGIN/../lib'`

### `scripts/build-linux.sh`
**New file:** Convenience script for Linux builds that:
- Checks dependencies
- Runs `./configure --prefix=$PWD/build`
- Cleans cross-platform binaries
- Builds and installs

---

## Testing

After making these changes, verify both platforms:

```bash
# On macOS
make clean && make build && make install

# On Linux
./scripts/build-linux.sh
# or manually:
./configure --prefix=$PWD/build
make clean && make build && make install
```

---

### `prot/zmodem.h` (line 117)
**Change:** Fixed `rclhdr` function declaration from `long rclhdr();` to `long rclhdr(char *hdr);`

**Reason:** K&R style declaration `rclhdr()` (unspecified arguments) conflicted with the proper declaration in `zsjd.h`. GCC is strict about conflicting prototypes.

**macOS Impact:** ✅ Safe - Clang was lenient about this, but the fix is correct.

---

### `prot/rz.c` (line 309)
**Change:** Removed local `time_t time();` declaration.

**Reason:** Redundant declaration conflicted with `time.h`. The function is already declared via included headers.

**macOS Impact:** ✅ Safe - removes unnecessary code.

---

### `comdll/wincomm.c` (after includes)
**Change:** Added stub functions for `ModemRaiseDTR()` and `ModemLowerDTR()`.

**Reason:** These functions were called but never defined anywhere in the codebase. They control modem DTR signals which aren't needed for telnet operation.

**macOS Impact:** ✅ Safe - `-Wno-implicit-function-declaration` flag was suppressing this error on macOS.

---

### `comdll/modemcom.c` (after includes)
**Change:** Added `extern void cdecl logit(char *format, ...);` declaration.

**Reason:** Function is defined in `max/log.c` but wasn't declared in a header accessible to comdll.

**macOS Impact:** ✅ Safe - `-Wno-implicit-function-declaration` flag was suppressing this error on macOS.

---

### `mex/mex_tab.y` (line 516)
**Change:** Changed `{ ElseHandler(&$$); }` to `{ ElseHandler(&$<elsetype>$); }`

**Reason:** Newer bison versions require explicit type annotations for `$$` in midrule actions. The type `elsetype` was already used in the subsequent action.

**macOS Impact:** ✅ Safe - macOS may use older bison or pre-generated files.

---

## Why These Worked on macOS

1. **`-Wno-implicit-function-declaration`** in `vars_darwin.mk` suppresses errors about missing function declarations
2. **Clang vs GCC** - Clang is more lenient about K&R style declarations and some prototype conflicts
3. **Bison versions** - macOS may have older bison or use pre-generated parser files

The fixes above are the **correct** approach - fixing the actual issues rather than suppressing warnings.

---

### `mex/mex.h` (line 29)
**Change:** Added `#include <stdint.h>` after typedefs.h include.

**Reason:** `uint64_t` type used for `VMADDR` on 64-bit systems requires stdint.h. GCC on Linux doesn't implicitly include it.

**macOS Impact:** ✅ Safe - stdint.h is standard C99.

---

### `mex/mex.h` (line 235)
**Change:** Renamed struct field from `word bool;` to `word boolval;`

**Reason:** C23 made `bool` a reserved keyword. GCC 14+ treats it as such, causing "two or more data types" error.

**macOS Impact:** ✅ Safe - older Clang doesn't enforce C23 keywords, but the rename is valid C.

---

### `mex/mex_tab.y` (lines 326, 331, 333)
**Change:** Updated all references from `.bool` to `.boolval` to match renamed field.

**Reason:** Companion change to mex.h field rename.

**macOS Impact:** ✅ Safe - bison will regenerate mex_tab.c with correct field name.

---

### `comdll/comstruct.h` (line 28)
**Change:** Changed `} _hcomm;` to `};  /* struct _hcomm */`

**Reason:** Original code defined both a struct AND a variable named `_hcomm` in the header. With `-fno-common`, including this header in multiple .c files causes multiple definition errors.

**macOS Impact:** ✅ Safe - struct was already typedef'd elsewhere; the variable was unused.

---

### `slib/Makefile` (line 72)
**Change:** Added `-Wl,--allow-shlib-undefined` to Linux libmax.so link command.

**Reason:** Equivalent to macOS `-undefined dynamic_lookup`. Allows libmax.so to have unresolved references (like `SquishHash` from libmsgapi) that will be resolved at runtime.

**macOS Impact:** ✅ N/A - change is Linux-specific (`else` branch of platform check).

---

### `vars_linux.mk` (line 10)
**Change:** Added `-lmsgapi` at the end of `OS_LIBS`.

**Reason:** Resolves circular dependency between libmax.so and libmsgapi.so. `libmax.so` calls `SquishHash()` which is defined in `libmsgapi.so`. By listing `-lmsgapi` both at the start (via EXTRA_LOADLIBES) and end (via OS_LIBS) of the link line, the Linux linker can resolve symbols in both directions.

**macOS Impact:** ✅ N/A - change is in Linux-specific vars file.

---

### `mex/vm_heap.c` (line 37-39)
**Change:** Added forward declaration for `hpcheck()` function.

**Reason:** `hpcheck()` is called at line 61 but defined at line 198. GCC requires functions to be declared before use. Added `#ifdef HEAP_PROBLEMS` guarded forward declaration.

**macOS Impact:** ✅ Safe - forward declarations are valid C and don't change behavior.

---

### `unix/include/io.h` (lines 71-75)
**Change:** Added `P_WAIT` define and `spawnlp()` declaration.

**Reason:** `spawnlp()` is a DOS/OS2/Windows function for spawning processes. Linux doesn't have it natively. Added declaration to the Unix compatibility header.

**macOS Impact:** ✅ Safe - declaration is in shared Unix header, implementation works on both.

---

### `unix/dosproc.c` (lines 105-149)
**Change:** Added `spawnlp()` implementation using `fork()`/`execvp()`/`waitpid()`.

**Reason:** Provides POSIX-compatible implementation of the DOS `spawnlp()` function. Supports `P_WAIT` mode (wait for child) and returns child exit status.

**macOS Impact:** ✅ Safe - uses standard POSIX APIs available on both Linux and macOS.

---

### `src/utils/sqafix/unixmisc.c` (lines 32-50)
**Change:** Removed incorrect `spawnlp()` implementation.

**Reason:** The old implementation had wrong signature `int spawnlp(char* cmd, ...)` - missing the `mode` parameter. The correct implementation is now in `unix/dosproc.c` with proper DOS API signature `int spawnlp(int mode, const char *cmdname, const char *arg0, ...)`.

**macOS Impact:** ✅ Safe - old buggy implementation was silently mismatched; new one is correct.

---

### `src/utils/util/l_attach.c` (line 110)
**Change:** Removed incorrect cast `(union _stampu*)` from `TmDate_to_DosDate()` call.

**Reason:** `scDateAttached` is type `SCOMBO` (which is `union stamp_combo`). The function expects `union stamp_combo *`, not `union _stampu *`. The cast was incorrect and unnecessary.

**macOS Impact:** ✅ Safe - fixes a type mismatch that Clang was lenient about.

---

### `max/ued_disp.c` (line 203)
**Change:** Added `(char *)` cast to `user.pwd` in conditional expression.

**Reason:** Conditional expression `brackets_encrypted : user.pwd` had type mismatch - `brackets_encrypted` returns `char *` but `user.pwd` is `byte *` (unsigned char *). GCC treats this as an error.

**macOS Impact:** ✅ Safe - explicit cast makes types match; no behavior change.

---

### `max/f_down.c` (line 334)
**Change:** Added `(char *)` cast to `fpath` in conditional expression.

**Reason:** Same pattern - `fpath` is `byte *` but `FAS(fah, downpath)` returns `char *`.

**macOS Impact:** ✅ Safe - explicit cast.

---

### `max/m_area.c` (line 290)
**Change:** Added `(char *)` cast to `usr.msg` in conditional expression.

**Reason:** `usr.msg ? usr.msg : "(null)"` - `usr.msg` is `byte *`, string literal is `char *`.

**macOS Impact:** ✅ Safe - explicit cast.

---

### `max/m_xport.c` (lines 102, 105)
**Change:** Added `(char *)` cast to `Address()` return values.

**Reason:** `Address()` returns `byte *` but `blank_str` is `char *` in conditional expressions.

**macOS Impact:** ✅ Safe - explicit casts.

---

### `maxtel/Makefile` (line 22)
**Change:** Added `-lmsgapi` to `MAXTEL_LIBS` for Linux.

**Reason:** `libmax.so` references `SquishHash` from `libmsgapi.so`. Linux linker requires explicit library dependency.

**macOS Impact:** ✅ N/A - Linux-specific branch.

---

### `Makefile` (lines 182, 184)
**Change:** Added `maxtel` to `build` target and `maxtel_install` to `install` target.

**Reason:** maxtel was not being built as part of the standard build process.

**macOS Impact:** ✅ Safe - adds maxtel to build on all platforms.

---

### `unix/include/compat.h` (lines 10-12)
**Change:** Added `adaptcase()` and `adaptcase_refresh_dir()` declarations.

**Reason:** Functions defined in `unix/adcase.c` but never declared in a header. GCC requires declarations.

**macOS Impact:** ✅ Safe - declarations don't change behavior.

---

### `max/protod.h` (lines 520-523)
**Change:** Added declarations for `mdm_nowonline()`, `xxspawnvp()`, `beep()`.

**Reason:** Functions defined in various max source files but called without declarations. GCC requires declarations.

**macOS Impact:** ✅ Safe - Clang was lenient about implicit declarations.

---

### `max/modem.h` (line 119)
**Change:** Added `ModemComIsOnlineNow()` declaration.

**Reason:** Function defined in `comdll/modemcom.c` but called from `max/fos_os2.c` without declaration.

**macOS Impact:** ✅ Safe - adds proper declaration.

---

### `comdll/comprots.h` (line 44)
**Change:** Added `ModemComIsOnlineNow()` declaration.

**Reason:** Same function, needed in comm headers too.

**macOS Impact:** ✅ Safe - adds proper declaration.

---

### `max/max_chng.c` (line 425)
**Change:** Added `(char *)` cast to `usr.name` in conditional expression.

**Reason:** `*temp ? temp : usr.name` - `temp` is `char *`, `usr.name` is `byte *`.

**macOS Impact:** ✅ Safe - explicit cast.

---

### `max/joho.c` (line 29)
**Change:** Added `#include <ctype.h>`.

**Reason:** File uses `tolower()` which requires ctype.h. GCC requires explicit include.

**macOS Impact:** ✅ Safe - standard C header.

---

### `max/xspawn.c` (line 16)
**Change:** Added `extern void Puts(char *s);` forward declaration.

**Reason:** `Puts()` is defined in `max_out.c` but `xspawn.c` doesn't include the full max headers.

**macOS Impact:** ✅ Safe - adds proper declaration.
