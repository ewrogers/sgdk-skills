# SRAM and cartridge setup

## Declare the cartridge, not just a RAM address

SGDK 2.11's SRAM helper uses 8-bit storage on odd addresses. The lab declares
`RA`, type `0xF820`, start `0x200001` and inclusive end `0x20FFFF` in its custom
`src/boot/rom_head.c`. That range provides 32 KiB of logical save bytes. The lab
uses just 64 bytes of it; it does not reserve 32 KiB of the console's main RAM.

Keep a project's existing header fields when adding SRAM. Verify the final ROM
at offsets `0x1B0` through `0x1BB`, not only the source file. An emulator uses
this declaration to recognize backup memory; actual cartridges must provide
compatible hardware. EEPROM, special mappers and flash-cart-specific storage
are separate protocols. These examples do not implement them.

## Use logical offsets

```c
SRAM_enable();
SRAM_writeByte(0, 42);     /* Logical byte 0, physical address 0x200001. */
bool matched = SRAM_readByte(0) == 42;
SRAM_disable();
/* Handle !matched without assuming the old save was replaced. */
```

`SRAM_enableRO()` selects read-only access. `SRAM_enable()` selects read/write;
`SRAM_disable()` locks SRAM and restores normal mapping. The byte API takes a
`u32` logical offset. SGDK performs the address stride internally: offset 1 maps
to `0x200003`. Do not pass `0x200001` or multiply the offset yourself.

`SRAM_readWord/Long()` and `SRAM_writeWord/Long()` also use logical byte offsets.
They transfer successive save bytes using the 68000's `movep` instructions.
The lab uses byte calls so its file format and endianness remain explicit.
None of these functions returns an I/O error or checks your declared capacity.

## Respect ROM overlap

The usual SRAM window starts at 2 MiB. Enabling it can hide the upper part of
a ROM from the 68000. The lab's entire ROM stays below that boundary, with no
music playback or streaming transfers during saving.

For a larger game, inspect the selected mapper and linker map. SRAM-access code,
constants and any interrupt code it can enter must remain accessible. Coordinate
Z80 sample reads and DMA source addresses too; do not leave a transfer reading
hidden ROM. A routine in RAM is one possible design, but all of its dependencies
must also remain accessible. Bank switching above 4 MiB adds its own mapper
requirements. Do not treat every 4 MiB ROM as automatically SRAM-safe.

Call save operations at a controlled point in the main loop, not from an HBlank,
VBlank or joypad callback. Restore mapping on every return path. Avoid a large
unbounded write inside a long interrupt-disabled section: that can disrupt
input, display and audio timing. Profile a representative save if it grows.

## What a successful write means

Read-back catches a missing or write-protected storage device when its reads
do not match. It cannot reliably identify a battery fault, predict retention
or prove that an emulator has flushed its host save file. Check persistence
after an orderly emulator exit and reopening the same ROM, then on a compatible
cartridge when available. Do not use an emulator save state as that test.

## Sources

- [SGDK 2.11 SRAM API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sram.h)
- [Byte access and mapper control](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sram.c)
- [Word/long transfers](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sram_a.s)
- [ROM header definition](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h)
- [Default header](https://github.com/Stephane-D/SGDK/blob/v2.11/src/boot/rom_head.c)
- [Mapper addresses](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/mapper.h)
- [Plutiedev: saving to SRAM](https://plutiedev.com/saving-sram)
