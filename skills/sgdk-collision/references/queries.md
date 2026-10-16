# Query contracts and arithmetic

## Use the right geometry

An overlap test answers whether interiors intersect **now**. A sweep answers
whether the moving box reaches another box during a finite step, and when.
A ray here means a segment `start + delta * t`, with `0 <= t <= 1`, rather than
an infinite line. Visibility rays can be points; a bullet with width should use
a box sweep so it does not slip through gaps smaller than itself.

Use last and proposed positions from one simulation tick. For two moving boxes,
keep the target at its old position and subtract its displacement from the
querying body's displacement. The resulting time applies to both original
motions. Testing against a target's already-updated position instead can create
order-dependent hits. Linear translation is assumed throughout the tick; split
an intended change of direction into separate bounded steps.

`collision.c` intersects the X/Y time slabs of a Minkowski-expanded box. It
compares signed numerator/positive denominator pairs, so choosing the earliest
hit does not depend on rounded display time. Parallel axes must overlap strictly;
an interval whose entry equals exit only grazes a corner and is rejected.

## Boundary and failure rules

| Situation | Lab result |
| --- | --- |
| Separated, no entry during the finite step | `CC_MISS` |
| Strictly overlapping at the start | `CC_OVERLAP`, no surface normal |
| Touching and moving into the face | `CC_HIT`, time zero |
| Touching and moving away or along the face | Miss unless another face is entered |
| Reaching a face exactly at the end | Hit at time one if it would enter afterward |
| Simultaneous entry on both axes | Hit with both normal components |
| Instantaneous corner graze or parallel ray on the boundary | Miss |
| Bad sizes/range/motion or too many candidates | `CC_INVALID` |

These are deliberate game-query choices, not a universal SGDK convention.
Changing edge inclusion requires changing both code and oracle tests. Overlap
alone returns no guessed normal or minimum translation. A stationary segment
strictly inside returns overlap; outside or on the boundary it misses.

`cc_cast` reports the earliest exact sweep hit among supplied static boxes.
Equal-time walls combine blocked axes; the lowest list index supplies metadata.
Any initial overlap takes precedence over later hits. Validate or handle that
status before reading time or normals. `cc_overlap` is only a convenience
predicate and returns false for invalid geometry; it is not a validator.

## Numbers are part of the interface

The example stores positions and sizes in signed 16-bit **Q4 local units**:
16 raw units per pixel. This is an application format, distinct from SGDK's
`fix16` (six fractional bits) and `fix32` (ten fractional bits) in SGDK 2.11.
Use explicit checked conversions at a boundary, never a pointer cast.

| Quantity | Accepted raw range |
| --- | --- |
| Body/segment origin, and each motion endpoint | −16,384..+16,384 |
| Box width and height | 1..4,096 |
| Each body's displacement per axis | −4,096..+4,096 |
| Relative displacement, computed in `s32` | −8,192..+8,192 |
| Supplied boxes per list query | 0..32 |

Origins cover a local −1,024..+1,024-pixel window. Rebase a larger world around
the mover and validate nearby boxes after conversion. Extents may extend beyond
the origin window; the slab proof includes them. A large wall can be represented
by bounded chunks, but do not split it in a way that changes collision semantics.

The largest absolute slab numerator is 36,864 raw units. Multiplying it by a
relative denominator of at most 8,192 gives 301,989,888, safely below signed
32-bit maximum. Actual hit numerators lie in 0..denominator; converting hit time
to 0..4,096 and multiplying a displacement by its hit numerator also fit `s32`.
Denominators are positive, with zero motion handled separately. Multiplication
widens **before** the operation; casting an already-overflowed 16-bit result is
not a fix. No negative left shifts, floating point or 64-bit divisions are needed.

The `time` field is `floor(num * 4096 / den)`, for display or a coarse fraction.
Do not use it to pick between close hits or advance a body to a wall. The Lab's
response uses `num / den` directly. Tangential movement truncates toward zero
by less than one raw unit (1/16 pixel) per contact; blocked face placement is
exact. Enlarging the bounds invalidates this proof even if a compiler accepts it.

## Small executable examples

These use the shipped header; values are raw Q4 units:

```c
CcBox bullet = {0, 0, 4 * CC_PIXEL, 4 * CC_PIXEL};
CcBox wall = {80 * CC_PIXEL, 0, 4 * CC_PIXEL, 32 * CC_PIXEL};
CcHit hit = cc_sweep(bullet, (CcVec){128 * CC_PIXEL, 0},
                     wall, (CcVec){0, 0});
if (hit.kind == CC_HIT) {
    /* Use both original motions at this fraction for a moving target. */
    bullet.x += (s16)((s32)(128 * CC_PIXEL) * hit.num / hit.den);
}

CcHit sight = cc_ray((CcVec){0, 8 * CC_PIXEL},
                    (CcVec){160 * CC_PIXEL, 0}, wall);
/* Check sight.kind first. This tests one box, not an entire tilemap. */
```

An enemy taking damage, a projectile expiring, a trigger emitting enter/exit or
a character bouncing are callers' policies. A continuous query only detects
contact; it does not imply those behaviors.

Sources for SDK types and formats:
[SGDK 2.11 types](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/types.h),
[fixed-point definitions](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/maths.h).
The slab algorithm and its boundary policy are implemented and tested in this
skill's original `assets/collision-lab/src/collision.c`.
