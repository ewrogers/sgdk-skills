# Check the ROM you will distribute

## SGDK has its own checksum

SGDK 2.11's normal build runs `sizebnd` with `-sizealign 131072 -checksum`.
The tool pads to a 128 KiB multiple, with zero fill by default. It clears the
checksum at `0x18E`, XORs every big-endian 32-bit word in the padded file,
folds the upper/lower halves with XOR, then stores the resulting 16 bits.

This is not the conventional Genesis additive word-sum from `0x200` onward.
A generic ROM utility may report or write a different checksum. The helper
matches SGDK's physical-file build tool; it does not simulate runtime bus reads.
`SYS_computeChecksum()` uses linked sizes and bank-sized reads that can extend
beyond a small file through cartridge mapping/mirroring. Startup does not make
this host check unnecessary.

The XOR checksum is weak: cancelling changes can pass. SHA-256 in the manifest
identifies the distributed bytes. Neither hash proves the program is correct,
and an unsigned manifest is not proof of who published it.

## Read headers without guessing

The helper expects a raw `.bin`: vectors at zero and the SEGA header at `0x100`.
It checks a plausible even reset entry after the header and inside the actual
linear ROM. SGDK's stock stack vector is `0xE1000000`; its address wraps on the
68000's 24-bit bus, and predecrement stack writes reach the top of work RAM.
Do not reject that vector because it is not written as `0x00FFxxxx`.

Important header fields include:

| Offset | Meaning |
| --- | --- |
| `0x120`, `0x150` | Domestic and overseas titles, 48 bytes each |
| `0x180` | Serial/version text, 14 bytes |
| `0x18E` | SGDK build checksum, 2 bytes |
| `0x190` | Supported I/O declaration, 16 bytes |
| `0x1A0`, `0x1A4` | ROM start/end, big-endian 32-bit values |
| `0x1B0` | Optional SRAM descriptor and address range |
| `0x1F0` | Region declaration, 16 bytes |

Customize the project's `src/boot/rom_head.c` before building, preserving its
structure and mapper configuration. Review stock sample titles, copyright,
controls and region declarations. Do not patch the final file after validation.

The stock linear ROM end is fixed at `0x000FFFFF`, and `sizebnd` does not update
it. A smaller Lab ROM can therefore produce a header-size warning and still
pass. Review the header for the target cartridge; do not use it to truncate
the file or infer meaningful code size.

An SRAM declaration is not proof that the game saves. The stock header includes
`RA`, type `0xF820` and `0x200000..0x20FFFF`, even though the runtime accesses
odd SRAM bytes starting at `0x200001`. Do not infer usable capacity from that
address span alone. If ROM reaches into the declared SRAM range, check mapper
switching, ROM/audio accesses, save/load and persistence separately.

## File size is not cartridge capacity

An ordinary linear cartridge maps 4 MiB, or 32 Mbit. SGDK's linker can produce
a larger file; a successful link does not establish a working mapper. The
helper rejects files above 4 MiB and special mapping signatures. This is its
validation boundary, not a claim that SGDK cannot build banked games.

For a banked game, inspect the selected SDK's configuration and rebuilt
libraries, linker layout, ROM header and mapper APIs. Review fixed reset code,
far resources, retained pointers, queued DMA and Z80 sound reads. A `SEGA SSF`
header alone does not establish that those accesses are safe.

Keep these sizes separate: linked payload, final padded `.bin`, and the
capacity/layout of the chosen cartridge. A 128 KiB multiple is not necessarily
a power-of-two cartridge capacity. Agree any further padding and mapping with
the cartridge provider, then rebuild/revalidate and test the final artifact.

## Sources

Verified against SGDK 2.11 in the selected container:

- [Build and padding recipe](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/makefile.gen).
- [sizebnd checksum and padding implementation](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/tools/sizebnd/src/sgdk/sizebnd/Launcher.java).
- [Stock ROM header](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/src/boot/rom_head.c), [header layout](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/inc/sys.h).
- [Reset vectors](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/src/boot/sega.s), [linker addresses](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/md.ld).
- [Runtime checksum](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/src/sys.c), [SRAM API](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/inc/sram.h).
- [Banking configuration](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/inc/config.h), [mapper contract](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/inc/mapper.h).
