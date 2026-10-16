# Coordinates, feet and elevation

## Start with a diamond

For a 32×16-pixel diamond, one map cell along X projects 16 pixels right and
8 down. One cell along Y projects 16 left and 8 down. Height moves straight up.
The Lab uses 256 raw units per horizontal cell and 16 raw units per vertical
pixel. For raw `x`, `y`, `z`, its projection is:

```text
screen_x = floor((x - y) / 16)
screen_y = floor((x + y - 2*z) / 32)
```

These are original integer formulas, not SGDK fixed-point helpers. In particular,
the vertical raw unit and horizontal cell scale are explicitly chosen; do not
reinterpret this format as SGDK `fix16` or `fix32`.
`il_project()` widens before arithmetic and uses floor division for negative
numerators. C division alone truncates toward zero, producing asymmetric results
around the origin. Its signed-16-bit inputs fit the signed-32-bit intermediates.
The box sorter accepts horizontal bounds within ±8,192 raw units and heights
0..2,048; boxes must have positive extent on all three axes.

The adapter adds the garden image origin `(256, 64)`, subtracts the camera, then
subtracts the asset's anchor. The hero's anchor is at its feet, not at the sprite
canvas center. Animation frames preserve that anchor, so a changing pose does
not move the character through the map.

Sources for the distinct SDK formats:
[SGDK 2.11 types](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/types.h),
[fixed-point definitions](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/maths.h).

## Inverse projection needs a surface

For an unrounded projected point and a **known** height:

```text
x = 8*screen_x + 16*screen_y + z
y = -8*screen_x + 16*screen_y + z
```

`il_unproject()` takes integer screen pixels, validates its range and writes
nothing on failure. Forward rounding discards subpixel information, so an
arbitrary round trip is not exact. The tests use exactly representable points
for exact round trips. In a mouse editor, first remove origin/camera offsets,
then test candidate surfaces from front to back. A screen position alone cannot
distinguish the bridge from the ground beneath it.

## Movement stays in map space

The Lab maps D-pad directions to map vectors before projection. Right increases
X and decreases Y; Down increases both. Right+Down moves along the map's X axis,
which is useful for climbing its stairs. A 3/4 diagonal factor keeps the map-space
speeds close without a square root. The 2:1 projection makes vertical screen
travel look slower than horizontal travel. Other games can choose a different
mapping, but test the stair and doorway feel rather than assuming one convention.

`world.c` checks the actor's 80×80-raw-unit footprint, moves X then Y and allows
sliding along blocked components. Its maximum step is 24 raw units per axis
under the chosen input mapping; authored solid footprints are much wider. This
is a small discrete room solver, not arbitrary swept or 3-D collision. Use
continuous queries or bounded substeps if another game introduces fast motion.
The collision footprint is smaller than the 24×32-pixel drawing.

## A bridge has two walkable surfaces

The lower avenue remains at height zero. The deck is at 32 pixels, with its solid
slab at 28..32 pixels. The hero is 24 pixels tall, leaving room to walk beneath.
Four risers at each end advance the upper surface through 8, 16, 24 and 32 pixels.
They are steps, not interpolated slopes.

The actor keeps an explicit surface/layer ID. Entering a stair through its foot
portal changes that surface; passing under the middle of the bridge does not.
The upper walkway rejects moves across its side edges. Leaving a stair through
its low end returns to ground. Merely choosing the highest surface at X/Y would
teleport a character upward while walking underneath the bridge.

The small footprint margin at a stair foot is allowed to approach or back away
without getting trapped. Rendering receives a support constraint for the stair
proxy during that transition. Collision, chosen surface and draw ordering are
related contracts, not interchangeable guesses based on the sprite's screen Y.
Jumping, falling, moving platforms and crushing need their own gameplay rules;
this Lab intentionally keeps the surfaces stationary.

The camera follows the projected feet after simulation, clamped to the authored
background's useful region. All world coordinates and ordering remain unchanged
by camera motion. A nominal 60-tick scheduler uses at most two logical steps on
PAL display frames and freezes its phase on pause; it does not catch up dropped
VBlanks or guarantee identical 50/60 Hz presentation.
