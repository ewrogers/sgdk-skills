# Mega Drive budgets for an SGDK plan

Use this as a planning reference for the normal tiled, non-interlaced display.
Special modes and raster effects need their own investigation. A hardware
ceiling is not a target to fill, and it is not an amount SGDK leaves free.

## Main RAM and Z80 RAM

The 68000 has **64 KiB of main RAM**. SGDK state, initialized data, `.bss`, game
objects, transfer/decompression buffers, heap allocations and stack share it.
The Z80 has a separate **8 KiB of RAM**, largely affected by the audio driver.
It is not another general-purpose 8 KiB to add to the game's main-RAM budget.

Record target `sizeof` results or linker/map output when available. Host
structure sizes can differ. Initialized writable data needs both a ROM load
image and RAM at runtime; `.bss` uses runtime RAM without storing all those zero
bytes as ROM content. A section-size report does not measure worst-case stack,
future heap allocations or fragmentation.

For frequently created objects, reusable storage can make cost and failure
handling predictable. SGDK also supports allocation. Choose from the actual
workload instead of imposing a universal no-allocation rule, and keep peak
temporary memory visible in the plan.

Sources: SGDK 2.11 [linker layout](https://github.com/Stephane-D/SGDK/blob/v2.11/md.ld),
[memory API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/memory.h),
[Plutiedev: using the Z80](https://plutiedev.com/using-the-z80).

## VRAM and tile residency

VRAM is a separate **64 KiB** shared by pattern graphics, plane tilemaps, the
sprite attribute table and scroll tables. A normal 8 by 8, 4 bpp pattern uses
**32 bytes**. The 11-bit pattern index permits **2048 addresses**, numbered
0 through 2047; tables and other reservations mean those are not 2048 free art
tiles.

Plan the patterns resident at the same time, not every frame or level in ROM.
Count table space, font/system tiles, the sprite engine's graphics region and
user-managed graphics without overlaps. Deduplication can reduce exported tile
data, but an unexplored spritesheet should not be credited with an assumed
compression ratio or guaranteed sharing.

In SGDK 2.11, `SPR_init()` uses a default sprite graphics reservation of 420
tiles. `SPR_initEx()` allows a chosen reservation; zero selects the default.
Changing it affects available graphics space elsewhere. Inspect the actual
VDP layout and `TILE_USER_INDEX`, `TILE_USER_LENGTH` and `TILE_USER_MAX_INDEX`
before deciding how much room is available to a project. Plane sizes and SDK
configuration can change the answer.

The reserved region and its occupants are two views of the same memory. Count
the region once in total VRAM, then show sprite use within it. With automatic
allocation, each logical sprite reserves its resource's `maxNumTile`, based on
the largest frame. Thirty 16 by 16 sprites may therefore need 120 tiles even
when they all share one definition in ROM. Live sharing requires a deliberate
preloaded/shared-graphics strategy; a shared resource name alone does not make
it automatic. Fragmentation can also prevent an allocation.

Sources: [Plutiedev: tiles and palettes](https://plutiedev.com/tiles-and-palettes),
SGDK 2.11 [VDP layout](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h),
[VDP setup](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp.c),
[sprite initialization](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sprite_eng.c).

## Palettes

The VDP provides **four shared banks of 16 color entries**. A background tile
or hardware sprite selects one bank. Color index 0 is transparent for that
tile or sprite; the backdrop is selected separately. Plan each bank's users,
including the HUD and effects, so a palette change does not unexpectedly
recolor something else.

Normal color values have **3 bits per RGB channel**. A source image's RGB colors
must be quantized and its indices arranged consistently. Four banks do not
mean each asset gets four palettes, or that 64 distinct opaque colors are
available to every object. Fades/cycling alter the existing entries; they do
not create more banks. Shadow/highlight and raster palette changes are separate
features with additional constraints, not a default budget escape hatch.

Sources: [Plutiedev: tiles and palettes](https://plutiedev.com/tiles-and-palettes),
[VDP color ramp](https://plutiedev.com/vdp-color-ramp),
SGDK 2.11 [palette API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/pal.h).

## Hardware sprites and scanlines

| Display mode | Total hardware sprites | Per scanline | Sprite width per scanline |
| --- | ---: | ---: | ---: |
| H40, 320 pixels wide | 80 | 20 | 320 pixels |
| H32, 256 pixels wide | 64 | 16 | 256 pixels |

Count **hardware pieces**, not game objects or SGDK `Sprite` instances. A large
logical sprite can use several pieces, and transparent areas can still consume
the width budget. Inspect the resource's actual cutting and the busiest
horizontal overlap. Being below the whole-screen count does not prevent
scanline dropout.

Sprite allocations also consume software state and VRAM. Raising a software
pool does not raise the VDP limits. Background plane tiles use different
display machinery and do not consume these sprite counts, although their
patterns and tables share VRAM with sprites.

Sources: [Plutiedev: sprite limits](https://plutiedev.com/sprites#sprite-limits),
SGDK 2.11 [sprite engine](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h),
[hardware sprite API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_spr.h),
[ResComp manual](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt),
[Sega software manual, VDP chapters](https://segaretro.org/images/a/a2/Genesis_Software_Manual.pdf).

## DMA and changing graphics

Track **bytes transferred in a busy frame**, queue entries and temporary buffer
use separately. Static resident graphics do not need uploading again every
update. An animation frame, scrolling strip, palette change, HUD update and
sprite table update can compete for the same transfer window.

There is no universal safe bytes-per-frame number for every video mode and
transfer strategy. H32/H40, vertical resolution, region, timing, source and
other transfers matter. DMA can stop the 68000 during transfers, so transfer
and CPU costs interact. Deferred transfers also require their source data to
remain valid until processing finishes.

SGDK 2.11's DMA initialization selects a 7200-byte nominal transfer capacity for
NTSC or 8000 for PAL, with an 8192-byte temporary buffer and an 80-entry default
queue. These are **SDK defaults**, not hardware maxima or guaranteed spare
bandwidth. Other constants in `dma.h` are not necessarily active defaults.
Check configuration and other SDK work before choosing a project ceiling.

Normal SGDK updates use `SYS_doVBlankProcess()` to service queued work and wait
for presentation. Plan ordinary uploads around that workflow. Active-display
VDP writes and broad interrupt disabling can create artifacts or interfere
with sound; they are not free ways to extend the ordinary budget.

Sources: SGDK 2.11 [DMA contracts](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h),
[actual defaults](https://github.com/Stephane-D/SGDK/blob/v2.11/src/dma.c),
[VBlank processing](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h),
[Plutiedev: hardware issues](https://plutiedev.com/hardware-issues).

## CPU, update rate and audio

Nominal 60 Hz gives about **16.7 ms per refresh**; nominal 50 Hz gives about
**20 ms**. Region and actual console timing matter. That is not all free game
logic time: transfers, SGDK services, interrupts, decompression and audio
interaction also cost time. Specify the game's simulation rate deliberately;
PAL support does not automatically make motion and timers region-independent.

The usual SGDK non-interlaced viewport is 224 lines tall. PAL can also use 240;
do not promise the same 240-line mode on NTSC. The chosen active height affects
the available transfer window as well as the visible picture.

The 68000 has no floating-point unit. SGDK offers fixed-point types and math
helpers. Identify expensive repeated work, divisions and wide intermediates,
but do not estimate a CPU percentage from the number of actors alone. Start
with an optimized release probe and repeatable input. SGDK's `SYS_getCPULoad()`
estimates load using VSync/VInt idle time, averaged over eight frames; it is
not a cycle-perfect profiler. `SYS_getFPS()` counts calls, so sample once per
game update rather than once per entity. Overlays add work too.

Choose the sound driver and representative music/SFX overlap early. The
YM2612 has six FM channels, with DAC playback using channel 6; the PSG has
three tone channels and one noise channel. The driver determines how the
game uses those resources. Z80 bus ownership, sample playback and DMA interact.
For ordinary playback, use the selected driver's verified APIs. Raw FM/PSG
register writes can conflict with it; moving work to the Z80 or writing 68000
assembly is a deliberate engineering task, not an assumed performance fix.

Sources: SGDK 2.11 [system counters](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h),
[counter implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c),
[video modes](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h),
[math helpers](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/maths.h),
[Z80 control](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/z80_ctrl.h),
[Plutiedev: YM2612 operations](https://plutiedev.com/ym2612-operations),
[PSG](https://plutiedev.com/psg).

## ROM, padding and banking

The ordinary cartridge ROM window is **4 MiB (32 Mbit)**. Larger games need a
supported mapping scheme and compatible cartridge/flashcart/emulator. Banking
does not increase main RAM, VRAM, palettes or per-scanline rendering capacity.
Do not postpone that hardware choice until after promising a large release.

SGDK 2.11's standard build pads the final ROM to a **128 KiB boundary**. This
is not automatic power-of-two cartridge sizing. Track meaningful linked
content, the padded file and the chosen physical cart capacity separately.
Use bytes or KiB/MiB consistently; **1 MiB = 8 Mbit**. Compressed assets save
ROM at a possible decompression/upload cost, and may still need decoded space.

The SGDK 2.11 implementation defaults `ENABLE_BANK_SWITCH` to zero. Its optional resource
banking uses the Sega/SSF mapper and requires compatible SDK configuration,
libraries and ROM metadata. Mapping changes can invalidate retained resource
pointers, including sources of deferred transfers or audio reads. A successful
link is not a mapper compatibility check or an enforcement of the ordinary
4 MiB window.

Sources: [Plutiedev: beyond 4 MiB](https://plutiedev.com/beyond-4mb),
[ROM padding](https://plutiedev.com/rom-padding),
SGDK 2.11 [build padding](https://github.com/Stephane-D/SGDK/blob/v2.11/makefile.gen),
[banking configuration](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/config.h),
[mapper contracts](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/mapper.h).

## Verify the selected SDK

For a new project with no chosen toolchain, the tested baseline is SGDK 2.11:

```text
ewrogers/sgdk:latest
```

The image supports native Linux ARM64 and AMD64; Docker normally selects the
host's architecture. On Windows, use Docker's Linux containers. The installed
SDK version determines which APIs are available. Source planning can continue
when Docker is unavailable; label any unverified implementation assumption.

Use `docker run --rm IMAGE doctor` with the actual image reference. Inspect
`/opt/sgdk/inc`, `/opt/sgdk/src`, `/opt/sgdk/sample` and
`/opt/sgdk/bin/rescomp.txt` in that image before relying on an unfamiliar API,
resource option or default. Versioned links above explain the baseline;
installed sources settle what the chosen build actually provides.

Keep feasibility probes in a scratch or explicitly chosen project directory.
Mount source and outputs through Docker for compilation; emulators run on the
host. Do not change emulator mappings, install a native toolchain or overwrite
an existing project just to answer a planning question. Record whether the
probe compiled, ran in an emulator, or ran on hardware: those are different
levels of evidence.
