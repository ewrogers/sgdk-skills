# Mega EverDrive PRO USB deployment investigation

Investigated 2026-10-19. Source inspection only: no cartridge connection, upload, console observation, or hardware acceptance test was performed.

Follow-on implementation: the user selected an independent Python protocol client instead of wrapping edlink. It is now bundled under `skills/sgdk-project/scripts/everdrive.py`; setup, supported firmware/status formats, and physical acceptance are documented in `skills/sgdk-project/references/everdrive-usb.md`. Both extended status and older Mega status with a verified SYS_INF device ID are supported. The official-tool details below remain protocol research, not runtime dependencies of that client.

## Conclusion

Yes. Krikzz's official USB tools support copying a Genesis ROM to the cartridge and launching it. The current official example documents a **full system hard reset through cartridge pin B2**, described as equivalent to a cold boot; its soft reset uses B27, equivalent to the console reset button. This is a supported reset signal, not an electrical power cut. [Official reset definitions](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/usb-cmd/README.md)

The suitable integration is an optional host-side deployment step after a successful SGDK build. No game-side USB code is required for upload and launch. Actual runtime correctness still needs an observation channel or a person watching the console.

## Official tooling and prerequisites

- Current unified **edlink**, rather than older **megalink**, is the reference implementation examined here. Official edlink supports PRO/CORE cartridges and Windows, Linux, and macOS. Linux/macOS require Mono. Its `edlink.py` is a Python launcher for the adjacent `edlink.exe`; it is not a Python implementation of the USB protocol. Our independent client does not need either tool or Mono. [edlink README](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/README.md), [Python launcher](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edlink.py)
- The official edlink release directory currently contains versions 1.0.0.1 and 1.0.0.2. The legacy Pro USB directory distributes megalink through v1.0.0.8; those tools have different CLI syntax. Do not mix commands across them. [edlink distributions](https://github.com/krikzz/edlink/tree/064874e2c89df6e8a8cf6dbd3e57661961374d46/dist), [legacy distributions](https://krikzz.com/pub/support/mega-everdrive/pro-series/usb-tool/)
- Use a powered console, working cartridge SD/menu firmware, a USB data connection, and a host-accessible serial port. Current edlink explicitly warns that connection failures may require firmware updates, but the inspected official sources do **not** establish a precise minimum Pro firmware version. Current firmware listed by Krikzz is v26.0925; record the installed version instead of asserting a guessed minimum. [Firmware changelog](https://krikzz.com/pub/support/mega-everdrive/pro-series/firmware/changelog.txt), [edlink README](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/README.md)
- Device information includes cartridge serial, bootloader and MCU version, and counters. Edlink opens serial ports at 921600 baud, identifies Mega protocol `0x05` and Pro device `0x18`, and supports explicit port/device/protocol selection. Without `.link`, it scans and selects the first available device. An automated integration should require an explicit port, verify identity, and avoid probing arbitrary serial devices. These identifiers are **protocol IDs**, not USB VID/PID values. [Link transport](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Device/Link.cs), [Mega identity/info](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/DEV_MEGA/DeviceCmd.cs), [CLI help](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Help.cs)

## Tool choice and reset policy

Decision recorded 2026-10-19 following the user's clarification:

- Implement the upload/reset subset directly in Python using pySerial. Ship the client and its optional pinned requirement inside sgdk-project. The official edlink source is the protocol reference; the client has no edlink/Mono dependency.
- Run USB on the host, separately from the Docker build, especially with Docker Desktop on macOS/Windows. Bundling a client inside an image does not automatically expose the host serial port. Native Linux device mapping is a separate possible deployment arrangement.
- **Default to hard reset on every hardware deployment. Use soft reset only when the user explicitly requests it.** Always pass the selected mode to edlink; do not inherit its soft default, remember a previous soft override for later deployments, or silently fall back to soft after a hard-reset failure. A hard-reset failure must fail deployment with a useful error.

These decisions govern the bundled client. Physical cartridge acceptance remains outstanding.

## Exact launch/reset behavior

Source-verified command structure, with real local paths and port substituted:

```sh
python3 /path/to/edlink.py .link --port /dev/ACTUAL_PORT --dev-id 0x18 --protocol-id 0x05 reset --mode hard run --file /absolute/path/out/rom.bin
```

On Windows the executable can be invoked directly. Pass each token as a subprocess argument rather than constructing a shell string. Keep `reset --mode hard` and `run` in **one invocation**: Mega `DeviceCmd` stores reset mode per process and defaults to **soft**. The parser accepts sequential commands, and the dispatcher executes them in order. [Parser](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Cli/CmdLine.cs), [dispatcher](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Cli/CliHandler.cs), [Mega run implementation](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/DEV_MEGA/DeviceCmd.cs)

The resulting path is:

1. Assert hard reset; `Run` then calls `ResetToMenu` with that hard mode.
2. Clear cartridge configuration, release reset, and wait up to six seconds for menu byte `r` (soft mode allows two seconds).
3. Copy the host ROM to SD `usb-games/<ROM basename>`, replacing an existing file of that name.
4. Send menu `*i` with the path and require installation response zero.
5. Send menu `*s` to start. There is no subsequent execution acknowledgement or final hard reset in this host implementation.

Sources: [reset/menu protocol](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/DEV_MEGA/MenuCmd.cs), [deployment destination](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Device/DeviceCmd.cs), [reset transport](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/DEV_MEGA/DeviceIO.cs)

Thus this is a supported hard-reset-before-deployment workflow. Source inspection alone does not prove that every launched SGDK ROM observes `main(TRUE)` on every console/firmware combination. Validate that explicitly. Do not blindly append another hard reset after launch: the Pro manual's **Reset To Menu** option determines whether reset returns to the menu or stays in game; **Mode-A** separately controls automatic game launch at next power-up. [Official manual](https://krikzz.com/pub/support/mega-everdrive/pro-series/mega-ed-pro-manual.pdf)

## SGDK startup evidence

The selected Docker image was pulled and its actual source inspected. SGDK 2.11's boot code classifies startup using I/O control registers, then chooses `_start_entry` or `_reset_entry`. The former clears RAM and restores initialized data; a warm reset retains application globals. That is why the acceptance test must observe both the startup flag and initial data, rather than treating a successful USB command as proof of fresh game state. Calling SGDK's `SYS_hardReset()` inside the game is a separate software mechanism, documented as forced hardware initialization and memory clearing; it is not required by the official host upload sequence. [SGDK boot source](https://github.com/Stephane-D/SGDK/blob/v2.11/src/boot/sega.s), [startup implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c), [reset API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h)

The existing [Hardware Lab](../../skills/sgdk-hardware/assets/hardware-lab/README.md) already provides the needed DATA/BSS startup display. Its [source](../../skills/sgdk-hardware/assets/hardware-lab/src/main.c) snapshots the initial values before mutating them. Reuse that diagnostic before changing game startup code.

## Protocol and integration reliability

The command envelope is four bytes: `2B D4 command (command XOR FF)`. Host reset command is `0x29`, followed by mode `0` off/release, `1` soft, or `2` hard. Mega integers use big-endian encoding. Menu communication uses the host FIFO at `0x1810000`; configuration clearing writes 256 zero bytes at `0x1800000`. File transfers and installation require additional status/ack handling. A bare reset packet is insufficient to implement the whole loader. The bundled client implements this focused sequence from the published sources. [Framing](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Device/Link.cs), [Mega transport](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/DEV_MEGA/DeviceIO.cs), [file protocol](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Device/DeviceIO_V1.cs)

**Important error handling:** inspected edlink `Program.Main` catches exceptions and prints `ERROR:` to stderr without setting a failing process exit status. Its Python launcher merely forwards that status. A future wrapper must not trust exit code zero alone: pin/record the tool version, enforce a wall-clock timeout, reject errors, and require the expected successful run completion output. Prefer fixing/verifying upstream exit semantics if maintained versions change. `run` completion establishes command transmission and installation, not successful game execution. [Program](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Program.cs), [run completion](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Cli/CliHandlerCmd.cs)

Edlink also offers `.stdio` for process integration, with length-prefixed binary payloads, but normal console completion output is suppressed there. It is not itself a documented success-ack protocol for `run`. Avoid assuming that switching to stdio fixes the exit-code issue. [stdio implementation](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Stdio.cs), [official integration sample](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/samples/link-integration/Program.cs)

## Repository boundary and physical acceptance

- `sgdk-project` owns the optional host-side deploy command and its portable Python helper/reference. Build the release ROM in `ewrogers/sgdk:latest`, then deploy only if the build succeeds. Keep host USB outside Docker, especially on macOS. Do not install tools globally or update cartridge firmware automatically.
- `sgdk-hardware` owns reset semantics and the physical acceptance checklist; `sgdk-testing` distinguishes upload/build checks from observed execution. `sgdk-cartridge` retains cartridge compatibility guidance. Do not create a parallel loader policy in each skill.
- Validate with the existing Hardware Lab: observe its HARD entry indication and initial DATA `1234` / BSS `0000`, then reupload after those variables mutate to `BEEF` / `CAFE`. Compare explicit soft reset, explicit hard reset, reupload, and actual power-off/on. Record console model, add-ons, region, firmware, edlink version, host OS/port, ROM hash, and observed results.
- Test useful host failures before calling the integration ready: absent/incorrect/occupied port, identity mismatch, missing Mono/tool/ROM, failed build, paths with spaces, timeout, failed SD transfer/install, and misleading zero exit status. Serialize deployment attempts. Report separate states such as build passed, upload/install passed, start sent, and runtime observed.

The Python client and build handoff are implemented; the broader Hardware Lab observations above remain proposed acceptance work. The limited diagnostic observations from the physical trial are recorded below; they do not establish audio, timing, other ROMs, or power-loss behavior.

## Evidence snapshot

Official source revisions inspected: `krikzz/mega-ed-pub` **23dbd5ef527999eb85590ece7ad0ba6e202c8a4c** (2026-06-08); `krikzz/edlink` **064874e2c89df6e8a8cf6dbd3e57661961374d46** (2026-10-14). Krikzz-hosted manual and firmware directory are live documents accessed 2026-10-19, not immutable versioned releases.

Local SGDK inspection: pulled `ewrogers/sgdk:latest`; `doctor` reported SGDK **2.11**, GCC **16.2.0**, binutils **2.47.20260726**, on **aarch64**. Docker reported manifest digest `sha256:1d8dd3f289ef9ed1c7186b68b47a1977bc02b2bcbd72903e7682f6b19ce7d0df`. This records the inspected image, not a proposed pinned build default. The startup implementation and host helper were inspected; no ROM was compiled or executed for this investigation.

Follow-on implementation validation on the same date: 34 focused client/project/resource tests passed, including copied-CLI deployment to a simulated cartridge through real pySerial and a POSIX pseudo-terminal. The macOS PTY test substitutes 115200 baud; the real client uses 921600. The copied project helper built SGDK Hello World release/debug ROMs, checked debug symbols, and rejected invalid C using the refreshed container above. The Skills CLI copied all 38 skills and their resources. These checks did not connect to physical USB hardware or execute a game on a console.

## Physical deployment trial, 2026-10-19

The user authorized a trial from the ROM-selection screen. macOS enumerated a
`Mega EverDrive` serial USB device (VID `0483`, PID `5740`). It returned legacy
`A5 00` status rather than extended identity. The documented read-only `SYS_INF`
command `0x26` returned 64 bytes with device ID `0x18` at offset 50, MCU version
`0x0112`, hardware version `0x0001`, and boot version `0x0101`. These are raw MCU
information fields, not a claimed menu OS version. Console revision/region and
menu OS version remain unknown.

The client was extended to verify legacy Pro identity through that field before
allowing reset or SD writes. Its transcript tests now cover successful legacy
deployment, wrong legacy device ID, and unavailable system information.

A small diagnostic was built as release with the refreshed SGDK 2.11/GCC 16.2.0
image. It displays startup classification, initialized-data/BSS acceptance and
a blinking heartbeat; it contains no save writes. The 131072-byte ROM's SHA-256
is `e4eb47b4ca95acf5f64fbbb0a4d1345d56792c2d41720be30c323f0768145d61`.
The real client requested hard reset, transferred the ROM to
`usb-games/sgdk-usb-reset-test.bin`, received successful file/installation
acknowledgements, and sent launch. The transaction returned success in about
3.9 seconds. The user then confirmed the test screen displayed **SGDK ENTRY:
HARD**, **RAM INIT: PASS**, and a blinking **RUNNING** indicator. This is
user-observed evidence that this diagnostic executed on the console, took SGDK's
hard-start entry, and observed initialized DATA `1234` / BSS `0000` before
mutating those globals. The agent did not directly view the TV.

This validates one hard-reset upload from the cartridge's ROM-selection screen
on this device. Replacing a running/hung ROM, repeated uploads, soft reset,
audio/timing behavior, other console/firmware combinations, and actual electrical
power-off/on were not tested by this trial.
