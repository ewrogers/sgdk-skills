# Motion and effects

## Angles and trigonometry

Current SGDK 2.11 functions `F16_sin(fix16 angle)`, `F16_cos(fix16 angle)` and
`F32_sin(fix16 angle)` take **degrees stored as `fix16`**. Their results are
scaled fixed-point values, not pixel positions.

```c
fix16 angle = FIX16(45);
fix16 dx = F16_mul(F16_cos(angle), FIX16(2));
fix16 dy = F16_mul(F16_sin(angle), FIX16(2));
```

With screen Y increasing downward, positive sine moves down. A circle therefore
turns clockwise as this angle increases. Keep that convention consistent when
aiming, drawing sprites or interpreting art facing directions.

`F16_normalizeAngle` wraps degrees into [0, 360). Keep a wider integer phase
bounded before converting it to `fix16`; `FIX16(720)` cannot fit. Normalize
before `F16_cos` too: its implementation adds 90 degrees in `fix16`, so an
already-large unnormalized input can exceed that type's range.

The old `sinFix16`/`cosFix16` functions use 1024 angle units per turn. They are
deprecated and are not interchangeable with the degree-based API. Check the
versioned header when adapting older tutorials.

## Vectors, aiming and distance

The SDK provides vector structs such as `V2s32` and `V2f16`. A struct does not
choose the correct coordinate space for you. Keep world positions, pixel
offsets and scaled velocities distinct.

For directional movement, combine input into (-1, 0, 1) components. When both
axes are active, scale each by roughly 0.7071 to avoid a diagonal speed boost.
In `fix16`, `FIX16(0.7071)` quantizes to 45/64. That is a small, explicit
approximation, not an exactly unit-length vector.

For aimed shots, subtract the positions in a type that fits the difference.
The SDK offers `F16_atan2(y, x)` and `F16_getAngle(x1, y1, x2, y2)` for values
inside their supported range. Handle coincident positions before choosing a
direction. Use the degree result with sine/cosine and multiply by speed.
For a large world, first form a bounded relative vector rather than narrowing
whole world positions into `fix16`.

`getApproximatedDistance(dx, dy)` is an inexpensive estimate. It can be useful
for rough sorting or steering, but it is not an exact circular collision test.
Use a bounded squared-distance comparison when a precise radius matters.
There is no reason to add a floating-point inverse-square-root trick simply
to check whether two objects are close.

## Interpolation

For a fade, camera target or UI position, decide whether the amount means a
fixed duration or a fraction of the remaining distance. Repeatedly approaching
the target by a fraction is easing; it does not reach the endpoint at a fixed
time without an explicit finishing rule.

The lab's `demo_lerp(from, to, amount)` uses an integer amount from 0 to 256,
clamps larger amounts and reaches both endpoints exactly. Its differences and
products are widened first. This is a small application helper, not a claim
that SGDK has a `lerp` API with that signature.

For gravity or projectile arcs, keep fractional position and velocity, add
acceleration once per simulation tick, then render the converted pixel
position. Bound the lifetime and velocity so repeated additions stay in range.
Do not run an unbounded catch-up loop when a frame is late.

## Perspective and lookup tables

The lab projects a star using `screen_x = center_x + x * scale[depth] / 256`.
Its shared reciprocal table is initialized with `scale[z] = 16384 / z` for
depths 1 through 255. The projection rejects depths below 16 and above 255
before indexing, then recycles stars that leave the viewport.

This replaces per-star division with a table lookup and bounded multiply.
The quantized projection stays within 1.5 pixels of its rational oracle over
the tested domain. It is an effect-specific tradeoff, not an exact 3D renderer.
Signed division in the lab explicitly truncates toward zero; it does not
depend on shifting a negative signed intermediate.

The 360-entry sine table takes 720 bytes and the 256-entry reciprocal table
takes 512 bytes. They are shared by every object. For an orbit, the same
calculation drives X from cosine and Y from sine; doubling one angle produces
a figure eight. Different radii produce the spiral formation.

The example uses fixed pools, no per-frame allocation and no runtime floating
point. That still does not guarantee a frame budget. Sprite scanlines, tile
uploads and diagnostic text have costs separate from the math. Measure with
the overlay hidden as well as visible.

For the star projection, the lab uses SGDK's `muls(s16, s16)` on the target:
an explicit 16-by-16 to 32-bit multiply. A portable-looking widened C multiply
can become a general software 32-bit multiplication helper on this compiler.
The scale is bounded at 1024 before narrowing it to the signed operand.
Likewise, the phase values are already bounded, so a single conditional
subtraction wraps them instead of a general modulo. Keep those preconditions
visible and tested; the same shortcut is not valid for an arbitrary angle.

The renderer marks sprites fully visible only because the model guarantees
their entire footprint stays on screen. It is not a general recommendation
to disable SGDK culling. HUD updates are padded and queued for VBlank instead
of clearing and rewriting VRAM during active display.

## Primary sources

- [SGDK 2.11 math API, angle units and deprecated functions](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/maths.h)
- [SGDK 2.11 trig tables and approximation implementations](https://github.com/Stephane-D/SGDK/blob/v2.11/src/maths.c)
- [SGDK 2.11 sprite visibility controls](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h)
- [68000 multiply/divide instructions](https://www.nxp.com/docs/en/reference-manual/M68000PRM.pdf)

Host tests check the lab's model against independent real-valued oracles.
Target-code tests separately run actual SDK math in release and debug builds.
Neither replaces checking motion, sprite pressure and frame timing in an
emulator or on a console.
