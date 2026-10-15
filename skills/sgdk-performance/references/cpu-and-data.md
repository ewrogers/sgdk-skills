# CPU work and data layout

## Remove repeated work first

Count candidates as well as contacts. An all-pairs scan over 64 objects visits
2,016 pairs before deciding whether anything touches. A useful spatial broad
phase can reduce those checks without weakening the collision rule.

The lab keeps an all-pairs reference beside a linked-cell grid. Every 8x8 body
occupies one 16x16 cell, and the grid searches that cell and its eight neighbors.
It checks only `j > i`, so each pair is visited at most once. Its half-open box
test treats touching edges as non-overlap. Host tests compare the complete pair
bitmaps over cell borders, several capacities and thousands of moving states.

Those assumptions matter. Larger shapes, negative/unbounded worlds, fast swept
collisions and moving obstacles require different coverage rules. The lab only
queries overlap after bounded movement; it does not resolve collisions or provide
a general physics engine. Putting every object in one cell restores the same
quadratic worst case. Small populations may favor the simpler all-pairs code.
Fewer checks alone is not a measured CPU speedup.

## Spend RAM deliberately

Dense arrays avoid pointer chasing and make traversal predictable. Cache a lookup
if its invalidation is clear; update a transform or tilemap only when it changes.
Batch work with the same state. Do not replace a correct ownership rule with
stale pointers just to avoid a lookup.

The lab reserves 64 eight-byte body records. The grid adds 160 byte-sized cell
heads and 64 next links as fixed scratch arrays, plus ordinary call stack and
locals. It needs no per-frame allocation. Sprite-engine descriptors, VRAM and
DMA queue storage are separate costs. The optional 512-byte pair bitmap belongs
to host tests; the ROM passes `NULL` and does not allocate it.

Choose pool capacity from the game's peaks and define what happens when full.
Reusing a slot should reset its state. More cached data can be worthwhile, but
inspect target `sizeof` and the linked memory layout rather than assuming host
pointer sizes or padding match the 68000. Never treat all 64 KiB of main RAM as
free for gameplay after SGDK, stacks and transfer buffers.

## Arithmetic is part of the contract

Prefer integer or SGDK fixed-point math for a measured hot path. Keep scaling,
signedness and conversion behavior explicit. The 68000's native word multiply
and divide do not make every C 32-bit expression cheap; widened operations can
require multiple instructions or helper routines.

Inspect generated assembly before replacing arithmetic. Constant divisions may
already be reduced by the compiler. A shift is not automatically equivalent to
signed division for negative values. Signed C overflow is undefined; widen,
bound or use deliberate unsigned wrap where the algorithm requires it.
Replacing division, square roots or trigonometry with a table needs range and
precision tests as well as a ROM/RAM cost comparison.

## Use the selected toolchain

Build optimized release code on both sides of a comparison. Use
`m68k-elf-objdump -d out/rom.out`, `m68k-elf-nm -S --size-sort out/rom.out` and
`m68k-elf-size out/rom.out` through the selected container to inspect target
instructions and symbols. Large symbols and library helpers are clues, not a
runtime call-frequency profile. LTO/inlining can make one C function difficult
to isolate; retain matching sources and ELF files.

Our selected image targets the base 68000 and includes its tested GCC byte-copy
workaround. Record its actual versions for a fair comparison; a toolchain
upgrade is a separate variable. Do not enable 68020 instructions or floating-point hardware
options to improve a Mega Drive benchmark.

Use assembly for a small, measured routine when C changes are insufficient.
Preserve the ABI, alignment and clobbers, retain an executable reference, and
test boundary values and real target execution. A quicker implementation that
occasionally corrupts state is not an optimization worth keeping.

## Sources and example

- [Collision reference and grid](../assets/performance-lab/src/workload.c)
- [SGDK fixed-point types and helpers](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/maths.h)
- [Pool API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/pool.h)
- [Memory APIs](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/memory.h)
- [Pinned toolchain and M68000 regressions](https://github.com/ewrogers/sgdk-docker)
