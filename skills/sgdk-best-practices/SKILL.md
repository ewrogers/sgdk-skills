---
name: sgdk-best-practices
description: Write or review SGDK C for defined arithmetic, clear module boundaries, resource lifetimes, safe queued transfers and interrupt sharing. Use for implementation quality and project code organization, rather than toolchain setup, game design or performance profiling alone.
---

# SGDK Best Practices

Make the important contracts visible: ranges, units, ownership, lifetime and
failure behavior. Preserve the game's existing architecture and creative
direction. These are implementation patterns, not a new engine or style guide.

Treat Labs as optional evidence for a specific technique. Explain the contract
being reused and adapt it to the game; do not inherit a Lab's controls,
progression, scene flow or module layout merely because working code exists.
Whole-game copying is appropriate only when the developer asks for it.

## Choose the relevant check

- [C correctness](references/c-correctness.md): promotions, overflow, shifts,
  alignment, aliasing and byte formats across host and target builds.
- [Module boundaries](references/modules.md): small interfaces, private state,
  borrowed resources and partial initialization without a prescribed layout.
- [SGDK ownership](references/sdk-ownership.md): sprite cleanup, queued DMA,
  foreground/interrupt responsibilities and the selected compiler's defaults.
- [Executable examples](assets/examples/README.md): checked arithmetic and byte
  decoding, transactional sprite acquisition, and a copied palette upload.

Read the reference that fits the change; do not load every specialist manual.
The examples build independently. Other installed SGDK skills are optional:
`sgdk-project` owns setup, `sgdk-math` numeric representations, `sgdk-scenes`
scene transitions, `sgdk-performance` profiling, and `sgdk-debug` fault diagnosis.
This skill supplies the C and lifetime contracts underneath those workflows.

## Apply a contract, then prove it

Before changing code, identify who calls it and what must remain true. Record
units and valid ranges where they are not apparent. Distinguish borrowed data
from owned resources and choose a useful response to exhaustion or invalid input.
Do not silently change gameplay to accommodate a convenient implementation.

Check unfamiliar APIs against the headers **and implementation** in the selected
`ewrogers/sgdk:latest` container. Pull it before validation and record the actual
SDK/compiler versions. The references were checked against SGDK 2.11 and the
container's GCC 16.2.0. C17 is the baseline; isolate SDK/GCC extensions at the
boundary rather than assuming desktop and target ABIs match.

Keep public headers focused on the operations callers need. Hide internal
state; avoid giving unrelated modules writable globals or raw ownership of
SDK objects. A simple struct and a few functions may be sufficient. Preserve
an established layout unless moving code solves a concrete dependency problem.

Test boundary values, rejected operations, repeated cleanup and recovery after
failure. Run the portable parts as optimized host C, then compile the SDK-facing
parts with the real SDK. Use target execution for ABI-sensitive behavior. Mocks
check your adapter's contract; they cannot establish what SGDK actually does.

Treat C correctness, SDK requirements and optional conventions separately.
Do not impose a universal ban on allocation, require hand-written assembly or
copy a Lab's art, music, mood, tuning or genre rules into another project.
