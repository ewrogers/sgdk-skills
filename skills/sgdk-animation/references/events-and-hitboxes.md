# Timed events and hitboxes

## Entry events are different from active intervals

A footstep, swing sound or projectile release is an event. An attack box that
exists for six ticks is an interval. Test that box on the required ticks, but
do not spawn a projectile or replay the sound on every tick of the held frame.
Animation Lab uses an `entered` flag for frame markers and a separate current
box for contact. Completion has its own latch after the last hold.

Its display-frame batch holds up to two logical event masks. Each step can
produce one of each event type; counters consume every mask. The audio adapter
then selects a cue by priority for that display frame. This cosmetic aggregation
does not discard gameplay contacts. A game with multiple projectiles or repeated
same-type events per step needs counted events or a bounded event queue instead
of assuming a bitmask preserves multiplicity.

Do not use an audio-event bit as persistent gameplay state. Effects can be
muted, culled or replaced without changing whether an action is active.

## Keep collision in world coordinates

Store authored boxes relative to a stable actor origin, then transform them to
world space. Camera offsets affect drawing, not contact rules. For a box
`{x,y,w,h}`, horizontal mirroring changes `x` to `-x-w`; merely negating `x`
moves the wrong edge. Keep dimensions positive and define edge contact.
The Lab uses half-open AABB overlap: touching edges alone do not hit.

Its cyan outlines show body boxes smaller than the decorative sprite, while
pink shows the active attack box. The gold arc is cosmetic. Neither the arc's
opaque pixels nor the debug outline decide damage. C can hide outlines without
changing gameplay. In a scrolling game, transform debug drawings with the same
camera used for sprites and retain collision coordinates in world space.

The generated ResComp sprites use collision `NONE`. This Lab's boxes come from
its action table. SGDK/ResComp collision metadata can supply shapes, but does not
automatically choose active attack windows, resolve contacts or award damage.
Source: [ResComp sprite collision options](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt).

## Identify an attack, not just its current frame

The training dummy receives at most one hit per attack. An action-entry latch
is enough here because there is only one target. Leaving and re-entering its
box during the same attack must not reset that latch. A new attack clears it.

A multi-target game needs an appropriate policy: one hit total, one per target,
or repeated damage with a cooldown. For one hit per target, retain a bounded
set of target IDs plus their pool generations for the current attack. Reusing
a destroyed target's slot must not make it an old target or let stale contact
records damage its replacement. Define what happens if that hit set is full.

Interruption clears the current attack box before new contacts are evaluated.
On pause the visible active box may remain, but no logical collision steps occur.
On teardown, retire queued events/references before reusing actor storage.
These rules belong with the controller; renderer callbacks should not retain
unowned actor pointers and apply damage later.

Test both sides, exact edges, the first/last active tick, prolonged overlap,
re-entry, restart, interruption and pause. Fast motion still needs the collision
sampling or swept-contact strategy appropriate to that game; this stationary
training target is not a general continuous-collision implementation.
