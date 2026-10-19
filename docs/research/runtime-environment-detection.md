# ROM-side cartridge, console and emulator identification

Investigated 2026-10-19 against SGDK 2.11 and Krikzz's published guest/FPGA
sources. No new detector was built or uploaded. These are independent axes:
EverDrive identification does not establish which console runs the cartridge.

## EverDrive Pro-family capability probe

Krikzz's guest header maps EDIO at `$A130D0`, with its 16-bit SYSSTAT member at
offset four (`$A130D4`). The FPGA assigns the status word's upper twelve bits
to `$55A`, and uses the lower nibble for live state. Its status strobe toggles
after a read. Thus a read-only presence probe can mask with `$FFF0` and compare
against `$55A0`; it must not demand equality of the entire word.
[Guest register layout](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio/everdrive.h#L64-L82),
[FPGA decode and status](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/fpga/mapper/lib_base/base_io.sv#L32-L60).

This is evidence for the compatible EDIO interface, not authenticated physical
hardware or a discriminator for every EverDrive model. The published repository
targets both PRO and CORE. A failed probe means unavailable/unknown in that
environment, not proof of an emulator or an original cartridge. A future
implementation should use volatile 16-bit reads, preserve live status bits and
avoid opening FIFOs merely to check presence. An emulator or custom mapper could
implement the same interface. Older EverDrive families require separate probes.
[Official scope](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/README.md).

For stronger model/firmware evidence, the guest command `ed_cmd_sys_inf()` sends
CMD_SYS_INF and retrieves a `SysInfoIO` containing `device_id`, software,
hardware and boot versions. Add bounded receive timeouts before using this in
a portable ROM: the sample reads a FIFO reply and is not a universal safe
probe for absent hardware. Our host uploader previously verified this user's
Pro identity, but that is separate from an implemented ROM-side test.
[Guest command implementation](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio/everdrive.c),
[Reply structure](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio/everdrive.h#L260-L283).

SGDK 2.11's optional `ext/everdrive` module uses a different, older register map
at `$A13000..$A1300A`. Its `evd_init()` writes configuration registers; it is
not a Pro presence query. Do not conflate it with Krikzz's current EDIO sample.
[SDK header](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/ext/everdrive.h),
[SDK implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/ext/everdrive.c).

## Emulator and Mega Sg detection

A scan of the selected SGDK 2.11 source and public headers found no general
`isEmulator` or Mega Sg identification API. `SYS_isPAL()` / `SYS_isNTSC()` expose
video-system state, not manufacturer or execution technology. `KDebug_*` is
explicitly Gens KMod debug support, not an emulator-presence API; the assembly
writes emulator-recognized VDP commands without a general identity response.
[System API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h),
[KDebug header](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/kdebug.h),
[KDebug implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/kdebug.s).

Historical detectors fingerprint implementation differences. They can identify
tested emulator/clone versions, but updates or accurate emulation can remove
those differences. Failure to match must remain unknown, never an automatic
claim of original Sega hardware. See the separate
[Mega Sg investigation](mega-sg-runtime-detection.md) for the original detector
artifact, intrusive test side effects, official firmware caveats, and the
HDMI-side SPD identifier.

For the development workflow, record the launcher-selected target, user-declared
console/firmware and ROM-observed capabilities separately. A future screen could
show `EDIO: DETECTED` with a raw status/model result, and `CONSOLE: UNKNOWN` unless
declared or established by a specifically validated detector. Instrumented
host/guest challenge-response could verify an active development link, but
would still not prove the underlying console manufacturer.
