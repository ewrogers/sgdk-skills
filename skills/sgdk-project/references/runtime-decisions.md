# Runtime decisions

## Frame work

Start from SGDK's current samples. `SYS_doVBlankProcess()` services SDK work
such as DMA and input updates according to the SDK's configuration. Read its
comments before changing the placement of input reads or adding interrupt work.
Calling it twice is not a harmless way to "wait a bit". Updating every entity,
uploading graphics and waiting for presentation are different costs.

Use optimized release ROMs to establish a baseline. Measure the same workload
before and after an optimization. Do not use host emulator speed, a static
screen or an empty sample to promise a game will sustain its target rate.
Choose NTSC/PAL and time units deliberately; SGDK supports both regions.

## Numeric work

The 68000 has no floating-point unit. SGDK provides `fix16`, `fix32` and math
helpers; inspect their representations and operations before choosing one.
Use suitable integer widths for ranges and operations, but do not mechanically
replace every 32-bit value with 16-bit storage. Check intermediate ranges and
promotion rules. Division and wide arithmetic in a hot loop deserve measurement.

The Docker image uses C17 with GNU extensions. Integer overflow, invalid shifts
and out-of-bounds memory are C correctness issues, not optimization tricks.
Keep target C portable where possible and use SDK helpers when appropriate.

## Memory and graphics

The console's main RAM and VRAM are separate 64 KiB spaces. The SDK, stack,
sprite system, buffers and game state share their respective budgets. A linker
section report is useful but does not by itself account for worst-case stack,
dynamic allocations or VRAM contents.

SGDK supports allocation. For frequently created objects, consider bounded
storage or reuse where it helps measured cost and predictable failure handling;
do not impose a universal heapless policy or require a scene graph. Check failure
returns and follow the chosen SDK subsystem's ownership/lifetime rules.

Sprites, background tiles, palettes and DMA transfers have different limits.
An SGDK sprite can use several hardware sprites. Scanline limits can be reached
before the whole-screen sprite limit. Keep static resources resident when
appropriate and account for tile uploads when changing animation frames.

Sources: SGDK 2.11 [system API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h),
[maths](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/maths.h),
[types](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/types.h),
[memory](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/memory.h),
[VDP](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h),
[sprite engine](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h),
[DMA](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h).
