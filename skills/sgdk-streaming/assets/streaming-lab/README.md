# Streaming Lab: Copperline Courier

Walk through Moss Garden, Amber Works, Tidal Quay and Moonlit Ruins. The courier
and HUD stay resident while a small tile cache replaces scenery as you travel.
The original native art is illustrative. The Lab is silent and has no combat,
collision or saves.

## Build and run

Requires Python 3.10+, Docker running Linux containers, and an emulator or console.
Run from this Lab directory, choosing a new output folder:

```sh
docker pull ewrogers/sgdk:latest
python tools/build.py --output "build/streaming review"
```

The helper copies a standalone project, generates native indexed PNG references
and raw tiles, then builds `out/rom.bin` and the matching `out/rom.out`. It uses
Python's standard library and refuses to overwrite output. Select an image with
`--platform linux/arm64` or `--platform linux/amd64`. Use release for timing;
`--mode debug` builds debug.

`--region pal` / `--region ntsc` changes the header hint; the emulator must really
run that region. Runtime detects 50/60 Hz. On macOS, load directly with:

```sh
/Applications/ares.app/Contents/MacOS/ares --system "Mega Drive" "/absolute/path/out/rom.bin"
```

Windows/Linux can use the emulator ROM picker. Helper portability is separate
from emulator and physical-console verification on every platform.

## Controls and loading

| Control | Action |
| --- | --- |
| Left / Right | Walk; camera and prefetch follow. |
| A | Warp to the next area with a covered redraw, retaining cached art. |
| B | Switch pattern chunks between 256 and 64 bytes per displayed frame. |
| C | Empty and reload the cache, resetting statistics. |
| Start | Pause movement/animation; background loading may finish. |

With the established ares mapping, A/B/C are J/K/L and Start is Enter.
HUD shows ready pages, eviction count (capped at 99), the pattern chunk allowance
and measured rendered-loop FPS. `CHUNK 064B` deliberately loads slowly: one
area's 3,072 bytes take 48 frames, versus 12 at 256 bytes.

Before revealing the world, loading warms a 128-pixel margin in both directions.
Walking should not snag at boundaries in either budget mode. Warps and explicit
reloads intentionally show a resident Window cover while art and tilemap columns
are prepared. A warm warp can still need a redraw.

## Repeatable walking test

Build a fresh directory with `--demo` for automatic walking through all four
areas and back, using the slower 64-byte budget. This replaces manual input.
`WAIT` counts movement blocked for missing art; `LATE` counts loop iterations
spanning more than one VBlank. Both cap at 99. Watch them alongside averaged
FPS, which can hide isolated missed frames.

Local NTSC ares autoplay held 60 FPS with no readiness waits. Its counter
recorded one startup late iteration and no further late iterations during
sustained walking; this is an emulator observation, not a hardware guarantee.

Host regressions repeat bidirectional walks with PAL/NTSC clocks and both
budgets. Emulator checks establish actual presentation and timing; the CPU
probe is not a VDP or physical mapper emulator.

## Reservations and ownership

| Resource | Lab use |
| --- | --- |
| Catalog | 24 pages × 16 tiles = 384 tiles / 12,288 bytes |
| Replaceable arena | 18 pages = 288 tiles / 9,216 bytes, indices 128–415 |
| Resident art | Courier: 8 tiles at 96–103; panel: 1 tile at 104 |
| Pattern upload | At most 8 tiles / 256 bytes per displayed frame |
| Other queued work | Up to four 48-byte columns during covered redraw, one while walking; palettes, HUD and one sprite |
| Explicit DMA allowance | 1,536 bytes shared by Lab producers |
| Cache + staging | 492 bytes on tested m68k compiler, including 256-byte staging and 24-byte direct lookup |
| Other persistent buffers | Columns: 192 bytes; palettes: 64 bytes |
| Palettes | PAL0 HUD, PAL1/PAL2 areas, PAL3 courier |

These are Lab choices, not hardware maxima. The whole catalog could fit a larger
arena; the small allocation makes eviction easy to prove. Counts exclude the
SDK font, name tables, SAT cache, DMA temporary buffers, other globals and stack.
The normal padded ROM is 128 KiB.

The Lab owns the low-level sprite table and explicit tile ranges; do not mix it
with an independent `SPR_update()` owner. There is no per-page heap allocation.
Visible pages are pinned and incomplete pages have no published tile index.
Queued RAM stays unchanged until the frame drains.

A circular 64-column plane handles walking. Teleports use a resident Window
cover and incremental redraw. Two world palettes let neighboring areas appear
together; a reused palette bank changes only when its previous area leaves the
view. This is not HBlank palette switching.

## Banks and compression

This ROM is linear. Its read adapter already uses `FAR_SAFE` to copy chunks into
owned RAM before queuing them. Banked games must rebuild the SDK with matching
configuration and verify linker placement for common code, metadata and audio.
Never let another mapping invalidate a queued reader. See
[uploads and banks](../../references/uploads-and-banks.md).

Pages are uncompressed to keep costs auditable. Compression needs independently
decodable blocks, bounded scratch memory and a CPU budget as well as DMA. Small
uploads do not make a whole-tileset decode cheap.

## Checks

Tests run actual cache/world C at `-O0`/`-O3`, with Unix undefined-behavior
sanitization. They cover delayed completion, failed reads/queues, pinned
exhaustion, LRU reuse/wrap, 10,000 mixed requests, every camera alignment and
forward/reverse walking. A recording SDK remaps the temporary FAR view after
queuing and verifies the retained RAM bytes.

Container tests execute cache/world contracts as base M68000 instructions and
compare every linked scenery/courier byte with native art. ARM64/AMD64 CI checks
release/debug hashes and budgets. Hardware timing, flash-cart support and a
particular game's total headroom still need separate validation.
