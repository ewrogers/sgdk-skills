---
name: sgdk-shmup
description: Build SGDK shoot-em-ups with scrolling stages, aimed/spread/ring/spiral bullets, bounded projectile pools, weapon upgrades, scoring, enemy waves and boss phases. Use for vertical or horizontal shooter design and implementation, rather than general projectile code alone.
---

# SGDK Shmup

Choose the scroll direction and playable rectangle first. Then prove one enemy,
one readable attack and one satisfying weapon before adding a whole stage.
The game's mood, artwork, music, difficulty and scoring rules belong to the
developer; Shooter Lab's choices are examples, not genre requirements.

## Choose the reference

- [Playfield and flow](references/playfield.md): vertical versus horizontal
  layouts, HUD placement, scrolling, waves, score and recovery.
- [Bullets and encounters](references/combat.md): patterns, pools, collision,
  power-ups, bombs and boss phases.
- [SGDK rendering and budgets](references/integration.md): verified APIs,
  shared sprite frames, scanlines, DMA, palettes and runtime validation.
- [Shooter Lab](assets/shooter-lab/README.md): a playable vertical example with
  a side HUD, six waves, weapon upgrades, bombs and a two-phase boss.

The lab uses ordinary C game logic and SGDK directly. SGDK provides rendering,
input, sound and frame processing; wave scripting, bullet patterns, hitboxes,
scoring and boss state are original game code. Use `ewrogers/sgdk:latest`, pull
before building, and check unfamiliar APIs in the selected SDK. The reference
was checked against SGDK 2.11.

## Design for readable pressure

Keep player, enemy, projectile and pickup collision separate. A ship's damaging
core can be smaller than its artwork. Make a focus/slow mode display that core
if it helps the chosen game. Keep HUD pixels outside movement, spawn and culling
bounds. Horizontal play is not just vertical coordinates swapped: reconsider
reaction distance, terrain and enemy entrances.

Snapshot aimed shots when fired unless homing is explicitly intended. Build
spreads, rings and spirals from bounded tables or verified SDK math. Pick a pool
exhaustion policy, such as skipping a complete volley, and track refusals while
tuning. Do not allocate per projectile or silently turn missing visuals into
invisible damaging bullets.

Separate stage progress from actor lifetime. Use stable spawn order, bounded
per-tick work and explicit rules when enemy slots are busy. Boss entry, damage
windows, phase changes and defeat should be visible. Give recovery room after
a player hit and clear only the objects the game's rules intend to clear.

## Budget the busy moment

A bullet is not free because its art is small. Budget simulation, collision,
RAM, sprite count, sprite pixels per scanline, VRAM and DMA separately. Culling
and retirement are different operations. Reuse resident animation frames where
appropriate and avoid needless uploads for repeated bullets.

Measure an optimized ROM with the busiest attack, player fire, effects and
music together. Check focused and normal movement, pause, power changes and
nominal PAL/NTSC timing. Input edges must not repeat on PAL's occasional second
simulation step. A fixed clock does not recover real time lost to missed frames.

## Test the rules, then play

Execute tests for pattern symmetry/direction, full pools, collision at maximum
speed, one hit per projectile, invulnerability, score bounds, wave ordering,
phase transitions, restart and pause. Compare broad-phase collision candidates
against an independent reference before optimizing. Include long deterministic
runs and target-code checks where host arithmetic cannot establish behavior.

Build tests establish API/resource compatibility; they do not prove VDP timing,
art readability or sound quality. Use the lab checklist in an emulator and on
hardware when available. Optional core skills such as `sgdk-graphics`,
`sgdk-input`, `sgdk-math`, `sgdk-audio` and `sgdk-performance` can help when
installed. This skill's resources work independently.
