# Save formats and recovery

## Choose the data you need

Save Lab stores a level, score, lives and a music option. Its music option is
only data; the lab is silent. Position, velocities, pointers and temporary UI
state are not saved. Your game can use different fields and capacities.

Give the format its own version. Adding a field to a C struct must not silently
change the bytes expected on an existing cartridge. Encode integers explicitly
in a chosen byte order, and reserve only what you have a reason to reserve.

## The lab's slot layout

Two slots occupy logical offsets `0..31` and `32..63`. Both use this envelope:

| Offset | Bytes | Meaning |
| --- | --- | --- |
| 0 | 4 | Magic `SGSV` |
| 4 | 1 | Format version, currently 1 |
| 5 | 1 | Payload length, currently 8 |
| 6 | 2 | Reserved, zero |
| 8 | 4 | Unsigned sequence, big-endian |
| 12 | 1 | Level, 1 through 63 |
| 13 | 1 | Lives, 1 through 9 |
| 14 | 1 | Music option, 0 or 1 |
| 15 | 1 | Reserved, zero |
| 16 | 4 | Score, 0 through 999,999, big-endian |
| 20 | 9 | Reserved, zero |
| 29 | 2 | CRC-16/CCITT-FALSE over bytes 0 through 28, big-endian |
| 31 | 1 | Commit marker `0xA5` |

CRC uses polynomial `0x1021`, initial value `0xFFFF`, no reflection and no final
XOR. The check value for ASCII `123456789` is `0x29B1`. This is error detection,
not authentication, and some multi-bit changes can collide.

## Loading without destroying progress

The loader reads both slots into bounded scratch space. It checks magic,
commit marker and CRC before interpreting fields. Version 1 also requires the
documented lengths, reserved zeros and game ranges.

One valid slot is enough. With two valid slots, the newer sequence wins, using
unsigned modular subtraction so `0xFFFFFFFF` can wrap to zero. This assumes
valid saves are fewer than `2^31` commits apart. Equal sequences with different
contents, or sequences exactly halfway apart, are a conflict. Identical copies
are accepted. Conflicts are reported rather than guessed away.

All-zero or all-`0xFF` slots count as empty. Other invalid contents count as
corrupt. Neither case changes the caller's state. Start with safe defaults for
a new session; offer an explicit reset or restore path if both slots are lost.

A valid envelope with an unknown version blocks loading and saving, even if
the other slot is an older readable version. This protects newer progress when
an older ROM is launched. Only an explicit erase may discard it. The lab has
one format version and no migration implementation. When adding one, keep a
version-specific decoder, validate the old data, convert it in RAM and commit
the new format to the other slot. Preserve a backup before trying migrations.

## Committing without overwriting the backup

`save_store()` finds the newest valid slot and chooses the other one. It writes
a zero commit marker first, writes the new body, reads back every byte, then
writes `0xA5` last and validates that committed record. It never edits the
previous newest slot. A successful fresh save starts at sequence zero.

If a write fails or power stops before the final marker, the earlier slot
remains available. If power stops just after a successful commit, loading may
find the new save even though the caller never saw success. That is normal for
this transaction. Do not promise that a failed I/O status means no bytes changed.

This is not a hardware guarantee against arbitrary brownout writes, simultaneous
damage to both slots, a dead battery or a device that lies on read-back. The
previous slot may itself be damaged. Erasing both slots is intentionally
destructive and is not power-loss safe. Never use erase as automatic recovery.

## RAM and work

The model uses fixed-size local arrays and no allocation. The largest operation
has 128 bytes of explicit slot/body/verification arrays, plus ordinary locals,
call stack and compiler overhead. Main RAM cost is separate from cartridge
SRAM capacity. CRC work is small for 32-byte records and happens when saving
or loading, not every gameplay tick. The lab redraws only after input changes.

## Sources and implementation

- [Complete byte format and transaction](../assets/save-lab/src/save.c)
- [SGDK adapter](../assets/save-lab/src/storage.c)
- [SGDK SRAM calls](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sram.h)
- [Cartridge write and power-loss considerations](https://plutiedev.com/saving-sram)

The two-slot layout, CRC choice, game ranges and version policy are this lab's
design, not built-in SGDK features or console requirements.
