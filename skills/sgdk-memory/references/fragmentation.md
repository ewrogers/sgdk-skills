# Fragmentation and allocation strategy

## Free bytes are not one continuous buffer

Consider six neighboring allocations. Free the first, third and fifth: the
total free space can exceed a new request, but the live blocks between those
holes prevent one contiguous allocation. A larger free tail could satisfy it,
so a meaningful reproduction must account for that tail and any earlier holes.
Memory Lab reserves those spare runs temporarily to expose this case; a game
should not fill its heap just to reproduce the Lab's layout.

In the verified SGDK 2.11 implementation a request for 1,024 payload bytes
occupies 1,026 allocator bytes. A request for one byte occupies four. The
rounding is `((size + 3) & 0xFFFE)` for these valid small sizes. Counters include
the header. Six blocks cost 6,156 bytes; freeing alternating blocks makes
3,078 allocator bytes available in three separated holes. A 1,536-byte request
needs a contiguous 1,538-byte block and cannot fit a 1,026-byte hole.

Source: [allocation and counter implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/memory.c).

## Packing is not moving live objects

`MEM_free` marks a block free. `MEM_pack` joins adjacent free blocks and resets
the search; the allocation path also searches/coalesces when its current free
block cannot satisfy a request. Packing cannot combine holes across a live
allocation. It does not move a live object, update your pointers, compact VRAM
or free leaked owners. The largest-free-block query already accumulates
adjacent free runs in this SDK, so a pack call need not increase its result.

The Lab checks that live pointers and marker bytes survive packing and that
the intentionally blocked request still fails. Releasing the remaining blocks
and reservations restores both total free bytes and the original largest run.
Its baseline can itself be fragmented by earlier SDK initialization.

Source: [packing and largest-run scan](https://github.com/Stephane-D/SGDK/blob/v2.11/src/memory.c).

## Choose a strategy from actual lifetimes

- Allocate long-lived shared resources early when that helps keep room storage
  together. Verify the resulting layout; allocation order is not a guarantee.
- Group room-owned allocations and release them together. An arena can use one
  larger allocation and bounded offsets, trading internal slack for cheap reset.
- Reuse scratch space only when consumers cannot overlap. A pending DMA reader
  can outlive the decoder that originally filled it.
- Use fixed pools for frequent same-sized objects when their capacity and
  exhaustion policy are known. Keep generation/validity checks if references
  can survive slot reuse. Do not trade a small heap cost for huge unused pools.
- Keep large read-only content in ROM where appropriate. Compression and
  banked reads introduce CPU, scratch and mapping lifetimes of their own.

Measure peak allocated bytes, largest free run, failure count and return-to-
baseline behavior at representative lifecycle points. Frequent pack/scans can
consume frame time, so investigate their cost before adding them to every tick.
The Lab invokes them at user actions or once per automatic step, not per entity.
Optional effects may degrade on allocation failure; essential room state needs
a recoverable transition or a clear error. Never dereference a failed request.
