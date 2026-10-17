---
name: sgdk-combat
description: Resolve real-time SGDK combat with damage filtering, directional guard, invulnerability, knockback, hit-stop, attack/target identity and simultaneous hits. Use when contacts deal repeated damage, blocks fail, reactions conflict or pooled targets receive stale hits; complement existing animation and collision systems.
---

# SGDK combat

Start with the game's combat rules, then connect its existing action clocks and
collision queries to a damage resolver. SGDK draws sprites and plays sound;
attack ownership, guard, damage and recovery are application C code.

## Keep responsibilities clear

Read [skill boundaries](references/boundaries.md) when the task spans animation,
collision, AI, RPG systems or multiplayer. Reuse the project's conventions and
implementations instead of adding a second action clock, input reader, collision
solver or player registry. Related skills are optional companions; this skill
and its Lab install independently.

Agree on teams/friendly fire, what counts as front guard, repeated-hit rules,
invulnerability, simultaneous attacks and interruption. Decide whether defeated
actors' projectiles persist, and what losing the last player means. These are
game policies, not universal SGDK behavior. Art, audio, combat style, camera,
timings and difficulty in the Lab are illustrative, never prescriptions.

## Resolve contacts deliberately

- Accept contacts from the established geometry layer, in world coordinates.
  Filter self hits, teams, inactive targets, stale generations and immunity
  before applying damage. Palette and sprite priority are not collision teams.
- Distinguish an attack instance from its animation frame and a target slot
  from its current occupant. Track contacts for the chosen attack or hit pulse.
  Decide whether blocking or touching an invulnerable target consumes contact.
- Gather competing hits against a consistent snapshot. Choose whether attacks
  can trade, or whether a priority rule suppresses one. Do not accidentally give
  the first actor updated an advantage. Use stable tie-breaks for reactions.
- Apply damage, defeat and reaction transitions once per target. Interrupting an
  attack closes its remaining hit windows; it does not reverse earlier damage.

Read [resolution and identity](references/resolution.md) for the Lab's bounded
batch, pooled generations, guard direction and regression cases. Use the game's
damage formula; do not import RPG stats or a fighting-game combo system just
because this skill is active.

## Make reactions readable and bounded

Keep knockback collision-aware. Separate hurt duration, invulnerability and
hit-stop; they need not be the same timer. Specify which clocks freeze during
hit-stop and pause, whether input buffers advance, and whether animation/audio
continue. Process logical events before rendering the resulting pose.

Provide an anticipation cue for important attacks, visible guard/hit feedback,
and a way to tell when an ability is ready. Favor the project's accessibility
and visual direction over the Lab's color choices. Effects must not displace
fighters or dangerous projectiles when sprite/scanline budgets are tight.

Pull `ewrogers/sgdk:latest` and verify the installed SDK before changing frame,
DMA, input or sound APIs. [SGDK presentation](references/sgdk.md) records the
checked interfaces and separates hardware limits from example capacities.

## Prove the policy

Test one hit per attack, multiple targets, reused slots, blocked/back/side hits,
unblockable moves, immunity boundaries, simultaneous killing blows, interruption,
buffer expiry, pool exhaustion, pause/hit-stop and both 50/60 Hz schedules.
Compare host and base-68000 execution and check real compiled resources.

[Combat Lab: Copper Court](assets/combat-lab/README.md) demonstrates a sword,
front guard, bolts, three enemies, three rounds and optional two-player co-op.
Its fixed arena keeps camera and adapter work outside the combat example.
Play it to check readable poses, alignment, sound and frame-time headroom;
CPU tests alone do not verify those or physical hardware compatibility.
