# SGDK contracts and hardware differences

Verified against the selected `ewrogers/sgdk:latest` image with SGDK 2.11,
ResComp 3.95 and m68k-elf-gcc 16.2.0.
Pull and inspect again when the image changes. Links below are versioned SGDK
source; Plutiedev explains the hardware separately.

## Startup and reset

[`src/boot/sega.s`](https://github.com/Stephane-D/SGDK/blob/v2.11/src/boot/sega.s)
selects `_start_entry` or `_reset_entry` using I/O control registers. It handles
the early Z80 halt and TMSS handshake. This is the SDK's entry classification,
not an electrical power-cycle detector.

In [`src/sys.c`](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c),
`_start_entry` clears RAM and copies initialized data before `main(TRUE)`.
`_reset_entry` runs `internal_reset` and calls `main(FALSE)` without clearing
application globals. Reinitialize game state, queues, resource owners and input
edge history on every entry. Do not test a BSS value for zero after a warm reset
and misreport the retained value as faulty RAM.

[`SYS_reset()` / `SYS_hardReset()`](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h)
request software reset / forced hardware initialization and memory clear.
Neither reproduces power loss or cartridge firmware boot. Do not make
uninitialized physical RAM a source of entropy or a save mechanism.

## Video and frame boundaries

`SYS_isPAL()` reports the PAL video flag. The Lab chooses 50 or 60 nominal
updates per second from that flag and uses 224 visible lines in both cases.
Physical rates are not exactly 50.000/60.000 Hz. Header `E` or `U` helps an
emulator choose a region; it cannot turn an NTSC console into a PAL console.

[`SYS_doVBlankProcess()`](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h)
handles queued DMA and normal controller/audio service. `vtimer` in
[`timer.h`](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/timer.h) is an
unsigned VInt counter. Sample once per loop, subtract unsigned values to handle
wrap, and record gaps. A gap is evidence of missed loop service, not a phase
profiler. Interrupt masking can hide elapsed interrupts entirely. The Lab does
not derive Hz or CPU percent from this counter.

In H40 (320px), the limits are 80 hardware sprites total, 20 per line and 320
sprite pixels per line. H32 (256px) allows 64, 16 and 256 respectively.
Transparency does not refund sprite width. The Lab stays in H40 and separates
21 narrow pieces from eleven 32px pieces to demonstrate the two scanline limits.
It uses SGDK's low-level SAT API so automatic sprite-engine culling does not hide
the intended overload. See [Plutiedev: sprites](https://plutiedev.com/sprites).

Palette banding, composite blending, overscan and audio balance depend on the
console and display path. Inspect a real display; a screenshot cannot establish
analog output quality. Do not infer a VDP revision from one visual artifact.

## DMA, sound and ports

[`DMA_canQueue` / `DMA_queueDma`](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h)
take lengths in **words**. `DMA_setMaxTransferSize` is **bytes**. A queue limit is
an application policy, not a guarantee of bandwidth. The Lab adds 0, 1024 or 2048
bytes per loop, plus text/SAT work. It writes only to VRAM `$8000..$87FF` and
retains the RAM source unchanged through queue consumption. It does not rewrite
visible art, the SAT, scroll tables or the font with the test upload.

XGM2 owns its Z80 driver. The Lab uses normal queued VBlank transfers and enables
[`Z80_setForceDelayDMA(TRUE)`](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/z80_ctrl.h).
That delays queued DMA by about three scanlines, costing some bandwidth. It
does not promise perfect PCM under every workload. The continuous reference
uses XGM2 channel 3 (preferred by the SDK automatic selector for lower CPU cost).
It starts once with looping enabled; there is no per-frame PCM-status bus request
or one-second replay gap to confuse the comparison. Do not sprinkle raw Z80 bus
requests or FM register writes around an active driver. See
[hardware issues](https://plutiedev.com/hardware-issues),
[DMA transfers](https://plutiedev.com/dma-transfer) and
[using the Z80](https://plutiedev.com/using-the-z80).

[`JOY_getPortType`](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/joy.h)
describes a port; `JOY_getJoypadType` describes a logical device. Unknown does
not identify a specific disconnected device. The Lab exposes raw types and
held bits for the default pad setup; it does not reconfigure a mouse or multitap.
Use sgdk-input for those support modes and logical player routing.

## Updating diagnostic text

Hardware Lab replaces each changing field as one padded 36-character row through
[`VDP_drawTextEx(..., DMA_QUEUE_COPY)`](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_bg.h).
The SDK converts text to tilemap data and keeps a queued copy, so a local string
may go out of scope after the call. Padding clears old digits without a separate
visible erase. Do not clear a live field and redraw it through CPU port writes
during active scanout; the empty intermediate state can become visible.
These text transfers count toward the total queue budget too.

## Cartridge memory

The Lab declares odd-byte SRAM `$200001..$20FFFF`. SGDK
[`SRAM_readByte(offset)` / `SRAM_writeByte(offset,value)`](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sram.h)
use **logical byte offsets**, not CPU addresses. They apply the odd-byte stride.
Use `SRAM_enableRO` for reads, `SRAM_enable` for writes and `SRAM_disable` when
finished. The Lab owns only logical bytes 0–11, with magic/version, a big-endian
sequence, pattern bytes and CRC-16/CCITT-FALSE. CRC is corruption detection, not
an atomic-save or anti-tamper guarantee.

This small ROM fits below 4 MiB. It tests neither bank switching nor interaction
with content in the SRAM window. Use sgdk-cartridge for those tests. Do not
reuse this write test in a game or another cartridge without assigning its own
storage. Read [saving SRAM](https://plutiedev.com/saving-sram) and document the
actual flashcart's save/flush procedure before testing power retention.
