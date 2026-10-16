# Effects Lab: The Lantern Garden

Move a flying lantern through a small courtyard, leaving a fading trail. Create
spark/smoke explosions, change rain to snow, and compare density settings. A
fountain farther to the right keeps emitting sparks. The Lab is intentionally
silent; its original pixel art and setting are illustrative.

## Build and run

Requires Python 3.10+, Docker with Linux containers, and an emulator or console.
The helper uses only Python's standard library and generates the native indexed
art offline. No image service, asset download or API key is needed.

```sh
docker pull ewrogers/sgdk:latest
python tools/build.py --output "build/effects review"
```

Run from this Lab folder; choose a new output directory. The helper copies the
complete project and tools before building. Existing output/source art is never
overwritten. The result is `build/effects review/out/rom.bin` and the matching
`out/rom.out`. Pass `--platform linux/arm64` or `--platform linux/amd64` to select
an image architecture; native builds are preferable to CPU emulation.

For regional checks, build another directory with `--region pal` or
`--region ntsc`. This changes the header hint; the emulator's actual region must
match. Runtime detects 50/60 Hz with `SYS_isPAL()`. `--mode debug` builds a debug
ROM; release is the performance baseline.

On macOS, ares can load it directly:

```sh
/Applications/ares.app/Contents/MacOS/ares --system "Mega Drive" "/absolute/path/to/out/rom.bin"
```

On Windows/Linux, use your emulator's ROM picker or corresponding executable.
The helper passes paths as argument lists and uses the invoking user's UID/GID
where supported. Host helper portability is distinct from Docker/emulator
verification on every OS.

## Controls and counters

| Control | Action |
| --- | --- |
| D-pad | Move the lantern; the camera follows horizontally. |
| A | Start/restart the lantern’s spark/smoke burst. |
| B | Cycle clear, rain and snow. Existing particles finish their lifetimes. |
| C | Cycle four emission densities. |
| Start | Pause/resume particles, movement, animation and shake. |
| Up + Start | Toggle shake, clearing any current shake immediately. |

With the established ares mapping, A/B/C are J/K/L and Start is Enter.
Presses are edge-triggered, so holding A does not continuously create bursts.

- **FPS** is SGDK's measured rendered-loop frame rate, sampled once per frame.
- **LIVE** counts occupied simulation slots, including offscreen particles.
- **SHOWN** counts accepted effect sprites; it excludes the lantern.
- **HIDDEN** counts visible candidates skipped for the frame or scanline budget.
  It does not count offscreen particles, capacity rejections or frames per second.
- **50HZ/60HZ** identifies the detected regional refresh rate, not measured FPS.

HUD fields use bounded digit formatting and upload only changed rows, at most
once every six rendered frames. FPS and particle counts have different labels.
Fast-forward rates above 99 are clipped by the two-digit FPS display.

## Resource ownership and budgets

| Item | Lab reservation/use |
| --- | --- |
| Simulation | 64 slots; 16 bytes per particle on the tested m68k compiler |
| Weather | At most 32 live rain/snow particles, within that same pool |
| Whole-frame rendering | Up to 64 effect pieces plus one lantern |
| Scanlines | At most 8 effect rectangles per line, plus the lantern |
| Animation cache | 24 tiles / 768 bytes at tile indices 128–151 |
| Background/panels | 9 tiles / 288 bytes at indices 16–24 |
| Palettes | PAL0 HUD, PAL1 effects, PAL2 lantern, PAL3 landscape |
| Model / render storage | 1,126 / 550 bytes on the tested target |
| Sprite upload | One queued SAT transfer, up to 65 × 8 = 520 bytes per frame |

These counts exclude SGDK's font, plane tables, persistent SAT cache, other SDK
state, stack and queue storage. They are not the entire game's RAM/VRAM cost.
The normal padded ROM is 128 KiB; that is not the amount of used VRAM.

An internal free-slot stack makes ordinary admission/reuse constant-time; only
a full-pool priority replacement searches for an older candidate.
The pool prioritizes impact feedback over trails over weather. At full capacity,
a higher-priority request replaces the oldest lower-priority particle. Weather
also has its own allowance so long-lived snow cannot occupy the whole pool.
The four rates emit every 12, 8, 5 or 3 simulation ticks. Capacity, weather and
render allowances are explicit Lab tuning choices, not SGDK or hardware caps.
Each burst has eight sparks and four smoke particles. Retriggering releases only
the previous lantern burst, preserving weather, trails and fountain particles.
This keeps rapid presses from accumulating an unlimited number of explosions.

The renderer owns the entire low-level sprite table. It preloads every frame
while display is disabled; no per-particle tile upload or heap allocation occurs
in the frame loop. Every accepted particle is one 8×8 hardware rectangle. The
last SAT link is always zero, including after the list shrinks. Do not call
`SPR_update()` alongside this adapter; integrate through a single compositor
if another renderer needs the table.

`fx_select()` requires non-overlapping state/output objects. It admits at most
eight effects in each touched eight-scanline band. This also guarantees the
per-line allowance, but may hide a few extra sprites near band boundaries. Drawing fewer particles does not
change simulation or RNG. Rotating equal-priority selection may visibly flicker
when many particles overlap; reduce density if that is undesirable.

## Checks and playtest

The repository's host tests run the actual C model at `-O0` and `-O3`, with
undefined-behavior sanitization on Unix. They cover admission/replacement,
counts, lifetime, world-space trails, pause/edges, nominal PAL/NTSC time, camera
limits, fair selection and a 100,000-frame bounded run. A byte-counter reference
checks 5,000 selections against the optimized band selector. The recording
SDK checks frame-upload failures, shared indices, SAT links, shorter chains,
HUD isolation/caching and idempotent cleanup.

Container tests build release/debug ROMs, execute core contracts as base M68000
instructions and compare the target trace with host results. Every linked sprite
frame/palette is decoded against its native PNG. A fixed full-pool selection
workload reports optimized/reference instruction counts. Another target workload
combines maximum snow, movement and repeated bursts, including spawn frames.
These counts are not hardware cycle measurements. ARM64/AMD64 reports and ROM hashes must match in CI.

When playtesting, try all weather/density combinations, repeated bursts,
moving to both camera boundaries, pause during shake and disabling shake while
it is active. Check FPS separately from SHOWN/HIDDEN. Long-lived maximum snow
exposed the first build's slowdown; keep that sustained case in future checks.
Build a fresh directory with `--stress` for automatic maximum-density snow,
horizontal movement and bursts every 12 simulation ticks (five per second at
NTSC). It displays `STRESS LATE`: rendered iterations that spanned more than
one VBlank, capped at 99. Let it run past snow’s three-second lifetime and watch
both FPS and that counter; an averaged FPS value can hide isolated missed frames.
This mode replaces manual input. Build without `--stress` for normal controls.

The revised NTSC ares run held 60 FPS with zero late frames during this combined
test. The earlier weather-only test missed the burst regression. A passing
CPU/resource probe does not certify frame timing, flicker or physical hardware. The 64-slot full-pool test is a correctness/stress case, not a promise
that any arbitrary mix of 64 effects fits a frame.

For scene handoff, stop emitters, call `fx_clear()` and `fx_view_end()`, then drain
that queued hidden table through VBlank before giving its cache/tiles to a new
owner. The standalone Lab has no scene manager or save data.
