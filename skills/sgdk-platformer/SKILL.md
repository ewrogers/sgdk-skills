---
name: sgdk-platformer
description: Build and tune side-scrolling SGDK platformers with responsive jumps, slopes, one-way and moving platforms, enemy stomps, power-ups and hidden passages. Use for platformer movement feel and interactions, rather than general top-down movement or SDK setup alone.
---

# SGDK Platformer

Start with a short course that proves the movement feels right. A character
that jumps well in an empty room can still feel awkward beside ledges, low
ceilings, moving platforms and enemies.

## Choose a starting point

- [Movement and collision](references/movement.md): units, jump tuning, coyote
  time, buffering, slopes, one-way platforms and moving-platform ownership.
- [Encounters and powers](references/encounters.md): patrols, falling walkers,
  hoppers, flyers, guarded enemies, stomps, charge moves and secrets.
- [SGDK integration](references/integration.md): verified APIs, map streaming,
  native pixel art, VRAM/DMA ownership and performance checks.
- [Platform Lab](assets/platform-lab/README.md): an independently buildable
  scrolling course, original art/music, controls and a playtest checklist.

The example implements its own physics in ordinary C. SGDK supplies input,
fixed-point formats, map/sprite rendering, audio and frame processing; it does
not supply this platformer controller. Use `ewrogers/sgdk:latest`, pull it before
building, and inspect the selected SDK headers before using an unfamiliar API.
The integration reference was checked against SGDK 2.11.

## Build the feel before the content

Agree on responsive movement versus momentum, jump height/time, air control,
stomp behavior and available abilities. Make tuning values easy to change.
Use small hitboxes that match the intended contacts, not every decorative
pixel. Keep movement coordinates independent of the camera.

Take one cached input snapshot per display frame. Give press edges to one
simulation step; held input can feed every step. Keep the simulation bounded.
If PAL uses two updates on some frames, test that case, including collision,
input edges, animation, cooldowns and audio event aggregation. A nominal clock
does not recover time lost when the game misses its hardware deadline.

Resolve solid terrain before enemy interactions. Use half-open footprints,
previous feet and descending motion to distinguish a stomp from a side hit.
Test ceilings, wall corners, maximum dash speed, slope transitions and landing
on a platform at the same height as another surface. Define what happens when
a platform cannot carry its rider instead of silently pushing through a wall.

## Make abilities readable

A power should change both gameplay and presentation. Show equipped boots,
charge progress and a distinct ready cue. Tell the player when armor is open
or dangerous. Reveal secret cover in both the rendering and saved game model;
keep its visual tiles separate from solid collision.

Use a small fixed roster/pool appropriate to the game. Every enemy should
have a bounded update and a defined retirement/reset rule. Separate damage
invulnerability from enemy state so a stunned, guarded or falling enemy does
not accidentally become harmless or harmful for unrelated reasons.

## Validate the actual course

Run executable model tests for jump windows, one-way crossings, moving support,
stomps, invulnerability, ability limits and checkpoint recovery. Include a long
collision-safe run and target-code checks for important arithmetic. Then build
and play the optimized ROM: confirm controls, telegraphs, streaming, sound and
frame rate. Compilation and host tests cannot establish Mega Drive timing.

Core skills such as `sgdk-input`, `sgdk-levels`, `sgdk-math`, `sgdk-graphics`,
`sgdk-audio` and `sgdk-performance` can help when installed. They are optional;
this skill's lab and references do not depend on sibling folders.
