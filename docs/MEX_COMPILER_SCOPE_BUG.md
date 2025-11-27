# MEX Compiler Parameter Scoping Bug

## Summary

The MEX compiler incorrectly reports "redeclaration of argument" errors when two different functions use the same parameter name. This is a pre-existing bug unrelated to 64-bit porting.

## Affected File

`m/card.mex` - The 21 card game

## Error Message

```
m/card.mex(199) : error 2408: redeclaration of argument 'hnd'
m/card.mex(222) : error 2508: too many arguments in call to 'deal_card'
m/card.mex(230) : error 2508: too many arguments in call to 'deal_card'
```

## Root Cause Analysis

### Symbol Table Scope Management

The MEX compiler uses a global symbol table (`symtab`) with scope tracking. Each symbol has a `scope` field indicating which lexical scope it belongs to.

**Relevant files:**
- `mex/mex_symt.c` - Symbol table operations
- `mex/sem_scop.c` - Scope open/close functions
- `mex/sem_func.c` - Function argument handling

### Flow of Scope Management

1. **Function Start** (`function_begin` in `sem_func.c:41`):
   - Called at scope=0 to register function name
   
2. **Scope Open** (`scope_open` in `sem_scop.c:41`):
   - Increments global `scope` variable to 1
   
3. **Parameter Registration** (`function_args` in `sem_func.c:101`):
   - Calls `st_enter()` to add each parameter to symbol table
   - Parameters stored with `scope=1` and `class=ClassValidate`
   
4. **Scope Close** (`scope_close` in `sem_scop.c:54`):
   - Calls `st_killscope(symtab, scope)` to clean up
   - Decrements `scope` back to 0

### The Bug

In `st_killscope()` (`mex_symt.c:174`), when processing function arguments (ClassArg or ClassValidate), the code at lines 191-203:

```c
if (ap->class==ClassArg || ap->class==ClassValidate)
{
  /* If it's a function argument, we need to keep it in the symbol  *
   * table to store type information.  However, we don't want it to *
   * be findable by any of the other symtab routines, so we clear   *
   * its name entry (and set to NULL), which will force any         *
   * name comparisons to fail.                                      */

  if (ap->name)
    _st_kill_attribute_rec(ap);

  last=ap;
  continue;  // <-- DOES NOT REMOVE FROM CHAIN
}
```

**Problem:** The entry is nullified (name set to NULL) but **remains in the hash chain** with `scope=1`. 

When the next function is parsed:
1. `scope_open()` sets global scope to 1 again
2. `st_enter()` hashes the parameter name
3. The lookup loop at line 106 checks: `ap->scope==scope`
4. The old entry (with `scope=1`, `name=NULL`) matches the scope check
5. The name check `ap->name && eqstr(...)` correctly skips it
6. BUT: The next entry in the chain may have issues

### Hypothesis

The bug appears to occur when:
1. Multiple functions use the same parameter name
2. The parameter names hash to the same bucket
3. There's interaction between the nullified entries and scope checking

The commented-out code at line 185 suggests this was a known issue:
```c
if (ap->scope < scope /*|| ap->class==ClassArg*/)
```

The `ClassArg` check was commented out, possibly because it caused other problems.

## Workaround

Rename parameters to use unique names across functions:

```mex
// Before (fails):
int sum_cards(struct hand: hnd) { ... }
void deal_card(ref struct hand: hnd) { ... }

// After (works):
int sum_cards(struct hand: hnd) { ... }
void deal_card(ref struct hand: h) { ... }
```

## Fix Applied

Changed `m/card.mex` line 199:
- From: `void deal_card(ref struct hand: hnd)`
- To: `void deal_card(ref struct hand: h)`

Also updated all references within the function body.

## Proper Fix (Not Implemented)

A proper fix would require one of:

1. **Remove entries completely**: Change `st_killscope()` to remove ClassArg/ClassValidate entries from the chain entirely, rather than just nullifying names.

2. **Track scope more carefully**: Use separate scope numbers for each function's parameters instead of reusing scope=1.

3. **Use separate symbol tables**: Create a new scope-specific symbol table for each function's parameters.

## Impact

- **Severity**: Low (workaround available)
- **Scope**: Only affects MEX scripts with duplicate parameter names
- **Relation to 64-bit port**: None - this is a pre-existing bug

## Test Case

The bug is triggered by the exact code structure in `install_tree/m/card.mex`. 

Minimal reproduction:
```bash
# Take first 198 lines of card.mex (through sum_cards function)
sed -n '1,198p' install_tree/m/card.mex > test.mex

# Add a simple function using same parameter name
echo "void deal_card(ref struct hand: hnd) { }" >> test.mex

# Compile - triggers error
mex test.mex
# Output: error 2408: redeclaration of argument 'hnd'
```

**Note:** Simpler test cases don't reproduce the bug. The issue appears related to:
- The specific combination of includes (max.mh, rand.mh, input.mh, swap.mh)
- The struct definitions with arrays
- The number/complexity of intermediate functions
- Possibly hash collisions in the symbol table

## References

- `mex/mex_symt.c:174` - `st_killscope()` function
- `mex/mex_symt.c:88` - `st_enter()` function  
- `mex/sem_func.c:101` - `function_args()` function
- `mex/sem_scop.c:54` - `scope_close()` function
