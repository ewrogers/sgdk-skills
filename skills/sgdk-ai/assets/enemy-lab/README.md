# Enemy Lab

A small top-down stealth challenge. Reach the exit in each of three areas
without getting caught. You have three shared tries; a hit restarts the current
area, and running out shows Game Over. Clear all three to finish the run.

Each run opens on enemy help. Left/Right switches between Bombardier (gold),
Shocker (red) and Sentry (purple); A or Start begins the game. There is no reading
timeout. Retries go straight back to the current area.

Bombardiers patrol and lob bombs, Shockers chase around obstacles, and Sentries
stop and aim before firing. An `!` and alert sound tell you
you've been spotted. Use the stun pulse to create an opening: `zZ` marks a
stunned enemy, which flashes during the last second before recovering.
Your pulse recharges in three seconds. Its HUD bar fills blue, then turns green
when ready; a second bar appears if P2 joins. Pause freezes the recharge.

Touching an enemy costs a try, even while it is winding up or recovering.
Stunned enemies are harmless. Your flashing robot is temporarily invulnerable
after entering an area, so contact during that brief period does not hurt.
Bombardiers can spot and throw across one cell of cover, but thicker walls
block them. A marked landing spot gives you time to dodge: the bomb flies for
half a second, waits one second, then explodes in a plus shape. Walls stop each
arm of the blast. Shockers use a red taser one tile in front of them. Attack
cooldowns are four seconds for Bombardier, one for Shocker and three for Sentry.
Stun interrupts an attack being prepared; a bomb already thrown keeps going.

The blue entry tiles mark your starting position, sheltered by nearby walls.
They do not grant immunity. Each new area starts at the previous area's exit,
and its music becomes faster and more ominous as you progress.

Robots use smaller body collision boxes and wider entry doorways to make tight
spaces easier to navigate. The outer wall marks the playfield boundary. Interior
walls make you cross each room rather than taking a shortcut along the edge.

Reaching an exit plays a short clear jingle. Caught, Area Clear, Game Over and
Mission Complete appear on solid panels above the actors. Music pauses at
Game Over and completion; the game is still waiting for Start, with no timeout.

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
| A | Stun nearby enemies; begin the game from help |
| B | Start a new run |
| C | Cycle Normal, Sense and Path views |
| P1 Start | Pause; begin the game from help; start a new run after Game Over or completion |
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

- Approach a Bombardier from the front and behind. Break sight behind a wall and
  watch it investigate the last seen position, then return.
- Make a Shocker chase around the long wall. Watch Path view; it should not
  cross solid cells or snap through a corner.
- Dodge a melee wind-up and a sentry shot. Both should face their aim; an alert
  should not blink the enemy's body. Listen for alert and firing sounds.
- Lure a Bombardier, move away from the landing marker and dodge the cross-shaped
  explosion. Thin cover permits a lob; walls still block the blast. Dodge to
  the side of a Shocker's red taser and check the different attack cooldowns.
- Stun an enemy between grid points. Check `zZ`, the final-second flash and
  recovery without becoming stuck. Pause should freeze those timers and paths.
- Reach all exits. Get caught and check that only one try is lost, the area
  restarts, and Game Over waits for a fresh Start press. B restarts the whole run.
- Listen for one clear jingle per exit. Check that banner text stays readable
  over walls and enemies, and that its background disappears on the next area.
- Leave Game Over waiting, then press Start. It should restart even after a long
  wait; stopped music alone does not mean the game has crashed.
- Walk through each entry doorway slightly off-center. Check that the visible
  bottom wall matches where movement stops, and that the exits require going
  around the interior walls.

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
exit graphics and clear jingle in a new folder and refuses to overwrite existing
files. `tools/make_music.py` reproduces the three FM area loops. The robot sheet,
music and samples remain resident in ROM. This example does not
require another installed skill.

Host tests execute the actual C model, compare shortest paths with an independent
oracle and check HUD calls with a recording SDK. Container tests check real
APIs and release/debug ROMs (256 KiB padded, with embedded symbols in debug).
Combat tests cover contact, bomb phases and cover, taser direction, attack
cooldowns and stun interruption. Help tests check browsing, input edges and
frozen gameplay with no reading timeout.
Adapter tests wait ten minutes or more at each end screen at both refresh rates,
then restart and check input polling, music restoration and one-shot clear audio.
NTSC ares playtests cover the controls and flow;
PAL visual timing, CPU measurements and physical hardware remain separate.

See [states and perception](../../references/behavior.md) for the exact rules.
