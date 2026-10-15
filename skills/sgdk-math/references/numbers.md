# Numbers and arithmetic

## Pick a scale that fits

SGDK's fixed-point values are signed integers with an implied binary scale.
They are not floating point. `FIX16(1.5)` is a constant with raw value 96;
the stored value is interpreted as 96 / 64.

SGDK 2.11 defines these formats:

| Type | Storage | Fraction bits | Step | Representable range |
| --- | --- | --- | --- | --- |
| `fix16` | signed 16-bit | 6 | 1/64 | -512 to 511 + 63/64 |
| `fix32` | signed 32-bit | 10 | 1/1024 | -2,097,152 to 2,097,151 + 1023/1024 |
| `fastfix16` | signed 16-bit | 8 | 1/256 | -128 to 127 + 255/256 |
| `fastfix32` | signed 32-bit | 16 | 1/65536 | -32,768 to 32,767 + 65535/65536 |

The integer-bit counts in `maths.h` include the sign bit. Do not assume that
`fix32` means 16.16, or that a 32-bit fixed-point multiplication uses a 64-bit
intermediate. Storage range and safe arithmetic range are different things.

For a scrolling level, a `fix16` world position can overflow after only 512
pixels. Keep a wider world position, subtract the camera in that wider type,
and then narrow a checked screen-space result. A `fix16` velocity can still
be a good fit for a few pixels per tick.

## Conversion and rounding

Use `FIX16`/`FIX32` for constants, and the SDK's named conversion helpers for
values already stored in another fixed-point format. A C cast changes the
storage type without rescaling the value.

```c
fix16 speed = FIX16(1.5);
fix32 distance = FIX32(240);
s16 whole_speed = F16_toInt(speed); /* 1 */
s32 screen_x = F32_toInt(distance); /* 240 */
```

In the selected M68000 toolchain, `F16_toInt(-1.25)` expressed with
`FIX16(-1.25)` produces -2: the SDK uses an arithmetic shift. This differs
from C signed integer division, which truncates toward zero. The round helper
adds half minus one raw unit before taking the integer part. Exact ties go
downward: 1.5 becomes 1, and -1.5 becomes -2. Test the intended behavior instead
of assuming ties round away from zero.

Keep runtime floating point out of hot paths. A decimal inside a folded
constant such as `FIX16(1.5)` does not itself require floating-point execution.
Passing a runtime `float` through the same macro is a different operation.

## Multiplication and division

For values that fit, prefer SGDK's `F16_mul` and `F16_div` rather than a plain
multiply or divide on scaled integers:

```c
fix16 displacement = F16_mul(FIX16(2), FIX16(1.5)); /* 3 */
fix16 ratio = F16_div(FIX16(3), FIX16(2));           /* 1.5 */
```

`F16_mul` uses a signed 16-by-16 multiply and shifts the 32-bit product.
The final result must still fit `fix16`. `F16_div` scales its numerator and
uses the SDK's signed division helper. The divisor must be nonzero and the
scaled quotient must fit; a 68000 divide can overflow even if both inputs fit.
These helpers are not checked-error APIs.

`F32_mul` shifts each input right by five bits, then multiplies the reduced
values. This sacrifices low input bits and can still overflow its signed
32-bit product. For example, one raw `fix32` unit multiplied by `FIX32(1)`
becomes zero, not one. This is the SDK's speed/precision tradeoff.

`F32_div` shifts the numerator left by five and the denominator right by five.
Bound the shifted numerator and check the reduced denominator, not just the
original denominator. A small positive denominator can become zero. SGDK's
header recommends `fix16` for multiply/divide where its range is sufficient.
Use a deliberately wider calculation only when the needed range/accuracy and
measured cost justify it; do not silently change the SDK contract.

## Integer intermediates

Widen before an operation, not after it. For two screen points, calculate
the difference in `s32` before squaring. For a radius check, compare squared
distances instead of calculating a square root every frame.

The lab's `demo_distance_sq` accepts signed 16-bit differences. Each square
fits `s32`, then the squares are added as `u32`. That handles even the
`(-32768, -32768)` case without signed overflow. It is a lab helper, not an
SGDK API. A difference between arbitrary 32-bit world coordinates needs its
own wider-range design.

Do not rely on unchecked signed wraparound, out-of-range narrowing or a
negative signed left shift in your own C. The toolchain's compatibility flags
for SGDK buffers do not make arbitrary C undefined behavior safe.

## Primary sources

- [SGDK 2.11 numeric definitions and API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/maths.h)
- [SGDK 2.11 signed storage types](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/types.h)
- [SGDK 2.11 arithmetic implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/maths.c)

The Math Lab tests execute the selected SDK's conversion, multiplication,
division and trig routines as M68000 code. They cover valid inputs and known
precision limits; they do not promise useful behavior outside those limits.
