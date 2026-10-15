# Playfield, HUD and stage flow

## Vertical and horizontal shooters

| Choice | Vertical scrolling | Horizontal scrolling |
| --- | --- | --- |
| Useful HUD starting point | A side panel preserves forward viewing distance. | A narrow top/bottom strip preserves horizontal distance. |
| Common entry edges | Top and sides; rear threats need clear warnings. | Front and top/bottom; attacks from behind need warning. |
| Space to plan | Spread width, lateral dodging, safe gaps between lanes. | Forward reaction distance, vertical routes and terrain clearance. |
| Background | Advance along Y; keep HUD independent. | Advance along X; consider foreground terrain and parallax. |

These are design options, not rules. A vertical game can use a top HUD and a
horizontal game can use a side panel. Choose based on the game, aspect ratio
and readability. Reserve the HUD before tuning movement or attack spacing.

Shooter Lab uses a 240×224 playfield and an 80-pixel side panel on a 320×224
screen. Player positions, projectile retirement and visible actor bounds all
use that 240-pixel limit. The Window panel is opaque, so art clipped at the edge
cannot leak through its text. In a horizontal variant, define the same explicit
rectangle with a reduced height and remap entrances/scrolling deliberately.

Keep world coordinates separate from screen coordinates if actors travel with
the scrolling level. The lab uses screen-space actors over a looping starfield;
it does not pretend to implement a large world or terrain collision. A game
with terrain should add tile collision and streaming independently of bullets.

## Authored waves

The lab has 24 authored spawns in six groups. Each entry has a nominal 60 Hz
time, position and enemy kind. A pending spawn waits for a free enemy slot;
later entries cannot overtake it. Only one entry is admitted per step, so a
busy pool cannot produce an unbounded burst when slots become free.

Scouts descend and fire aimed shots. Sweepers travel sideways while descending
and fire fans. Turrets hold briefly and fire rings, then leave. Escaping enemies
break the example's kill chain. Their offscreen retirement frees a slot; drawing
an enemy offscreen is not what frees it.

After the final wave and its settling interval, the boss entrance retires the
remaining enemies and clears their bullets. This is the lab's chosen stage
handoff, not a universal requirement. A score-focused game might instead wait
for every enemy to leave, or retain specific threats during entry.

## Bosses and game states

The lab distinguishes Title, Play, Boss, Clear and Game Over, with pause as a
separate flag. The warning and fly-in leave player movement active. The health bar then
expands before the boss becomes vulnerable or starts firing. Its attack
cycle moves through aimed fans, rings and a rotating spiral. Below half
health the fan cadence tightens.

Both shots and bombs use the same damage/defeat path. Defeat awards its bonus
once, clears hostile bullets and triggers a banner, fanfare and effects.

The end states stop gameplay while keeping display and restart input alive.
A or Start begins a new run. Pause freezes actor movement, attack clocks and
background scroll; music pauses and active PCM effects are stopped.

## Scoring and recovery

Kills award a base score plus an increasing chain bonus, capped at ten links.
A hit or escaping enemy breaks the chain. Score saturates at eight digits;
it does not wrap into a small number. Extra points replace pickups that would
exceed the weapon/bomb limit. These are example rules; another game might
reward grazing, medals, survival time or accuracy instead.

The player has three lives. A hit costs one, lowers weapon power by one and
clears enemy bullets. A surviving player returns to a safe position with two
seconds of flashing invulnerability. This avoids repeatedly losing lives to
the same overlapping threat. The lab does not offer SRAM persistence or Continue.

## Timing

A nominal 60 Hz model runs once per NTSC display frame and once or twice per
PAL frame. The adapter reads the SGDK pad cache once. Held controls feed every
step; press edges, such as a bomb, are consumed once. Sounds observe aggregated
frame events rather than running for each individual simulation step.

This preserves nominal movement, attack and cooldown time between 50/60 Hz
cadences. It cannot compensate for an overloaded game missing VBlank. Check
actual FPS and the selected SDK's load estimate; a model clock test is not a
performance measurement.
