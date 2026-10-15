# Prove timing separately from arithmetic

## Keep the interrupt small

Prepare colors, packed pairs and scroll tables in ordinary code. The handler
should only synchronize and write its bounded, precomputed data. Avoid SDK
operations that allocate, decompress, manipulate the sprite engine or take the
Z80 bus. The lab's assembly is confined to the edge-wait/two-write sequence;
the model and frame setup remain normal C.

An H interrupt can arrive between a main-loop VDP command and its data. Protect
ordinary port operations with `SYS_disableInts()` / `SYS_enableInts()` and keep
that protected work short. Disabling interrupts for a long map update can make
the waterline late. The lab leaves at least 64 screen lines for setup and skips
late splits rather than applying the palette at an arbitrary position.

VBlank restores the dry palette only while the VBlank status flag is set. That
flag also avoids treating PAL's blank-period V-counter wrap as active display.
If setup misses its budget, its diagnostic increases and that split is skipped.
Do not fix overruns by removing the check or globally blocking vertical interrupts.

## Prevent moving-water flashes

SGDK 2.11 has two different hooks. `SYS_setVIntCallback()` runs from the actual
vertical interrupt near the start of VBlank. `SYS_setVBlankCallback()` runs from
`SYS_doVBlankProcess()` after DMA, scrolling and fade tasks. They are not equivalent
deadlines. Restore a split palette before that DMA work, not after it.
The V interrupt hook takes an ordinary C callback because SGDK wraps its entry
and return. Only the directly dispatched H hook uses `HINTERRUPT_CALLBACK`.

Raster Lab uses the early hook for dry colors and H-counter setup, and the later
hook for its HUD. This is a deliberate exception to SGDK's usual preference for
the later hook when doing ordinary VDP work. The early hook owns VDP ports too:
protect main-loop command/data sequences, keep protected sections short, and
do not overwrite another subsystem's interrupt callback. A real game must compose
these jobs under one owner. Do not let an independent queued palette upload or
fade overwrite the restored banks afterward.

Prepare the surface sprites and scroll data, then publish matching line/enable
settings together before that frame's VBlank. The lab uses one aligned 16-bit
word, which the 68000 can publish atomically. The H handler uses the latched
target, even if gameplay has started preparing a later frame. Larger state needs
an explicit protected snapshot or buffer handoff, not this assumption applied
to arbitrary structs or 32-bit updates.

Finish long startup operations, including Z80 driver loading, before installing
the callbacks. During play, keep allocation, decompression, logging and asset
uploads away from the deadline. Share resident animation frames where useful;
ten identical surface strips need not upload ten copies of every frame.

Watch both late setup and late H-interrupt counters while sweeping the surface
up/down, panning and changing animation frames. If an H interrupt arrives late,
skip the tint change for that frame instead of writing at the wrong line. An
early dry restore gives that frame a normal palette. If even the vertical
interrupt is delayed beyond blanking, that protection has missed its deadline:
reduce the blocking/transfer work and recover on the next safe VBlank. Do not
claim the effect can hide arbitrary overruns or clear its counters to conceal them.

## PAL, NTSC and music

The lab uses 320x224 in both regions. A small accumulator advances the model at
60 nominal updates/second on either a 50Hz or 60Hz display. The wave has the same
real-time cadence, although PAL displays fewer intermediate images. This does
not recover lost frames or make PAL's blanking budget identical to NTSC's.

For region builds, copy the project, build once to get the selected SDK's
`src/boot/rom_head.c`, change only its padded region string from `JUE` to `U`
or `E`, then clean/rebuild. Do not change installed SDK files or host emulator
preferences. Confirm the ROM's actual `NTSC 60` or `PAL 50` display; a header
alone is not evidence that the emulator selected that mode.

Music uses an original slow FM pad/bell phrase through XGM2, without percussion. Ordinary
graphics DMA remains in `SYS_doVBlankProcess()` with SGDK's bus protection and
its optional short delay enabled. The H handler performs no ROM-to-VDP DMA and
does not request/reset the Z80. Those choices reduce avoidable interference,
but listening is still necessary. Compare music on/off with waves, panning and
palette switching enabled; do not infer audio quality from an FPS counter.

## Evidence levels

- Host tests execute the C model across phases, bounds and 50/60Hz schedules.
  Fake ports record palette ranges, arming, pending/late events and restoration.
  They simulate edge changes and stuck FIFO/counter paths, not VDP timing.
  Frame tests execute the registered early callback, verify dry colors before
  simulated late main-loop work, sweep both directions, and check that publishing
  a later target cannot change the armed frame. These do not simulate DMA timing.
- Container tests compile the real APIs/resources in release/debug and region
  variants. Disassembly checks require interrupt returns, bounded polling and
  register-to-port writes without helper calls in the target handler.
- Emulator checks cover the visible seam, controls, robot crossing, unchanged
  HUD and audible music under scrolling DMA. Record the emulator/version and
  actual region, and check diagnostic counts after moving the waterline.
- Physical-console checks remain separate. Different VDP revisions and display
  paths can expose artifacts an emulator does not show. Do not advertise universal
  raster timing or clean CRAM writes from compile tests or screenshots alone.

Sources: SGDK 2.11 [interrupt/main-loop VDP warning](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h),
[VBlank/DMA processing](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c),
[XGM2 driver](https://github.com/Stephane-D/SGDK/blob/v2.11/src/snd/xgm2.c)
and [optional DMA delay](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/z80_ctrl.h).
