---
name: sgdk-memory
description: Account for SGDK work RAM, allocation lifetimes, heap fragmentation and allocation failures. Use for memory growth across levels, failed allocations despite free bytes, heap/stack budgeting and safe resource cleanup, rather than frame profiling or VRAM tile streaming alone.
---

# SGDK Memory

Start with the matching ELF, selected SDK configuration and the point in the
game where memory is tight. Distinguish a capacity problem, fragmented free
space, a leaked owner, stale references and corruption before changing sizes.
Do not impose a heapless architecture or copy the Lab's allocation sizes.

## Account for the right space

Separate static work RAM, heap allocations and stack reservation from VRAM,
ROM and the sound CPU's memory. A heap counter is not a total-RAM meter.
Compare a consistent post-initialization baseline with busy play, transition
overlap and repeated return-to-title cycles. Keep unknown peak stack and
temporary allocations visible rather than treating them as zero.

- [RAM accounting](references/ram-accounting.md): ELF sections, SDK reservations,
  counter meanings, stack limits and a standalone report helper.
- [Lifetimes and failures](references/lifetimes.md): shared/room/scratch owners,
  rollback, retained pointers and queued DMA cleanup.
- [Fragmentation](references/fragmentation.md): rounded block costs, largest
  free runs, packing and when pools or arenas help.
- [Memory Lab](assets/memory-lab/README.md): actual SGDK allocator demonstration,
  lifetime page, automated cycles, builds and validation boundaries.

## Give each allocation an owner

Record who releases it and the last consumer that may retain its address.
Build a new resource set privately; publish it only after every required
allocation succeeds. On failure, roll back that attempt and keep the previous
usable state. Null the owning pointer after release; that does not repair aliases.

Stop callbacks and producers before teardown, and drain queued transfers before
freeing their borrowed RAM. Keep long-lived resources separate from short-lived
work when that improves measured behavior. Pools and room arenas are options,
not substitutes for checking their capacities and references.

## Prove recovery

Pull `ewrogers/sgdk:latest` and inspect the selected headers and implementation.
The Lab is verified against SGDK 2.11 / GCC 16.2.0. In that allocator,
`MEM_pack()` coalesces adjacent free blocks; it cannot move live allocations
or merge holes separated by them. Free/allocated counters include block headers.

Exercise allocation rejection at each construction step, delayed DMA completion,
live-data preservation and repeated teardown. Compare both total free space and
the largest free run with the same baseline. Use target `sizeof` and execute
target allocator tests; a host allocator model cannot establish SGDK behavior.

`sgdk-best-practices` covers broader C contracts; `sgdk-performance` covers
frame cost; `sgdk-streaming` covers tile caches. They are optional complements.
This skill and its Lab install independently.
