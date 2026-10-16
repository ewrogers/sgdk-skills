# SDK lifetimes and shared work

## Start from the actual SDK contract

The references were checked against SGDK 2.11, source commit
`ef9292c03fe33a2f8af3a2589ab856a53dcef35c`, in `ewrogers/sgdk:latest` with
GCC 16.2.0. The image can change: pull it, inspect the installed headers/source
and recheck version-sensitive details. Build target code for the base 68000.
The container's compiler currently defaults to `-fno-strict-aliasing`, verified
with `m68k-elf-gcc -O3 -Q --help=optimizers`. This accommodates SDK buffer idioms;
it does not legitimize out-of-bounds accesses, expired pointers, misalignment,
signed overflow or arbitrary casts. Keep portable model code valid with normal
strict aliasing too. Do not copy optimization flags from a failing experiment
without retaining a reproducer and checking what the flag actually changes.

## Queueing does not necessarily copy

`DMA_queueDma(location, from, to, len, step)` retains a source address for a later
transfer. Keep its storage valid **and its contents appropriate** until that
transfer is consumed. Returning from a function with a queued stack array is
unsafe. A persistent array can also be wrong if rewritten before the queue drains.
ROM data avoids stack lifetime problems but must remain mapped when read; see
`sgdk-cartridge` for banked and far resources.

`DMA_copyAndQueueDma` copies into the SDK's temporary buffer, then queues that
copy. It returns false if temporary storage or the queue cannot accept it.
In the checked implementation a failed queue insertion releases its temporary
allocation. `len` is in 16-bit words, `to` is a destination address, and `step`
is the byte increment. Check the particular memory destination rather than
assuming every parameter is a count of bytes.

[upload.c](../assets/examples/src/upload.c) queues one sixteen-color palette.
It copies 32 bytes inside a short foreground critical section and pairs interrupt
disable/enable on success and failure. Its caller owns the destination palette
slot. The caller's source may be reused after return; a false result should
leave the redraw pending for a later retry. Copying consumes temporary RAM and
queue capacity; it is not free bandwidth or a reason to copy every resident asset.
Higher-level `DMA_QUEUE_COPY` options follow their own calling APIs and may have
different failure reporting. Check those APIs instead of assuming a bool return.

`DMA_allocateAndQueueDma` queues a buffer before the caller fills it. The header
explicitly warns about an interrupt flushing it in between. Fill before consumption
and protect that interval. Keep work bounded; masking interrupts around a large
copy, asset decode or full frame is not a general solution.

## Foreground and interrupt code have different jobs

In SGDK 2.11, `SYS_disableInts` / `SYS_enableInts` support nested foreground
calls, saving the previous mask on the outermost pair. They return without
performing that work when called from an interrupt. Balance pairs on every
path; do not assume they are locks usable in callbacks. The palette adapter is
explicitly foreground-only. Avoid resetting the interrupt mask to a guessed
constant when returning from a borrowed critical section.

`volatile` is appropriate for accesses that must be observable, such as SDK
MMIO and selected ISR-shared flags. It does not make a read/modify/write atomic,
protect a multi-field snapshot or impose a general memory barrier for ordinary
objects. A 32-bit C field is not guaranteed to be an indivisible access on a
16-bit bus. Choose ownership, a short critical section or a specifically designed
snapshot protocol for data shared with interrupts. Host thread atomics and
spinlocks are not automatic replacements for interrupt ownership on this CPU.

Prefer callbacks that record bounded work for the main loop when the feature
allows it. Use the SDK's appropriate callback/task APIs and preserve their
ordering. Raster effects, VDP port protection, Z80 bus ownership and audio driver
coordination need their specialist contracts: use `sgdk-raster`, `sgdk-audio` or
`sgdk-debug` when available. Do not add parallel VDP or sound drivers casually.

## Sources and proof boundaries

- SGDK 2.11 [DMA API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h)
  and [implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/dma.c).
- [System API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h)
  and [nesting implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c).
- GCC 16.2 [volatile semantics](https://gcc.gnu.org/onlinedocs/gcc-16.2.0/gcc/Volatiles.html).
- [Toolchain defaults and regression tests](https://github.com/ewrogers/sgdk-docker).

The example's target tests execute the actual SDK queue/copy routines, inspect
copied words and check failure and interrupt-mask restoration. They do not flush
VRAM/CRAM, schedule real interrupts or prove DMA timing. Sprite lifecycle tests
inject failures through a small test double; target builds verify signatures,
not live sprite rendering or allocation behavior.
