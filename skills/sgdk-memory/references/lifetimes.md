# Ownership and allocation lifetimes

## Write down the owner

| Lifetime | Example | Release condition |
| --- | --- | --- |
| Shared | Common font, persistent player/session data | No future scene or callback needs it |
| Room | Room collision data and entity storage | Producers stopped and retained references retired |
| Scratch | Decode workspace | Decoder and downstream consumers finished |
| Queued source | RAM passed to `DMA_queueDma` | Queue has consumed that source |

These are useful categories, not a required game framework. A fixed array may
be easier for a known small maximum. A room arena can make teardown predictable,
but resetting it invalidates every pointer into it. If rooms share a pointer,
that pointer needs a longer-lived owner or an explicit ownership transfer.

SGDK `MEM_alloc(u16 size)` returns an uninitialized block or `NULL`.
Initialize successful allocations before exposing them. `MEM_free(NULL)` is a
no-op; freeing a block leaves your pointer unchanged. Invalid and repeated frees
are not supported ownership operations, even when a logging build catches one.
Null the owner after freeing; copied pointers can still dangle.

Source: [memory API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/memory.h).

## Construct privately, publish once

Memory Lab's `ml_room_load` allocates collision-sized storage and a second
room buffer into local owners. If the second request fails, it frees the first
without replacing the active room fields. Only after both succeed does it
publish the pair. A real level change can keep the old room playable until
this staging succeeds, or use a covered unload-first transition if RAM cannot
hold both. Choose and test the fallback explicitly.

Check count/stride multiplication in a wider type before converting to the
allocator's `u16` parameter. Do not allow a truncation, zero count or rounded
size overflow to become an unexpectedly small allocation. The selected
allocator adds a two-byte header, rounds to even bytes and stores the adjusted
size in `u16`; sizes near that type's maximum need rejection before adjustment.
The Lab uses bounded small constants; it is not a general unchecked-size wrapper.

Do not allocate or free through interrupt handlers while main code can mutate
the same heap. Schedule requests through a single owner instead of assuming
the allocator is reentrant or wrapping a long scan in an interrupt blackout.
Sources: [allocator implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/memory.c),
[interrupt handling](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c).

## A queue extends a buffer's lifetime

`DMA_queueDma` borrows its source. Returning from the function, hiding the
sprite or requesting a scene change does not make that RAM disposable.
Check queue admission, then retain the source until completion. A full or
budget-constrained queue must not be treated as a successful transfer.
`DMA_QUEUE_COPY`/copy variants move the ownership obligation to SGDK's bounded
temporary storage; they do not provide unlimited space.

Memory Lab allocates a 128-byte staging buffer, queues four actual tiles, drains
the queue through `SYS_doVBlankProcess`, then releases it. Its model rejects
room teardown while staging is retained. Tests deliberately delay completion
and verify that both pointers and contents remain valid. If admission fails,
the buffer can be released immediately because no transfer borrowed it.

Source: [DMA API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h),
[DMA implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/dma.c).

For a larger game, stop new submissions, disconnect callbacks, drain the last
readers, release in dependency order and invalidate references. A blanket
`MEM_init()` is internal startup, not a scene cleanup API: it would forget SDK
and game allocations still in use. Memory Lab's CPU probe calls it only because
that test bypasses the normal boot sequence.
