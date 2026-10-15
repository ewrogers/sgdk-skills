---
name: sgdk-save
description: Add SGDK SRAM saves, explicit versioned byte formats, interrupted-write recovery and controller-friendly passwords. Use for persistent progress and save/load failures, rather than emulator save states or cartridge-specific EEPROM drivers.
---

# SGDK Save

Help a game remember progress without losing the player's last good save.
Start with what needs to survive a restart: checkpoint, score, options or a
small campaign record. Saving every live object is rarely necessary.

Check the project's cartridge mapping, ROM size, existing save format and
installed tool versions first. Preserve existing save files and make a backup before
testing a migration. Never erase storage just because loading failed.

## Choose the right reference

- [SRAM and cartridge setup](references/sram-and-cartridges.md): the ROM header,
  logical byte offsets, SRAM locking and ROM overlap above 2 MiB.
- [Formats and recovery](references/formats-and-recovery.md): explicit bytes,
  checksums, version policy and two-slot commits.
- [Passwords](references/passwords.md): a small alternative to battery-backed
  saves, with bounded fields and typo detection.
- [Save Lab](assets/save-lab/README.md): a standalone ROM with save/load,
  deliberate corruption, recovery and joypad password entry.

Use `ewrogers/sgdk:latest`; the lab includes its build command. Check unfamiliar calls against that image's `inc/sram.h`,
`src/sram.c`, `src/sram_a.s`, `inc/sys.h` and `src/boot/rom_head.c`.
The SGDK SRAM calls do not report capacity, battery health or write success.
Supply your own bounds and read-back checks rather than inventing such APIs.

## Keep saving separate from gameplay

Choose a small, documented on-cart format. Encode fields byte by byte; do not
dump C structs, pointers or heap layouts. Validate the envelope, version,
checksum and game ranges before changing live state. Return a useful status
for empty, corrupt, unsupported and failed I/O cases.

Write to a second slot, verify its contents and commit it last. Keep the previous
good slot intact until the replacement is valid. A checksum detects damage;
it cannot restore data without another copy. Read-back cannot prove that the
battery will retain bytes after power-off.

Keep SRAM access short and outside interrupt callbacks. Always restore the
mapper state, including on failure. A ROM over 2 MiB needs special attention:
SRAM can hide ROM code, constants and sound data while enabled. This is a
cartridge mapping problem, not solved by adding a checksum.

Passwords should encode game progress deliberately, with fixed input bounds and
a checksum. They are not encryption or an automatic replacement for arbitrary
save data. A successfully decoded password updates RAM; write SRAM separately.

## Prove recovery, not just a successful save

Execute format roundtrips, range failures, unknown versions, damaged slots,
sequence wrap and a simulated interruption after every write step. Check that
failed loads leave the current game state unchanged and failed saves preserve
the previous slot. Compile the adapter and verify the linked ROM's SRAM header.

Then try the lab in an emulator: save twice with different values, damage the
newest slot, recover the previous one and restart the ROM. Check password entry
and a rejected typo. Emulator save states are separate from cartridge SRAM;
record emulator persistence and real-console checks separately.
