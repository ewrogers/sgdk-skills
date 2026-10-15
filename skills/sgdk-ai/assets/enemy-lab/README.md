# Enemy Lab

A small top-down stealth challenge. Reach the exit in each of three areas
without getting caught. You have three shared tries; a hit restarts the current
area, and running out shows Game Over. Clear all three to finish the run.

Gold guards patrol with a sight cone, red hunters chase around obstacles, and
purple sentries stop and aim before firing. An `!` and alert sound tell you
you've been spotted. Use the stun pulse to create an opening: `zZ` marks a
stunned enemy, which flashes during the last second before recovering.

## Build

Copy this whole folder into a new working directory. With Docker running:

```sh
docker pull ewrogers/sgdk:latest
docker run --rm ewrogers/sgdk:latest doctor
docker run --rm --mount "type=bind,source=$PWD,target=/workspace" ewrogers/sgdk:latest make release
```

Use `make debug` for a debug build, or `make clean` to clean this copy. The
supplied makefile keeps an SGDK CPU-buffer compatibility flag; use `make`
rather than the container's `build` shortcut for this lab.

PowerShell uses `${PWD}` instead of `$PWD` inside the mount. Docker selects the
native ARM64 or AMD64 Linux image; add `--platform linux/arm64` or
`--platform linux/amd64` to pull/run to choose explicitly.

Open `out/rom.bin` as a Mega Drive cartridge. With ares configured on macOS:

```sh
open -a ares --args --system 'Mega Drive' "$PWD/out/rom.bin"
```

## Play

| Control | Action |
| --- | --- |
| D-pad | Four-way movement |
| A | Stun nearby enemies through clear sight |
| B | Start a new run |
| C | Cycle Normal, Sense and Path views |
| P1 Start | Pause; start a new run after Game Over or completion |
| P2 Start | Join optional co-op with the same shared tries |

The usual ares keys are arrows/WASD and J/K/L/Return for A/B/C/Start. The first
exit is in the upper-right corner, the second upper-left, and the third near
the upper-right. Either active player can reach it to advance the team.

The HUD shows tries and one alert light per enemy. Red means chasing or
preparing an attack; blue means stunned. Inactive lights are dim. The footer
shows the nearest enemy's state and that frame's search expansions. `NAV 16/16`
means 16 cells expanded, not 100% CPU usage.

Sense view outlines the nearest enemy's acquisition range/cone, without showing
wall occlusion. Path view marks its currently stored waypoints. Pending searches
can briefly have no markers. Walls can hide you; chasing enemies remember the
last place they saw you for two seconds, then return home.

## Try these checks

- Approach a guard from the front and behind. Break sight behind a wall and
  watch it investigate the last seen position, then return.
- Make a hunter chase around the long wall. Watch Path view; it should not
  cross solid cells or snap through a corner.
- Dodge a melee wind-up and a sentry shot. Both should face their aim; an alert
  should not blink the enemy's body. Listen for alert and firing sounds.
- Stun an enemy between grid points. Check `zZ`, the final-second flash and
  recovery without becoming stuck. Pause should freeze those timers and paths.
- Reach all exits. Get caught and check that only one try is lost, the area
  restarts, and Game Over waits for a fresh Start press. B restarts the whole run.

## How it works

`game.c` handles input, tries and area flow. `senses.c` decides what enemies can
see; `enemy.c` owns states and movement. `navigation.c` shares one bounded BFS
planner fairly across enemies. `world.c` is the common solid-cell geometry;
`shots.c` owns the fixed projectile pool. `view.c` and `main.c` use SGDK for
sprites, palettes, joypads, VBlank and XGM2 audio.

There are six enemies, eight projectile slots and 16 stored waypoints per
enemy. The planner uses 648 bytes for its queue/parents and expands at most
16 cells per displayed frame. These are example settings, not engine or console
limits. See [navigation and budgets](../../references/navigation.md) before
increasing them or adapting the grid to larger creatures.

All art/audio is original work reused or made for this skill collection.
`tools/make_assets.py` reproduces the indexed pulse, projectile, status, try and
exit graphics in a new folder and refuses to overwrite existing files. The
robot sheet and FM/PSG music remain resident in ROM. This example does not
require another installed skill.

Host tests execute the actual C model, compare shortest paths with an independent
oracle and check HUD calls with a recording SDK. Container tests check real
APIs and release/debug ROMs. NTSC ares playtests cover the controls and flow;
PAL visual timing, CPU measurements and physical hardware remain separate.

See [states and perception](../../references/behavior.md) for the exact rules.
