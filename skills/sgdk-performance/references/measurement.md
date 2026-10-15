# Measuring frame work

## Know what the number measures

| API or counter | Meaning and limit |
| --- | --- |
| `SYS_getFPS()` | Call frequency, sampled over time. Call exactly once per main-loop update; it is not a VDP presentation counter. |
| `SYS_getCPULoad()` | SGDK's frame-load estimate, averaged across eight frame-load samples. It can exceed 100%; it is not host CPU use or a count of instructions. |
| `vtimer` | SGDK's V-Interrupt counter. Consecutive loop endpoints can reveal display refreshes skipped by a one-update-per-refresh loop. |
| `getSubTick()` | Approximate elapsed units of 1/76,800 second. It is deliberately constant during VBlank and is unsuitable for precise short VBlank measurements. |
| `DMA_getQueueTransferSize()` | Bytes currently queued, not total VRAM use or measured transfer time. Sample before flushing. |

`SYS_getFPSAsFloat()` and `VDP_showFPS()` share FPS state with `SYS_getFPS()`.
Do not use them as independent counters. `SYS_showFrameLoad(mean)` adds a
scanline cursor using hardware sprite 0 and palette 0/color 15; reserve those
resources if using it. The lab uses text and does not enable that cursor.

## Keep the loop honest

`SYS_doVBlankProcess()` waits for VBlank and performs queued transfers, palette
fades and input polling. Its default requests the start of VBlank; arriving too
late can defer work to the next refresh. Its boolean return is not a general
missed-frame result. Do not add a second frame wait to make a profiler convenient.

For a loop that deliberately updates once per displayed frame, measure
`now - previous` from two `u32` snapshots of `vtimer` after the normal VBlank
processing. A delta above one means additional refreshes elapsed. Unsigned
subtraction handles counter wrap over these short intervals. This diagnostic
assumes interrupts remain enabled and does not describe every fixed-step or
intentional 30 Hz loop. Call it skipped refreshes, not dropped emulator frames.

Nominal NTSC 60 Hz and PAL 50 Hz have different frame/blanking budgets. Record
which is running. Performance Lab takes one simulation step per main loop; it
does not normalize motion to equal wall-clock speed across regions or missed
refreshes. Compare its algorithms at the same region and tick sequence.

## Make a useful before/after record

Record ROM/image revision, release flags, region, scene or replay, seed, counts,
animation state, sound state and transfer settings. Keep both ELF files and the
linker/size reports when inspecting code or memory changes.

Warm up before recording. Run the same bounded number of updates after reset,
then keep the result fixed while reading it. A variable-length manual test can
accidentally compare different enemy waves. Record average and peak observations
plus skipped updates. A peak of SGDK's smoothed load is still a smoothed peak;
it cannot establish the worst individual frame time.

Compare the same run with the overlay hidden. Keep the workload alive with
observable results so an optimizer cannot delete a supposedly expensive loop.
Disable per-object logging; it can cost more than the operation being measured.
The lab leaves its sampled globals available with text hidden, but reading them
requires a debugger/emulator that actually supports that connection.

## Investigate a smaller section

If using `getSubTick()` around a phase, state its units and the VBlank plateau.
An interval entirely inside VBlank can read as zero despite real work. Interrupts
and waits can also be included. Aggregate representative work, check the sampling
position and corroborate with whole-frame results or an emulator's verified
cycle profiler. Do not silently convert these readings into exact 68000 cycles.

An emulator may expose instruction tracing or memory inspection, but support is
emulator-specific. ares is useful for running the lab; this skill does not assume
a GDB connection or a live telemetry channel from it. Real hardware remains a
separate check, especially for DMA/audio interaction and marginal timing.

## Sources

- [SGDK 2.11 counter and frame contracts](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h)
- [Counter implementation and startup](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c)
- [Timer units and VBlank warning](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/timer.h)
- [Timer implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/timer.c)
- [DMA queue byte counter](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h)
