# Math Lab

A small demoscene-style example: fly through a starfield while sixteen glowing
orbs trace a circle, figure eight or spiral. All motion uses bounded integers
and SGDK-derived fixed-point trig. There is no floating-point animation loop,
game engine or external art dependency.

## Build

Copy this folder to a new project folder, then run:

```sh
docker pull ewrogers/sgdk:latest
docker run --rm -v "$PWD:/workspace" ewrogers/sgdk:latest make release
```

Open `out/rom.bin` in your emulator. `make debug` builds a debug ROM and
`make clean` removes generated files. Use `make` here so the local compatibility
flags also work with older toolchain images. In PowerShell, replace `-v` with
`--mount "type=bind,source=$($PWD.Path),target=/workspace"`. Docker Desktop
should use Linux containers.

## Controls

| Input | Action |
| --- | --- |
| Left / Right | Circle, figure eight, spiral |
| Up / Down | Speed from 1 through 8 |
| A | Show or hide diagnostic text |
| B | Reset the seeded demonstration |
| Start | Pause or resume motion |

Input uses edges, so holding a button does not repeatedly change a setting.
The camera is fixed at 320x224. The nominal simulation runs at 60 updates per
second on NTSC and PAL; PAL occasionally takes two steps before rendering.
This does not compensate for missed VBlanks or make 50 Hz presentation look
identical to 60 Hz presentation.

## What to look for

- Stars grow brighter as they approach and recycle without a jump across the screen.
- The three formations change cleanly and remain inside the display.
- Pause freezes the stars, formations and sprite shimmer. Menus still respond.
- Reset produces the same sequence, and holding Reset does not restart each frame.
- Hiding metrics leaves the effect running. Compare the visible/hidden load.
- Try both NTSC and PAL and watch for missed frames or severe sprite dropout.

The load number is SGDK's `SYS_getCPULoad()` estimate, including rendering and
diagnostics. It is not an instruction profiler or a performance guarantee.
FPS comes from `SYS_getFPS()`, called once per rendered frame. The Hz label
identifies the display region; it is not a measured simulation rate.
The overlay refreshes every ten display frames. The ROM is intentionally silent.

## Budget and layout

There are 48 star sprites and 16 orb sprites: 64 hardware sprite entries in
total. Each is one hardware piece. A crowded scanline can still exceed H40's
20-sprite or 320-sprite-pixel limit even while the frame total is below 80.
Check the densest pattern; total sprite count alone is not enough.

`SPR_initEx(160)` reserves 5,120 bytes of sprite tile VRAM. The instances need
112 tiles at their largest frame: 48 one-tile stars and 16 four-tile orbs.
SGDK manages the additional rendering buffers and SAT. Text uses the standard
font. PAL0 is reserved for text/backdrop, PAL1 for stars and PAL2 for orbs.
No HBlank effects or mid-frame palette writes are used.

Shared tables use 1,232 bytes of RAM: 720 for sine and 512 for reciprocal
depth. The fixed model and sprite pointers take additional RAM, as does SGDK.
Use the matching ELF/map for complete totals; this is not a claim that the
whole ROM uses only those bytes. The padded ROM is 128 KiB and uses no SRAM
or mapper.

`src/demo.c` owns the portable simulation. `src/main.c` initializes the actual
SGDK tables, samples input and renders the model. The host tests use independent
trig values; separate M68000 execution tests check the selected SDK routines.
The target uses SGDK's native `muls` helper for bounded 16-bit products and
conditional wrapping for bounded angles. All sprites remain fully on screen,
so the renderer skips redundant visibility calculations. HUD updates use the
VBlank DMA queue.
The original indexed sheets can be regenerated into a new folder with
`python3 tools/make_art.py new-art`; it refuses to overwrite source art.

This folder is an ordinary standalone SGDK project, not a framework requirement.
Its fixed counts, near plane, seed and formation choices are demonstration
policies that a game can change.
