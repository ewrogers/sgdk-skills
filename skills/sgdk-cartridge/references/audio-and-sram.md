# Banked music and SRAM

## Music keeps reading ROM

XGM2 playback does not copy an entire song and its samples into Z80 RAM.
`XGM2_load()` keeps addresses that the driver reads later. A FAR pointer that
was correct when loading can become wrong during another resource access.

`XGM2_load_FAR()`/`XGM2_play_FAR()` help map a resource. They do not reserve those
FAR regions for the lifetime of playback. Read the chosen driver's source
before deciding which data can move.

Bank Lab reserves CPU region 3 (`0x180000` through `0x1FFFFF`) for one song.
Its music banks do not change when SGDK uses FAR regions 6/7 for graphics.
The mapped addresses also stay below the common `0x200000` SRAM overlay area.
Shared click PCM stays in fixed bank zero. Each song must be 256-byte aligned
and fit wholly inside its reserved 512 KiB bank; the linker and loader check it.

## Switch only after old readers stop

The lab's stage handoff is:

1. Flush old graphics queues.
2. Load/copy the new stage graphics and palette safely.
3. Call public `Z80_unloadDriver()` to replace the old driver with the idle
   driver and silence/quiesce its old music and sample readers.
4. Map region 3 to the new physical song bank with `SYS_setBank()`.
5. Call `XGM2_load()` with its stable region-3 address, set looping explicitly,
   then start the selected track if music is enabled.

`XGM2_stop()` alone is an asynchronous command. It is not a safe barrier before
remapping audio. Likewise, the internal XGM2 unload hook is not a substitute for
public `Z80_unloadDriver()`. Inspect the actual implementation when upgrading.

This transition intentionally cuts off the old sound. Seamless crossfades,
concurrent banked sample sets and PCM relocation need a different residency
plan. Do not assume that samples may cross a bank just because the FM phrase
fits. The demo songs contain FM only, with separate fixed-bank click PCM.

Sources: [XGM2 implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/snd/xgm2.c),
[XGM2 API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/xgm2.h),
[Z80 control](https://github.com/Stephane-D/SGDK/blob/v2.11/src/z80_ctrl.c) and
[sound driver handoff](https://github.com/Stephane-D/SGDK/blob/v2.11/src/snd/sound.c).

## Save without resetting banks

Use `SRAM_enable()` for writes, `SRAM_enableRO()` for reads and `SRAM_disable()`
on every completion path. In SGDK's byte API, offsets are logical SRAM bytes;
the library handles the odd-byte wiring used by its normal cartridge profile.
Do not add the physical address yourself.

Keep save code, interrupt data and active audio reachable while SRAM is enabled.
Avoid reading ROM from the overlaid area. The precise overlay depends on the
cartridge; a compatible SSF mapper does not automatically prove compatible SRAM.

SGDK's SRAM helpers change the SRAM control register, not all bank registers.
Do not call `SYS_resetBanks()` after saving: that would discard the reserved
audio mapping while the Z80 could still be using it. Bank Lab compares SGDK's
remembered assignments before and after each operation. Its data CRC and
cross-bank probe also check actual bytes, rather than trusting those assignments.

The lab saves stage, position and music preference in a 16-byte record. It uses
explicit byte order, a version, CRC16 and commit bytes, and validates before
updating live state. Readback catches an immediate failed write.

This is a single-slot demonstration. Interruption may leave no valid save.
Production recovery should use independent slots with sequence numbers and a
commit protocol; a checksum alone cannot preserve the previous record. Do not
serialize compiler structs, pointers, mapped addresses or transient bank views.

Sources: [SRAM API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sram.h),
[SRAM implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sram.c) and
[Plutiedev SRAM guide](https://plutiedev.com/saving-sram).

## Cartridge and release checks

Bank Lab declares an SSF system header, a 6 MiB ROM ending at `0x005FFFFF`,
and odd-byte SRAM from `0x200001` through `0x203FFF`. Its large gaps make bank
placement easy to inspect; they are not useful game content.

Padding and checksums come after linking all loadable sections. The helper
pads to its declared layout and runs the selected SDK's `sizebnd.jar`. SGDK
2.11 uses its own XOR-folded checksum, not the conventional Genesis word sum.
Keep the matching ELF, symbol/layout report and tested image when debugging.

A linear-only release checker should reject this ROM rather than certify an
unsupported mapper. Check the board's supported size, mapper, save wiring and
flash-cart configuration separately. Test SRAM while music runs and stage
content changes on the intended hardware before claiming support.
