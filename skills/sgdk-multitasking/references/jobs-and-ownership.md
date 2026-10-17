# Jobs, ownership and deadlines

## Pick suitable work

Good candidates include pure-RAM decompression, procedural map/field generation,
large checksum calculations, or an optional path calculation over an immutable
snapshot. Use a bounded incremental algorithm first when available. Tile uploads,
audio driver calls and input polling belong to their foreground owners.

A worker can be interrupted inside a decoder, but the decoder is still synchronous
to its caller. To cancel it safely, wait for a return/boundary; do not reuse its
stack or output just because its next frame has been withheld. For banked ROM,
keep the input mapping valid throughout the suspended call. Do not change banks
behind a decoder or sound driver.

## One-job handshake

Lab `work.c` makes ownership explicit:

1. IDLE: foreground can write immutable job inputs and publish REQUESTED last.
2. RUNNING: worker alone writes output/progress. Foreground may request cancel,
   but cannot replace inputs or read unfinished output as a result.
3. READY: worker publishes after its last output write and stops writing it.
4. UPLOADING: foreground loans output to queued DMA in 512-byte chunks. A queue
   rejection retries unchanged bytes; accepted loans remain immutable until drained.
5. IDLE: only after every accepted chunk has completed. Foreground can switch the
   visible tilemap to the fully prepared VRAM slot, or start another job.

A running cancellation produces CANCELLED at a row/pass boundary. Foreground must
acknowledge it before reuse. Cancellation during a pending DMA loan is rejected;
finish that loan first. Cancelled partial VRAM content stays in the hidden slot.

Aligned volatile word flags and GCC compiler barriers implement this **single
68000 exception schedule**. They are not portable multithreaded C synchronization.
Publish state last, observe it before reading dependent data, and preserve sole
writers. Volatile alone does not order arbitrary nonvolatile buffer accesses.
Do not share SDK scratch state or allocators across the two contexts.

The Lab stages each text row in persistent RAM and drains queued transfers before
rewriting that RAM. The same lifetime rule applies to UI and SAT data, not only the
worker's output. Never queue a pointer into a returned stack frame.

## Budgets and fallbacks

- Foreground has priority. Track actual VBlank pacing separately from job latency.
  A long foreground or disabled interrupts can break smooth operation.
- A held worker makes no progress and cannot acknowledge cancellation. Resume it
  before waiting for acknowledgement; do not block the supervisor on that flag.
- Bound staging RAM, user stack, queue entries, bytes uploaded per frame and VRAM
  slots independently. A background task does not remove any of these costs.
- Tag jobs with a generation/version if the world can change. Reject stale AI/map
  results; never apply them just because they eventually completed.
- Preload ahead of the deadline, retain resident art, or fall back to a covered
  loading screen. No spare-time scheme guarantees completion under an unbounded
  foreground workload.

## Evidence and limits

Host tests exercise accepted/rejected requests, cancellation before and during
work, 200 cancel/reuse cycles, queued source lifetime, retry, guards and exact
weather bytes at O0/O3 with undefined-behavior checks where supported. Target tests
execute actual APLIB decoding and worker steps on base M68000 and compare all bytes.
They do **not** emulate SDK interrupt/context scheduling, VDP timing or audio.
An attempted synthetic interrupt harness could not reliably reproduce context
flags; it was excluded rather than treated as task-scheduler evidence.

ARM64/AMD64 build tests cover release NTSC/PAL headers and debug, with matching
ROM/output hashes. Runtime checks should include movement throughout each job,
hold/resume mid-job, cancel/retry, publication without partial images, and both
video standards. Real console interrupt latency, raster coexistence, worst-case
stack depth and sound/DMA contention remain hardware checks.

Sources: [SGDK DMA API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h),
[frame service](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c), and the
[execution reference](execution-and-stacks.md) for task source links.
