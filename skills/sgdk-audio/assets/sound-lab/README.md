# Sound Desk — expanded Sound Lab

An original audio workshop with FM arrangements, instrument auditions, sampled
effects and a small scene-music conductor. Its static recording-console art,
font and music are illustrative; they do not prescribe how another game sounds.

## Build

With Docker running, export/build a **new** standalone project:

```sh
python tools/build.py --output /path/to/new-sound-desk
```

From the repository root, the helper is
`skills/sgdk-audio/assets/sound-lab/tools/build.py`. Optional flags:
`--platform linux/arm64|linux/amd64`, `--mode debug`, `--region pal|ntsc`.
It uses `ewrogers/sgdk:latest`, preserves exit codes and refuses existing output.
Open the exported project's `out/rom.bin` in a Mega Drive emulator. After export,
Docker's `make release` / `make debug` can rebuild that project directly.

Generation needs Python 3.10+ and its standard library; no tracker, API key or
third-party art is required. Original VGM/WAV/PNG/font fixtures are generated
inside the new project. `tools/prepare.py NEW_DIRECTORY` exports without building.
Each export contains the helpers/sources needed to export another fresh copy.
The original three diagnostic source assets and their public regeneration script
remain available from the installed skill.

## Controls

**Left/Right changes pages; Up/Down selects an item.** In the usual ares mapping,
A/B/C are J/K/L, Start is Return. No emulator settings are changed.

| Page | A | B | C | Start |
| --- | --- | --- | --- | --- |
| MUSIC | Play selected arrangement | Pause/resume | Stop | Pause/resume |
| PATCH | Play selected audition | Pause/resume | Stop | Pause/resume |
| SFX | Play clean sample | Play abrupt-edge sample | Stop samples | Stop samples |
| MIX | Request selected scene | Play fanfare, then return | Send a competing SFX burst | Pause/resume |
| CHECK | Play/restart diagnostic phrase | Pause/resume | Click | Stop owned audio |

CHECK keeps special tests: Up plays the alarm, Down fades for one second,
C+Up toggles looping, C+Down changes FM/PSG volume, C+Left switches normal CH2
versus contested music CH1, and C+Right toggles 2048 bytes/frame of extra DMA.
Hold C before the direction. Its initial click may sound; let that finish before
comparing music alone. Plain Left/Right always changes pages.

Navigation stops owned playback. It waits for an active SDK fade to finish;
release and press the direction again afterward. MIX also declines pause while
fading, since pausing music does not freeze SGDK's VBlank fade task. Held buttons
are consumed on CHECK entry; they do not accidentally start music or toggle DMA.

## What to hear

- MUSIC: three original arrangements, from a gentler harbor phrase to a more
  active road and darker watch. They demonstrate choices, not genre templates.
- PATCH: FM Bell/Reed/Bass, isolated PSG tone and PCM drums. Listen across note
  ranges and releases. These are executable fixtures, not a tracker or synth UI.
- SFX: each pair has the same oscillator/pitch and sample length. A has shaped
  edges/decay; B begins/ends abruptly. The UI reports accepted/rejected requests,
  not audible completions. Holding a button does not retrigger or force redraw.
- MIX: scene requests fade and coalesce to the latest selection. A fanfare waits
  for observed playback before finishing, then restarts the latest scene from
  the beginning. It does not seek back into the old song or crossfade two players.
- MIX's burst coalesces duplicate coins and chooses the strongest ordinary cue
  for CH2, plus the alert on CH3. Music percussion stays on CH1. The channel bars
  show estimated admission leases, not measured sample position. HELD counts
  duplicate/cooldown suppression; DROP counts requests that lost admission.

PCM source levels retain headroom. FM/PSG volume/fades do not fade PCM loudness.
The adapter stops music-owned CH1 at a music handoff so a previous drum tail does
not cross into the next track. Pause stops effect channels rather than pretending
to resume stopped samples at their previous positions.

The original CHECK tests still demonstrate pending commands, completion, priority
rejection, loop count, music volume, fade ownership and deliberate CH1 contention.
It is useful for isolating audio from the larger arrangements.

## PAL, NTSC and DMA

Build with `--region pal` or `--region ntsc`, reload/power-cycle and verify the
on-screen PAL 50 Hz / NTSC 60 Hz label. Do not change emulator preferences.
The songs remain 60-Hz sources; XGM2 adapts sequencing to the console. CHECK's
`LAST 4S` should stay near 240 source frames, with WINDOW near 200 VBLANKS on PAL
or 240 on NTSC; its fade lasts about one second. The window uses actual VInt
time, not main-loop iteration count.
The MIX transition uses half a second in either region. Effect cooldowns and
leases use 60 logical ticks/second through a region adapter.

CHECK's extra DMA is a controlled 2048-byte/frame upload to unused pattern tiles.
The normal UI rebuilds only for changed state/input/metrics and uploads only changed rows; holding a sound button does not add a
full-screen transfer. Worst-case Window update is 2240 bytes, plus the optional
2048-byte transfer. Retained current/previous maps use 4480 bytes; the static
console uses one palette bank and no hardware sprites. This is not whole-system
RAM accounting or a worst-case game benchmark. The ROM is padded to 256 KiB.

The Lab enables SGDK's short DMA delay and retains normal VBlank/bus service.
Compare music alone, an effect alone, their mix and the same playback with DMA.
Correct tempo or zero missed music frames cannot establish clean PCM. PCM Hz is
N/A on PAL: SGDK 2.11's byte-sized per-frame counter cannot represent nominal
13.3 kHz at 50 Hz. Driver counters wrap. Listen as well as reading them.

## Checks and limits

Host tests run real C transport/conductor behavior, pending starts, fade ownership,
jingle restore, timeouts, cooldowns, bursts and rejection. They compare optimized
traces, inspect VGM structure/layers/loops and test non-destructive WAV processing.
Container tests compile actual resources/APIs, inspect linked alignment and
sample padding, build both modes/regions, and execute conductor contracts on a
base-M68000 model. ARM64/AMD64 ROM comparison is separate from listening tests.

A CPU model does not emulate the Z80 mixer, VDP, analog output or controller
protocol. Check audible seams, clicks, timing and DMA behavior in an emulator,
then on physical PAL/NTSC hardware before claiming console compatibility.
