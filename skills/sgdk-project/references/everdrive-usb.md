# Mega EverDrive Pro USB deployment

The bundled [Python client](../scripts/everdrive.py) implements the serial
protocol directly. It needs Python 3.10+ and optional pySerial; it does not invoke
edlink or require Mono/game-side hooks. One physical Pro accepted a hard-reset
upload from the ROM-selection screen, and the user confirmed a diagnostic's HARD
entry, RAM-init PASS and blinking heartbeat. Repeated replacement, recovery from
a hung game and other firmware/console combinations remain separate checks.
The CLI reports protocol results; console observations require their own evidence.

## Responsibility boundary

sgdk-project owns building and host-side deployment. sgdk-hardware owns console
and reset acceptance; sgdk-testing distinguishes upload/build evidence from
runtime observations; sgdk-cartridge owns mapper/cartridge compatibility.
The client ships inside this skill and needs no sibling skill or repository test.

## Host setup and use

Install the optional dependency in a virtual environment; normal Docker builds
do not need it. Replace the example skill location and port with actual paths.

macOS/Linux:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r /path/to/sgdk-project/scripts/requirements-usb.txt
.venv/bin/python /path/to/sgdk-project/scripts/everdrive.py --port /dev/cu.usbmodemACTUAL info
.venv/bin/python /path/to/sgdk-project/scripts/sgdk.py --project /path/to/my-game deploy --port /dev/cu.usbmodemACTUAL
```

PowerShell:

```powershell
python -m venv .venv
.venv\Scripts\python.exe -m pip install -r C:\skills\sgdk-project\scripts\requirements-usb.txt
.venv\Scripts\python.exe C:\skills\sgdk-project\scripts\everdrive.py --port COM4 info
.venv\Scripts\python.exe C:\skills\sgdk-project\scripts\sgdk.py --project C:\games\my-game deploy --port COM4
```

Use a powered console, USB data cable, working SD/menu firmware, and the explicit
native serial port from the host's device list. `info` sends the official
connection preamble/status queries to verify Pro identity without reset or SD
writes; it never probes other ports. Only one process may own the link. The
client requests POSIX exclusive access (advisory locking) and uses native serial
open semantics on Windows.

The combined `sgdk.py` command builds release using the standard container
workflow, then invokes its sibling USB script with the same host interpreter.
A nonzero build status is returned unchanged without opening USB. Missing ROM
output fails deployment. `--rom PATH` selects an output relative to the project
or an absolute path. Keep a project's custom Makefile/build system: after its
build succeeds, use the direct uploader instead of the combined standard build:

```sh
.venv/bin/python /path/to/sgdk-project/scripts/everdrive.py --port /dev/cu.usbmodemACTUAL deploy "/path/to/my-game/out/rom.bin"
```

**Hard reset is the default on every deployment.** Add `--reset soft` to the
deploy subcommand only on an explicit user request. The override is not retained
for later runs and a hard-reset error never falls back to soft. Krikzz describes
B2 hard reset as cold-boot equivalent and B27 soft reset as the console reset
button. Neither removes electrical power.
[Official reset definitions](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/usb-cmd/README.md)

Deployment asserts reset for at least 60 ms, clears cartridge configuration,
releases reset, waits for menu `r`, creates `usb-games` if needed, writes/closes
`usb-games/<ROM basename>`, installs it, and sends launch. The same SD filename
is **replaced** each time. The client does not deliberately modify other paths
or saves; normal firmware save handling remains the firmware's responsibility.
ROM basenames must use printable ASCII without FAT-reserved characters, within
the client's 255-byte cartridge-path bound. Host parent paths can contain Unicode
and spaces. The complete ROM is read before resetting the console.

Success prints JSON with ROM SHA-256, byte count, reset mode, `installed: true`,
`launch_sent: true`, and `runtime_verified: false`. Errors go to stderr and exit
nonzero; interruption exits 130. Installation is acknowledged by the menu;
launch has no acknowledgement of game execution.

## Docker placement

The client/dependency can be bundled in an image, but USB device access matters
more than language. Keep building in `ewrogers/sgdk:latest` and run USB on the
**host** with Docker Desktop on macOS/Windows: the build container does not
automatically see the host's serial ports. No SGDK image change is needed.
Native Linux can alternatively map one serial device into a container with
appropriate device permissions, this script and pySerial installed. That
arrangement is not device-tested here. Do not require privileged Docker or
install tools globally to make USB work.

## Supported protocol and failures

The implemented subset is Mega **Gen1/Gen2 discovery/status**, with `A5/status`
operation replies. Extended status identifies protocol `0x05`, device `0x18`.
For older Mega firmware that ignores extended status, read-only `SYS_INF` returns
64 bytes; its device ID at offset 50 must be `0x18` before reset/SD writes are
allowed. Legacy N8 responses, CORE/other devices, and unidentifiable replies are
rejected. The sources do not establish an exact minimum Pro firmware version;
record the installed version and check `info`. Firmware updates, custom mapper
uploads, saves, debug monitors and game logging are outside this client.

Per-operation reads/writes allow two seconds; menu readiness allows six for hard
or two for soft reset; installation allows thirty. These are host wait allowances,
not performance promises. `--timeout SECONDS` sets the overall USB deadline
(default 120; before the direct client's subcommand, after `deploy` in sgdk.py).
An already-held reset gets a bounded release attempt on failure/interruption,
even after that deadline. A disconnected cable can prevent release; the error
is reported and the console may need manual recovery.

Transfer/status errors stop before installation/launch. No cleanup command is
injected into an incomplete ROM data stream and a partial SD file is not claimed
removed. Recover/reconnect the console as needed before retrying. Deployment is
serialized by port ownership; do not run concurrent uploader processes.

OS examples describe intended use, not hardware-tested support on every OS.
POSIX integration tests use real pySerial and a simulated cartridge over a PTY;
Windows lacks that test. macOS PTYs reject the custom-baud ioctl, so only that
test substitutes 115200. The actual client always uses 921600 baud.

## Primary sources and physical acceptance

The wire implementation was checked against Krikzz edlink revision
`064874e2c89df6e8a8cf6dbd3e57661961374d46`:

- [Link.cs](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Device/Link.cs): baud/preamble,
  identity/status formats, command envelope, endianness, 1024-byte ACK blocks,
  and the 512-byte USB write workaround.
- [DeviceIO_V1.cs](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Device/DeviceIO_V1.cs): file/directory/status commands and memory/FIFO writes.
- [Mega DeviceIO.cs](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/DEV_MEGA/DeviceIO.cs): identity, SYS_INF field offsets, reset modes, configuration/FIFO addresses.
- [Mega MenuCmd.cs](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/DEV_MEGA/MenuCmd.cs): reset/release/readiness, installation ACK, and unacknowledged launch.
- [DeviceCmd.cs](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Device/DeviceCmd.cs): official `usb-games` directory and file create/overwrite flags.
- [pySerial API](https://pyserial.readthedocs.io/en/latest/pyserial_api.html): native serial, bounded I/O and POSIX exclusivity; dependency pinned at 3.5.

Before claiming compatibility, deploy a small known ROM, then use Hardware Lab
when sgdk-hardware is installed. Observe HARD/RESET entry and initial DATA `1234`
/ BSS `0000`; reupload after those globals change to `BEEF` / `CAFE`. Compare hard
deployment, explicitly requested soft deployment, the reset button, and actual
power-off/on separately. Record host OS, client/dependency versions, port, console
revision/region/add-ons, firmware, ROM hash, and each observed result. Confirm
replacement while running and after a deliberate hang. Check SD error and cable
disconnection recovery before relying on unattended iteration.

SGDK's startup flag is an SDK classification, not a power-cycle detector.
[SGDK 2.11 boot code](https://github.com/Stephane-D/SGDK/blob/v2.11/src/boot/sega.s)
selects the entry using I/O registers; [sys.c](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c)
clears RAM/restores globals on cold entry. The host hard reset occurs before the
menu/install sequence; observing fresh game state is a separate acceptance check.
