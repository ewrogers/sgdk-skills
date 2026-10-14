# Large ROMs and sound faults

Use this for failures correlated with cartridge growth, banked resources,
DMA or audio. Do not replace a working driver just to investigate a symptom.

## A build can succeed beyond the cartridge window

The normal ROM window is 4 MiB (`0x000000`–`0x3FFFFF`). SGDK's permissive linker
script does not prove a larger binary will work. Compare actual `rom.bin` bytes
with the intended mapper, ROM header, flashcart/board and emulator support.
The ROM helper warns above that boundary; it does not reject a valid banked ROM.

Stock SGDK 2.11 pads to 128 KiB boundaries with zero fill and its XOR-folded
checksum. That is distinct from selecting a physical cart capacity or padding
to a power of two. A generic Sega word-sum validator can give a false alarm.

The pinned SDK's `ENABLE_BANK_SWITCH` defaults to `0`. Its automatic resource
banking supports the Sega/SSF scheme. Check rebuilt SDK libraries and project
configuration together; changing project flags alone is not evidence that the
prebuilt library implements the selected mapper.

When banking is enabled, `FAR` access can remap `0x300000`–`0x3FFFFF`. A cached
pointer may then refer to different data. Check cross-bank ranges, concurrent
resource reads, DMA source lifetime and audio accesses. Use the selected SDK's
`mapper.h`/`mapper.c` and generated resource sections; file offsets are not
automatically stable CPU pointers. Keep the exact ELF and mapping state for a
fault rather than resolving an address against a different build.

## Sound ownership before register experiments

The Z80 has separate 8 KiB RAM. Request and acknowledge bus ownership before
68000 access, use byte accesses to its RAM, and release promptly. A long bus hold
stops audio work; reset is not a music pause. SGDK's bus/driver contracts matter
when several routines can hold the bus, especially around DMA.

The YM2612 has six FM channels, with channel 6 used for DAC playback. The PSG
has three tone channels and one noise channel. Their address/data or latch/data
write sequences must not fight the active music driver.

In SGDK 2.11, `YM2612_writeReg(part, reg, data)` observes chip busy/timing but
does **not** acquire the Z80 bus. Read the implementation and arrange ownership
explicitly for intentional low-level operations. Do not assume raw PSG writes
are safe alongside a driver simply because they use a different 68000 port.

First isolate the driver, sample/channel contention, bus holds and transfer
timing under the same workload. Do not hide glitches with broad interrupt
disabling or an invented chip-reset sequence. Preserve a known-good ROM and
test the actual emulator/hardware path; compilation cannot prove bus timing.

## Sources

- Plutiedev: [beyond 4 MiB](https://plutiedev.com/beyond-4mb),
  [padding](https://plutiedev.com/rom-padding),
  [Z80 bus](https://plutiedev.com/using-the-z80),
  [FM operations](https://plutiedev.com/ym2612-operations),
  [PSG](https://plutiedev.com/psg), [hardware issues](https://plutiedev.com/hardware-issues).
- SGDK 2.11: [config](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/config.h),
  [mapper](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/mapper.h),
  [linker](https://github.com/Stephane-D/SGDK/blob/v2.11/md.ld),
  [build](https://github.com/Stephane-D/SGDK/blob/v2.11/makefile.gen),
  [Z80](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/z80_ctrl.h),
  [YM2612 implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/ym2612.c),
  [PSG](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/psg.h).
