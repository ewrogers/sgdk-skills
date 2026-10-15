# Integrating with SGDK

Checked against SGDK 2.11 in `ewrogers/sgdk:latest`. Check the selected image's
headers and resource manual when upgrading; do not invent a similarly named
helper if an API has changed.

## Input, rendering and sound

| Job | Actual SGDK interface used |
| --- | --- |
| Cached pad snapshot | `JOY_readJoypad(JOY_1)` and `BUTTON_*` constants |
| Region and frame boundary | `SYS_isPAL()`, `SYS_doVBlankProcess()` |
| Diagnostics | `SYS_getFPS()` once per displayed frame, `SYS_getCPULoad()` |
| Streamed background | `MAP_create()`, `MAP_scrollToEx()`, `MAP_setDataPatchCallback()` |
| Shared resident sprite frames | `SPR_initEx()`, `SPR_loadAllFrames()`, `SPR_addSpriteEx()` |
| Frame/position selection | `SPR_setFrame()`, `SPR_setVRAMTileIndex()`, `SPR_setPosition()`, `SPR_setHFlip()` |
| Fixed HUD | Window plane, `VDP_setWindowVPos(FALSE, 4)`, `VDP_drawTextEx(..., DMA_QUEUE)` |
| Music and effects | `XGM2_load()`, `XGM2_playTrack()`, `XGM2_pause()`, `XGM2_resume()`, `XGM2_playPCM()` |

Sources: [joy.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/joy.h),
[sys.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h),
[map.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/map.h),
[sprite_eng.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h),
[vdp.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h),
[vdp_bg.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_bg.h),
[snd/xgm2.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/xgm2.h), and
[ResComp manual](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt).

## Keep art at its native size

The original assets are drawn directly as indexed pixels: 16×24 hero frames,
16×16 enemies/items and 16×16 logical terrain cells. They are not reduced from
larger illustrations. The generator uses only 16 entries per sheet, with entry
zero transparent for sprites. PAL0 is stone terrain, PAL1 the hero, PAL2 the
shared enemy/item colors and PAL3 the HUD. Color components use the eight
quantized levels; final display appearance can still vary between emulators
and consoles.

The VDP's hardware tile is always 8×8. ResComp converts the logical map cells
and sprite dimensions into hardware tiles/pieces. A 16×24 hero is not a new
hardware tile size. Inspect compiled resources and scanline pressure after
changing dimensions or animation frames.

## Own the VRAM reservation

The lab reserves 192 sprite tiles, or 6,144 bytes of VRAM, and preloads every
frame once. Instances share these indices. `SPR_addSpriteEx(..., flags=0)`
leaves allocation/upload ownership with the cache. Set frame geometry before
applying the matching cached tile index. Do not mix automatic sprite tile
allocation into the same reserved area: the allocator does not know which
manual frames are live.

The retained cache contains 60 hero tiles, 80 enemy tiles and 52 item/effect
tiles before any SDK deduplication. Check the actual upload count against the
reservation. `SPR_loadAllFrames()` returns a temporary allocated index table;
copy the needed entries and free the table after upload. Allocation is setup
work, not a per-tick operation.

Terrain tiles must end before `TILE_USER_MAX_INDEX`, which reflects the sprite
reservation. Font tiles and VDP tables also occupy VRAM. Do not add these
budgets as though each subsystem owns the full 64 KiB.

## Streaming and terrain edits

The Tiled map has 128×32 logical 16-pixel cells, padding the active 80×18 course
with empty cells. This backs MAP's prefetch beyond the playable camera edge.
The simulation clamps the camera separately to 0..960 and 0..64 pixels.

`MAP`'s patch callback receives **8×8 hardware tile coordinates**. Divide by two
to identify the logical cell. Row updates advance X; column updates advance Y.
Removing a covered or broken cell must affect all four entries and future
streamed copies, not just the current on-screen quadrant. See the callback
sites in [map.c](https://github.com/Stephane-D/SGDK/blob/v2.11/src/map.c).

Normal movement streams only new rows/columns. A checkpoint teleport blanks
the screen while a full redraw drains. An occasional terrain edit forces one
plane refresh without blanking. Keep queued source buffers alive until VBlank
consumes them. Before releasing a MAP in a different game flow, drain its DMA
work, then call `MAP_release()`.

The plane is 64×32 hardware entries: a full refresh is 4,096 tilemap bytes,
plus sprite table, scroll and changed HUD data. Measure the combined peak
against the selected SDK DMA capacity and VBlank time. A completion banner has
its own one-time HUD writes; do not upload it every frame.

## Keep budgets separate

The lab has 23 logical sprite instances, each fitting one hardware sprite:
hero, five enemies, ten pickups, two platform halves, three checkpoints, boots
and sparks. The same ten pickup slots become celebration particles. H40's
hardware limits are 80 sprites per frame, 20 per scanline and 320 sprite pixels
per scanline; a safe total count does not prove a safe scanline. Culling removes
fully offscreen sprites; the VDP clips the remaining partial footprints.
See [Plutiedev](https://plutiedev.com/) for hardware background.

The model is fixed storage with a 180-byte broken-cell bitset. MAP, sprite
records, DMA buffers, audio state and stack are additional RAM. Read ELF section
sizes and inspect the warmed-up heap rather than estimating total RAM from
`sizeof(Game)` alone. The cartridge image also includes the SDK runtime, audio
driver, tilemaps, sprite data and music/samples; 128 KiB padding is a build policy,
not the amount of game data.

The HUD's load value is SGDK's estimate, not a cycle-accurate profiler or a
promise of spare VBlank bandwidth. Test release builds at the busiest location,
with scrolling, charging, terrain edits and audio together. Use the PAL
one/two-step cadence too. No assembly optimization is required merely because
the target is a 68000; first find repeated work and unnecessary transfers.
