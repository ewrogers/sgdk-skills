# Hardware Lab

A small original diagnostic ROM for observing SGDK startup, video, sprite
scanlines, DMA/audio, controllers and an opt-in SRAM record. No external art or
music is needed. The quiet FM theme and finite PCM tone are reference sounds.

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

## Controls and observations

Left/Right changes page. C clears counters. A/B are Mega Drive buttons (often
J/K in ares); configure your own keyboard/controller mapping as needed.

| Page | Controls | What to observe |
| --- | --- | --- |
| Boot / Region | A software reset; B forced init | Entry classification and retained globals; nominal PAL/NTSC cadence, VInt gaps |
| Video Patterns | None | Color ramps, fine stripes, edge cropping and display artifacts |
| Sprite Scanlines | A toggles overload | At-limit rows versus expected trailing dropout above the piece/pixel limits |
| DMA / Audio | A music; B 0/1/2 KiB extra upload; hold Start PCM | Compare music alone first, then PCM; listen for changes and check missed service / rejected queues |
| Controllers | Press/release either pad | Raw port/device type and held bitfields; physical pads still need testing |
| SRAM Record | A read; **hold B then press A write** | CRC-valid sequence survives read/reset; boot never writes |

The SRAM action overwrites **logical bytes 0–11 of this ROM's SRAM**. Use this
ROM's own save allocation. It is a single test record, not a production save
system. Empty/invalid is normal before the first write. Readback cannot establish
power-loss persistence or flashcart flush behavior.

The two sprite rows are independent: 20 versus 21 eight-pixel pieces, then
320 versus 352 sprite pixels on another line. All pieces are in one SAT; the
test stays below the total sprite limit. Low-level SAT writes deliberately avoid
sprite-engine culling. There is no automatic visual pass/fail judgement.

Queued upload data stays allocated and unchanged. Its private VRAM range is
`$8000..$87FF`; the workload is **additional** to text and sprite-table transfers.
An 8192-byte software allowance and forced three-line DMA delay are test
policies, not console bandwidth guarantees. VInt counters cannot detect time
lost while interrupts are masked, independently measure Hz or certify sound.

Follow the skill's [console checklist](../../references/checklist.md). Automated
tests cover the helper contracts and base-68000 execution, not physical devices.
Until tested on a console, cold power/reset behavior, analog output, real pads,
DMA/Z80 timing and save retention all remain unverified.
