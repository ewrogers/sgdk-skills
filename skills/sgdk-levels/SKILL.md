---
name: sgdk-levels
description: Build SGDK scrolling levels from Tiled maps, with map streaming, separate collision data and camera boundaries. Use for worlds larger than the visible plane, TMX/TSX import and level loading or cleanup, rather than isolated sprite animation or general movement code.
---

# SGDK Levels

Keep three things separate: the world's graphics, its gameplay data and the
camera that shows part of it. ResComp can optimize visual tiles without
changing what a wall or doorway means to the game.

## Choose the right reference

- [Importing Tiled maps](references/tiled-import.md): TMX/TSX resources,
  visual layers, collision data and useful import failures.
- [Streaming and loading](references/streaming-and-loading.md): MAP setup,
  world/plane sizes, VRAM ownership, teleports and cleanup.
- [Collision and cameras](references/collision-and-cameras.md): world-space
  occupancy, fractional boundaries, sliding and viewport limits.
- [Level Lab](assets/level-lab/README.md): an original top-down outpost with
  a streamed Tiled map, robot, solid walls, camera bounds and fixed HUD.

Use `ewrogers/sgdk:latest`, pulled before validation. The references are verified
against SGDK 2.11. Recheck the selected image's `inc/map.h`, `src/map.c`,
`bin/rescomp.txt` and relevant sprite/VDP headers before changing unfamiliar
calls or promising additional formats. Do not invent a generic SGDK scene,
physics or entity importer.

## Pick a small authoring contract

Agree on map orientation, cell size, visible layer names, collision source,
spawn positions and camera viewport. Start with finite orthogonal CSV maps
and square cells when that fits the game. The lab uses 16px cells made from
four 8px hardware tiles; these are different units.

Resolve external TSX files and their image paths. Keep source maps editable in
Tiled. Collision can come from tile properties, occupancy layers or objects,
but choose one convention and validate it. The lab supplies a portable helper
for a named occupancy layer; it is not a universal Tiled parser.

## Budget the level and its working set

MAP streams tilemap entries into a backing plane; it does not automatically
stream every tileset pattern into VRAM. Reserve the art, font, sprite tiles
and tables separately. Check map creation and rendering allocation failures.
Account for compressed-map unpacking in main RAM as well as ROM size.

Check SGDK's block indexing and prefetch behavior before choosing storage
dimensions. A clamped camera alone does not prove non-power-of-two storage is
safe. The lab uses a 1024x1024 world and a 64x32-tile backing plane so it can
cross both ring-buffer boundaries without relying on unsafe edge reads.

## Prove the whole route

Move through walls and passages, scroll beyond both backing-plane dimensions,
reverse direction and visit every world edge. Test large camera jumps while
keeping the HUD fixed. Blank or stage expensive initial redraws; flush queued
work before releasing its map or source buffers.

Test collision import and failures, edge-touching boxes, camera bounds and
resource cleanup. Build the real Tiled resources and ROM, then playtest the
result. Host tests and successful compilation do not prove VDP timing,
sprite occlusion or hardware readability. Adapt the game's architecture
rather than requiring the lab's small module layout.
