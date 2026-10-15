# Gameplay Lab

A small scrolling target range. Move a blue square through the corridors,
shoot the gold targets and reset to play again. It demonstrates game logic
without requiring a particular engine architecture.

Copy this folder to a **new** project directory, then build there:

```sh
docker run --rm --pull=always --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/sgdk:latest build
```

In PowerShell, replace `$(pwd)` with `${PWD}` and use Docker's Linux-container
mode. The image supports ARM64 and AMD64. Load `out/rom.bin` in a Mega Drive
emulator; `build debug` adds symbols and `build clean` removes generated output.

## Controls

| Button | Action |
| --- | --- |
| D-pad | Move; diagonal movement keeps approximately the same speed |
| A, held | Fire in the last movement direction |
| B | Reset targets, bullets and player position |
| Start | Pause or resume simulation |

The first wall has a passage below it; the second has a passage above it.
The camera follows horizontally, while the HUD stays fixed. `Full` counts shots
refused when all twelve bullet slots are occupied. This is a lab capacity, not a
console limit. Firing along a long clear corridor can fill it; expired bullets
and wall hits make slots available again.

## How it is split

`src/game.c` owns movement, boxes, wall queries, bullet admission and camera
bounds. `src/main.c` adapts SGDK input, sprites, scrolling and the frame loop.
`inc/game.h` holds the small game state. Host tests execute the same game model;
its host-only type aliases are not extra runtime dependencies on the console.

Positions use SGDK's `fix32` scaling. The nominal simulation rate is 60 ticks:
NTSC takes one step per display frame; PAL occasionally takes two. Rendering
and SGDK sprite updates happen once per display frame. This does not recover
arbitrary dropped VBlanks or normalize automatic animation holds. The lab has
static sprite frames. Pause freezes bullet lifetime and fire cooldown as well
as motion; B can reset even while paused.

The world fits one preloaded 128x32-tile plane. Its collision cells are 32x32
pixels, made from ordinary 8x8 graphics tiles. The HUD uses Window. This is not
a map-streaming or platformer physics example.

All nineteen SGDK sprites are allocated during setup: player, six targets and
twelve bullet visuals. Unused/offscreen visuals are hidden, but logical bullets
keep moving. Hidden sprites still reserve their sprite-engine resources. The
VRAM reservation sums each definition's `maxNumTile` for these instances.

The original shapes share one art palette; backgrounds use palette 0 and sprites
use palette 1 with those same colors. Palette 3 belongs to the HUD. From the
installed skill directory, `python3 scripts/make_lab_art.py NEW_DIRECTORY`
recreates the PNGs without third-party libraries and refuses to overwrite art.

## Playtest checklist

1. Move and hold diagonal input. Stop against walls from both directions and
   slide along a corner. The player should never enter a solid cell.
2. Travel through both passages. The background and targets should stay aligned
   with collisions, the HUD should stay fixed, and the camera should stop at the
   two world edges.
3. Shoot a target and keep firing through its old position: it should score
   once. Bullets should disappear at walls or when their lifetime expires.
4. Hold fire along a clear corridor. Check full-pool refusal, then move or wait
   for slots to become available. There should be no allocation fault.
5. Pause with bullets in flight, wait, then resume. Reset and confirm that all
   targets return and the counters and camera reset.
6. Try both NTSC and PAL. Compare elapsed travel/cooldown time rather than
   expecting the same number of displayed frames. Measure optimized builds
   separately before drawing performance conclusions.

Automated tests cover game-state behavior, asset reproducibility and real SGDK
release/debug builds. They do not prove hardware timing or replace this visual
checklist.
