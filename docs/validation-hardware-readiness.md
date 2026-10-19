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
