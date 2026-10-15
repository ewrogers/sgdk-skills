# Split Lab

Two independent cameras follow blue and red robots through the same stone
outpost. The red robot follows a CPU route by default, so one controller is
enough to try both views. Art and map are original work reused from Level Lab;
this folder builds on its own without another installed skill. This ROM is silent.

Copy this folder to a **new** directory, then build there:

```sh
docker run --rm --pull=always --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/sgdk:latest build
```

Use `${PWD}` in PowerShell and Docker's Linux-container mode. Load `out/rom.bin`
in a host Mega Drive emulator. `build debug` adds debug information;
`build clean` clears generated output. The image supports ARM64 and AMD64.

## Controls

| Control | Action |
| --- | --- |
| P1 D-pad | Move the blue robot and upper camera |
| P1 A | Toggle the red robot between CPU and P2 control |
| P2 D-pad | Move the red robot and lower camera when CPU is off |
| P1 B | Reset positions, preserving CPU and pause choices |
| P1 Start | Pause/resume both players |

In the usual ares mapping A/B/C are J/K/L and Start is Return. The lab does not
change mappings. It demonstrates cameras, not wall collision: robots can walk
through the map's stone walls.

## What to look for

- Move P1 while watching the red robot's independent lower view. A robot can
  appear in both views when their cameras overlap.
- Reach the world's edges. Cameras should clamp while robots keep moving to
  the edge. Sprites disappear wholly when crossing a viewport edge; they are
  not cropped into partial sprites.
- Toggle CPU, try P2, pause and reset. Check the divider stays black and each
  HUD stays fixed. `LATE` and `TIMEOUT` should stay zero; increasing values mean
  investigate the raster setup before using it in a game.
- Repeat in actual detected PAL and NTSC modes. The model advances at 60 nominal
  steps per second in either region, with fewer displayed steps in PAL.

## Display and costs

This is normal 320x224 noninterlaced output, not Sonic 2's double-resolution
interlace mode. Both cameras share a fully resident 512x512 world: 8 KiB of
BG_A name-table VRAM plus its graphic tiles. There is no map streaming during
play. Larger independent scrolling worlds need a separate cache design.

One H interrupt changes vertical scroll and Window HUD placement at the middle.
VBlank restores the upper view. A persistent horizontal table costs 448 bytes
of RAM and 448 bytes of DMA per displayed frame. Ten 32x8 black divider sprites
mask the transition; four actor sprite projections serve the two views. All
four palettes are assigned to terrain, blue robot, red robot and HUD/divider.

The divider alone uses the H40 limit of 320 sprite pixels on its scanlines.
Actor projections are culled away from it. Both views share the normal sprite,
palette and VRAM limits; splitting the display does not double those budgets.

`split_model.c` handles players, cameras, cadence and clipping. `split_video.c`
owns the interrupt and register handoff. `main.c` loads the resident world and
projects sprites. Small inline 68000 assembly bounds the HBlank wait and writes
prepared registers; ordinary game logic stays in C.

Host tests cover camera arithmetic, region cadence, clipping, ownership and
failure paths. Container checks compile real APIs and inspect interrupt code.
Neither proves a stable seam on a physical console. Record emulator and hardware
observations separately, including the actual video region.
