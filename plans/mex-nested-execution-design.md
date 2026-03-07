# Plan: Nested MEX Execution Design

**Date:** 2026-03-07
**Status:** **Proposed**
**Scope:** Engine and runtime design planning for calling one MEX program from another without updating tutorial/wiki behavior yet

---

## Summary

This plan separates nested MEX execution into two distinct feature models:

1. **Spawned MEX execution**
   - A parent MEX launches a child MEX as a separate execution instance.
   - The child runs to completion.
   - Control returns cleanly to the parent MEX.
   - This is the model needed for chaining, lightbar menu actions, quick utility scripts, and "run out / come back" behavior.

2. **In-VM MEX execution**
   - A parent MEX invokes another MEX within the current interpreter/VM context.
   - The child can observe and potentially modify parent-visible state.
   - This is the model needed for a "master script" architecture, shared globals, reusable script modules, and LP-style script composition within a single session.

These two models should be treated as different runtime features, not as aliases of the same mechanism.

---

## Problem Statement

The current runtime does not provide the functionality needed for either of the above use cases in a clean and explicit way.

### Current findings

- `menu_cmd(MNU_MEX, ...)` is explicitly unsupported.
- `shell()` routes through `Outside()` and the UNIX external-process path.
- On Linux, the `shell(0, ":...")` path does not appear to dispatch into the MEX runtime as a dedicated MEX call mechanism.
- Direct recursive MEX startup is blocked by the current non-reentrancy guard.
- The present behavior is therefore not suitable for:
  - reliable spawned child-MEX execution from MEX code
  - shared-state in-interpreter child execution

### Design consequence

The project should not treat nested MEX execution as a single bug fix.
It should treat it as a runtime design decision with two explicit execution models.

---

## Design Goals

- Provide a clean and documented way for one MEX to invoke another.
- Preserve node stability on Linux.
- Avoid overloading `shell()` with undocumented or platform-specific MEX behavior.
- Define semantics clearly enough that tutorials and wiki documents can be updated later without ambiguity.
- Keep the two execution models conceptually separate:
  - **spawn out / spawn in**
  - **run inside the current VM**

---

## Non-Goals

- Do not change the wiki yet.
- Do not retrofit `menu_cmd(MNU_MEX, ...)` into a supported mechanism unless there is a strong reason to do so.
- Do not silently weaken the non-reentrancy guard without defining runtime ownership and state rules.
- Do not assume DOS-era `shell(":...")` semantics should remain the API on Linux.

---

## Use Case A: Spawned MEX Execution

### Concept

The parent MEX launches a child MEX as a new execution instance with its own interpreter state. When the child exits, the parent resumes after the call site.

### Example scenarios

- A lesson menu MEX launches a sub-lesson MEX and resumes the menu loop afterward.
- A lightbar menu item runs a helper MEX to perform one focused task.
- A script calls a utility-style MEX to do work and return a status code.
- A script chain is built from multiple small MEX files without requiring one giant monolith.

### Expected semantics

- Parent call frame is suspended, not destroyed.
- Child gets a fresh VM/interpreter instance.
- Child does not automatically share parent globals.
- Child may receive arguments.
- Child may return:
  - success/failure status
  - optional return code
  - optional return string or structured result later, if desired
- Parent resumes in a well-defined UI/runtime state.

### Mental model

This is closer to a function call across isolated interpreters than to an OS shell call.
It should feel like:

- `mex_spawn("scripts/learn/learn-file-io", args)`
- child runs
- parent resumes

not like:

- `shell(0, ":scripts/learn/learn-file-io")`

### Required runtime properties

- Stable suspension and resumption of parent MEX execution.
- Clear separation of parent and child VM memory.
- Explicit argument passing.
- Explicit return value handling.
- Safe restoration of screen, input, modem/session, and user state.
- No dependence on `/bin/sh`, `Outside()`, or external process control.

### Major design questions

- Should the child inherit display context, current colors, and cursor position?
- Should the child inherit current menu context, or only user/session state?
- Should the child receive a copy of some parent-visible variables as arguments, or only strings?
- What return types are practical for the first version?
- How should runtime errors in the child affect the parent?
  - terminate parent
  - propagate an error code
  - raise a MEX runtime exception equivalent

### Risks

- Suspending and resuming the parent interpreter may expose hidden global state in the runtime.
- Existing runtime helpers may assume only one active interpreter context per node.
- UI/video/input restoration bugs could reproduce the current node instability in a different form if not designed carefully.

---

## Use Case B: In-VM MEX Execution

### Concept

The parent MEX invokes another MEX inside the current interpreter/VM context. The child runs as part of the same logical execution environment rather than as an isolated child interpreter.

### Example scenarios

- A master script loads helper scripts that extend behavior.
- A script library modifies top-level variables defined by the caller.
- A stateful application is split into multiple MEX files while preserving shared session state.
- Scripts behave more like modules, includes, or callable program units.

### Expected semantics

- Parent and child share a compatible execution context.
- Child can access or mutate agreed-upon shared state.
- Execution returns to the caller at the next statement.
- The mechanism is explicit about what is shared.

### Possible sub-models

There are at least three plausible ways to implement this model:

#### Model B1: Shared global namespace

- Child runs in the same interpreter context.
- Top-level variables and globals are shared.
- Fastest to use conceptually.
- Highest collision risk.

#### Model B2: Module import / namespace model

- Child loads into its own namespace or module table.
- Parent accesses exported functions or values explicitly.
- Safer and more maintainable.
- More engineering effort.

#### Model B3: Same VM, separate stack frame with explicit bindings

- Child runs in the same VM instance.
- Some state is shared.
- Bindings are passed in explicitly or through a controlled environment table.
- Stronger long-term design if the language is to gain compositional features.

### Mental model

This should behave more like:

- `mex_call_in_vm("lib/inventory", env)`
- or `import "lib/inventory"`
- or `call script "lib/inventory"`

rather than a process-style spawn.

### Required runtime properties

- Clear definition of what constitutes VM state.
- Clear distinction between:
  - globals
  - locals
  - heap objects
  - intrinsic/runtime state
  - user/session state
- Safe nested call stack behavior.
- Deterministic name resolution.
- Protection against accidental corruption of parent execution.

### Major design questions

- Are MEX globals currently node-global, interpreter-global, or program-global?
- Can two compiled MEX programs safely coexist in one VM without symbol collision?
- How should function lookup work across files?
- Should top-level code execute on load, or only exported entry points?
- Does the language need an import/module concept before this is safe?
- How should unload/lifetime work for loaded child scripts?

### Risks

- This model is much more invasive than spawned execution.
- Existing MEX format/runtime may not be structured for multi-module execution.
- Symbol tables, globals, heap layout, and static assumptions may all require redesign.
- It may be easy to create a fragile version that works for demos but breaks under larger scripts.

---

## Comparison of the Two Models

| Dimension | Spawned MEX | In-VM MEX |
|---|---|---|
| Interpreter state | Separate child | Shared or partially shared |
| Parent globals visible to child | No, unless passed | Yes, depending on design |
| Isolation | High | Low to medium |
| Risk to runtime stability | Lower | Higher |
| Ease of reasoning | Higher | Lower |
| Use for menu chaining | Excellent | Possible but unnecessary |
| Use for script modules / master script architecture | Weak | Excellent |
| Likely implementation effort | Moderate | High |

### Recommendation

Treat **spawned MEX execution** as the first implementation target.

Reasoning:

- It directly addresses the current tutorial and menu-chaining need.
- It is conceptually cleaner.
- It avoids forcing immediate redesign of the MEX VM into a module system.
- It can establish explicit runtime context objects that may later help with in-VM execution.

Treat **in-VM MEX execution** as a second-phase design track.

Reasoning:

- It is a more ambitious feature.
- It needs explicit decisions about shared state, namespaces, and execution model.
- It should likely be built on top of a clearer runtime abstraction than exists today.

---

## Proposed API Direction

This section is conceptual only. Names are placeholders.

### For spawned execution

Possible intrinsic/API shapes:

- `mex_spawn(path)`
- `mex_spawn_args(path, args)`
- `mex_call(path)`
- `mex_call_args(path, args)`

Desired properties:

- returns status/result code
- does not use shell parsing
- does not require `:` prefix
- validates target as a MEX program directly

### For in-VM execution

Possible intrinsic/API shapes:

- `mex_invoke(path)`
- `mex_invoke_env(path, handle)`
- `mex_import(path)`
- `mex_module_load(path)`

Desired properties:

- explicit shared-state contract
- explicit rules for exported names / entry points
- predictable behavior on repeated loads

### Naming recommendation

Do not overload `shell()` for either feature.

Do not rely on prefix syntax alone for these semantics.

Use a dedicated MEX intrinsic for each model so that behavior is visible in code and portable across platforms.

---

## Implementation Track A: Spawned MEX Execution

### Phase A1: Runtime investigation and invariants

- Identify all state that must be preserved when a MEX suspends and resumes.
- Identify all global runtime variables used by the current MEX engine.
- Determine whether the current interpreter can be paused and resumed safely.
- Determine where user/session/video/input state belongs versus interpreter state.

### Phase A2: Execution context abstraction

- Define a MEX execution context structure.
- Separate interpreter-owned state from node-owned state.
- Ensure a parent context can suspend while a child context executes.

### Phase A3: Dedicated child execution path

- Add a direct runtime path for invoking a child MEX without `Outside()`.
- Load and execute child MEX as a child interpreter instance.
- Return a status code to the parent.

### Phase A4: Error model

- Define how compile/load/runtime errors propagate.
- Decide whether child failure aborts parent or returns an error.
- Add logging that distinguishes parent and child execution context.

### Phase A5: Validation targets

- Parent MEX launches child and resumes.
- Nested depth greater than 1 either works or is explicitly bounded.
- UI and modem/session state remain stable.
- No node crash on child completion or error.

---

## Implementation Track B: In-VM MEX Execution

### Phase B1: Language/runtime model decision

Choose one of:

- shared namespace
- module namespace
- shared VM with explicit bindings

This decision should be made before coding.

### Phase B2: Symbol and storage model

- Define how symbols are loaded and resolved.
- Define how globals are stored across multiple scripts.
- Define collision handling.
- Define whether top-level code executes on load.

### Phase B3: Entry point model

- Decide whether child scripts expose:
  - one implicit entry point
  - named exported functions
  - initialization plus callable procedures

### Phase B4: Lifetime and unloading

- Decide whether imported/invoked scripts remain resident.
- Decide whether repeated calls reload or reuse prior compiled state.
- Decide how resources and handles are cleaned up.

### Phase B5: Validation targets

- Child script can read parent-shared state when allowed.
- Child script can mutate allowed shared state.
- Symbol collisions are deterministic.
- Repeated loads do not leak or corrupt interpreter memory.

---

## Decision Points Before Any Implementation

### Decision 1: Priority

Decide whether the next milestone is:

- only spawned execution
- or architectural groundwork for both models

### Decision 2: API commitment

Decide whether user-facing scripts should eventually say:

- `mex_spawn(...)`
- `mex_call(...)`
- `mex_import(...)`
- some other explicit form

### Decision 3: Backward-compatibility posture

Decide whether `shell(":...")` should:

- remain unsupported on Linux
- become a compatibility shim to spawned MEX execution
- or be rejected with a clear error message when a MEX target is detected

### Decision 4: Shared-state scope for in-VM model

Decide whether shared state means:

- all globals
- only exported bindings
- an explicit environment object
- something else

---

## Recommended Immediate Next Step

The best next step is to define and implement **spawned MEX execution** first as a dedicated runtime feature.

That path should:

- avoid `shell()`
- avoid `Outside()`
- avoid `/bin/sh`
- preserve parent execution cleanly
- return a controlled status/result

Once that model is stable, revisit **in-VM execution** as a second design phase with explicit decisions about module semantics and shared state.

---

## Suggested Questions to Answer Next

- What is the minimum useful return value for spawned execution?
- Should spawned child MEX receive only strings as arguments in v1?
- Is nested depth greater than one a requirement from the start?
- For in-VM execution, do you want true shared globals, or would explicit exports/imports be safer?
- Should the long-term MEX model feel more like:
  - process-style call/return
  - module import
  - script-to-script function call
  - or a hybrid of those concepts?

---

## Exit Criteria for Future Wiki Updates

Do not update the tutorial/wiki material until all of the following are true:

- one of the two models is implemented intentionally
- the runtime behavior is stable on Linux
- the user-facing API is explicit and documented
- nested invocation semantics are tested and predictable
- the tutorials can describe one supported mechanism without ambiguity
