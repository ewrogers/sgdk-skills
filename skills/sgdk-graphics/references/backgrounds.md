# Backgrounds and scrolling

Use planes for scenery and repeated tiles. Moving a plane changes its scroll
offset; it does not spend one hardware sprite per tile.

## Give each plane a job

The VDP has scrolling planes `BG_A` and `BG_B`. The Window replaces part of
plane A with fixed content. It is useful for a HUD, but is not a third full
independent scrolling background.

A simple layout is foreground on A, distant scenery on B, and a Window HUD.
Transparent tile pixels let the plane behind show through. Tile and sprite
priority bits affect overlap; do not assume draw-call order defines depth.

Call `VDP_setPlaneSize(width_tiles, height_tiles, TRUE)` during setup, before
loading resources. It changes table placement and the available tile range.
Do not change the layout after loading and expect existing VRAM allocations
to remain valid. A 64x32 plane is 512x256 pixels and its tilemap takes 4,096 bytes.

VRAM is 64 KiB shared by tile patterns, both plane tables, Window, sprite and
scroll tables, font and sprite-engine reservation. Each 8x8 pattern takes 32
bytes; each tilemap entry takes two. The 2,048 theoretical pattern slots are
not all available for art once tables and reserved data are present.

## Load once, scroll often

After layout and sprite initialization, allocate static images from
`TILE_USER_INDEX` through the inclusive `TILE_USER_MAX_INDEX`. Advance by each
resource's `tileset->numTile`, not its image area: ResComp may deduplicate tiles.
Use widened arithmetic when checking the exclusive end of a range.

```c
/* During setup, with the display disabled and a checked tile range. */
bool loaded = VDP_drawImageEx(BG_B, &scenery,
    TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, first_tile),
    0, 0, FALSE, FALSE);

/* In the frame loop, for a small wrapping background. */
VDP_setHorizontalScrollVSync(BG_A, -near_offset);
VDP_setHorizontalScrollVSync(BG_B, -far_offset);
```

`VDP_drawImageEx()` loads graphics and tilemap, and can unpack compressed data.
Check its result, but do not treat it as a VRAM bounds checker. The last two
arguments here disable automatic palette loading and DMA during setup.
Loading an image every frame is not necessary for scrolling.

Horizontal negative offsets move scenery left. Vertical positive offsets move
it up. Keep world coordinates separate from wrapped, signed scroll offsets.
For parallax, move the distant plane more slowly. Preserve enough phase before
dividing, so a slower plane does not jump halfway through its own wrap.

Use the `VSync` setters for whole-plane offsets that should be applied during
VBlank. The immediate setters are useful in controlled display-off setup;
they are not interchangeable with queued frame updates.

## A map larger than the plane

Scrolling alone wraps the tiles already in VRAM. For a larger level, use
ResComp `TILESET` and `MAP` resources and SGDK's map streamer:

```c
Map *level = MAP_create(&level_map, BG_A,
    TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, level_tile_base));
if (level == NULL) { /* Handle allocation failure. */ }
else MAP_scrollTo(level, camera_x, camera_y);
```

Load the referenced tileset and palette into their checked ranges first.
`MAP_scrollTo()` updates the visible tilemap through queued DMA and adjusts
scrolling. It does not load arbitrary new pattern graphics, manage your camera
bounds, or implement collision physics. Choose valid coordinates for your game
and inspect the resource format when using map data for collision.

The initial map update or a large camera jump can upload much more than a small
scroll. A full 64x32 plane table alone is 4 KiB. Stage loading across VBlanks
with the display hidden when necessary, especially for two maps. On teardown,
finish transfers that use map data before `MAP_release(level)`.

## Line and column effects

For horizontal bands or waves, choose `HSCROLL_TILE` or `HSCROLL_LINE` with
`VDP_setScrollingMode()`, then use `VDP_setHorizontalScrollTile()` or
`VDP_setHorizontalScrollLine()`. Vertical column scrolling uses `VSCROLL_2TILE`
and `VDP_setVerticalScrollTile()`, with 16-pixel column granularity.

Use persistent scroll buffers or `DMA_QUEUE_COPY`; a stack array cannot back an
ordinary deferred transfer. Budget the buffer's RAM, its transfer size and the
CPU to fill it. Start with whole-plane parallax if per-line effects are not
needed. HBlank register effects are a separate timing-sensitive implementation,
not a prerequisite for scrolling backgrounds.

## Sources

- [SGDK 2.11 plane and scrolling API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_bg.h)
- [VDP layout and tile ranges](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h)
- [Map API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/map.h)
- [Map streaming implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/map.c)
- [ResComp TILESET and MAP formats](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt)
