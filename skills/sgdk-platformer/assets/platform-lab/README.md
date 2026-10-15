# Platform Lab

A small side-scrolling course for testing how an SGDK platformer feels. Run,
jump, stomp five enemy types, collect boots, charge through blocks, ride a
platform and uncover a hidden passage. Original pixel art and upbeat FM/PSG
music are included under the repository's MIT license.

## Build

Copy this directory into a new project folder. With Docker running, build from
that folder:

```sh
docker pull ewrogers/sgdk:latest
docker run --rm --mount "type=bind,source=$PWD,target=/workspace" ewrogers/sgdk:latest make release
```

In PowerShell use `${PWD}` in place of `$PWD`. The ROM is `out/rom.bin`; keep
`out/rom.out` for matching debug symbols. `make debug` builds a diagnostic version.
For ares, select Mega Drive explicitly when loading the ROM. No BIOS is needed.

## Controls

| Control | Action |
| --- | --- |
| D-pad | Move left/right |
| A | Jump; release early for a short hop; hold for a higher stomp bounce |
| Down + A | Drop through a one-way platform |
| B | Hold to charge after collecting the battery; release gold sparks to dash |
| C | Return to the latest checkpoint |
| Start | Pause/resume; after the goal, start a fresh run |

With the review machine's ares mapping, J/K/L are A/B/C and Enter is Start.
Other installations should use their own controller mappings.

The course introduces abilities in order. Gold boots grant a double jump.
The battery enables a charged dash; blue arcs become gold and a sound plays
when ready. The red speed pickup lasts six seconds. Marked gold blocks break
under a dash. Green flags mark checkpoints. The patterned wall near the end
hides bonus coins; entering reveals the passage.

Green enemies avoid ledges; amber walkers do not. Red enemies hop, violet
ones fly and pale armored enemies expose themselves between guarded phases.
Stomp from above or dash; touching an enemy from the side costs a heart.
The armored enemy is dangerous to stomp while its shell is closed.

## What to try

- Compare a tap jump with a held jump. Press A just before landing and just
  after walking off a ledge. Neither should require perfect frame timing.
- Jump through a thin platform, land on it, then drop through deliberately.
- Walk both directions over the slopes. Check foot placement and headroom.
- After the boots, use one extra air jump and wall jumps to climb the chute.
- Ride the moving platform, jump off and drop through it. Pause mid-crossing.
- Release B early, then hold until the gold sparks and release. Confirm the
  dash breaks blocks and cannot chain indefinitely in midair.
- Compare a stomp, side collision and guarded enemy contact. Check knockback,
  flashing invulnerability and checkpoint recovery after losing three hearts.
- Find the secret, reach the goal, hear the fanfare and restart with Start.
- Check scrolling, terrain edits and sound together. Read FPS/load after warmup;
  do not treat a single startup reading as a steady-state measurement.

The nominal simulation is 60 steps/second on 60 Hz NTSC and 50 Hz PAL displays.
PAL sometimes executes two steps before drawing. This does not compensate for
an overloaded game missing VBlank. Hardware timing and final movement feel
still require playtesting; host tests cannot prove them.

## Source and artwork

`game.c`, `terrain.c` and `enemies.c` are portable game logic. `main.c`, `view.c`
and `sound.c` are the SGDK adapter. Physics is original example code, not a
built-in SGDK controller. The solver supports this course's floor slopes and
horizontal one-way platform, not arbitrary rotated surfaces or crushers.

`tools/make_assets.py NEW_FOLDER` regenerates the indexed PNGs, Tiled TMX/TSX
and matching collision header together. `tools/make_audio.py NEW_FOLDER`
regenerates the original VGM/WAV sources. Both refuse to overwrite existing
source files. Edit the generator or update both map and collision data when
changing the course; the checked-in `layout.h` does not automatically follow
an external Tiled edit.

Animation frames are preloaded and shared. Boots and sparks are small overlays,
not a duplicate hero sheet. The ten pickup sprite slots become celebratory coins
after the goal. There is no per-frame allocation.

Powers, collected coins, broken blocks and revealed cover persist across a
checkpoint reset. Start after completion creates a fresh run. This lab does
not write SRAM or include a general scene framework.
