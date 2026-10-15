# Level Lab

Explore a small abandoned outpost with a robot scout. The world is a
1024x1024 Tiled map, larger than both the screen and its backing plane.
Walls block movement, the camera follows within its limits, and a fixed HUD
shows camera coordinates. The lab is silent.

## Build

Copy this folder to a **new** project directory. From that directory, import
collision and build through the published toolchain:

```sh
docker run --rm --pull=always --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/sgdk:latest sh -c 'python3 tools/tiled_collision.py res/outpost.tmx && make -f "$GDK/makefile.gen"'
```

In PowerShell, replace `$(pwd)` with `${PWD}` and use Docker's Linux-container
mode. The image supports ARM64 and AMD64. Append `debug` after the makefile
path for a debug build. Load `out/rom.bin` in a Mega Drive emulator.

For later builds, run the same command after editing the map. `build clean`
removes SGDK output; the collision helper regenerates its two ignored C files.
No host SGDK, Tiled installation or Python package is required to build.
Install Tiled separately if you want to edit the supplied TMX/TSX visually.

## Controls

| Button | Action |
| --- | --- |
| D-pad | Move, including diagonals |
| A | Return home |
| B | Warp to the next corner of the outpost |
| Start | Pause/resume movement |

The usual ares mapping is arrows/WASD, J for A, K for B and Return for Start.
Warp/home remain available while paused so you can inspect camera edges.
The lab does not change emulator settings.

The robot faces the direction of travel, with a mirrored side view for left.
It walks while moving and has a subtle idle bob when stopped. Both animations
use simulation ticks and freeze during pause. Diagonal movement faces sideways.
The sheet has three direction rows (down, right, up), with four walking frames
followed by four idle frames per row. SGDK automatic playback is disabled;
the renderer selects the frame from the simulation phase.

## Edit the level

Open `res/outpost.tmx` in Tiled. `Ground` is the visual tile layer imported by
ResComp. Hidden `Collision` marks full solid cells; nonzero tiles are solid,
zero is clear. Update both when changing a wall. Keep Collision top-level,
unshifted, 64x64 and CSV encoded for this example.

The robot starts at (64,64). Its four warp points are near the four corners.
If moving walls, keep those points clear or update `src/main.c` too. The PNG
sheet contains 16px cells; the underlying hardware patterns are still 8px.

`tools/tiled_collision.py` writes a 512-byte const bitset in
`src/level_data.c`, declared by `inc/level_data.h`. It validates collision
geometry/IDs before writing output and preserves previous files on an input
error. Generated C/header/build files are ignored; edit the TMX instead.

## How it is split

- `src/world.c` handles world-space collision, movement, timing and camera bounds.
- `src/level_view.c` owns SGDK map/sprite resources, HUD drawing, redraws and cleanup.
- `src/main.c` samples input once per frame and runs the small lab loop.

The 64x32 hardware plane acts as a 512x256 ring buffer. MAP streams its entries;
the small pattern tileset stays loaded in VRAM. Window owns the bottom four
rows, leaving a 320x192 world viewport. Large warps blank and redraw the map.
This is not a multi-level transition, general physics or pattern-streaming demo.

PAL runs the same nominal 60-step movement using occasional extra simulation
steps per 50Hz display frame. Rendering remains once per display frame. The
lab does not recover arbitrary missed frames or promise the same display rate.

## Art and palette

The terrain and robot were drawn in PixelLab's exact pixel workbench at their
final sizes. Each terrain cell and the robot are 16x16 pixels, with no resizing,
anti-aliasing, dithering or lossy color reduction. Terrain uses PAL0,
the robot PAL1, and the HUD PAL3; PAL2 is free. The sprite's index 0 is
transparent. The other palette lines also reserve their index 0 for transparency;
the global backdrop remains separate from ordinary visible tile colors.

`res/palette.json` records the exact VDP-compatible colors: charcoal/stone for
terrain and silver/copper for the robot. Unused slots are black. The robot
also has an opaque black outline entry separate from transparent index 0.
The finished lab art is provided under this repository's MIT license.
PixelLab drawing IDs for provenance:

- Terrain: `358bd6bf-a947-4a0a-b64a-7e04a4b9d555`.
- Robot walking/idle sheet: `288ec046-558c-4309-97c7-1128e826a3be`.

## Playtest checklist

1. Move into the outside walls and interior barriers. Slide along corners and
   find the passages through the first vertical wall and the lower wall.
2. Walk far enough horizontally and vertically to cross the backing plane's
   512px/256px spans. Reverse direction and check for stale strips or seams.
3. Press B through all four corners. The camera limits should reach X=704
   and Y=832 at the far edges, with the correct part of the map redrawn.
4. Confirm the robot stays aligned with walls and the HUD remains fixed.
   Return home with A; pause, wait and resume without a movement jump.
5. Try NTSC and PAL. Compare travel time rather than displayed frames.
   Check console readability and transfer timing when hardware is available.
6. In a disposable copy, remove a Collision cell in Tiled, rebuild, and verify
   the robot can enter it. Restore it and verify the wall blocks again.

Host tests execute the importer, collision/camera model and a recording SGDK
adapter. Container checks compile the actual Tiled resources and both ROM
build modes, then compare linked collision bytes with the source. They do not
emulate the VDP, measure frame rate or prove hardware behavior.
