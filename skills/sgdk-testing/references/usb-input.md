# Optional EverDrive USB input

Use this only when the user wants a development ROM controlled over USB. The
game must include and call the hook. It supplies two logical input slots, each
independently a six-button pad or Mega Mouse; it does not change the SDK driver,
emulate electrical controller ports, control an arbitrary commercial ROM, or
press buttons before the program starts.

## Responsibility and installation

This skill owns the optional `UIV1` protocol, portable C module, installer and
host client. `sgdk-input` owns physical polling and the game's input snapshots.
`sgdk-hardware` owns the independently buildable readiness fixture and EDIO
contract. Project deployment/reset stays in `sgdk-project`. No additional input
manager, interrupt, clock, registry or game framework is installed.

The three source files in `assets/usb-input` are freestanding C17 using GCC/Clang
exact-width types. From this installed skill directory:

```sh
python3 scripts/install_usb_input.py /path/to/existing-game
```

This copies `src/usb_input.c`, `inc/usb_input.h` and `inc/usb_input_ed.h`, refusing
existing files. It does not edit the game, Makefile or controller settings. Only
include these optional sources in a ROM intended to accept remote input. The
Lab contains its own identical snapshot, without a sibling-skill runtime dependency.

## Connect the existing input boundary

- At program entry, initialize `UsbInput` using `ui_init(&usb, ui_ed_detect(), hz)`.
  `hz` comes from the game's existing actual PAL/NTSC flag. Initialize explicitly
  on both SGDK entry paths; warm entry may retain old globals.
- Once per existing input frame, obtain the two **already sampled physical**
  states from the current owner. Set `UiPort.kind` to UI_PAD, UI_MOUSE or
  UI_PHYSICAL/unknown, `held` to the normalized mask, and mouse `dx/dy` to this
  frame's relative movement. Do not poll/reset accumulated axes twice.
- Call `ui_ed_poll(&usb, now, legacy_handler, context)`, then
  `ui_ed_finish(&usb, now, physical)`, even when no override is active. `now` is
  the existing VInt counter, not an independent timer. A null legacy handler is
  fine for a ROM without the readiness protocol.
- Route gameplay through `usb.port[0]` and `[1]`, or copy these into the game's
  existing normalized snapshot. Fields are kind, held, pressed, released, dx,
  dy and remote. Read the immutable result throughout that frame. Check kind
  before interpreting buttons; mouse buttons use a separate namespace.

This is an explicit integration point. Direct calls to `JOY_readJoypad()` still
return physical state. Avoid replacing global SDK state or installing a second
joy callback. [Hardware Lab main](https://github.com/ewrogers/sgdk-skills/blob/main/skills/sgdk-hardware/assets/hardware-lab/src/main.c)
is an executable adapter example when that skill is available. Its direct-port
adapter reads differences of accumulated axes without resetting the SDK's axes;
an existing Input Lab-based game should use its existing InputFrame instead.
Do not transplant the Lab's UI, cursor bounds or navigation policy.

Pad bits match SGDK 2.11: UP=1, DOWN=2, LEFT=4, RIGHT=8, B=10, C=20, A=40,
START=80, Z=100, Y=200, X=400, MODE=800 (hex). Mouse bits are LEFT=1, RIGHT=2,
MIDDLE=4, START=8. Relative X is positive right; Y is positive screen-down.
Normalize a physical device's conventions once in its existing adapter. The two
slots do not add multitap or physical-device support.

## Host use

Use an explicit native host port and one process at a time. Requires Python
3.10+ and pySerial, installed in a host venv from `scripts/requirements-usb.txt`.

```sh
python scripts/usb_input.py --port ACTUAL_PORT --pad1 RIGHT+B --pad2 DOWN+Z --frames 60
python scripts/usb_input.py --port ACTUAL_PORT --mouse1=20,8,left --mouse2=-16,-7,right
python scripts/usb_input.py --port ACTUAL_PORT --pad2 X --seconds 5
python scripts/usb_input.py --port ACTUAL_PORT --release
```

Default is a single 120-VInt pulse. The host can exit; the guest releases
automatically. `--frames` accepts 1..120. `--seconds` accepts 0..300 and renews
held buttons with **zero** additional mouse movement, then releases selected
slots; it requires at least 30 frames and an overall timeout at least four
seconds longer than the run. Defaults: overall `--timeout 30`, cooperative
`--ack-timeout 5`. ACK is a sampled snapshot, not an HDMI/input-latency guarantee.
Some games need a larger bounded ACK limit; never use unbounded polling.

Each CLI call opens a fresh session and clears earlier overrides. For several
commands in one session, use `RemoteInput` with an identified `USBTransport`
inside `open_serial`: call `open()`, `state({1: pad(mask), 2: mouse(dx,dy,mask)},
ticks)`, `status()` and `release(mask)`. Unselected slots keep their current
source; release mask 1/2/3 clears the selected overrides. Opening a fresh session
or resetting the ROM clears both. After a timeout, fail/report it and let the
lease expire; do not blindly retry movements or reset the console.

## Guest behavior and ownership

`ui_ed_detect()` makes two volatile Main-CPU reads at A130D4 and requires the
55A signature plus bit-3 strobe. If absent, no FIFO reads/writes occur and both
slots use physical snapshots. The host separately requires Pro protocol 05 /
device 18. This interface detection does not classify the base console.

A selected slot replaces its physical snapshot until expiry; it is not ORed with
physical buttons. The other slot remains unaffected. After expiry/release it
returns to the **current** physical state, which may include held buttons.
Edges are computed once per input frame; a kind change releases the old
namespace and presses the new one. Mouse deltas are consumed once, while its
buttons remain held for the lease. Movement is bounded to -2047..2047 per packet.
These are helper bounds, not console limits. The lease is at most 120 actual
VInts (two seconds at 60 Hz, 2.4 at 50 Hz), independent of host liveness.

The readiness check's separate HLV1 five-second lease reports host liveness.
The shorter UIV1 per-slot lease bounds remote button holds. Both use the same
existing counter for different purposes. One FIFO owner demultiplexes HLV1 and
UIV1; do not keep the old independent FIFO reader or add USB logging beside it.
Per input iteration, read at most 32 bytes. Partial frames expire after one
second. At most one UIV1 ACK plus two legacy replies means at most 82 outgoing
command bytes; no busy waits, disk setup, flush, DMA or IRQ owner is introduced.

## Wire contract v1

This is our cooperating-ROM protocol, carried by published EverDrive FIFO/USB
commands. Each UIV1 packet is 40 bytes, big-endian:

| Bytes | Request | ACK (kind 128) |
| --- | --- | --- |
| 0..3 | ASCII UIV1 | Same |
| 4 / 5 | Kind / selected-slot mask | 128 / active remote mask |
| 6..7 | VInt lease | Largest remaining slot lease at sampling |
| 8..11 / 12..15 | Nonzero session / sequence | Echo request session/sequence |
| 16..23 / 24..31 | Slot 1 / 2: kind, reserved zero, held u16, dx/dy s16 | Sampled kind, remote flag, held, dx/dy |
| 32..37 | Zero | Sampling VInt u32, result u8, capabilities u8 (3=pad+mouse) |
| 38..39 | CRC16-CCITT of first 38 bytes, polynomial 1021, init FFFF | Same |

OPEN=1, sequence=0, zero mask/lease/slots, establishes a fresh session. STATE=2
requires strictly increasing nonzero sequence, mask 1/2/3, lease 1..120 and
zero bytes for unselected slots. RELEASE=3 uses mask 1/2/3 and zero lease/slots.
STATUS=4 uses zero mask/lease/slots. Results: 0 accepted, 1 unknown kind,
2 unopened/wrong session, 3 stale/duplicate sequence, 4 invalid arguments.
Validate the whole update before mutating either slot. Bad CRC gets no reply.
Duplicates never replay mouse motion. The host requires matching session,
sequence, CRC and successful result before proceeding. This is cooperative
testing, not authentication or frame-perfect playback.

Primary transport facts: [Krikzz EDIO registers](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio/everdrive.h),
[CMD_USB_WR/FIFO implementation](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio/everdrive.c),
[edlink transport](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Device/DeviceIO_V1.cs).
[SGDK 2.11 joy API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/joy.h)
and [mouse implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/joy.c)
establish native types, buttons and direct-port axis handling.

## Readiness acceptance

Build/upload Hardware Lab with the normal hard-reset default. Send four separate
RIGHT presses to reach Controllers, releasing between them; A locks page
navigation. Verify both pad masks and raw physical fields independently, then
mouse on each slot and both together. Compare pointer coordinates across
renewals: zero-delta renewals must not repeat movement. Release/let the lease
expire, and inspect source/mask fallback. The script's ACK alone is not visual
evidence; use fresh capture frames with the visible build ID. Do not trigger the
Lab's guarded SRAM action while automating input. The Lab's other controls still
perform their normal reset/audio/save actions if sent the corresponding inputs.
