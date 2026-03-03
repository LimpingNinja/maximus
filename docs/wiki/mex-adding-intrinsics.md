---
layout: default
title: "Adding Intrinsics"
section: "mex"
description: "How to add new MEX functions backed by C code"
permalink: /mex-adding-intrinsics/
---

Every MEX function that scripts can call — `print`, `sock_open`,
`json_get_str`, all of them — is an **intrinsic**: a C function with a
specific signature, registered in a table, and callable from the MEX VM.
This page walks through adding your own.

The examples here are drawn from the socket and JSON intrinsic families,
which were built using exactly these patterns.

---

## The Intrinsic Signature

Every intrinsic has the same C signature:

```c
word EXPENTRY intrin_my_function(void);
```

No arguments in the C prototype. The MEX VM passes arguments through its
own stack — you pull them out with `MexArgGet*` functions inside the body.
Return values go into global registers (`regs_2`, `regs_4`) or via
`MexReturnString`.

This uniform signature is what makes the intrinsic table work — every
entry is a function pointer with the same type.

---

## Argument Handling

### MexArgBegin / MexArgEnd

Every intrinsic body starts with `MexArgBegin` and ends with `MexArgEnd`.
These set up and tear down the argument-reading context:

```c
word EXPENTRY intrin_my_function(void)
{
    MA ma;

    MexArgBegin(&ma);

    /* ... pull args, do work, set return value ... */

    return MexArgEnd(&ma);
}
```

**Always return `MexArgEnd(&ma)`** — it cleans up the VM stack. Forgetting
this will corrupt the stack and crash the session.

### Pulling Arguments

Arguments are pulled in the **same order** as the MEX declaration:

| C function | MEX type | Notes |
|-----------|----------|-------|
| `MexArgGetWord(&ma)` | `int` | Returns `word` (unsigned 16-bit) |
| `MexArgGetDword(&ma)` | `long` | Returns `dword` (unsigned 32-bit) |
| `MexArgGetString(&ma, FALSE)` | `string` | Returns `malloc`'d copy — **you must `free()` it** |
| `MexArgGetRef(&ma)` | `ref` param | Returns an `IADDR` for writing back |

**String ownership:** `MexArgGetString` allocates a copy. If you don't use
it (early exit on validation failure), free it. If you pass it to a library
that takes ownership, don't double-free.

**Example — two args (string + int):**

```c
word EXPENTRY intrin_sock_open(void)
{
    MA ma;
    char *host;
    int port, timeout_ms;

    MexArgBegin(&ma);
    host       = MexArgGetString(&ma, FALSE);   /* arg 1: string */
    port       = (int)MexArgGetWord(&ma);       /* arg 2: int    */
    timeout_ms = (int)MexArgGetWord(&ma);       /* arg 3: int    */

    /* Validate */
    if (!host || !*host || port <= 0 || port > 65535)
    {
        if (host) free(host);
        regs_2[0] = (word)-1;
        return MexArgEnd(&ma);
    }

    /* ... do work ... */

    free(host);
    return MexArgEnd(&ma);
}
```

---

## Return Values

### Integer Returns: regs_2 and regs_4

The VM reads return values from global register arrays:

| Register | Size | MEX type | Usage |
|----------|------|----------|-------|
| `regs_2[0]` | `word` | `int` | Primary return value |
| `regs_4[0]` | `dword` | `long` | Primary return value (for `long` returns) |

Set the return value **before** calling `MexArgEnd`:

```c
/* Return an integer */
regs_2[0] = (word)slot;
return MexArgEnd(&ma);
```

```c
/* Return -1 on error */
regs_2[0] = (word)-1;
return MexArgEnd(&ma);
```

**Convention:** Set the error return value at the top of your function body,
then overwrite it on success. This way, any early `return MexArgEnd(&ma)`
automatically returns the error code:

```c
word EXPENTRY intrin_json_enter(void)
{
    MA ma;
    int jh;

    MexArgBegin(&ma);
    jh = (int)MexArgGetWord(&ma);

    regs_2[0] = (word)-1;              /* default: error */

    if (jh < 0 || jh >= MAX_MEXJSON || !g_mex_json[jh].root)
        return MexArgEnd(&ma);         /* returns -1 */

    /* ... success path ... */

    regs_2[0] = 0;                     /* success */
    return MexArgEnd(&ma);
}
```

### String Returns: MexReturnString

For intrinsics that return a `string` to MEX:

```c
MexReturnString("hello world");
return MexArgEnd(&ma);
```

`MexReturnString` copies the string into the VM's string heap. Pass a
regular C string — the VM handles the rest.

For empty/error returns:

```c
MexReturnString("");
return MexArgEnd(&ma);
```

**Example — auto-converting JSON values to strings:**

```c
word EXPENTRY intrin_json_str(void)
{
    MA ma;
    int jh;

    MexArgBegin(&ma);
    jh = (int)MexArgGetWord(&ma);

    if (jh < 0 || jh >= MAX_MEXJSON || !g_mex_json[jh].cursor)
    {
        MexReturnString("");
        return MexArgEnd(&ma);
    }

    cJSON *node = g_mex_json[jh].cursor;

    if (cJSON_IsString(node) && node->valuestring)
        MexReturnString(node->valuestring);
    else if (cJSON_IsNumber(node))
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%g", node->valuedouble);
        MexReturnString(buf);
    }
    else if (cJSON_IsBool(node))
        MexReturnString(cJSON_IsTrue(node) ? "true" : "false");
    else if (cJSON_IsNull(node))
        MexReturnString("null");
    else
        MexReturnString("");

    return MexArgEnd(&ma);
}
```

### Writing to Ref Parameters

For `ref` (by-reference) parameters — like `sock_recv` writing received
data back to the caller's buffer:

```c
IADDR ref_addr = MexArgGetRef(&ma);

/* ... later, after receiving data ... */

MexStoreString(ref_addr, received_data);
```

`MexStoreString` writes a C string into the MEX variable identified by
`ref_addr`.

---

## Handle Table Pattern

Most resource intrinsics (files, sockets, JSON) use a **fixed-size handle
table** — a static array of structs, where the index is the handle returned
to MEX.

### Socket Handle Table

```c
#define MAX_MEXSOCK 8

typedef struct _mex_sock {
    int   fd;           /* OS socket fd, -1 = unused slot          */
    int   connected;    /* 1 = connected, 0 = closed, -1 = error   */
    char  host[256];    /* Remote host (for logging)               */
    int   port;         /* Remote port                             */
} MEX_SOCK;

static MEX_SOCK g_mex_socks[MAX_MEXSOCK];
```

### JSON Handle Table

```c
#define MAX_MEXJSON  16
#define MAX_JSON_DEPTH 16

typedef struct _mex_json {
    cJSON *root;                        /* Parsed tree (NULL = slot free) */
    cJSON *cursor;                      /* Current navigation position   */
    cJSON *stack[MAX_JSON_DEPTH];       /* Parent stack for enter/exit   */
    int    depth;                       /* Current stack depth           */
} MEX_JSON;

static MEX_JSON g_mex_json[MAX_MEXJSON];
```

### Allocation Pattern

Scan for a free slot, return `-1` if the pool is full:

```c
int slot = -1;
for (int i = 0; i < MAX_MEXJSON; i++)
{
    if (!g_mex_json[i].root)
    {
        slot = i;
        break;
    }
}

if (slot == -1)
{
    logit("!MEX json_open: no free slots");
    regs_2[0] = (word)-1;
    return MexArgEnd(&ma);
}
```

### Deallocation Pattern

Clear the slot on close:

```c
word EXPENTRY intrin_json_close(void)
{
    MA ma;
    int jh;

    MexArgBegin(&ma);
    jh = (int)MexArgGetWord(&ma);

    if (jh >= 0 && jh < MAX_MEXJSON && g_mex_json[jh].root)
    {
        cJSON_Delete(g_mex_json[jh].root);
        memset(&g_mex_json[jh], 0, sizeof(MEX_JSON));
    }

    return MexArgEnd(&ma);
}
```

---

## Registration

Three places need changes when you add a new intrinsic:

### 1. Prototype in mexint.h

```c
/* JSON lifecycle */
word EXPENTRY intrin_json_open(void);
word EXPENTRY intrin_json_create(void);
word EXPENTRY intrin_json_create_array(void);
word EXPENTRY intrin_json_close(void);
```

### 2. Table Entry in mex.c

Add an entry to the `intrin_tab[]` array. Each entry maps a MEX function
name to the C function pointer:

```c
{"json_open",         intrin_json_open,         0},
{"json_create",       intrin_json_create,       0},
{"json_create_array", intrin_json_create_array, 0},
{"json_close",        intrin_json_close,        0},
```

The third field is reserved flags — use `0` for standard intrinsics.

**Order doesn't matter** for correctness, but group related intrinsics
together for readability.

### 3. MEX Header (e.g., json.mh)

Declare the function with MEX syntax so scripts can call it:

```mex
int json_open(string: text);
int json_create();
int json_create_array();
void json_close(int: jh);
```

The header lives in `scripts/include/` and is found automatically by the
MEX compiler when scripts use `#include <json.mh>`.

---

## Cleanup in intrin_term()

The `intrin_term()` function runs when a MEX session ends (script exits or
crashes). **You must clean up your handle table here** to prevent resource
leaks:

```c
void intrin_term(void)
{
    /* Close any open sockets */
    for (int i = 0; i < MAX_MEXSOCK; i++)
    {
        if (g_mex_socks[i].fd != -1)
        {
            close(g_mex_socks[i].fd);
            g_mex_socks[i].fd = -1;
            g_mex_socks[i].connected = 0;
        }
    }

    /* Free any open JSON handles */
    for (int i = 0; i < MAX_MEXJSON; i++)
    {
        if (g_mex_json[i].root)
        {
            cJSON_Delete(g_mex_json[i].root);
            memset(&g_mex_json[i], 0, sizeof(MEX_JSON));
        }
    }
}
```

This is non-negotiable. If a script crashes mid-execution, `intrin_term()`
is the only chance to release file descriptors, memory, and other OS
resources.

---

## Checklist for a New Intrinsic

1. **Design the MEX-side API** — decide on function name, argument types,
   return type, and error conventions
2. **Write the C implementation** — `word EXPENTRY intrin_foo(void)` with
   `MexArgBegin`/`MexArgEnd` and proper argument handling
3. **Add the prototype** to `mexint.h`
4. **Add the table entry** to `intrin_tab[]` in `mex.c`
5. **Write the MEX header declaration** in the appropriate `.mh` file
6. **Add cleanup** to `intrin_term()` if the intrinsic manages resources
7. **Test** — write a `.mex` script that exercises normal and error paths

---

## Tips

- **Validate everything.** MEX scripts can pass garbage. Check handle
  bounds, NULL strings, and out-of-range values before doing real work.

- **Default to the error return.** Set `regs_2[0] = (word)-1` at the top
  and only overwrite on success. This makes early-return paths safe.

- **Free strings you don't use.** `MexArgGetString` allocates. If you
  bail out early, free the string or you leak memory.

- **Log failures.** Use `logit()` for unexpected conditions. It goes to
  the BBS log and helps sysops diagnose script issues.

- **Keep handle pools small.** 8–16 slots is plenty. Scripts that exhaust
  handles have a bug — don't encourage it.

- **Don't block.** Use `select()`/`poll()` with timeouts for any I/O.
  A blocked node is a dead node.

---

## See Also

- [Extending with MEX/C]({% link extending-mex-c.md %}) — the language
  string API (a simpler starting point)
- [VM Architecture]({% link mex-vm-architecture.md %}) — how the MEX VM
  executes bytecode and manages the stack
- [Standard Intrinsics]({% link mex-standard-intrinsics.md %}) — the
  built-in intrinsic families
