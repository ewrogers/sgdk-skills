# Save Lab

An original, silent SGDK ROM for trying SRAM saves and joypad passwords. Change
some progress, save it, then test recovery without needing a whole game.

Copy this folder to a **new** project directory and build there:

```sh
docker run --rm --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/sgdk:2.11-gcc16.2.0-binutils2.47@sha256:fd7a891d37773f792aad528bb6640d63bc0bd8e86ed140b8ef43d296f035f51c build
```

In PowerShell, use `${PWD}` instead of `$(pwd)` and Docker's Linux-container
mode. The image supports ARM64 and AMD64. Load `out/rom.bin` as a Mega Drive
game in ares or another compatible emulator. `build debug` adds symbols;
`build clean` removes generated build output. Keep the ROM in the same location
while checking persistence; changing its path/name can change its save-file key.

## Controls

| Button | Action |
| --- | --- |
| Up/Down | Select level, score, lives or music setting |
| Left/Right | Change the selected value; score changes by 100 |
| A | Save working values to SRAM |
| B | Load the newest valid save |
| Start | Reset working values in RAM; leave SRAM untouched |
| C | Edit the current password |
| Hold A, then press C | Damage the newest slot to test recovery |
| Hold B, then press C | Erase both slots, including unsupported versions |

In password entry, Left/Right moves the cursor and Up/Down changes its character.
A applies the code; B cancels. Applying a password does not save to SRAM.
With our current ares mapping, A/B/C are J/K/L and Start is Return. Emulator
keyboard mappings are host settings, not ROM requirements.

The test combinations are deliberately destructive to **this lab's save**.
Pressing A on the way to the damage combination first creates another save;
change values before doing so if you want the backup to be visibly different.
There is no music playback: the option demonstrates saving a boolean.

## Try recovery and persistence

1. Change level/score and press A. Change them again and press A: both slots
   now hold a valid record, with the newer one selected.
2. Press Start, then B. The latest saved values should return.
3. Change a value, hold A and press C. This saves the change, then damages that
   newest slot. Press Start, then B: the earlier values should return instead.
4. Press A after recovering. Saving should repair the other slot. The host
   tests cover both slots damaged and every interrupted-write position;
   the UI demonstrates one damaged slot. B+C exercises the empty-save path.
5. Save, exit ares normally and reopen the **same ROM**. The saved values should
   load at startup. Avoid loading an emulator save state during this check.
6. Open password entry and apply its untouched code. Change one character and
   apply: it should be rejected while keeping progress. Cancel and reopen to
   recover the current valid code.

Emulators may flush cartridge save files on close rather than on each SRAM
write. Do not force-kill the emulator for the persistence check. Real battery
retention and interruption during an electrical write still need cartridge
testing. The UI's damage action is a checksum-recovery demo, not a power-loss
simulator.

## How it is split

`src/save.c` owns explicit bytes, range checks, two-slot commits and passwords.
`src/storage.c` is the small SGDK SRAM adapter. `src/main.c` provides the joypad
UI; `src/boot/rom_head.c` declares 32 KiB of odd-address cartridge SRAM. The
format uses only its first 64 logical bytes. Main RAM is separate.

The lab stays below 2 MiB of ROM so enabling SRAM does not obscure its code or
constants. Its fixed scratch buffers need no heap. The save schema, CRC and
password helpers are original MIT-licensed examples; SGDK provides the hardware
access and text rendering. No external artwork or audio is required.

Automated tests execute corruption and interrupted-write cases, unknown-version
protection, sequence wrap, I/O failures and password rejection. Docker checks
compile both build modes and inspect the linked SRAM header. Those checks do
not replace emulator persistence or real-cartridge tests.
