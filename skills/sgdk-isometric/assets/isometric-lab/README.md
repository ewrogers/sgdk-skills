# Isometric Lab: Lantern Aqueduct

Walk through a floating garden, climb its stairs and cross the raised aqueduct.
Then walk beneath it: elevation changes which surfaces should cover the player.
Original native pixel art and a quiet original FM garden theme illustrate the
technique, without prescribing the look or sound of another game.

## Build and play

From this Lab directory:

```sh
docker pull ewrogers/sgdk:latest
python3 tools/build.py --output /path/to/new/isometric-lab --platform linux/arm64
```

Use `python` on Windows and `linux/amd64` for an x64 container. Omit platform for
Docker's native choice. `--mode debug`, `--region pal` and `--region ntsc` are
available. The helper refuses existing destinations and creates a standalone
project with art/music generation and no external Python dependencies.
Open `out/rom.bin` in ares as Mega Drive.

| Control | Action |
| --- | --- |
| D-pad | Walk in screen directions; diagonals are supported |
| A | Tour garden → stairs → bridge → underpass |
| B | Cut away the upper deck and stairs to inspect the underpass |
| C | Toggle geometry/status information |
| Start | Pause / resume, including music |

The stairs are at both ends of the bridge. Approach their foot along the bridge
axis. The upper walkway has edges you cannot fall from; the lower teal avenue
passes beneath its center. The hero's collision footprint is smaller than the
art, so pillars and garden objects do not require pixel-perfect alignment.

`iso.c` provides projection, known-height inverse projection, projected-box
relations and bounded topological sorting. `world.c` provides this Lab's surfaces,
steps, ground collision, tour points and logical clock. `main.c` is the SGDK
adapter. They are original C algorithms, not built-in SGDK scene/physics APIs.

## What is being demonstrated

- 32×16 diamond projection, foot anchors and a constrained following camera.
- An explicit ground/upper surface, four-riser stairs and a genuine underpass.
- Projected 3-D bounds and a bounded far-to-near ordering graph, with stable IDs.
- Cached relationships for 15 static scenery objects plus one moving character.
- Separately sortable deck/pillars, cutaway presentation and animated art.

The general query code permits 24 items and 24 explicit links. Those are example
work budgets, not SGDK or hardware scene limits. Invalid input is rejected;
interpenetrating proxies and graph cycles return diagnostics with a deterministic
fallback. The stepped surface owner resolves its actor/support overlap explicitly.
The example does not implement jumping, falling or a general 3-D physics engine.

The base-M68000 cache uses 436 bytes; the game state uses 22. The retained HUD is
480 bytes. Compiled art uses 269 ground tiles and 278 tiles of maximum sprite
instance allocations under a 320-tile reservation. Four palette lines are used.
The 17 logical sprites include multi-piece tall scenery and a hidden debug
marker. Tests check the actual layout's conservative scanline budget. DMA shares
a configured 4,096-byte allowance; startup warms sprite uploads before display.

Art and music are original and generated from the included Python sources. All
pixels are authored at native resolution, with consistent 3-bit RGB palettes;
no API key, downloads or image library are needed. The font has an opaque tile
background so scenery cannot bleed through the HUD. This is one small scene's
art direction, not a required style for other isometric games.

Repository tests cover 6,000 projected pairs against an independent convex-hull
oracle, full/cached ordering agreement, staircase boundaries, ground collision,
20,000 movement steps and nominal PAL/NTSC/pause schedules. Native ARM64/AMD64
container checks build release/debug and region variants, execute base-M68000
contracts and a 128-step digest, and verify compiled geometry, palette/font data
and scanline limits. Those checks do not emulate VDP/audio timing or certify
real hardware performance.

## Emulator check

On October 16, 2026, the user tested Lantern Aqueduct in ares v148 (NTSC) and
confirmed the art/music, walking behind scenery, stair/bridge/underpass tour,
cutaway, info and pause look and work well. PAL presentation and physical
hardware remain separate checks; the automated PAL coverage is build/logic only.
