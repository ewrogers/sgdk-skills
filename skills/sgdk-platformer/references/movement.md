# Movement and collision

## Pick units you can reason about

Platform Lab uses signed 32-bit positions with 10 fractional bits, matching
SGDK `fix32` storage. One pixel is 1,024 units. Velocity is pixels per simulation
step, acceleration is pixels per step squared, and the simulation runs at a
nominal 60 steps per second. The game deliberately bounds movement to eight
pixels per axis per step. These are example policies, not SGDK restrictions.

The active course is 1,280 by 288 pixels. Products stay within signed 32-bit
range; do not extend the world or velocities without checking intermediates.
Use multiplication rather than left-shifting negative C values. A `fix16`
world coordinate would run out of useful range for this course.

SGDK's formats and operations are documented in
[types.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/types.h) and
[maths.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/maths.h).
The example's collision solver and helpers are ordinary game code, not SDK APIs.

## Tune a jump around a target

A useful first approximation is `time_to_apex = jump_speed / gravity` and
`height = jump_speed * jump_speed / (2 * gravity)`, using positive magnitudes.
Discrete integration and collision change the exact result, so measure the
actual controller too. Start with one intended gap and one ledge height.

The lab starts at 2.75 pixels/step running speed, about 0.45 acceleration,
0.6 braking, a 6.75 upward jump impulse and about 0.32 gravity. A held jump
rises roughly 68 pixels. Releasing A early caps upward velocity at 2.5
pixels/step for a shorter hop. Air control remains responsive.

Six-step coyote and buffer windows soften common near misses:

- Coyote time lets a newly pressed jump work just after leaving a ledge.
- The buffer remembers a recent press until landing. Holding A does not
  repeatedly jump; a new press is required.
- A wall jump pushes away from a contacted wall and briefly locks horizontal
  steering. Wall slides cap downward speed.
- Boots grant one extra air jump. Landing or stomping refreshes it; wall
  jumping alone does not. There is no unlimited air-jump chain.

Stomp bounces use their own impulse. Releasing A should not accidentally apply
the voluntary-jump cutoff to an automatic bounce.

## Resolve a small, bounded footprint

Use `[left, right)` and `[top, bottom)` bounds. Touching edges are not overlaps.
The 16×24 hero has a 12×20 hitbox; the 16×16 enemies use 12×14. Render offsets
keep those boxes aligned with their art.

The lab moves horizontally, then vertically, in substeps of at most one pixel.
This catches a 16-pixel wall at full dash speed without an unbounded collision
loop. It is a simple tile solver, not swept arbitrary-shape physics. Contacts
can leave less than one pixel of fractional clearance. Enemy checks use final
AABBs plus previous feet; they are not continuous collision detection for
arbitrarily fast moving actors. Increase solver complexity only for a demonstrated
need, and test relative motion as well as static walls.

A full solid tile blocks every direction. Floor slopes are 45-degree
heightfields sampled at the foot center. Grounded motion may step up or snap
down at most eight pixels to follow them. This is intentionally limited to the
course's floor slopes; it does not implement ceiling slopes, loops, arbitrary
angles or Sonic-style surface rotation. Keep sufficient headroom and test both
ends of each authored slope.

## One-way platforms

A descending body lands only when its previous feet were above the platform
surface and its next feet cross it. Ascending bodies pass through. Down + A
starts a short drop-through window and clears ground/coyote state, so the
controller does not immediately jump or reattach.

Do not classify every surface by the current overlapping tile alone. That
makes a character snap onto a platform from underneath. The moving-platform
support ID must also check horizontal overlap; a static tile at the same Y
coordinate is not the same support.

## Moving platforms

The lab has one horizontal, one-way platform. Move it first, then carry a rider
by its displacement before applying the player's own movement. Recheck terrain.
If carry is blocked, release the rider; this example does not crush or push the
player through a wall. Jumping and drop-through clear the support reference.

Only the player receives moving-platform carry in this lab. General actor
riders, vertical lifts, stacked bodies and crushers need their own explicit
rules and tests. They are not implied by this example.

## Timing and pause

At 60 Hz the lab executes one step per display frame. At 50 Hz an accumulator
executes one or two steps, totaling 60 nominal steps per second. Input edges
are consumed once; events are collected for the frame's audio adapter.

Pause freezes physics, animation phases, charge, powers and the accumulator.
The display loop and controller cache keep running so Start can resume. Music
is paused and active PCM effects are stopped. This is a policy for this lab;
a pause-menu soundtrack could be added separately.

A missed VBlank still means lost real time. Show measured display FPS and
SGDK's estimated CPU load, and test the busiest course section on an optimized
ROM. Neither a fixed timestep nor a successful host test guarantees 60 FPS.
