# Hardware Lab readiness on a real console

On 2026-10-19, built Hardware Lab's revised first page, uploaded it to the user's
Mega EverDrive Pro with the hard-reset deployment path, and inspected direct
Elgato HD60 X captures on macOS. The screen uses original generated tile art,
boot/DATA/BSS results and the Lab's existing VInt clock for its heartbeat.
The build ID is top-right; the generic pending-hardware-verification footer
was removed. The other diagnostic pages and opt-in SRAM guard remain available.

## Build and protocol evidence

- Refreshed `ewrogers/sgdk:latest`; native ARM64, SGDK 2.11, GCC 16.2.0,
  binutils 2.47.20260726. Release build completed without warnings.
- Preparation ID: `HL-2449D9A0-RA` (release, automatic region).
- Preparation source SHA-256:
  `2449d9a0ccaef7623b2ef4f4f7ea2188bf9c03f973fd73da04f9866c57db4820`.
- ROM: 131072 bytes; SHA-256
  `0e5ed9dc32f4cd383f0a3a320792651e7a12dd72c0c203584f7399d14653a052`.
- Python uploader verified legacy Pro identity (device `0x18`, MCU `0x0112`,
  hardware `0x0001`, boot `0x0101`), performed hard-reset deployment, installed
  `usb-games/hardware-lab-readiness.bin`, and sent launch.

The build ID identifies preparation inputs, mode and requested region, not an
electrical power cycle or the binary checksum. Upload acknowledgement alone
does not establish runtime execution.

## Direct video observations

FFmpeg 9.0.2 captured six PNG frames from the named `Elgato HD60 X` device,
1920x1080, 30 fps input, `uyvy422`, sampled at 4 fps after a one-second delay.
Audio was disabled. No preview window was captured.

The agent reviewed the first and last frames: both showed the expected build
ID, ROM RUNNING, HARD START, INIT DATA `1234` PASS, ZERO/BSS `0000` PASS, and
VIDEO NTSC / NOMINAL 60. The heartbeat moved; V and LOGIC advanced from
`00000028` to `00000078`; MISS remained `00000000`. The title, chip/check art,
panels, top-right ID and lower information line were readable and unclipped.

Local ignored evidence is under
`build/hardware-readiness/top-right-if1xge0c/`: the prepared `Hardware Lab`, build
and upload logs, `hardware-validation.json`, and `capture/` PNGs plus capture
metadata/hashes. Generated files are not committed.

## Automated checks and limits

All four Hardware Lab tests passed, including actual release/debug and region
header builds, base-M68000 helper execution, generated resources, copied-project
preparation, build-ID reproducibility and useful rejection cases. The art uses
59 tiles / 1888 VRAM bytes below the Lab's private DMA workload region.
The 44 USB/project/capture/distribution checks also passed (48 focused tests
total), and the Skills CLI copied all 38 skills with their bundled resources.

This trial verifies the readiness page's visible boot values and short heartbeat
sequence on this console. It does not certify every diagnostic page, physical
power cycling, full RAM integrity, USB guest telemetry, audio or console timing.
The current capture backend is macOS AVFoundation; concurrent short capture was
previously confirmed while Elgato Studio stayed live on this Mac.

## Follow-up: EverDrive detection and expiring USB verification

The subsequent `HL-E4B7CB31-RA` release adds an EDIO signature/read-strobe probe
and a bounded guest FIFO protocol. It leaves the console model unknown. Source
SHA-256 is `e4b7cb3180d4bc1dc20685ca570ddc659ea143c67ce197bee8d15879dccb37c6`;
the 131072-byte ROM SHA-256 is
`fcad4f5de16b3cf8f766cd4f7404224a47754c09a79de4715a0706175dff7550`.
The same refreshed image built release without warnings.

An initial attempt at the newer FCI mailbox interface read `$FFFF` on this Pro
and timed out without verification. The shipped implementation instead uses
Krikzz's established FIFO and CMD_USB_WR paths. It performs no electrical
power cycle, save writes or console-model inference.

After a hard-reset reupload, HD60 X captures showed EVERDRIVE EDIO DETECTED /
USB LINK WAITING. `verify-lab --duration 10` completed nine challenge-response /
ACK-completion exchanges in 10.267 seconds, with the host independently
verifying Pro device `$18`, MCU `$0112`, hardware `$0001`, boot `$0101`.
Inspected captures during the exchange showed USB LINK VERIFIED, HARD START,
DATA `1234` PASS, BSS `0000` PASS, and NTSC. The meter moved and V/LOGIC advanced
from `000000E6` to `00000113`, with MISS zero in both reviewed frames. Capturing
after six seconds without renewal showed USB LINK EXPIRED with continued
execution. The cable was not unplugged for this expiry check.

Ignored evidence is in `build/hardware-usb-link/fifo-pratnn5s/`: standalone
project, build/upload logs, host link JSON and waiting/verified/expired capture
manifests and PNGs. These observations establish recent bidirectional liveness
for this Lab and this Pro, not authentication, debugger features, cable-presence
sensing, every firmware/mapper, or full-console certification. The protocol and
its independent skill ownership are documented in sgdk-hardware's installed
EverDrive-link reference.
All 55 focused tests passed, including the updated release/debug target probes
and packet-byte comparison. The Skills CLI again copied all 38 skills/resources;
both edited skills passed the skill-creator validator.

The subsequent layout-only release `HL-83127FEB-RA` aligns HARD/SOFT and the
three PASS labels in column 29 and moves the video label to row 25, leaving
row 24 clear below the counter panel. It built without warnings, was deployed
with hard reset, and was inspected through HD60 X capture. HARD, DATA/BSS PASS,
EDIO DETECTED and USB VERIFIED were visible; the host completed six handshakes
in 6.264 seconds. ROM SHA-256:
`f11565cb8ed44ed5fcbc06860af646d081f3dd9b7c09c6d558eb0f7a60c52c95`.
Ignored evidence: `build/hardware-usb-link/aligned-d3ek1i91/`.

## Follow-up: all diagnostic pages share the visual style

The five diagnostic pages now have original native icons, 2x titles, matching
panels and a top-right build label. Shared presentation lives in `lab_ui.c` and
`lab_pages.c`; the main loop retains input decisions, SRAM access and its VInt
clock. All 283 UI tiles (9056 VRAM bytes) load with display disabled and stay
below the private DMA workload. PAL1 preserves the saturated diagnostic palette
on non-readiness pages. Music/PCM/load/lock changes refresh fields instead of
clearing the screen. The approved readiness layout is retained.

All five pages were hard-reset uploaded and inspected through the same HD60 X
path. Ignored preparation variants selected the initial page solely to capture
each layout; the SRAM variant performed its normal read on entry. These were
not controller automation or a new shipped startup mode:

| Page | Variant build ID | Observed |
| --- | --- | --- |
| Video | `HL-933A2481-RA` | Color ramp, fine checker/stripes, four corner crop markers |
| Sprites | `HL-41FE7B6E-RA` | At-limit piece/pixel rows with unchanged coordinates and sizes |
| DMA / Audio | `HL-AC25C03D-RA` | Sound controls, zero-byte selection bar, zero rejected queues |
| Controllers | `HL-613BCD95-RA` | Two raw port/device/held columns, button marks, navigation/lock hints |
| SRAM | `HL-115532AB-RA` | EMPTY/INVALID, sequence zero, guarded-write guidance, LAST WRITE NOT RUN |

A separate ignored variant cycled pages every three nominal seconds. Reviewed
capture frames 0-9 showed all six pages and a return to readiness with restored
colors and no stale page graphics. Page-entry work contributes to missed-loop
counts (the cycle accumulated gaps); this is not a steady-service performance
benchmark. The trial did not test audio quality, held-button interaction,
over-limit dropout, or save writes/power retention.

The normal default build `HL-8624579A-RA` was then rebuilt without warnings and
hard-reset uploaded. Source SHA-256:
`8624579aa1eb87cb96c0be82f4d35517da218e45e5597923103ef7d3c03755e3`.
ROM SHA-256 (131072 bytes):
`e3ac5ac5b64838b9413e9afc0547b554b2d60f455026dc44f6511641fa272914`.
Its final capture showed the preserved layout, HARD, DATA/BSS PASS, EDIO DETECTED,
USB VERIFIED and MISS zero. Five host exchanges completed in 5.258 seconds.
All 55 focused tests passed, including release/debug/region-header target builds,
base-M68000 contracts and expanded native-art/copy checks. Skills CLI copied all
38 skills/resources. Local evidence is under
`build/hardware-pages/final-qy8urdj0/` (`page-1` through `page-5`, `transitions`,
and final default build/capture in `normal`). Generated ROMs and captures remain
excluded from Git.
