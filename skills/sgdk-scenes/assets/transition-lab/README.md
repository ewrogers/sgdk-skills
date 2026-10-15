# Transition Lab

A small two-player coin adventure that shows how screens fit together:
Title, Continue, Options, character selection, four rooms, pause and Credits.

The first three rooms have different coin patterns, solid stone blocks and
their own music. The last room adds a guardian that chases players, routes around the blocks, stops to
charge, then fires an eight-way laser. Hits scatter only the hit player's
coins outward, with a separate visual hop. Collect all eight coins to defeat
the boss, then take the M coin to finish.

## Build and run

Copy this whole folder to a new working directory. With Docker running, use:

```sh
docker pull ewrogers/sgdk:latest
docker run --rm ewrogers/sgdk:latest doctor
docker run --rm --mount "type=bind,source=$PWD,target=/workspace" ewrogers/sgdk:latest build release
```

PowerShell uses `${PWD}` instead of `$PWD` in the mount. Docker selects its
native Linux ARM64 or AMD64 image. Add `--platform linux/arm64` or
`--platform linux/amd64` to pull/run to choose explicitly. Use `build debug`
for a debug ROM or `build clean` to clean this copy.

Open `out/rom.bin` as a Mega Drive cartridge. On macOS with ares configured:

```sh
open -a ares --args --system 'Mega Drive' "$PWD/out/rom.bin"
```

## Controls

| Screen | Controls |
| --- | --- |
| Title / menus | D-pad chooses, A or Start selects, B goes back |
| Character select | Each player's Start joins; Left/Right changes character |
| Countdown | A skips one second per press; B cancels selection |
| In a room | D-pad moves; Start pauses |
| Pause | Resume or Title / Keep Checkpoint |

Either player may join first; both may choose the same character. The first
join starts a full ten-second countdown. A second join does not restart it.
On ares, the usual P1 mapping is arrows/WASD and J/K/L/Return for A/B/C/Start.

## Checkpoints

Entering each fresh room saves its starting state to SRAM. Continue returns to
that room's beginning, with the score earned in earlier rooms. Leaving for
Title or finishing the game keeps the existing checkpoint. A failed write
keeps the older valid slot and shows a warning.

This lab uses two versioned 48-byte records, not a raw C struct or an emulator
savestate. Earlier development saves can be rejected when their format changes.
Do not delete or replace a user's save files as part of testing.

## What to check

- Visit Options and Credits, then return. Check music/effects options.
- Join with P1 first, P2 first and both together. Change characters; try holding
  Start and A through the transition. Each physical press should count once.
- Collect every coin in each room, walk around the blocks and pause/resume.
  Check that coin patterns, backgrounds and music change.
- In the boss room, avoid its contact and charged laser. Hits should drop only
  your coins. They should scatter nearby, bounce, settle and remain collectible.
- Collect all eight, watch the smoke, take the M coin and reach Credits after
  the fanfare. Continue should still restart the boss room.
- Reset and Continue to check emulator SRAM persistence. Repeat screen changes;
  the Title heap reading should return to a comparable baseline.

Host tests check model behavior, save recovery, reachability, partial loading
and the order of SDK calls. Container builds check actual APIs and resources.
Those do not replace emulator checks or physical-console tests, especially
fade/DMA timing, audio and SRAM retention.

## How the example is split

- `lab.c`: menus, joining, pause and transition requests.
- `level.c` / `arena.c`: room layouts, movement, coins and bounded boss effects.
- `path.c`: bounded breadth-first routing for the 32-pixel boss.
- `scene_switch.c`: one pending transition and safe fallback.
- `scene_video.c` / `view.c`: SGDK cleanup, palettes, text and sprites.
- `checkpoint.c` / `storage.c`: byte format and short SRAM transactions.
- `main.c`: cached input, audio, level-entry saves and the frame loop.

The game keeps its coin/projectile storage fixed and lets SGDK manage sprite
allocations. It retains one sprite engine, reserves 80 sprite tiles and has eight coins and
four laser slots. Boss-room rendering uses at most 17 SGDK sprite objects.
The boss uses an 8-pixel navigation grid and about 2.4 KiB of reusable search
workspace. Searches happen at waypoints, with a retry delay when no move is
needed; this is a small-room example, not a general pathfinding service.
Hardware scanline limits and DMA remain separate budgets. PAL/NTSC clock
behavior is host-tested; this scene lab's visual playtest used NTSC in ares.

Title and boss art were made for this example with PixelLab, then indexed to
the lab palettes without rescaling their pixels. The small robot and click
are original assets reused from this collection. `tools/make_assets.py`
recreates the coin/effect art, sounds and seven FM/PSG themes in a new folder;
it refuses to overwrite existing files. All game assets are resident in ROM.

See [transition ownership](../../references/transitions.md) and
[game flow](../../references/game-flow.md) for the verified APIs and tradeoffs.
