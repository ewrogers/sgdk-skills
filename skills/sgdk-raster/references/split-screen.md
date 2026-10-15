# Two cameras, one display

A versus screen needs two camera views, not two copies of the game simulation.
Update the world once, then project it separately into each viewport. Split Lab
uses normal 320x224 output with a horizontal divider. Raster Lab remains a
separate ROM for water palettes and music testing.

## Share a resident world first

The lab fills a 64x64-cell BG_A name table with a 512x512-pixel world at startup.
That table uses 8 KiB of VRAM, separate from its pattern graphics. Both cameras
can read the same table because it contains the whole world and never streams.

`MAP_getTilemapRect()` takes coordinates and sizes in 16x16 metatiles. Read one
32-metatile row into 128 `u16` values, then upload it synchronously with
`VDP_setTileMapData()`. This needs a 256-byte staging buffer instead of an 8 KiB
stack buffer. Release the temporary MAP after the uploads finish.

Do not assume an initial `MAP_scrollToEx()` fills an entire 512-pixel-high plane.
It initializes a scrolling cache. For larger worlds, design separate camera
caches or an explicitly partitioned renderer. Two MAP objects cannot safely
stream independently into the same name-table space.

## Give each view its own scroll

A persistent 224-entry horizontal table supplies the upper camera's X offset
above the midpoint and the lower camera's X offset below it. It uses 448 bytes
of RAM and transfers 448 bytes per displayed frame.

Vertical scroll must account for where the viewport starts on screen:
`camera_y - viewport_screen_y`, wrapped to the resident plane's height. VBlank
restores the upper value; one H interrupt writes the prepared lower value to
VSRAM. The handler also moves the Window plane from the top HUD to the bottom
HUD. It owns these registers until the next VBlank restores them.

The handler ignores pending interrupts during VBlank, accepts only its small
active-line window and uses bounded HBlank polling before its port writes.
Keep ordinary VDP operations protected against interruption. The lab's
`LATE` and `TIMEOUT` counters expose missed setup or edge waits.

## Sprites still share the hardware

The VDP has no per-camera sprite clipping rectangle. The lab creates two
projections of each actor, then hides any projection that is not wholly inside
its viewport. An actor can appear in both views. Partial sprites disappear at
the edges rather than being cropped pixel by pixel; this is a deliberate limit
of this small example.

Ten opaque 32x8 sprites cover the midpoint transition. They consume all 320
sprite pixels on those H40 scanlines, so actor sprites are excluded from that
band. Four actor projections plus ten divider sprites use fourteen user sprite
entries, in addition to SGDK's internal reservation. The normal H40 limits of
80 sprites per frame, 20 per line and 320 sprite pixels per line remain shared
across both views. Palettes and VRAM are shared too.

## How this differs from Sonic 2

Sonic 2 versus uses double-resolution interlace, separate rendering paths and a
mid-frame change of name table, vertical scroll and sprite attributes. SGDK's
`INTERLACED_MODE2` uses 8x16 pattern cells. Enabling it alone does not convert
ordinary fonts, sprite mappings or resource layouts into a working renderer.

Split Lab demonstrates two independent cameras without interlace or a second
sprite-table upload. It is a starting point for versus games, not a reproduction
of Sonic 2's display mode. Validate a dedicated interlace pipeline separately
if that exact appearance is required. Do not copy proprietary code or assets.

Sources: SGDK 2.11 [scroll API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_bg.h),
[scroll implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp_bg.c),
[MAP extraction](https://github.com/Stephane-D/SGDK/blob/v2.11/src/map.c),
[tile upload API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_tile.h),
[interlace API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h),
and preserved [Sonic 2 versus initialization](https://github.com/sonicretro/s2disasm/blob/15e9fd52a65318bec3d5e746ce48dba6db6910db/s2.asm#L4849)
and [H interrupt](https://github.com/sonicretro/s2disasm/blob/15e9fd52a65318bec3d5e746ce48dba6db6910db/s2.asm#L1181).
