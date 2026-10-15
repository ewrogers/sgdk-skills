# Cartridge and sound boundaries

Read this when a project approaches a large cartridge size, changes mapper
configuration or adds low-level sound code. Ordinary setup does not require a
custom mapper or audio driver.

## ROM size and padding

The normal cartridge ROM window is `0x000000` through `0x3FFFFF`: **4 MiB
(32 Mbit)**. A larger file needs an explicitly supported cartridge mapping
scheme. Check the board, flashcart and emulator; they do not all support the
same mappers. A successful link is not proof of compatibility: SGDK's linker
script does not enforce the ordinary cartridge-window limit.

SGDK 2.11's standard makefile pads the ROM to a **128 KiB boundary** using
`sizebnd -sizealign 131072 -checksum`, with zero fill by default. This is a build
convention, not automatic power-of-two cartridge sizing. Distinguish meaningful
ROM data, final padded file size and the capacity of the chosen physical cart.
Do not confuse SGDK's XOR-folded checksum with a generic Sega word-sum checksum.

## SGDK banking is a deliberate configuration

The SGDK 2.11 implementation sets `ENABLE_BANK_SWITCH` to `0`. Its supported automatic binary
resource banking uses the Sega/SSF mapper; that does not imply support for every
mapper. Enabling it requires consistent SDK configuration, rebuilt libraries,
project headers and the appropriate ROM header. Do not assume that adding one
project compiler flag changes the prebuilt SDK library correctly.

With banking enabled, `FAR(data)` and `FAR_SAFE(data, size)` map resource data.
Automatic accesses remap the `0x300000`–`0x3FFFFF` area. Another mapping can
change what a retained pointer refers to. Coordinate resource access, deferred
DMA, interrupts and audio reads instead of caching mapped pointers indefinitely.
`FAR_SAFE` handles bank crossings within the supported scheme; it is not
arbitrary virtual memory or a guarantee that any size fits.

Inspect `inc/config.h`, `inc/mapper.h`, `src/mapper.c`, `md.ld`, the ResComp
manual and `src/boot/rom_head.c` for the selected SDK. Use the current generated
sections and symbols; a stale comment or older tutorial is not a linker contract.

## Z80 and audio ownership

The Z80 has its own **8 KiB RAM**. From the 68000, its RAM begins at `0xA00000`,
BUSREQ is at `0xA11100`, and RESET is at `0xA11200`. Access requires the correct
bus ownership and acknowledgement. Use byte accesses for Z80 RAM. Holding the
bus stalls the audio CPU; resetting it is not an ordinary pause operation.

SGDK provides bus/reset and driver operations in `inc/z80_ctrl.h`. Read their
contracts and existing driver use before calling them. Preserve ownership,
release the bus promptly, and do not improvise a broad interrupt-disable/reset
wrapper around DMA or music. The DMA/Z80 interaction has real hardware hazards.

The YM2612 has six FM channels; DAC playback occupies channel 6. Its 68000-side
address/data ports are `0xA04000/1` and `0xA04002/3`. The PSG write port is
`0xC00011`, with three tone channels and one noise channel. Register writes
have ordering and timing rules and can conflict with the active audio driver.

For normal music and effects, use the project's selected driver and its matching
SGDK resource/playback APIs. For deliberate low-level work, inspect `ym2612.h`,
`psg.h`, their implementations and the driver. In SGDK 2.11,
`YM2612_writeReg(part, reg, data)` handles chip busy/timing but **does not acquire
the Z80 bus**. Do not infer ownership safety from the helper's name, or assume
PSG latch/data sequences can be interleaved freely with driver writes.

## Sources

- Plutiedev: [beyond 4 MiB](https://plutiedev.com/beyond-4mb),
  [ROM padding](https://plutiedev.com/rom-padding),
  [using the Z80](https://plutiedev.com/using-the-z80),
  [YM2612 operations](https://plutiedev.com/ym2612-operations),
  [PSG](https://plutiedev.com/psg), [hardware issues](https://plutiedev.com/hardware-issues).
- SGDK 2.11: [configuration](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/config.h),
  [mapper contracts](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/mapper.h),
  [mapper implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/mapper.c),
  [build recipe](https://github.com/Stephane-D/SGDK/blob/v2.11/makefile.gen),
  [Z80 API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/z80_ctrl.h),
  [YM2612 implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/ym2612.c),
  [PSG API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/psg.h).
