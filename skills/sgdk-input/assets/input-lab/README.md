# Input Lab

A small controller-test ROM for SGDK 2.11. Copy this folder into a new working
directory before building. It needs no art or audio assets.

The screen shows all eight SGDK joy slots: reported device, held buttons,
latest nonzero press/release masks and a Confirm action. Masks are hexadecimal;
`DOWN`/`UP` retain the latest event so a short press remains readable. Mouse
rows also show accumulated X/Y movement, bounded to -9999 through 9999.

Confirm uses A or X on a pad and left click on a mouse. All button bits remain
visible in the raw masks, including six-button and Mega Mouse extras. The
display refreshes every six frames, while input is sampled every frame.

## Build

From the copied directory, macOS or Linux:

```sh
sgdk_image='ewrogers/sgdk:latest'
docker run --rm --user "$(id -u):$(id -g)" --mount "type=bind,source=$PWD,target=/workspace" "$sgdk_image" build
```

Windows PowerShell, with Docker Desktop using Linux containers:

```powershell
$sgdkImage = 'ewrogers/sgdk:latest'
docker run --rm --pull=always --mount "type=bind,source=$($PWD.Path),target=/workspace" $sgdkImage build
```

Open `out/rom.bin` in a Mega Drive emulator. The same image provides native
Linux ARM64 and AMD64 toolchains; Docker supplies that Linux environment on
desktop hosts. Pull `ewrogers/sgdk:latest` before building.
The project uses SGDK's default ROM header; for a distributed controller-test
cartridge, customize the supported-device field to match its features.

## Try it

Configure emulator devices before resetting the ROM. Release buttons during
boot: the SGDK 2.11 implementation has a Team Player startup callback issue described in
[devices](../../references/devices.md). `OTHER` means the reported type is not
one this example handles, not a reliable unplugged indicator.

Test pads and mice on both ports, two Team Players in MULTI mode, and EA
4-Way Play occupying both ports. Follow the
[device checklist](../../references/testing.md). Enable mouse capture and map
relative X/Y axes in the emulator; keyboard button mappings alone do not move
a mouse.

Start+A+B+C on a working device explicitly rescans and clears edge history.
For Mega Mouse those bits are Start+Left+Middle+Right. It pauses normal work
while SGDK redetects devices. If no working input remains, configure devices
and reset the ROM instead. Automatic hotplug is not implemented.

## Reuse the pattern

`inc/input.h` and `src/input.c` form a small optional adapter. Call
`input_init()` once, then `input_poll()` once after each
`SYS_doVBlankProcess()`. Consumers read the same snapshot with `input_frame()`.

It owns SGDK's single joy callback and mouse accumulators. Integrate those
with any existing input layer before copying it into a game. The module uses
fixed arrays for eight slots and no heap allocation. It does not select player
numbers, manage a binding editor, or bypass SGDK to repair driver limitations.

The code is tested with simulated SDK state on the host and compiled/linked
against real SGDK. Physical devices and emulator protocols still need the
manual checks; compilation does not establish that they work.
