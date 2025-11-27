# 64-bit Compatibility Fixes for MEX VM

This document tracks all changes made to support 64-bit platforms (ARM64/x86_64).

## Summary

The MEX (Maximus Extension Language) Virtual Machine was originally designed for 32-bit systems. These fixes address pointer size, alignment, and offset calculation issues on 64-bit platforms.

## Files Modified

### mex/mex.h

**Change 1: VMADDR type definition**
```c
// OLD:
typedef dword VMADDR;  /* Virtual machine address */
#define VMADDR_FORMAT "%ld"

// NEW:
#if defined(__LP64__) || defined(_LP64) || defined(__x86_64__) || defined(__aarch64__)
typedef uint64_t VMADDR;
#define VMADDR_FORMAT "%llu"
#else
typedef dword VMADDR;  /* 32-bit systems */
#define VMADDR_FORMAT "%lu"
#endif
```
**Reason:** VMADDR needs to be pointer-sized on 64-bit systems.

**Change 2: Added TEMP_BASE constants**
```c
// NEW:
#define TEMP_BASE_BYTE   1000
#define TEMP_BASE_WORD   2000
#define TEMP_BASE_DWORD  4000
#define TEMP_BASE_ADDR   6000

#define TEMP_BASE_FOR_TYPE(td) \
  ((td)->form == FormString || (td)->form == FormAddr ? TEMP_BASE_ADDR : \
   (td)->form == FormDword ? TEMP_BASE_DWORD : \
   (td)->form == FormWord ? TEMP_BASE_WORD : TEMP_BASE_BYTE)
```
**Reason:** Original code used `type->size * 1000` which breaks when IADDR grows from 6 to 10 bytes.

---

### mex/vm_ifc.h

**Change: regs_6 array type**
```c
// OLD:
vm_extern char pascal regs_6[VM_LEN(MAX_REGS)][6];

// NEW:
vm_extern IADDR pascal regs_6[VM_LEN(MAX_REGS)];
```
**Reason:** IADDR is now 10 bytes on 64-bit (was 6 bytes). Using `char[6]` caused buffer overflow.

---

### mex/vm.h

**Change: Removed packed attribute from _usrfunc**
```c
// OLD:
vm_extern struct _usrfunc
{
  char *name;
  word (EXPENTRY *fn)(void);
  VMADDR quad;
} __attribute__((packed)) *usrfn;

// NEW (no packed):
vm_extern struct _usrfunc
{
  char *name;
  word (EXPENTRY *fn)(void);
  VMADDR quad;
} *usrfn;
```
**Reason:** Packed structs with pointers cause alignment issues on ARM64.

---

### mex/sem_vm.c

**Change: Fix pointer-to-int cast**
```c
// OLD (line ~640):
((int)o2 != 0 || o->form.addr.indirect)

// NEW:
(o2 != NULL || o->form.addr.indirect)
```
**Reason:** Casting 64-bit pointer to int truncates the value.

---

### mex/sem_func.c

**Change 1 (line ~293):**
```c
// OLD:
fret->form.addr.offset=fret->type->size*1000;

// NEW:
fret->form.addr.offset=TEMP_BASE_FOR_TYPE(fret->type);
```

**Change 2 (line ~677):**
```c
// OLD:
ret->form.addr.offset=f->func->c.f.ret_type->size*1000;

// NEW:
ret->form.addr.offset=TEMP_BASE_FOR_TYPE(f->func->c.f.ret_type);
```
**Reason:** Using `size*1000` breaks when IADDR size changes from 6 to 10.

---

### mex/sem_expr.c

**Changes (lines ~236, 253, 267):**
```c
// OLD:
new->addr.offset=1+(td->size*MAX_TEMP);
tl->addr.offset==reg+(td->size*MAX_TEMP);
new->addr.offset=reg+(td->size*MAX_TEMP);

// NEW:
new->addr.offset=1+TEMP_BASE_FOR_TYPE(td);
tl->addr.offset==reg+TEMP_BASE_FOR_TYPE(td);
new->addr.offset=reg+TEMP_BASE_FOR_TYPE(td);
```
**Reason:** Same as above - temp register base calculation.

---

## How to Rollback

If these changes cause issues, revert to 32-bit mode:

1. In `mex/mex.h`, change VMADDR back to:
   ```c
   typedef dword VMADDR;
   #define VMADDR_FORMAT "%ld"
   ```

2. In `mex/vm_ifc.h`, change regs_6 back to:
   ```c
   vm_extern char pascal regs_6[VM_LEN(MAX_REGS)][6];
   ```

3. In `mex/sem_func.c` and `mex/sem_expr.c`, revert `TEMP_BASE_FOR_TYPE()` calls back to `type->size*1000` or `td->size*MAX_TEMP`.

4. Clean rebuild:
   ```bash
   make clean
   make build
   make install
   ```

## Testing

After applying fixes:
1. `make clean && make build && make install`
2. Run MEX scripts (Statistics, User List, etc.)
3. Verify no crashes or "non-global segment" errors

---

### max/mex.c

**Change: Add NULL terminator to intrinsic function array**
```c
// OLD (last entry):
  {"xfertime",                intrin_xfertime,                0}
};

// NEW:
  {"xfertime",                intrin_xfertime,                0},
  {NULL,                      NULL,                           0}  /* Terminator */
};
```
**Reason:** VmRun loop uses `uf->name` as termination condition, needs NULL sentinel.

---

### mex/vm_run.c

**Change: Add NULL check in add_intrinsic_functions loop**
```c
// OLD:
for (uf=usrfn, i=(VMADDR)-2L; uf < usrfn+uscIntrinsic; uf++, i--)

// NEW:
for (uf=usrfn, i=(VMADDR)-2L; uf < usrfn+uscIntrinsic && uf->name; uf++, i--)
```
**Reason:** Prevents iterating over the NULL terminator entry.

---

### m/max.mh

**Change: Fix _ffind struct for 64-bit pointer**
```mex
// OLD:
struct _ffind
{
    long:           finddata;      // 4 bytes - WRONG on 64-bit!
    string:         filename;

// NEW:
struct _ffind
{
    long:           finddata_lo;   // Pointer split for 64-bit compatibility
    long:           finddata_hi;   // (C uses 8-byte pointer)
    string:         filename;
```
**Reason:** C code stores an 8-byte pointer in `finddata`. MEX must allocate matching space.

---

### max/mex_max.h

**Change: Replace `long` with `dword` in shared structs**

C's `long` is 8 bytes on 64-bit, but MEX's `long` is always 4 bytes. Changed to `dword` (4 bytes) for compatibility:

```c
// mex_usr struct:
dword   xp_mins;      // was: long
dword   up;           // was: long
dword   down;         // was: long
dword   downtoday;    // was: long

// mex_ffind struct:
dword   filesize;     // was: long
```
**Reason:** Ensures struct field offsets match between C and MEX on 64-bit systems.

---

### Makefile

**Change 1: Add .vm cleanup to make clean**
```makefile
// NEW (added to clean target):
-find $(PREFIX)/etc/m -name "*.vm" -delete 2>/dev/null || true
```
**Reason:** Old .vm files are incompatible after 64-bit changes; must be recompiled.

**Change 2: Add macOS codesigning to max_install**
```makefile
// NEW (added to max_install target):
ifeq ($(PLATFORM),darwin)
	@echo "Codesigning binaries for macOS..."
	@for f in $(BIN)/*; do codesign -f -s - "$$f" 2>/dev/null || true; done
	@for f in $(LIB)/*; do codesign -f -s - "$$f" 2>/dev/null || true; done
endif
```
**Reason:** macOS requires ad-hoc signing for locally built binaries.

---

## Known Limitations

- `.vm` files are architecture-specific (32-bit vs 64-bit incompatible)
- Array indexing limited to 16-bit offsets (existing limitation, not a regression)
- MEX structs with C pointers need manual size matching (e.g., `_ffind`)
