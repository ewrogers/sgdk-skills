---
name: sgdk-math
description: Choose and verify SGDK fixed-point math for movement, angles, aiming, interpolation, distance checks and perspective effects. Use for numeric precision, overflow, trigonometry or math-heavy game effects on the Mega Drive, rather than general gameplay architecture or profiling.
---

# SGDK Math

Choose the coordinate range, smallest useful fraction and largest intermediate
before choosing a type. A wider storage type does not automatically mean a
more accurate or safer multiply/divide implementation.

## Choose the right reference

- [Numbers and arithmetic](references/numbers.md): SGDK's actual formats,
  conversions, rounding, overflow and multiply/divide limits.
- [Motion and effects](references/motion.md): degrees, vectors, aiming,
  diagonal movement, squared distances, interpolation and bounded tables.
- [Math Lab](assets/math-lab/README.md): a runnable perspective starfield and
  orbit/figure-eight/spiral formations, with controls and a playtest checklist.

Use `ewrogers/sgdk:latest` and check its `inc/maths.h`, `inc/types.h` and
`src/maths.c` before using an unfamiliar helper. These references were checked
against SGDK 2.11. In particular, current `F16_sin`/`F16_cos` take degrees;
deprecated `sinFix16` uses a different angle convention.

## Make the calculation safe

Keep world positions separate from screen coordinates. Prefer `fix32` or
integer world positions when a level exceeds `fix16`'s range. Use small
`fix16` offsets and velocities where they fit. Convert at explicit boundaries;
do not cast between fixed-point formats as though their scales were identical.

Widen before subtracting or multiplying. Establish a valid divisor and bound
the quotient before division. Avoid signed overflow, shifting negative signed
values in your own C and assuming that a wrapped result will be useful.
SGDK's helpers have implementation-specific limits too; follow the reference
rather than replacing them with floating point or an untested fast-math trick.

For a new effect, test known points, negative inputs, near-zero divisors,
maximum supported coordinates and a full angle cycle. Compare with an
independent higher-precision oracle within a stated tolerance. Execute the
actual target SDK routines when host arithmetic cannot establish their behavior.

## Keep repeated work small

Cache a shared sine or reciprocal table when the memory tradeoff is worthwhile.
Bound the angle/depth index before looking it up. Prefer squared-distance tests
to square roots when only a threshold matters. Do not replace an exact hit test
with SGDK's approximate distance without allowing for its error.

Use a consistent simulation clock and cache input once per display frame.
Separate simulation coordinates from rendering so a PAL display or a missed
frame does not silently change the units of velocity. Measure optimized ROMs;
a successful build or smooth host animation does not establish a 68000 budget.
