# SGDK integration and validation

Verified against SGDK **2.11**, GCC **16.2.0** in the pulled
`ewrogers/sgdk:latest` container. Check the image's headers again after an update.
These versions are an observation, not a pinned toolchain requirement.

SGDK supplies numeric types, fixed-point helpers, input and graphics/audio tools.
Collision Lab's `cc_sweep`, `cc_ray`, `cc_cast` and `cc_slide` are **original C**.
ResComp collision metadata describes frame shapes; it does not automatically
resolve a level's contacts or provide the response policy used here. Sprite
priority/palette selection and sprite visibility are unrelated to collision
membership and solidity.

Sources: [types](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/types.h),
[maths](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/maths.h),
[sprite definitions/API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h),
[ResComp syntax](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt).

## Use SGDK directly

The Lab reads one `JOY_readJoypad(JOY_1)` snapshot per displayed frame. Its bounded
scheduler runs nominal 60 Hz logic with a 50/60 Hz accumulator; paused frames do
not consume simulation phase. At most two steps happen on a PAL frame. This is
not a real-time catch-up loop for missed VBlanks. Edge-triggered view changes are
presentation controls; continuous motion uses the held snapshot.

Sprites use `SPR_addSprite`, `SPR_setAutoAnimation(sprite, FALSE)`,
`SPR_setPosition`, `SPR_setVisibility` and a single `SPR_update()` per displayed
frame. Rendering converts local Q4 positions to integer pixels after collision.
The 12×12 body is deliberately smaller than the 16×16 robot drawing. Art size
and collision size are independent choices, not a mandate to use pixel masks.
The bottom shot sweeps a 4×4 body although its resource canvas is 8×8.

Source: [input](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/joy.h),
[sprite implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sprite_eng.c),
[system timing](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h).

## Account for debug presentation too

There are 23 single-piece sprite instances with a 40-tile reservation and 32
tiles of maximum instance allocations. The aiming ray uses 16 two-pixel dots
within 8×8 pieces. With the robot and both markers this is at most 19 pieces
on one scanline, below H40's 20-piece limit. That is fine for a small Lab, not an invitation to represent
all rays with sprites in a game. Debug geometry still consumes scanline slots,
RAM, VRAM, DMA and CPU. Visualize selectively and compare with overlays disabled.

A retained 2,240-byte HUD/obstacle tilemap queues under a 4,096-byte DMA allowance,
with `DMA_canQueue(DMA_VRAM, 1120)` using a **word count**, and enough queue entries
for 28 rows. The backing buffer survives until queued reads finish. Initial
palette and static tile writes occur while display is disabled. No raster
interrupts, active-display palette writes or per-query graphics uploads are used.
A finite PSG channel-zero impact cue has no music-driver conflict in this Lab;
route it through an audio owner in a game that already owns PSG.

Sources: [DMA API and units](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h),
[VDP tilemap API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_bg.h),
[PSG API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/psg.h).

## What the tests prove

- Hand-computed cases cover finite time, zero/negative motion, touching/moving
  away, parallel segments, corner grazing, simultaneous walls and initial overlap.
- 12,000 deterministic sweep/segment queries compare against independent Python
  `Fraction` interval arithmetic. Host O0/O3 execute with undefined-behavior
  sanitization where supported. Cases span the declared local coordinate range.
- The same contracts run as actual base-M68000 code, followed by a 1,000-query
  host/target trace digest. They check SGDK's target type widths and arithmetic,
  rather than assuming host `long` is the same size as M68000 `long`.
- Release/debug plus PAL/NTSC-region ROMs build with the container. Logical
  schedules compare 600 NTSC and 500 PAL display steps, including pause phase.
  ARM64/AMD64 reports compare ROM hashes, type sizes and traces.

These checks do **not** emulate VDP/audio timing, certify real hardware speed,
or prove a moving-wall physics solver. Use the Lab in an emulator and eventually
on hardware to assess contact behavior and rendering; record those observations
separately from compiled-code tests.
