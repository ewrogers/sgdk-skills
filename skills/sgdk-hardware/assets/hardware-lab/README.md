# Hardware Lab

A small original diagnostic ROM for observing SGDK startup, video, sprite
scanlines, DMA/audio, controllers and an opt-in SRAM record. No external art or
music is needed. The quiet FM theme and seamless PCM tone are reference sounds.

## Build

Docker must be running. From this directory:

```sh
docker pull ewrogers/sgdk:latest
python3 tools/build.py --output "/path/to/new hardware lab"
```

Open that folder's `out/rom.bin` in ares as Mega Drive. `--platform linux/arm64`
or `--platform linux/amd64` selects the container architecture. `--mode debug`
builds a debug ROM. `--region pal` / `--region ntsc` changes the region header;
also select the matching actual system region in the emulator. The ROM still
queries the running video flag. Output must be new and outside the source Lab.

The first page is a native tile-graphics readiness screen: chip/check artwork,
large title, boot/RAM results, animated running meter and a top-right build ID.
`build-info.json` records its source fingerprint, mode and requested region.
The default label is `HL-<8 source-hash digits>-<mode><region>` (`RA` means release,
automatic region). `--build-id READY-01` supplies an optional 1-20 character
uppercase/digit/`._-` label. The label is reproducible, not a timestamp or a ROM
checksum. It identifies the preparation snapshot: make a fresh prepared copy
after changing source or build settings, and record the actual ROM SHA-256 too.

Startup PASS requires SGDK's hard-entry flag, initialized DATA `1234`, and BSS
`0000` captured before the Lab changes those globals to `BEEF` / `CAFE`. A warm
entry or retained values shows REVIEW/CHECK; that is not a claim of bad
RAM. ROM RUNNING and the heartbeat show local execution, not a verified USB
round trip, electrical cold power cycle, or a complete hardware RAM test.
The region label is the actual SGDK video flag, not just the ROM header.

The five diagnostic pages share the same header, original 2x titles, page icons
and rounded panels. Video keeps its full-color ramp, fine patterns and four
corner crop markers. Sprite rows retain their piece counts, sizes and scanlines,
with horizontal spacing centered inside the panel in both modes.
DMA/Audio shows the selected extra upload as a bar, not a total VRAM budget.
Controllers shows both raw port/device/held values, native D-pad arrows and sampled button marks
(`S` = Start, `M` = Mode). Save keeps its explicit read and guarded write controls.
Sound's four animated bars follow actual `XGM2_isPlaying()` state using the
existing VInt clock. They indicate playback activity, not measured amplitude.

The page also probes EverDrive's EDIO signature/read strobe and shows DETECTED
or UNKNOWN. It does not infer the console model or prove physical hardware.
USB HOST CHECK starts NOT RUN and becomes PASS/LIVE only after a fresh host/ROM
challenge-response and acknowledgement. After five seconds without renewal it
shows PASS/IDLE, preserving the last successful result without claiming a current
host session. This includes when a bounded verifier run ends; it does not mean
the cable disconnected. These states track host verification activity, not cable
presence. With sgdk-project
installed, run its host client against this Lab:

```sh
python /path/to/sgdk-project/scripts/everdrive.py --port ACTUAL_PORT --timeout 40 verify-lab --duration 30
```

The USB helper requires host pySerial. No host check means NOT RUN; no detected
EDIO means UNAVAILABLE. C only clears display counters, not link state or its
lease; both reset entry paths clear the link state. Read the
[protocol and ownership notes](../../references/everdrive-link.md) before reuse.

## Controls and observations

Left/Right changes page. C clears counters. A/B are Mega Drive buttons (often
J/K in ares); configure your own keyboard/controller mapping as needed.

| Page | Controls | What to observe |
| --- | --- | --- |
| Readiness / Boot | A software reset; B forced init | Build ID, boot/DATA/BSS checks, moving heartbeat; VInt/logic/gap counters |
| Video Patterns | None | Color ramps, fine stripes, edge cropping and display artifacts |
| Sprite Scanlines | A toggles overload | At-limit rows versus expected trailing dropout above the piece/pixel limits |
| DMA / Audio | A music; B 0/1/2 KiB extra upload; Start toggles PCM | Compare music alone first, then continuous PCM; listen for changes and check missed service / rejected queues |
| Controllers | A locks/unlocks page; press/release either pad | Raw port/device type and held bitfields; physical pads still need testing |
| SRAM Record | A read; **hold B then press A write** | CRC-valid sequence survives read/reset; boot never writes |

The SRAM action overwrites **logical bytes 0–11 of this ROM's SRAM**. Use this
ROM's own save allocation. It is a single test record, not a production save
system. Empty/invalid is normal before the first write. Readback cannot establish
power-loss persistence or flashcart flush behavior.

The two sprite rows are independent: 20 versus 21 eight-pixel pieces, then
320 versus 352 sprite pixels on another line. All pieces are in one SAT; the
test stays below the total sprite limit. Low-level SAT writes deliberately avoid
sprite-engine culling. There is no automatic visual pass/fail judgement.

Changing numeric fields use one padded row copied into the VBlank DMA queue.
They are not erased and rewritten during active display.
The readiness page instead uses fixed-width hexadecimal fields: `V` is elapsed
VInts, LOGIC is nominal 60 Hz units, and MISS is missed loop intervals. Its meter
uses the existing VInt clock and refreshes every ten samples, not a second clock.
Counters begin after the first screen's queued setup reaches VRAM; C resets them.
B forces SDK initialization and memory clear, not electrical power cycling.

The shared art adds 296 tiles (9472 bytes) in VRAM `$0260..$275F`: 59 readiness
tiles plus 237 for page titles/icons, arrows and playback bars. It loads once per entry
with display disabled and remains resident below the private DMA workload.
Readiness uses all four palette banks; other pages reserve PAL1 for saturated
diagnostic colors and use the remaining banks for the shared UI. Page changes
clear both planes/sprites and set the destination palettes. Art/font generators and references
ship inside this Lab; there are no downloaded assets or repository test dependencies.

Queued upload data stays allocated and unchanged. Its private VRAM range is
`$8000..$87FF`; the workload is **additional** to text and sprite-table transfers.
The tone loops a 256-sample waveform on XGM2 channel 3; it has no one-second
restart gap and does not poll/stop the Z80 every frame. Turn it off with Start
before leaving the page if desired; changing page also stops it.

An 8192-byte software allowance and forced three-line DMA delay are test
policies, not console bandwidth guarantees. VInt counters cannot detect time
lost while interrupts are masked, independently measure Hz or certify sound.

Follow the skill's [console checklist](../../references/checklist.md). Automated
tests cover the helper contracts and base-68000 execution, not physical devices.
Until tested on a console, cold power/reset behavior, analog output, real pads,
DMA/Z80 timing and save retention all remain unverified.
Use a capture card to review the visible build label, startup values and several
heartbeat positions when available. Match those observations to the ROM hash;
upload acknowledgement alone is not visual verification.

## USB-controlled input test

The readiness ROM includes a local copy of the testing skill's optional `UIV1`
hook. A detected EverDrive interface enables it; without detection there is no
FIFO access and input follows the existing physical sampler. The normal sampler
still runs during overrides. Two logical slots independently accept a full pad
or Mega Mouse (left/right/middle/Start, relative X/Y). Held overrides expire in
at most 120 actual VInts; mouse deltas apply once. Explicit release or expiry
returns to the current physical state. No saves or device configuration are
changed by the hook itself; normal game actions still respond to mapped buttons.

Install the testing skill's host pySerial requirements, then use its helper:

```sh
python /path/to/sgdk-testing/scripts/usb_input.py --port ACTUAL_PORT --pad1 UP+B+X --pad2 DOWN+C+Z --seconds 5
python /path/to/sgdk-testing/scripts/usb_input.py --port ACTUAL_PORT --mouse1=20,8,left --mouse2=-16,-7,right --seconds 5
python /path/to/sgdk-testing/scripts/usb_input.py --port ACTUAL_PORT --release
```

The input helper and readiness verifier share one multiplexed FIFO owner. Do not
run two serial processes concurrently. `HLV1` is the separate readiness check;
`UIV1` controls logical input. Both use the existing frame counter. The source
files copied into this Lab are tested against the testing skill's reusable
snapshot, without requiring that skill at build/runtime.

On Controllers (four Right presses from readiness), A locks D-pad navigation for
input testing. Port source/type, logical HELD, raw physical bits, per-frame DX/DY
and pointer XY are shown in hex. Two small pointer tracks make movement visible.
The normal direct-port SDK owner supplies physical state; the hook does not
change port support, SDK joy state or accumulated axes. Check the kind before
interpreting buttons. Normal Lab A/B/reset/audio/SRAM bindings still apply when
those inputs are sent on their respective pages. The release ROM now fits the
Lab's 256 KiB diagnostic budget; this is not a console cartridge limit.
