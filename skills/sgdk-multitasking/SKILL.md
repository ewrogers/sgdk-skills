---
name: sgdk-multitasking
description: Schedule spare-time CPU work with SGDK's single user task, including background decompression, procedural calculations, safe publication and cancellation. Use when foreground gameplay must stay responsive during CPU work; not parallel threads or a replacement for DMA streaming.
---

# SGDK multitasking

Use time left after foreground work. The Mega Drive still has one 68000: a
background task makes a long calculation interruptible; it does not make that
calculation cheaper or guarantee a completion deadline.

Start from the user's game and its existing ownership and frame loop. Spare Time
Lab demonstrates execution contracts, not a prescribed game, UI or architecture.

## Workflow

1. Pull `ewrogers/sgdk:latest` and inspect its `task.h`, `task.s`, frame waits and
   startup stack setup. Record versions. Online master may differ from the image.
2. First consider a small bounded step each frame. Use the SDK task when a pure
   CPU routine, such as a synchronous decoder, needs to continue across frames
   without rewriting its internal state machine. Short jobs may not justify it.
3. Reserve foreground time first: gameplay, input, rendering preparation, audio
   commands and VBlank service. Choose a deadline/fallback for late results.
   A busy foreground may leave the worker no useful time.
4. Keep one persistent, non-returning callback. Give it an audited stack and one
   explicitly owned job. Preserve its PC/stack when holding work; `TSK_stop()`
   followed by `TSK_userSet()` is not pause/resume.
5. Restrict worker code to immutable inputs and private RAM. Audit every called
   routine for global state and hardware access. Keep VDP, DMA, input, audio,
   allocation and interrupt control in the foreground. Keep ROM mappings stable.
6. Publish complete output through a documented single-CPU handshake. Cancellation
   is a request until the worker acknowledges it. Keep queued DMA sources alive
   and immutable until the foreground knows transfers have drained.
7. Test deterministic bytes, rejected requests, cancellation boundaries, retry,
   guards and publication. Build release/debug on ARM64 and AMD64. Check actual
   frame pacing, input, stack headroom and PAL/NTSC behavior in an emulator;
   console interrupt timing and audio contention still need hardware.

Read [SDK execution and stacks](references/execution-and-stacks.md) before using
TSK APIs or combining them with interrupts.
Read [jobs and ownership](references/jobs-and-ownership.md) when designing the
mailbox, cancellation, deadlines and output handoff.

[Spare Time Lab](assets/task-lab/README.md) unpacks verified APLIB data and computes
a weather map while a small foreground activity continues. It includes its own
source generator and build instructions; no other installed skill is required.

## Boundaries

- `sgdk-compression` selects codecs, verifies decoded bytes and measures costs.
  This skill controls when CPU work runs; ordinary `unpack()` remains synchronous
  to its caller even when that caller is a background task.
- `sgdk-streaming` owns caches, residency and bounded graphics uploads. A worker
  can prepare bytes; the foreground streaming owner decides when to publish them.
- `sgdk-ai` owns detection/pathfinding algorithms; prefer bounded searches before
  using spare-time execution. Never apply an obsolete result to a changed world.
- `sgdk-memory`, `sgdk-performance`, `sgdk-testing` own broader budgets and QA.
  Supply stack/buffer costs and completion measurements, not a competing clock.
- `sgdk-raster`, `sgdk-audio`, `sgdk-cartridge` own raster timing, sound drivers and
  bank mappings. A task is not permission to access those subsystems concurrently.

These links describe responsibilities, not mandatory companion installations.
