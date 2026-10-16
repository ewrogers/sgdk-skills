# Simulation and emission

## Effects are consequences

Let gameplay decide damage, rewards and collision first. Then request a visual
burst with a copied position and optional direction. Failure to admit that
burst must not undo the gameplay event. Avoid storing a pointer to an object
that can despawn while its smoke remains alive.

A fixed particle array with an inactive marker is often enough. A bounded free
list or active list can reduce searches when profiling supports the extra RAM.
Track counters carefully through admission, replacement, expiry and cleanup.
Do not grow the array or allocate a heap object for every spark.

Effects Lab accepts priorities: impacts/smoke, then trails, then weather. At
capacity an incoming effect can replace the oldest particle at a strictly
lower priority; equal-priority requests are rejected. A separate 48-particle weather allowance
leaves room in the 96-slot pool for other effects. Its `rejected` counter
counts valid requests refused for capacity, not invalid arguments or render
culling. These rules are examples, not SGDK features or a mandatory policy.

## Time and numeric ranges

Give each particle an age/lifetime and integrate using the game's simulation
clock. Normalize emission and animation speed for PAL/NTSC. A first-order fixed
step can use positions/velocities scaled by a power of two, with documented
ranges and widened intermediates where needed. Check before narrowing.

The Lab uses signed Q4 coordinates in a 640×152-pixel world region. Admission
bounds velocity to ±6 pixels per tick and lifetime to 180 ticks. Expired or
out-of-world particles become inactive before selection. Cached animation
frames advance from age; particles do not borrow Timer nodes or SGDK timers.
Its 60-tick clock takes one step at 60 Hz and one or two at 50 Hz, while button
press edges are consumed once per rendered frame. This is a nominal regional
clock, not an unlimited catch-up system for missed frames.

Keep cosmetic RNG state separate from gameplay. Stable seeds help repeat a
visual test, but rendering or reducing quality must not advance that state.
Effects Lab's selector takes separate, non-overlapping state/output objects;
`restrict` expresses that C contract. Never add `restrict` to aliased buffers
just to make a benchmark faster.

## Trails and weather

Copy trail samples in world space so old samples do not follow the emitter.
Space emissions by time or distance according to the desired look; interpolate
long movements only within a bounded sample count. Reset the previous sample on
teleport/scene change rather than drawing a streak across the map.

Rain can expire quickly; slow snow can fill the same pool despite an identical
emission rate. Test lifetime × rate, not just a short burst. Define what changing
weather does to existing particles. The Lab stops emitting the previous type
but lets existing particles expire. World-space weather persists at its sampled
position when the camera moves; a screen-space field is a different convention.

On transition, stop emitters, invalidate attached sources, clear particles and
remove their rendered entries. Drain pending transfers before reusing their
source buffers or tile region. The Lab has no scene manager; `fx_clear()` clears
simulation and `fx_view_end()` queues a hidden sprite table. Its caller must
process that VBlank before handing the table to another scene.
