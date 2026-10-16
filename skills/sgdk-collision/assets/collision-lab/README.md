# Collision Lab

Three small demonstrations of collision queries in an SGDK game. The cyan robot,
red walls and amber shots are original native-resolution test art. Their style is
illustrative; use your own game's presentation.

## Build and play

Pull the current toolchain, then generate a fresh project:

```sh
docker pull ewrogers/sgdk:latest
python3 tools/build.py --output /path/to/new/collision-lab --platform linux/arm64
```

Run this command from this Lab directory. Use `python` on Windows and
`linux/amd64` for an x64 Linux container. Omitting `--platform` uses Docker's
native choice. `--mode debug` builds debug; `--region pal` or `--region ntsc`
sets the ROM region preference. Open the resulting `out/rom.bin` in ares as a
Mega Drive game. The helper refuses existing output folders and copies all
source, art generation and build helpers, so the copied project stands alone.

| Control | Action |
| --- | --- |
| C | Next view: shots, rays, sliding |
| A | Fire again, or reset the current ray/player |
| B | Shots view: switch 4 / 40 pixels per logical tick |
| Up | Shots view: toggle wall movement |
| D-pad | Rays: aim the crosshair. Sliding: move the robot. |
| Start | Pause / resume |

In **Fast Shots**, the top shot checks only its destination. At 40 pixels per
tick it skips the thin wall. The bottom shot sweeps its entire motion and stops
at first contact. Turning on wall motion uses relative velocity for that sweep.
A shot flashes at contact; walls keep moving rather than physically pushing it.

In **Aiming Rays**, amber dots stop at the nearest wall. The white crosshair is
your requested endpoint; the extra marker at a hit points out its contact side.
The `TIME /4096` value is the segment fraction, not a distance or CPU counter.

In **Wall Sliding**, move diagonally into the obstacles and corners. The model
removes the blocked component and spends the remaining displacement along the
wall. `CANDIDATES` counts inspected boxes and `NARROW` counts exact slab tests
for the most recent logical tick, across at most three queries. These are work
counts, not milliseconds or FPS.

## Code and contracts

`inc/collision.h` / `src/collision.c` are the reusable query example.
`src/demo.c` is pure gameplay logic; `src/main.c` owns SGDK display and controls.
The routines are original game C, not new SGDK APIs.

- Coordinates use **Q4 local units**, 16 units per pixel. They are not SGDK
  `fix16` or `fix32`. Origins and motion endpoints must lie in −1,024..+1,024
  pixels; box sizes are positive and at most 256 pixels per axis. Each body's
  motion is at most 256 pixels per axis per tick. Rebase larger worlds locally.
- Sweep hits retain an exact numerator/denominator. The displayed 0..4096 time
  rounds down; nearest-hit selection and movement use the exact fraction.
- Boxes have open interior for queries. Touching alone is not overlap; moving
  into a touching face hits at time zero. Parallel edge travel and instantaneous
  corner grazes miss. End-face contact at time one hits if it would enter after.
- Equal-time corner hits block both axes. List index breaks metadata ties.
- Initial overlap returns `CC_OVERLAP`, with no automatic depenetration. Use a
  safe spawn, previous safe position or a separately bounded recovery rule.
- `cc_ray` is a finite segment, with the same boundary policy. A start strictly
  inside returns overlap; a stationary point outside misses.
- `cc_cast` accepts up to 32 supplied boxes, validates them all, then rejects
  distant swept bounds before exact tests. It is a bounded list query, not a
  spatial index. A ray through multiple boxes can select the nearest exact hit
  as shown in the demo; an inside result needs its own policy.
- `cc_slide` handles static walls only, with at most three queries. Invalid or
  overlapping input stops and reports the problem. `CC_EXHAUSTED` returns the
  safe partial position and unapplied displacement if the budget is exhausted.
  Moving-target sweep detection does not implement pushing, carrying, impulses,
  rotation, slopes or a general physics solver.

The scheduler runs 60 logical ticks per nominal second, taking at most two
steps per displayed PAL frame. Pause preserves its fractional phase. This does
not compensate for dropped VBlanks or prove equivalent presentation at 50/60 Hz.
Collision response itself uses no allocation, floating point or unbounded search.
The bounds are part of its arithmetic proof: do not enlarge them casually.

## Presentation budget and validation

23 sprite instances reserve 40 tiles (32 tiles are needed at their maximum
frames); each is a single hardware piece. The ray uses 16 dots, submitted once
per display frame. A retained 2,240-byte HUD/map buffer queues at VBlank under a
4,096-byte DMA allowance and is not rewritten until queued reads finish. One
stone tile lives at index 96. UI and sprites use two palette lines. Initial
palette/tile setup occurs with display disabled; there are no HBlank writes.

A finite three-tick PSG tone marks swept impacts. It owns channel zero in this
Lab; use an audio owner if a real game's music also uses PSG.

Repository tests compare 12,000 queries with an independent Python rational
oracle at host O0/O3, exercise contacts/sliding/errors, compare PAL/NTSC logical
schedules and execute the same contracts plus a 1,000-query digest on an emulated
base M68000. Container checks build release/debug and PAL/NTSC region variants
on ARM64 and AMD64. These checks do not emulate VDP timing or prove real hardware
performance. Emulator playtesting is recorded separately when completed.

## Emulator check

On October 16, 2026, the NTSC build was loaded in ares v148. The user confirmed
all three views work: fast/moving-wall shots, aiming rays and wall sliding,
including their controls and pause. PAL runtime presentation and real hardware
remain separate checks; the automated PAL coverage above is logical/build testing.
