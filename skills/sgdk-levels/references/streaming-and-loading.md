# Streaming and loading

## A world is larger than its backing plane

A 64x32-tile plane holds 512x256 pixels of tilemap entries. It can act as a
ring buffer for a much larger world. SGDK's MAP code replaces entering rows
and columns as the camera moves. It streams tilemap entries, not an unlimited
library of new pattern graphics.

The lab preloads its small tileset once. All world tiles use that resident art.
A larger collection of unique patterns needs a separate residency strategy
and budget. A large world made from repeated tiles is different from a large
world made from unique full-screen images.

SGDK 2.11's MAP indexing uses power-of-two masks for its block coordinates.
Prefetch can read beyond the camera's visible rectangle. Use suitably padded
storage or establish another safe edge strategy; do not assume a visually
clamped non-power-of-two map is safe. Level Lab uses 8x8 blocks (1024x1024px).
The playable area could be smaller than padded storage if collision and
camera bounds describe that choice separately.

## Set up ownership before loading

Finish screen/plane layout before creating the map and reserving graphics.
The lab uses BG_A for the world, Window for the bottom HUD, PAL0 for terrain,
PAL1 for its robot and PAL3 for text. PAL2 remains free. Palette-line ownership
is an example choice, not a requirement that every level spend three lines.

Initialize sprite storage, then check that the terrain's exclusive end fits:

```c
SPR_initEx(scout_image.maxNumTile);
if ((u32)TILE_USER_INDEX + outpost_map_tileset0.numTile > TILE_USER_MAX_INDEX) {
    /* Terrain would overlap the sprite reservation. */
}
```

In this SGDK layout, `TILE_USER_MAX_INDEX` is the first reserved sprite tile,
despite the name suggesting an inclusive last index. A range ending exactly
there is valid. Fonts, sprite patterns and VDP tables also consume VRAM;
there are not 2,048 freely available pattern slots alongside those tables.

Load the tileset, upload its palette, then create the map with a matching
base tile and palette attribute:

```c
if (!VDP_loadTileSet(&outpost_map_tileset0, TILE_USER_INDEX, DMA)) {
    /* Clean up and report a loading failure. */
}
Map *map = MAP_create(&outpost_map, BG_A,
    TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX));
if (!map) {
    /* Allocation failed; keep the game in a usable loading/error state. */
}
```

Do not apply a tile offset twice if the resource already bakes one in. Keep
base priority low unless you intentionally account for authored tile priority;
the selected SDK warns about combining those attributes.

Uncompressed map data stays in ROM, but the Map working structure and transfer
buffers still use RAM. Compressed components can require unpacked storage at
creation. Account for those allocations separately from collision data.

## Scroll once after simulation

Keep camera coordinates signed while clamping, then pass nonnegative pixel
positions to `MAP_scrollTo` or `MAP_scrollToEx`. Passing a negative value to
their unsigned arguments is not an edge clamp.

```c
MAP_scrollToEx(map, camera_x, camera_y, force_redraw);
SPR_update();
SYS_doVBlankProcess();
```

One MAP call per displayed frame is enough after all movement steps. Avoid
also setting conflicting scroll registers for that plane. The first draw or a
forced teleport can replace the whole backing tilemap. A 64x32 plane has
4,096 bytes of entries; this is a larger transfer than an ordinary entering
row or column. Do not initialize multiple full maps in one VBlank without
budgeting or staging the work.

The lab blanks the display for initial/teleport redraws and flushes the queued
work before showing the result. That is a simple loading policy, not a
seamless-transition promise. Its normal scrolling uses incremental updates.

## Release only after transfers finish

MAP queues rendering work using DMA temporary storage and scroll state.
Some modes can retain pointers to map-owned data until VBlank. Drain pending
work before freeing the map or reusing source memory:

```c
SYS_doVBlankProcess();
MAP_release(map);
SPR_end();
```

A real transition should first stop old gameplay/rendering producers, then
drain, release and load the next owners. Level Lab tests the teardown order and
failure cleanup with a recording SDK fixture. It does not show a multi-level
game or prove DMA timing on a physical console.

## Sources

- [SGDK 2.11 MAP API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/map.h)
  and [streaming/indexing implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/map.c).
- [DMA buffers and queue ownership](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h).
- [Sprite reservation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sprite_eng.c)
  and [VDP layout boundaries](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp.c).
