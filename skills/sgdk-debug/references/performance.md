# Frame time and graphics problems

## Measure the emulated console

The host emulator's FPS or host CPU percentage is not a measurement of the
68000 workload. It can present at 60 Hz while the game updates every other
refresh. Conversely, host overload or a speed limiter can make a healthy ROM
run slowly. Record NTSC/PAL mode, emulator speed, and actual game-update rate.
Nominal 60 Hz NTSC and 50 Hz PAL are different budgets and viewport modes.

SGDK 2.11 provides:

| API | What it means |
| --- | --- |
| `u32 SYS_getFPS(void)` | Counts calls per second. Call once per game update, not once per object. |
| `u16 SYS_getCPULoad(void)` | Estimated frame load from idle time in VSync/VInt waits, averaged across eight frames. Can exceed 100%. |
| `void SYS_showFrameLoad(bool mean)` | Scanline-position cursor; `TRUE` averages eight frames. Uses hardware sprite 0, palette 0/color 15. |
| `void SYS_hideFrameLoad(void)` | Removes that cursor. |

Do not call `SYS_getFPSAsFloat()` or `VDP_showFPS()` alongside `SYS_getFPS()`
as independent counters; they share the FPS measurement. `VDP_showCPULoad(x, y)`
draws the load estimate, adding its own text work. Compare overlay on/off under
the same scripted workload and throttle text updates. Debug instrumentation
has a cost, especially repeated VRAM writes and formatting.

[assets/profile.c](../assets/profile.c) is a build-tested starting point with
one sample per update and a normal VBlank wait. Add game work at the comment;
it intentionally does not upload a text overlay every frame. A debugger can
read its globals only if the chosen emulator/debugger supports that access.

`SYS_doVBlankProcess()` waits for VBlank and handles queued DMA, palette fades,
and input polling. Calling too late can delay processing until the next VBlank.
Its boolean result is **not** a generic missed-frame flag: the header documents
`FALSE` for cancellation when called from a V-Int callback. Avoid a second wait
elsewhere in the loop that silently halves the update rate.

## Find the expensive work

Compare optimized builds with the same deterministic inputs, random seed,
entities, graphics, and sound. Debug builds change both optimization and SGDK
library behavior. Keep diagnostics useful, but do not compare a debug baseline
against an optimized result and attribute all gains to one source edit.

Reduce one subsystem at a time: simulation/collisions, transform traversal,
sprite updates, tile streaming, palette changes, then sound interaction.
Measure the idle case, normal play, and a busy scene. Averages hide one-frame
spikes; profile spawning, scene changes, multiple collisions, and DMA-heavy
animation frames too. Do not remove behavior to claim a successful optimization.

Start with algorithmic work and data layout: cache repeated lookups, update
dirty transforms, use bounded pools where useful, avoid unnecessary divisions
and expensive full-screen uploads. Preserve overflow, signedness, and negative
coordinate behavior. SGDK fixed-point helpers are preferable to inventing
floating-point physics or assuming fixed multiplication is free. Inspect the
generated m68k assembly for a measured hot path before adding assembly by hand.

## Separate CPU, DMA, and display limits

DMA consumes transfer time and can stop the 68000 during transfers. A CPU-load
estimate alone does not tell you whether the VBlank transfer budget fits. Check
queued transfer sizes, SGDK queue/temp-buffer capacity, decompression, and when
data is committed. Preserve source buffers until deferred transfers finish.

VDP writes during active display can cause artifacts. Palette writes can make
CRAM dots; a bottom-edge dot during a change is still a timing clue. Do not
disable interrupts broadly to hide the symptom or upload whole palettes every
game update without a need. Schedule ordinary changes through the appropriate
SGDK VBlank/DMA path and re-test.

For sprites, distinguish SGDK logical sprites from VDP hardware pieces. In
320-pixel (H40) mode the VDP allows 80 hardware sprites per frame and 20 per
scanline, with a 320-pixel sprite-width budget per scanline. In 256-pixel (H32)
mode those limits are 64, 16, and 256. Transparent portions still count toward
hardware sprite-width limits. A large logical sprite may use several pieces.
Sprites disappearing on a crowded horizontal line can be a scanline limit even
when the total is below 80; raising an engine pool cannot raise a hardware limit.

VRAM also holds tiles, plane maps, the sprite table, and other configured tables.
Check layout and indices rather than treating all 64 KiB as free image storage.
There are four palettes of 16 entries shared by planes and sprites, not four
new palettes for every asset. Color index 0 is transparent within each palette;
the backdrop uses a selected palette entry. More RAM cannot buy more CRAM.

## Sources

Verified against the SGDK 2.11 implementation:

- [Counters and VBlank contracts](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/inc/sys.h)
- [Counter implementation](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/src/sys.c)
- [Sprite limits and engine behavior](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/inc/sprite_eng.h)
- [VDP sprite hardware interface](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/inc/vdp_spr.h)
- [DMA queue APIs and lifetime notes](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/inc/dma.h)
