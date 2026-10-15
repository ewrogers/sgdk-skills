---
name: sgdk-raster
description: Implement SGDK per-line scrolling and H-interrupt display effects, including waterline palette switches. Use for water ripples, heat haze, road-style scroll tables and mid-frame palette changes, rather than ordinary sprites or whole-plane scrolling.
---

# SGDK Raster

Choose the least timing-sensitive technique that gives the desired effect.
Water ripples usually need a line-scroll table, not an interrupt on every line.
A Sonic-style underwater tint does need a mid-frame palette change: the same
color indexes acquire different colors below the waterline.

Use `ewrogers/sgdk:latest`, pulling before validation. Check the selected SDK's
headers, implementations and H-interrupt samples before using an unfamiliar
call. These references were verified with SGDK 2.11; do not silently transfer
their timing assumptions to another video mode or driver.

## Choose the effect

- [Scroll tables](references/scroll-tables.md): per-line waves, camera offsets,
  persistent DMA sources, MAP ownership and separate RAM/transfer costs.
- [Waterline palettes](references/waterline-palettes.md): the interrupt ABI,
  HBlank synchronization, palette ownership and a deliberately masked split.
- [Timing and validation](references/timing-and-validation.md): interrupt/main
  coordination, PAL/NTSC, audio interaction and what each test proves.
- [Raster Lab](assets/raster-lab/README.md): Level Lab's original map/robot and
  Sound Lab's original music reused in a standalone, installable example.

## Make the ownership explicit

Keep ordinary VDP work and queued DMA in VBlank. Prepare scroll tables and
palette variants outside the handler. Protect main-loop VDP port operations
against an H interrupt; an interrupted command/data sequence can corrupt the
display. Do not add allocation, decompression, logging, formatting or audio
calls to a scanline handler.

`SYS_setHIntCallback()` jumps directly to its callback. Use
`HINTERRUPT_CALLBACK`, which must return with `RTE`, not a normal C return.
`VDP_setHIntCounter(N)` means every `N + 1` lines. Re-enabling may deliver a
pending event during VBlank; do not let that consume the intended active split.

Do not claim a whole palette fits inside one HBlank. Display blank alone does
not hide CRAM dots. The lab uses a visible transition band and four colors per
fresh HBlank across eight lines. That is a conservative teaching design, not
a cycle-exact reproduction of Sonic's renderer or a universal raster API.

## Prove the result

Exercise table bounds, phase wrap, camera limits, pause and region cadence.
Record palette restoration, one-shot arming, pending/late events and bounded
timeouts. Inspect the target handler's disassembly: correct interrupt return,
preloaded port writes and bounded polling matter here.

Then inspect the ROM in actual detected PAL/NTSC modes. Move the waterline,
pan the camera, cross the split with a sprite and listen with graphics DMA
active. Check the HUD stays unchanged and late/timeout diagnostics stay stable.
Host tests and a clean emulator image do not certify CRAM timing on a console;
record hardware and emulator evidence separately.
