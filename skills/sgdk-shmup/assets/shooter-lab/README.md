# Shooter Lab

A small vertical shooter built directly with SGDK. Play two stages with six waves each, collect
weapon/bomb/shield pickups and defeat two patterned bosses. The side HUD leaves a tall
240×224 playfield; the remaining 80 pixels show score, ship-shaped lives and weapon status.
Native pixel art, original FM/PSG music and PCM effects are included.
The PixelLab title illustration shares the ship palette at its native size;
`res/title-art.json` records its generation and palette conversion.
Their space theme and musical style belong to this example, not every shooter.

## Build

Copy this directory to a new project folder. With Docker running, build there:

```sh
docker pull ewrogers/sgdk:latest
docker run --rm --mount "type=bind,source=$PWD,target=/workspace" ewrogers/sgdk:latest make release
```

PowerShell can use `${PWD}` instead of `$PWD`. The ROM is `out/rom.bin`; keep
`out/rom.out` for matching symbols. Use `make debug` for a diagnostic build.
Choose Mega Drive when loading in ares. No BIOS is required.
The full-screen title has a START option, slow space music and a starfighter
illustration. Starting a run switches to a more driving, hopeful space track.

## Controls

| Control | Action |
| --- | --- |
| A or Start at title/end | Start a new run |
| D-pad | Move |
| Hold A | Fire |
| B | Bomb, spending one stock |
| Hold C | Slow focus movement and show the ship's core |
| Start during play | Pause/resume |

The review machine maps arrows/WASD to movement, J/K/L to A/B/C and Enter to
Start. Other ares installations should use their own mappings.

P pickups increase the weapon to two, then three parallel lasers. B pickups replenish
bombs up to three. B drops a pulsing payload. After 0.75 seconds its blast expands outward,
clearing bullets and small enemies as it reaches them. It damages the boss
once, but leaves at least one HP for normal fire to finish. Three hits end the run; surviving hits remove weapon upgrades and Twins, clear bullets and
give flashing recovery time. Kills build a capped score chain; missed enemies
or damage break it. Surplus pickups already on screen become points. New S/T drops are skipped
while their upgrades are capped. A new pickup announcement replaces the current voice.

After roughly 33 seconds, BOSS INCOMING warns you while you keep flying.
The atmospheric stage track fades out and the boss track fades in. The boss
flies down, then its overhead health bar expands before it can attack or take
damage. Its three repeating phrases are aimed fans, rotating rings and a
spiral stream, each with a wind-up and a recovery gap. Below half health, the
fan cadence tightens and, at its next movement decision, the boss follows wider sweeps with a repeating
downward lunge. Its bar changes from red to purple. Pause freezes the entrance, music fade and attacks.
The green S pickup gives a ten-second bubble that absorbs one hit. Its rim
flashes near expiry. A fresh S refreshes it rather than stacking extra hits.

Normal fire delivers the final hit. The boss erupts, loses hull fragments and
becomes a bright star before the clear banner and fanfare;
After Stage 1, the star/fanfare leads into Stage 2 after six seconds; A or Start
can advance once the destruction finishes. The second clear waits for a new
run, with the final star still pulsing and bobbing. The lab is a demonstration, not a complete campaign.

## Playtest checklist

- Start with A/Start, move normally and while holding C, and check the HUD stays
  outside movement. Focus should expose the core without changing its hitbox.
- Hold A and collect P. Watch the parallel paired and triple laser weapon. Collect B,
  then tap/hold B to verify one payload at a time. Move away while it arms.
- Collect S and take a hit. Check the bubble breaks without losing a life or
  weapon power, then verify an unused bubble eventually expires.
- Compare aimed shots, three-way fans and eight-way rings. Dodge close to a
  bullet and check the damaging core is smaller than the ship art.
- Take a hit, recover and confirm lives/power. Lose all lives and restart.
- Reach the boss, watch the warning, music handoff, expanding bar and three attack phrases, defeat it and restart.
- Check title particles, the squared bitmap font, music and the robot
  announcements for BOMB, POWER UP, SHIELD, BOSS INCOMING and GAME OVER.
- Defeat the boss and check the explosions, dissolving hull, star and fanfare.
- Pause during a dense pattern; actors, attacks, scroll and music should stop.
  Resume without replaying a held bomb.
- Read FPS/load after warmup with fire, effects and the boss together. Inspect
  crowded scanlines for disappearing bullets. Compile success does not establish
  frame-time headroom or console behavior.

The nominal model is 60 steps/second with one/two steps on a 50 Hz PAL display.
A missed VBlank still costs real time. PAL visuals/audio and physical-console
checks are separate from host timing tests.

## Source and assets

`game.c`, `patterns.c`, `stage.c`, `weapons.c` and `combat.c` are original portable game code.
`main.c`, `view.c` and `sound.c` use SGDK. No game framework or custom physics API
is hidden behind SGDK names. All actor pools are fixed; no per-shot allocation
or per-instance animation uploads are needed.

`tools/make_assets.py NEW_FOLDER` regenerates the indexed sheets and font at native
size. `tools/make_audio.py NEW_FOLDER` regenerates the original VGM/WAV sources.
Both refuse to overwrite existing source files.

The six committed robot WAVs are ordinary assets on every build host. Optional
`tools/make_voices.py NEW_FOLDER` uses eSpeak NG to regenerate them; it is
not needed to build on any host. The title particles reuse the hostile
bullet sprites while gameplay is inactive, rather than allocating another pool. The lab does not load sibling
skills or repository-root helpers at runtime.

For a horizontal game, reconsider the playfield/HUD, scroll axis, entry edges,
weapons and terrain together. The skill references discuss that variant;
this particular ROM remains a vertical shooter.

The HUD uses gold for Power, blue for Bombs and purple for Chain. An empty bomb
request makes a short click; it neither spends stock nor creates a payload.

## Stage 2

The second stage has a purple backdrop and a more cautious, ominous space track.
Its waves mirror the entrances and mix the enemy roles differently. The mobile
Aegis Reaver alternates SHIELD UP and EXPOSED windows; shots disappear against
its shield without damaging it. Plan a bomb so its expanding wave reaches an
exposed boss, then finish with fire. The enraged defensive window lasts longer.

Purple T pickups grant up to two Twin satellites. They orbit on opposite sides
of the ship, use its blue/white palette and fire straight upward while A is held.
At Power 3 each satellite fires two parallel lasers. Extra Twins
become points. Losing a life removes both Twins and resets weapon power. A shield
absorption does not count as losing a life. A heavier hit sound marks damage;
Game Over has its own sad looping track and delayed robot announcement.

## Dense-projectile rendering

The lab now builds one compact hardware sprite list, using SGDK's compiled
frame geometry and VBlank SAT transfer. It avoids general Sprite-engine object
updates for its single-piece art. The 48 projectile slots are split into 20
player lasers and 28 hostile shots; the full scene still reserves 78 pieces.
Each boss attack has its own projectile shape. Aegis Reaver has a separate
H-shaped hull. Test the crowded case with both Twins and maximum Power; a
successful build alone does not prove 60 FPS.
