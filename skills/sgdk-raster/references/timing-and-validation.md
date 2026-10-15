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
