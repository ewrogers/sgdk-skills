# Cameras and frame budgets

## Keep the world still

Store an object's position in world coordinates. The camera determines which
part is visible; rendering subtracts the camera to get screen coordinates.
Scrolling the background must not change collision positions.

A simple follow camera centers the player and clamps to the map edges. For a
world smaller than the viewport, clamp the maximum offset to zero as well.
A dead zone can reduce camera jitter; interpolation or look-ahead are optional
game choices. Apply one final camera position per rendered frame.

```c
s32 screen_x = world_x - camera_x;
/* Check range and visibility before narrowing for SPR_setPosition(). */
```

The lab follows horizontally in a 1024x224 world and clamps to 0..704. It
preloads a 128x32-tile plane and changes the plane's scroll offset. It does not
stream an arbitrarily large map. The Window covers the top four tile rows for
a HUD and replaces that portion of plane A; it is not a third scrolling plane.

For larger maps, load a checked tileset range, create a `Map` with `MAP_create()`
and call `MAP_scrollTo()` with your chosen camera coordinates. Check creation
failure. The map streamer uploads tilemap changes and adjusts scrolling; it
does not move sprites, simulate offscreen enemies or implement collision.
A large jump can queue much more DMA than an ordinary scroll.

## Decide what offscreen means

Culling drawing is separate from stopping simulation. A bullet outside the
viewport may still hit something or expire; hiding it must not make it immortal.
Likewise, turning away should not silently freeze an enemy unless that is the
game's chosen behavior.

For a long level, activate nearby objects from compact level records and reuse
slots when their lifetime ends. Budget a margin beyond the camera so enemies
can enter naturally. Define which persistent state survives deactivation, and
avoid creating the entire level's dynamic population just to hide most of it.

## Account for separate costs

- **Main RAM:** logical records, pools, pointers, contact state, stack, SGDK and
  audio overhead. Use target `sizeof` or linker data, not host pointer sizes.
- **VRAM:** patterns and all plane/Window/sprite/scroll tables share 64 KiB.
  A 128x32 plane table uses 8,192 bytes; that is not its art-pattern storage.
- **Sprites:** H40 permits 80 hardware sprites total, with 20 and 320 sprite
  pixels per scanline. An object may use multiple hardware pieces. Transparent
  padding is still evaluated; game capacity does not prove scanline safety.
- **CPU/DMA:** broad-phase queries, live-object updates, sprite processing,
  decompression and transfers. Count worst-case overlaps and camera jumps,
  not just an empty scene or average object count.

For a few bullets and targets, a bounded nested loop is easy to reason about.
As counts grow, query tile cells, spatial buckets or another simple broad phase
before testing candidate boxes. Choose this based on measured work; do not add
an elaborate structure simply because it is fashionable.

Preload art where practical, avoid repeated resource loading in the loop, and
keep debug text/event spam out of a performance baseline. `SYS_getFPS()` and
`SYS_getCPULoad()` are useful SGDK measurements, not a universal frame-time
profiler. Inspect their implementation and compare optimized builds under the
same emulator settings. Emulation speed percentage is not the game's CPU load.

## Sources

- [SGDK 2.11 map API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/map.h)
- [Map streaming implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/map.c)
- [Plane layout and Window API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h)
- [Scrolling API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_bg.h)
- [Sprite hardware definitions](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_spr.h)
- [Profiling API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h)
