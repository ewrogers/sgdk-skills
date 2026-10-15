# Encounters, powers and secrets

## Give enemies different jobs

Platform Lab uses five fixed enemy slots, one per behavior. No per-frame
allocation or pathfinding is needed for these encounters.

| Enemy | Behavior | What the player learns |
| --- | --- | --- |
| Green patrol | Reverses at walls and probes ahead for a floor. | Stomp a predictable walking target. |
| Amber walker | Reverses at walls but walks over a ledge. Resets after falling out of the course. | Enemies can obey different terrain rules. |
| Red hopper | Jumps toward the player after a grounded wait. | Time a landing rather than chasing blindly. |
| Violet flyer | Follows a bounded horizontal route with a gentle vertical bob. | Approach an aerial target from above. |
| Pale armored guard | Alternates between covered armor and an exposed body. | Wait for an opening or use the charged dash. |

The hopper's launch cadence is 60 steps; the armored guard is covered for 90
of each 150 steps. These are starting values for this course, not required
game conventions. Resetting a checkpoint restores the roster. A stomped enemy
stays retired until that reset; a walker that falls away returns after 90 steps.

Keep a probe far enough ahead of a ledge-aware enemy's footprint. A probe
inside its current support tile is not evidence that the next step is safe.
Flyers should not accidentally inherit ground gravity or wall-jump rules.

## Stomps and damage

Resolve player and enemy movement, then compare overlapping footprints. A
stomp requires downward motion and previous feet no lower than the enemy's
upper contact tolerance. A side or underside overlap causes damage instead.
Hold A during a stomp for a stronger bounce. A guarded enemy rejects a stomp;
a charged dash can defeat it even while guarded.

After damage, apply knockback and temporary invulnerability. The lab flashes
the player during that interval. Three lost hearts or a fall below the course
returns the player to the latest checkpoint. Do not continue processing old
contacts after a respawn has replaced the actors.

The example checks five contacts directly. For a much larger roster, spatially
filter candidates before narrow-phase checks. Sprite visibility and collision
activity are separate decisions: an offscreen enemy can still need simulation,
while a decorative sprite may never need a hitbox.

## Power-ups should explain themselves

- **Boots:** one extra air jump, shown as gold boots on the hero.
- **Battery:** enables a horizontal charge dash. Hold B for 36 steps, then
  release. Blue sparks show charging; gold sparks and a cue indicate readiness.
  Early release does not dash. The dash lasts 12 steps and breaks marked blocks.
- **Speed pickup:** raises running speed to 3.5 pixels/step for six seconds.
  Its timer pauses with the game.

An air dash is consumed until landing or a stomp. A charged state, an active
dash and a one-frame sound event are different things: never use a frame's
aggregated sound-event bits as proof that the player is still dashing during
another simulation step.

The lab preserves acquired powers and broken blocks across checkpoint resets.
That choice avoids softlocks after returning beyond a power pickup. A full new
run clears them. If a game instead restores an earlier snapshot, restore the
pickups, collision edits and abilities together.

## Hidden passages

Keep secret cover in a distinct visual tile class, separate from solid walls.
Entering the passage removes its cover and reveals bonus coins. A small edge
or material cue can invite exploration without revealing the whole route.

The lab updates collision immediately for broken blocks. Its MAP patch callback
then removes the corresponding four hardware tiles, including later streamed
copies. The reveal flag persists for the run. A single terrain revision causes
one forced plane refresh; repeated frames do not repeatedly redraw the map.

For frequent edits or larger planes, batch dirty cells instead of copying the
whole plane. Budget those transfers separately from sprite animation and map
scrolling. Secret discovery should not cause a surprising missed frame.

## Make completion unmistakable

Freeze gameplay after reaching the goal, but keep the display loop and Start
input alive. The lab changes music to a short fanfare, shows a clear banner and
animates celebratory coins. Start begins a fresh run.

A stopped character and silent music alone can look like a crash. Completion,
pause and Game Over should each have a visible identity and a documented way
forward.
