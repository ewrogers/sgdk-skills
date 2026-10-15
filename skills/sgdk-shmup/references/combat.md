# Bullet patterns and combat

## Coordinates and numeric bounds

Shooter Lab uses signed 16-bit coordinates in sixteenths of a pixel. This
original Q4 convention fits its screen-sized field and an offscreen margin.
It is not SGDK `fix16`, which has six fractional bits. Do not pass these values
to an SDK fixed-point function without converting units explicitly.

Velocity is units per nominal 60 Hz step. Player shots are bounded to six
pixels per axis; hostile shots to four. The supplied attacks are slower than
that hostile maximum. Intermediate vector products use signed 32-bit arithmetic
before narrowing. If a game needs a much larger world, change storage/ranges
and test every intermediate; do not rely on wrapped 16-bit coordinates.

Normal and focus speed are three and 1.5 pixels/step. Diagonal scaling uses a
181/256 approximation to 1/sqrt(2), then Q4 quantization. It deliberately does
not increase diagonal speed. That quantization makes diagonal motion slightly
slower; use a finer format if that matters to the chosen feel.

## Build patterns from a small vocabulary

The lab stores sixteen approximate unit vectors, clockwise from up, scaled by
256. No projectile performs trigonometry during its update. Other games can
use SGDK's verified degree-based math or a finer shared lookup table.

- **Aimed:** choose the direction with the largest dot product toward the
  target's current position. The direction is snapped when fired, not homing.
- **Fan:** emit the aimed direction and one neighboring direction on each side.
- **Ring:** emit eight evenly spaced directions, with a configurable rotation.
- **Spiral:** advance the emission direction on a schedule. Each existing bullet
  continues straight; its direction does not rotate with the emitter.

The table's coarse aim and speed quantization are visible design choices, not
SGDK limitations. Keep safe gaps readable and avoid unavoidable attacks when a
player is next to an emitter. Delay or telegraph shots when the game needs it.

## Pools and exhaustion

The example reserves 32 hostile bullets, 12 player shots, six enemies, three
pickups and six effects. Records are reused in stable index order. A ring or
fan requires enough free hostile slots for the entire volley; otherwise the
whole volley is skipped and a diagnostic counter increments. A single aimed
shot can fail independently. No heap allocation occurs per shot.

Choose the policy deliberately. Smaller volleys, retrying on a later attack
beat or reserving a portion for a boss are alternatives. Do not silently erase
an old threatening bullet to make room. Decorative effects may simply be
omitted when their pool is full. A fixed pool does not imply that every game
must use the lab's capacities.

## Collision and lifetime

Use separate shapes for player shots versus enemies and hostile bullets versus
the player's core. The lab uses a small core inside the 16×16 ship, shown while
holding C. Body contact is also damaging. Artwork is not the hitbox.

Projectile motion uses at most eight power-of-two substeps, with no variable
division in the inner loop. Samples are at most one pixel apart within the
supported speed domain. A shot retires on its first hit, and invulnerability
prevents multiple life losses in the same frame. The checks are bounded
screen-space AABBs, not arbitrary continuous moving-shape collision.

Before substepping, the lab compares a projectile's swept bounds with each
possible target once. It substeps only when a hit is possible, preserving the
same samples, first-hit ordering and damage rules. Twelve thousand randomized
cases compare the complete resulting game state against the earlier simple
routine. Invulnerable-player bullets can move directly to their endpoint.

For more objects, reject distant candidates before narrow-phase testing. A
spatial grid or banded enemy list can help, but compare its hit results with a
simple independent reference. Do not shrink collision tolerances merely to
improve a benchmark.

Culling decides what to draw; retirement decides whether an object still
exists. Offscreen bullets are retired beyond a small margin. A projectile
hidden by sprite overload must not remain an invisible, damaging hazard.
Address the attack density or render policy, rather than treating flicker as
proof that collision should continue unchanged.

## Upgrades and bombs

The P pickup upgrades from one shot to a pair, then a three-way weapon. The B
pickup replenishes bombs, capped at three. Guaranteed upgrades supplement drops
in the lab so the example remains approachable after misses. Another game can
use a different drop rule or no weapon progression.

A bomb is a press-edge action. It clears hostile bullets, defeats ordinary
enemies, damages a vulnerable boss and gives temporary invulnerability. It can
finish the boss through the same defeat path as normal fire. At zero stock it
does nothing; holding B does not repeatedly spend bombs.

Keep power state, the active attack and one-frame presentation events separate.
The PAL second step must not replay a bomb or turn an old firing event into
proof that an attack is still active.
