# Modules with clear responsibilities

## Put the boundary where ownership changes

A useful module lets callers do a task without coordinating its internals.
For example, [pair.c](../assets/examples/src/pair.c) owns two SDK sprite handles.
Its header exposes open, close and a state query. Callers cannot release one
handle behind its back or publish a half-created pair.

It stages acquisitions in local variables, rolls back if the second allocation
fails and publishes private state only when both succeed. Closing twice is safe.
Opening an already-open pair is rejected without disturbing it. The sprite
definitions are borrowed, and must remain valid until close. That is a small
ownership pattern, not a recommendation that a game organize actors in pairs.

State whether a function borrows an argument, copies it, retains it or takes
ownership. A `const` pointer restricts writes through that pointer; it does not
extend the data's lifetime, imply a deep copy or make every alias immutable.
Avoid returning pointers into short-lived stack storage. For retained pool
references, specify when recycling invalidates them; use a generation check
when stale references are a real risk, not as mandatory machinery for every ID.

Release resources through the API that owns them. Clear stored handles when
released and keep one clear cleanup path for partial setup. Do not mix module
cleanup with a global SDK reset behind another owner's back. Coordinate whole
screen changes with the existing transition code; `sgdk-scenes` covers that flow.

## Make dependencies visible

Keep only the declarations callers need in a public header. Use include guards;
make a header compile when included first and twice. Include its own header
first in the implementation so declaration drift becomes a compiler error.
Use `static` for implementation-only functions and file state. Give public C
symbols a project/module prefix because the linker has no C++ namespace boundary.

Keep initialized resource definitions in one translation unit and declare them
with `extern` where needed. Avoid defining writable storage in a shared header.
Do not put `<genesis.h>` in every portable model header merely for convenience;
keep SDK includes at the boundary when doing so makes the model independently
testable. Conversely, do not build a large abstraction layer just to hide a few
stable SDK calls.

A project may use `src/`, `inc/` and `res/` with the standard SGDK makefile, or an
existing structure that already builds well. Organize by coherent responsibility
and change together where practical. Split a file when it contains unrelated
owners or exposes too many internals, not when it crosses an arbitrary line count.
Keep generated resource declarations distinct from hand-maintained gameplay code.
Use `sgdk-project` for the build/resource setup rather than duplicating it here.

## Errors and memory are design decisions

Use a status, nullable handle or explicit result when a caller can recover.
Explain whether failure leaves prior state intact or requires cleanup. Validate
before committing a change; do not return failure after silently publishing a
partial object unless that is the documented contract. Assertions are useful
for developer invariants, but release behavior still needs to be defined for
expected exhaustion and user-controlled input.

SGDK supports dynamic allocation. Use bounded storage or reuse where it improves
predictability, and check failure either way. Account for resident resources,
heap and worst-case stack separately. Large local arrays and deep recursion can
consume RAM even without a single heap allocation. Do not copy host `sizeof`
results into a Mega Drive budget. Pool policy belongs to gameplay; measurements
and data-layout optimization belong to `sgdk-performance`.

## Sources and checks

- SGDK 2.11 [sprite engine](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h)
  and [allocation](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/memory.h).
- [Executable examples](../assets/examples/README.md) separate portable contracts
  from the SDK adapters. Host tests inject failure at each acquisition, retry,
  reject reentry and repeat cleanup; real SDK builds check the actual API types.
