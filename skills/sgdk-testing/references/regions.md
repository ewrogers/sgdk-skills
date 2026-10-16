# PAL, NTSC and presentation checks

Decide what the game's clock means before comparing regions. A game can use
region-specific frame tuning, elapsed-time integration or a fixed simulation
clock. Keep its intended policy; 60 simulation ticks is the Lab's example,
not a requirement for all Mega Drive games.

## Compare equivalent work

Test Lab adds 60 phase units per display frame and consumes units in groups of
50 or 60, based on `SYS_isPAL()`. This gives 60 logic ticks over 50 nominal PAL
frames or 60 nominal NTSC frames. Some PAL frames run two updates. Playback
fetches a separate recorded input for each logic tick, so the same logical
tape reaches the same checkpoints under either schedule.

Live input is sampled once per display frame. If that frame runs two updates,
both see the same held state. Deriving edges inside the step means a press is
used once. This does **not** make PAL's physical input sampling identical to
NTSC, nor does it prove equal input latency.

Test the awkward cases: a press on a two-update frame, held confirmation across
a screen transition, pause/resume edges, a partially completed final frame,
and restarting with a nonzero accumulator. Check button-repeat, animation,
cooldowns and audio tempo separately if they use different clocks.

## Do not hide missed frames

The Lab advances once per completed main-loop/VBlank iteration. It is not a
wall-clock catch-up system. If the game misses a VBlank, it can slow down even
though its nominal PAL/NTSC traces match. A real catch-up policy needs a
bounded work limit and a decision about input/history; unlimited catch-up can
make an overloaded game fall further behind.

Use `SYS_doVBlankProcess()` for SGDK's frame work, including queued transfers,
input refresh and music processing. In SGDK 2.11, its documented false result
means processing was canceled because it was called from a V-interrupt callback;
it is **not** a missed-frame counter. `SYS_getFPS()` counts
calls per second, so call it once per presented frame when measuring FPS.
Neither value is a substitute for measuring busy gameplay.

## Check the actual console mode

The ROM header's region text helps emulator/console selection; it is not a
runtime timing measurement. The build helper can make separate U and E header
variants. Confirm Test Lab says **NTSC 60HZ** or **PAL 50HZ** after loading; an
emulator with a forced region can ignore the hint.

Use the same tape and inspect its PASS result in both modes. Also check full
viewport/HUD visibility, motion, pause, transitions, music tempo, SFX, palette
updates and crowded-sprite scenes where relevant. Keep physical-console checks
separate and name the tested setup. Nominal 50/60 cadence is an approximation;
this test does not measure the hardware's exact refresh frequency.

## Verified sources

- [SGDK 2.11 system API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h):
  region detection, frame processing and FPS semantics.
- [SGDK 2.11 implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c):
  VBlank scheduling and input refresh.
- [SGDK 2.11 ROM header](https://github.com/Stephane-D/SGDK/blob/v2.11/src/boot/rom_head.c):
  region field and cartridge header layout.
