# Collision and cameras

## Keep gameplay in world coordinates

Moving the camera does not move the world. Store the robot and collision in
world coordinates. Subtract the camera only when placing the rendered sprite.
The lab's sprite has extra pixels above and around its 12x12 collision box;
its visual origin therefore includes a small offset. Artwork and collision
do not need identical bounds.

The HUD uses screen coordinates on Window. Level Lab reserves the bottom
32 pixels for it, leaving a 320x192 world viewport. Its camera clamps against
that viewport, not the whole 320x224 display. At the bottom edge, MAP can
prefetch hidden rows; the power-of-two storage keeps those reads valid.

## Define what touching means

The lab uses half-open boxes: touching a wall's edge is allowed; crossing it
is blocked. With fixed-point positions, test the last included subpixel at
`position + size * scale - 1`. Converting to whole pixels too early can miss
a fractional overlap.

Reject negative/out-of-world bounds before division, since C integer division
truncates toward zero. Treat outside cells as solid. Test every occupied cell
under the box, not just its center. Collision reads the bitset directly and
does not query optimized graphics tile attributes.

Move each axis separately to slide along walls. Level Lab subdivides movement
into steps no larger than one pixel so its actor cannot skip a solid tile.
That is a small sample algorithm, not a universal high-speed physics solver.
Slopes, one-way platforms, moving bodies and triggers need explicit rules and
their own tests rather than being inferred from this full-cell grid.

## Follow the player, then clamp

Center the camera on the player's collision box, then clamp each axis to:

```text
0 .. max(0, world_pixels - viewport_pixels)
```

A world smaller than the viewport should not create a negative maximum. Near
world edges, the camera stops while the player continues moving on screen.
Large jumps need a map redraw; they must not just move the sprite and leave
the old tilemap behind.

The lab uses SGDK's `fix32` scale (10 fractional bits) and a nominal 60-step
simulation. NTSC executes one step per frame; PAL occasionally executes two.
It samples input and renders once per displayed frame. Cardinal movement is
2px per simulation step; diagonals use approximately 1.414px on each axis.
Pause stops simulation without accumulating catch-up steps. This policy does
not repair arbitrary missed frames or make PAL render at 60 Hz.

## Check the route, not only the math

Test a corner, a narrow passage and a wall from both sides. Try fractional
positions immediately before/after an edge. Walk beyond the backing plane's
512px horizontal and 256px vertical span, reverse direction, and check that
graphics and collisions remain aligned. Visit all four camera limits.

The lab includes warp points to reach the far corners quickly. Its host tests
exercise their validity and a long movement run; container tests compare the
linked collision bytes with the Tiled source. Visual agreement between the
two authored layers still needs an emulator/console check.

## Sources

- [SGDK 2.11 fixed-point definitions](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/maths.h).
- [MAP coordinates and scroll behavior](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/map.h).
- [Window region configuration](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h).
- [Tiled coordinate conventions](https://doc.mapeditor.org/en/stable/reference/tmx-map-format/).
