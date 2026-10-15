# Resource ownership at a screen change

A title screen and a level can use the same VRAM, sprite pool and palettes at
different times. The important part is finishing the old screen's work before
those resources are reused.

## Fade, drain, then release

Use `PAL_fadeOutAll(frames, TRUE)` for an asynchronous fade and keep calling
`SYS_doVBlankProcess()` while waiting for `PAL_isDoingFade()` to become false.

There is one easy trap: SGDK's normal VBlank work flushes DMA before advancing
the palette fade. The last fade step can therefore clear the fade flag while
its last palette upload is still queued. Run another normal VBlank with the
old data alive before changing palette storage or releasing resources.

`DMA_waitCompletion()` waits for a hardware transfer; it does not flush the
software queue. `DMA_clearQueue()` discards work globally. Neither is a
replacement for an orderly handoff between owners.

Sources: [palette implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/pal.c),
[system VBlank work](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c),
[DMA API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h).

## Sprites, maps and heap storage

`SPR_reset()` releases every sprite in the sprite engine and invalidates the
old handles. It keeps the engine initialized. It resets the CPU-side sprite
table, so follow it with `SPR_update()` and normal VBlank processing to remove
the old sprites from the display too. A project with persistent global sprites
should instead release only the sprites that belong to the outgoing screen.

`SPR_end()` also frees the engine itself. Reinitializing it every time is
usually unnecessary when screens share one sprite engine.

Drain queued map/scroll work before `MAP_release(map)`. Free only allocations
your code owns, once each. Do not free nested pointers or ROM descriptors
because they happen to be reachable from a map or sprite.

Use `MEM_getFree()` or `MEM_getLargestFreeBlock()` to compare repeated visits
after resident services have warmed up. These describe the SGDK heap, not all
64 KiB of main RAM. A stable total can still hide fragmentation or a stale
handle; check allocation failures and lifetimes as well.

Sources: [sprite engine](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sprite_eng.c),
[map API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/map.h),
[memory API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/memory.h).

## Video state and callbacks

While hidden, establish the incoming screen's plane size, scroll modes,
Window position, tile ownership and palettes. Clear only what the new layout
requires. Keep palette sources alive until queued transfers finish.

If a screen owns a raster handler, disable HBlank generation and replace its
handler in a short interrupt-safe handoff before releasing referenced data.
Remove only callbacks it owns. Keep SGDK's normal VBlank/interrupt services
running, and never wait through a fade with interrupts disabled.

`SYS_setVBlankCallback()` runs from normal frame processing;
`SYS_setVIntCallback()` runs in the vertical interrupt. They are different
hooks. Transition Lab owns neither and does not clear them.

Sources: [system API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h),
[VDP API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h).

## Audio lifetime

Transition Lab keeps all songs and samples resident in ROM. It sends
`XGM2_stop()` and stops its PCM channels before loading the next song. The
stop is a driver command, not proof that the Z80 has already stopped reading
old addresses.

For banked or reclaimable audio data, quiesce the driver before remapping or
releasing its stream/sample storage. The public `Z80_unloadDriver()` is the
stronger teardown operation. Complete any XGM2 fade task before unloading;
do not call the driver's private cleanup directly.

Sources: [XGM2 implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/snd/xgm2.c),
[Z80 control](https://github.com/Stephane-D/SGDK/blob/v2.11/src/z80_ctrl.c).
