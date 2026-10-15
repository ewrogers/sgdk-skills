# Raster Lab

A scrolling stone outpost with a Sonic-style underwater tint and ripples.
The robot moves through the surface so you can see its colors change while
the HUD stays dry. Art/map are reused from this repository's original Level
Lab. Original animated foam, bubbles, splashes and a slow FM pad/bell phrase
complete the water scene. No Sonic assets or music are included, and no other
installed skill is required.

Copy this folder to a **new** directory, then build from that directory:

```sh
docker run --rm --pull=always --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/sgdk:latest build
```

Use `${PWD}` in PowerShell and Docker's Linux-container mode. The image supports
ARM64 and AMD64. Load `out/rom.bin` in a host Mega Drive emulator. `build debug`
adds debug information; `build clean` clears generated output.

## Controls

| Control | Action |
| --- | --- |
| Up/Down | Move the waterline between lines 64 and 192 |
| Left/Right | Pan through the original map |
| A | Toggle the underwater palette switch |
| B | Toggle per-line ripples |
| C | Stop/restart the music |
| Start | Freeze/resume wave phase, robot movement and camera controls |

In the usual ares mapping A/B/C are J/K/L and Start is Return. Music continues
while the visual model is paused; use C to silence it. Toggles remain available
while paused so you can compare a stable scene. The lab does not change mappings.

## What the surface costs

PAL0/PAL1 hold the map/robot colors. The handler changes those same banks below
the surface; PAL2's water effects and PAL3's HUD stay unchanged. The scene uses
all four physical banks. Dry/wet copies and the scroll table occupy predictable RAM.

Ten animated 32x16 foam sprites cover the mixed-palette rows at the surface.
Display stays enabled so the sprites remain visible. Two colors change per fresh
HBlank, and unchanged pairs are skipped. The current art needs eight such writes;
setup rejects a palette requiring more than its surface can cover. This is an
opaque, textured mask, not translucency or a seamless one-line palette switch.
Sprites cannot hide CRAM dots; smaller HBlank bursts address those separately.

The ten foam sprites consume the H40 limit of 320 sprite pixels on their rows.
They have first sprite priority, so the robot is occluded while crossing the
surface. Four pooled bubbles rise below it, retiring beneath the foam, and a
24-tick splash plays above it whenever the robot's center crosses the waterline
in either direction. All effects freeze on pause and keep their nominal cadence
in PAL and NTSC. A full bubble pool simply omits the next cosmetic bubble.
The scene uses sixteen user sprites at most, plus SGDK's internal reservation.
The ten foam strips share four resident frames (up to 1 KiB of pattern VRAM),
so animating them does not repeatedly upload ten copies of the same graphics.

The wave buffer is 448 bytes and uploads 448 bytes per displayed frame. Panning
also causes MAP streaming and its own baseline line-table upload. These are
DMA/CPU costs, not additional sprites. The masked transition occupies main CPU
time across its scanlines, so profile it alongside the rest of a real game.

`src/raster_model.c` owns bounded tables, tint and controls. `src/water_fx.c`
owns the fixed bubble pool and crossing effect. `src/water_split.c`
owns the H/V interrupts and snapshots persistent colors. Only its small edge-wait
routine uses inline 68000 assembly. `src/main.c` owns streaming, sprites, sound
and VBlank setup. The globals are standalone example ownership, not a framework.

Dry colors are restored by the early vertical interrupt, before normal DMA work.
The later VBlank callback only updates the HUD. Each completed frame publishes
its matching waterline and tint flag together, so moving water cannot change an
already armed split. Z80 driver loading finishes before these callbacks are
installed. Keep this order when adapting the example; a restore after DMA can
miss its deadline and leave the whole next frame blue.

## Read the diagnostics

`LINE` is the requested surface. `IRQ` is the most recent active interrupt's
observed V-counter; counter `LINE - 1` normally arrives just before that surface.
`L` counts late setup or active interrupts, `T` counts bounded-wait timeouts,
and `S` identifies the late-setup subset. These 16-bit counters wrap eventually.
An ignored pending interrupt during VBlank is not an error.

Nonzero/increasing L/T/S means investigate timing before using the effect in a
game. A skipped split or timeout can leave a different palette for that frame;
the following VBlank restores dry colors. Zero counters do not prove there are
no visible CRAM dots or that audio is clean.

## Try it

1. Watch the robot cross the foamy surface. Map and sprite should tint below it
   while the HUD keeps its normal colors. Look for a splash on entry/exit and
   rising bubbles underwater. Check the boundary for colored dots/flicker.
2. Repeatedly sweep Up/Down to both limits, then pan Left/Right. Watch for whole
   screen tint flashes and growing diagnostic counts. The wave must remain
   attached to the camera rather than replacing its scroll offset. A stationary
   waterline alone does not test this timing.
3. Compare A/B independently, including while paused. Turning tint off should
   restore dry colors across the screen; turning waves off keeps plain scrolling.
4. Listen with music enabled during panning and the palette split. C stops and
   restarts it without changing video state.
5. Repeat in actual detected NTSC 60 and PAL 50. See the skill's timing reference
   for disposable region headers. Compare cadence, seam and diagnostic counts.

Tests cover model bounds, palette packing, interrupt ownership/failure paths,
SDK API compatibility and target code shape. They do not emulate the VDP or
certify a physical console's raster timing. Keep emulator and hardware results
separate from build evidence.

`tools/make_assets.py` regenerates the original water PNGs and twelve-second FM
loop in a new directory, refusing to overwrite files. It uses only Python's
standard library. The water sprites share one palette, and the music has soft
pads and spaced bell notes without percussion. No audio capture is needed to build.
