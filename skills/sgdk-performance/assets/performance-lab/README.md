# Performance Lab

Compare two collision searches over the same moving bodies, then add animation
or transfer work. A third case crowds sprites onto one scanline to show why
graphics dropout and CPU overload are different problems.

Copy this folder to a **new** project directory and build there:

```sh
docker run --rm --pull=always --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/sgdk:latest build
```

In PowerShell, replace `$(pwd)` with `${PWD}` and use Linux-container mode.
The image supports ARM64 and AMD64. Open `out/rom.bin` as a Mega Drive game in
ares or another compatible emulator. Use normal emulator speed and record the
region. `build debug` is useful for symbols; compare performance in **release**.

## Controls

| Button | Action |
| --- | --- |
| Up/Down | Add/remove eight bodies, from 0 through 64 |
| Left/Right | All pairs, grid, or grid with one rendered scanline |
| A | Toggle two-frame animation |
| B | Cycle extra DMA: 0, 512, 2,048 or 4,096 bytes/update |
| C | Hide/show diagnostic text |
| Start | Restart the same deterministic run and recording window |

On our current ares setup, J/K/L map to A/B/C and Return maps to Start.
Those are host mappings, not requirements of the ROM.

Workload changes and hiding metrics restart positions and counters. A run skips its first
16 updates for warm-up, records 120 updates, then freezes the window summary.
Motion and the live counters continue. Use Start to repeat without changing
settings. Showing metrics again freezes and reveals the hidden run instead of
replacing it. Check that it reached 120 samples; an early reveal is partial.
A completed window is a fixed update interval, not always two seconds
if the region differs or the workload misses refreshes.

## Reading the display

- **Hz** is calls per second to the game loop, measured by `SYS_getFPS()` once
  per update. It is not the emulator's presentation rate.
- **CPU est**, **Mean** and **Peak** come from SGDK's eight-sample load estimate.
  The window mean and peak are observations of that smoothed counter, not
  exact individual-frame CPU timings. Values above 100 are possible.
- **Checks** counts tested collision candidates; **Hits** counts overlapping
  pairs in the current simulation state. They do not count response/physics work.
- **DMA queued** includes sprite-engine work and the optional extra transfer,
  sampled before the regular VBlank flush. **DMA peak** is the recording
  window's maximum pending bytes. **Refused** counts rejected extra transfers;
  it is not a complete sprite-engine failure counter.
- **Missed refreshes** counts additional V-Interrupts between loop endpoints,
  after warm-up. It is specific to this one-update-per-refresh loop and is not
  a universal missed-frame API.

Text refreshes every 30 updates. C removes that recurring text work but leaves
the same simulation, sprite submission and counter sampling running. The sampled
globals remain in the ELF for a compatible debugger; this does not imply ares
provides a remote debugger. Frozen windows say whether their metrics were on
or off. Live CPU/Hz values continue updating and are separate from that summary.

## Repeatable checks

1. Set 64 bodies, animation off and extra DMA zero. Compare All Pairs / Spread
   with Grid / Spread after each recording window completes. All pairs visits
   2,016 candidates; the grid usually visits fewer. Both must find the same
   contacts for the same tick. A smaller population can favor the simpler code.
2. Repeat with A animation enabled. The orbs alternate every six simulation
   updates, all at once, making synchronized upload spikes visible. This is
   manual frame selection through SGDK, not a separate timer pool.
3. Cycle B through the extra transfer sizes. Pending bytes and possibly load
   should change. The source is fixed ROM storage, and the destination is the
   unused BG_B tilemap. No real art is streamed by this switch.
4. Note the completed Mean, Peak and Missed Refreshes. Press C to start a hidden
   run, wait at least a few seconds, then C to reveal its frozen window. Check
   120 samples before comparing. Start records a new visible run. Simulation
   and animation remain active while text is hidden.
5. Select Grid / One Scanline with more than 20 bodies. Some orbs should stop
   being drawn because of the H40 line limit. The simulation still visits all
   bodies. Switch back to Spread and confirm they reappear.
6. Try zero bodies for an idle baseline, then repeat a busy case. Test PAL
   separately; this lab does not normalize motion between PAL and NTSC.

Do not infer a whole game's headroom from this lab. It has no audio, map
streaming, collision response, spawning or general scene graph. A CPU reduction
is only established by measurements on the selected target/emulator; candidate
counts alone are not timing results.

## Structure and budgets

`src/workload.c` contains bounded movement, exact overlap queries, grid traversal
and the recording window. `src/main.c` adapts SGDK sprites, joypad input, DMA and
the display. Host tests compare exact pair bitmaps, not only a collision count.

All 64 sprite instances are allocated during setup, including hidden ones at
lower body counts. Each orb uses one hardware piece and one current-frame tile;
SGDK uploads a new tile when its frame changes. The two-frame source image and
optional zero-filled 4 KiB transfer source occupy ROM. BG_B already reserves a
64x32 tilemap. The lab does not repeatedly allocate in the frame loop.

The body array has 512 bytes of fields plus world metadata. The grid uses
224 bytes of fixed local link/head arrays, plus ordinary stack/locals. SGDK's
sprite descriptors, sprite tiles, DMA queue and scratch buffer are additional
costs. A larger pool is not free even when most sprites are hidden.

The orb is original MIT-licensed art. From the installed skill directory,
`python3 scripts/make_lab_art.py NEW_DIRECTORY` recreates it without overwriting
existing files. Release/debug builds and resource slicing are checked through
the real container; electrical timing and a real console remain separate tests.
