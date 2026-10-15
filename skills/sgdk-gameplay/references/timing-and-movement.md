# Timing and movement

## Give each clock a purpose

A display frame, a simulation tick and a timer unit are different things.
NTSC has roughly 60 refreshes per second; PAL roughly 50. Use nominal 60/50
rates deliberately and validate both regions when the game targets them.

A simple game can simulate once per VBlank and use region-specific movement
and timer constants. Another option is a fixed simulation rate with an integer
phase accumulator. The lab adds 60 to its phase per display frame, takes one
step for each whole refresh-rate unit, and keeps the remainder. At 50 refreshes
this occasionally runs two simulation steps, while rendering only once.

This preserves nominal game time; it does not make a 50 Hz display show 60
separate images. Budget the two-step frame. The lab does not recover arbitrary
missed VBlanks or promise real-time catch-up under overload. A general elapsed-
time accumulator needs a bounded catch-up policy to avoid an endless backlog.

Keep `SYS_doVBlankProcess()` in the normal frame loop. It services SGDK's queued
work and frame synchronization. Do not replace it with a busy wait or move
all gameplay into an interrupt. Call `SPR_update()` once per rendered frame;
automatic sprite-animation holds count these calls, so they need their own
regional policy even when your game logic uses fixed ticks.

## Snapshot input and pause deliberately

Use the frame's cached `JOY_readJoypad()` result. Compute edges from the current
and previous held masks. Apply menu/start/reset edges once before any simulation
substeps; reuse held movement for each substep. If a press must survive a display
frame with no simulation step, queue it until consumed instead of losing it.

The lab's nominal 60-tick model always gets at least one step on its supported
50/60 Hz displays. Start and B are display-frame events; A is held auto-fire
with a simulation-tick cooldown. Pause clears the phase and runs no game ticks,
so movement, bullet lifetime and cooldown do not accumulate catch-up work.
SGDK music or sprite animation will not automatically pause with game state.

## Use the SDK's fixed-point format

Target the original 68000, without an FPU. The published toolchain rejects other
CPU/FPU settings; do not use later 68k instructions in handwritten assembly.

SGDK 2.11's `fix16` has six fractional bits and `fix32` has ten. `fastfix16` and
`fastfix32` are different formats; do not mix them by casting. Use the actual
header rather than assuming that every 32-bit fixed-point type is 16.16.

```c
fix32 x = FIX32(48);
fix32 speed = FIX32(1.5);  /* Pixels per simulation tick. */
x += speed;
s32 pixel_x = F32_toInt(x);
```

`FIX32()` accepts a constant expression; this example does not perform floating-
point work each frame. Use `F32_mul()` for fixed-by-fixed multiplication, not
ordinary `*`, and inspect ranges and cost before putting division or square
roots in an inner loop. Old names such as `fix32ToInt` are deprecated in 2.11.

Keep world positions wide enough for the level, then narrow only after checking
the visible range. Signed overflow is undefined C behavior. Integer division
of negative values truncates toward zero, which is not floor division for
negative tile coordinates. The lab rejects out-of-world coordinates before
converting to cell indices and keeps its world within a proven small range.

Normalize diagonal movement when equal travel speed matters. The lab uses
1.5 pixels/tick on an axis and approximately 1.0607 on each diagonal axis.
It gives horizontal input priority for facing when both axes are held; that is
a game choice, not a controller restriction.

## Sources

- [SGDK 2.11 numeric types](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/types.h)
- [Fixed-point formats and helpers](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/maths.h)
- [Frame synchronization and profiling API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h)
- [VBlank processing implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c)
- [Joypad sampling API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/joy.h)
