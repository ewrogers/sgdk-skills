# Shooter Lab

A small vertical shooter built directly with SGDK. Survive six waves, collect
weapon/bomb pickups and defeat a two-phase boss. The side HUD leaves a tall
240×224 playfield; the remaining 80 pixels show score, tries and weapon status.
Original native pixel art, FM/PSG music and PCM effects are included under MIT.
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

P pickups increase the weapon to two, then three shots. B pickups replenish
bombs up to three. Bombs clear hostile bullets, defeat small enemies and damage
the boss. Three hits end the run; surviving hits lower power, clear bullets and
give flashing recovery time. Kills build a capped score chain; missed enemies
or damage break it. Surplus pickups become points.

The boss arrives after roughly 33 seconds of nominal play. Its entry is briefly
invulnerable, then it fires aimed fans. Half health adds rings and spiral shots.
Shots or a bomb can deliver the final hit. Clear shows a banner and fanfare;
A or Start restarts. The lab is a demonstration, not a complete campaign.

## Playtest checklist

- Start with A/Start, move normally and while holding C, and check the HUD stays
  outside movement. Focus should expose the core without changing its hitbox.
- Hold A and collect P. Watch the paired and three-way weapon. Collect B,
  then tap/hold B to verify only one bomb is spent per press.
- Compare aimed shots, three-way fans and eight-way rings. Dodge close to a
  bullet and check the damaging core is smaller than the ship art.
- Take a hit, recover and confirm tries/power. Lose all tries and restart.
- Reach the boss, watch its entry and half-health phase, defeat it and restart.
- Check music, warnings, firing/pickup/hit cues and the clear fanfare.
- Pause during a dense pattern; actors, attacks, scroll and music should stop.
  Resume without replaying a held bomb.
- Read FPS/load after warmup with fire, effects and the boss together. Inspect
  crowded scanlines for disappearing bullets. Compile success does not establish
  frame-time headroom or console behavior.

The nominal model is 60 steps/second with one/two steps on a 50 Hz PAL display.
A missed VBlank still costs real time. PAL visuals/audio and physical-console
checks are separate from host timing tests.

## Source and assets

`game.c`, `patterns.c`, `stage.c` and `combat.c` are original portable game code.
`main.c`, `view.c` and `sound.c` use SGDK. No game framework or custom physics API
is hidden behind SGDK names. All actor pools are fixed; no per-shot allocation
or per-instance animation uploads are needed.

`tools/make_assets.py NEW_FOLDER` regenerates the seven indexed sheets at native
size. `tools/make_audio.py NEW_FOLDER` regenerates the original VGM/WAV sources.
Both refuse to overwrite existing source files. The lab does not load sibling
skills or repository-root helpers at runtime.

For a horizontal game, reconsider the playfield/HUD, scroll axis, entry edges,
weapons and terrain together. The skill references discuss that variant;
this particular ROM remains a vertical shooter.
