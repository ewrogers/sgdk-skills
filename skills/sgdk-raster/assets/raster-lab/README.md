# Raster Lab

A scrolling stone outpost with a Sonic-style underwater tint and ripples.
The robot moves through the surface so you can see its colors change while
the HUD stays dry. Art/map are reused from this repository's original Level
Lab, and the original FM/PSG/drum phrase comes from Sound Lab. No Sonic assets
or music are included, and no other installed skill is required.

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
the surface; PAL3's HUD is unchanged. The scene still uses three physical banks,
not five. Dry/wet copies and the scroll table occupy predictable RAM.

There is an intentional, roughly tile-high blanked band at the surface. It
allows four colors per fresh HBlank across eight scanlines, then restores
display after the FIFO empties. This is a conservative masked waterline, not
a seamless one-line reproduction of Sonic's original handler. Disabling display
alone does not hide CRAM dots; the HBlank synchronization is essential.

The wave buffer is 448 bytes and uploads 448 bytes per displayed frame. Panning
also causes MAP streaming and its own baseline line-table upload. These are
DMA/CPU costs, not additional sprites. The masked transition occupies main CPU
time across its scanlines, so profile it alongside the rest of a real game.

`src/raster_model.c` owns bounded tables, tint and controls. `src/water_split.c`
owns the H interrupt and snapshots persistent colors. Only its small edge-wait
routine uses inline 68000 assembly. `src/main.c` owns streaming, sprites, sound
and VBlank setup. The globals are standalone example ownership, not a framework.

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

1. Watch the robot cross the surface. Map and sprite should tint below it while
   the HUD keeps its normal colors. Check the boundary for colored dots/flicker.
2. Hold Up/Down to each limit, then pan Left/Right. The wave must remain attached
   to the camera rather than replacing its scroll offset.
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
