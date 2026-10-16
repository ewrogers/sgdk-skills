# C correctness at the host/68000 boundary

## Check intermediates, not just fields

In the checked toolchain, `short` is 16 bits; `int`, `long` and pointers are
32 bits. SGDK's `u16` is unsigned short and `u32` is unsigned long. A desktop's
`long`, pointer size, padding and plain-char signedness need not match.
Use the SDK types at SDK boundaries and explicit-width types in portable code.
For an ABI assumption that code actually needs, use `_Static_assert` in a target
build. Do not assume every 16-bit console uses a 16-bit C `int`.

Two `u16` operands promote to signed `int` here. Thus `u32 area = width * height`
can overflow before assignment: 65,535 squared exceeds signed 32-bit range.
Cast an operand before multiplication, as [bp_square](../assets/examples/src/checked.c)
does. Casting the already-computed product is too late. Unsigned wrap is defined,
but only useful when it is part of the algorithm's contract; signed overflow is
not a reliable wrapping operation.

The examples distinguish clamping from wrapping. `bp_add_clamped` widens before
addition; `bp_elapsed` explicitly returns the modulo-65,536 difference. That
elapsed value cannot distinguish intervals separated by a whole counter wrap.
Choose a sampling interval and counter width that make this unambiguous.
For fixed-point scaling, aiming and rounding policies, use `sgdk-math` if available.

Check signedness before comparing or converting. A negative index converted to
unsigned may become a large positive number. Narrowing is not a substitute for
range checking. Avoid `abs` on a signed type's minimum value, overflowing
subtraction before a distance calculation, and multiplication whose intermediate
is narrower than its destination. Widen first, then verify the widened range too.

## Shifts and expression order

Validate a shift count before shifting: negative counts and counts at least as
large as the promoted left operand's width are undefined. Build masks in an
unsigned type, then narrow after checking the intended bit range.
`bp_layer` rejects indices outside its 16-layer domain without changing output.
A large positive signed left shift can overflow; shifting a negative signed
value right is implementation-defined in C17. Division and right shift need not
have the same rounding for negative inputs.

Avoid macros that evaluate arguments more than once. Use a small function or
`static inline` when side effects or types matter. Do not rely on function
arguments being evaluated left-to-right; split dependent mutations into separate
statements. Optimization must preserve the chosen language's defined behavior,
not a hoped-for instruction sequence.

## Decode bytes rather than reinterpreting them

A pointer cast does not align bytes, create a valid object of the new type,
remove struct padding or specify byte order. The 68000 also requires even
addresses for ordinary word/long accesses. Packed structures and disabling
strict aliasing do not solve every one of these problems.

[The record codec](../assets/examples/src/checked.c) reads bytes, widens before
large shifts, validates into a local record and publishes only after success.
Its eight-byte format is independent of `sizeof(BpRecord)` and handles a source
at a byte offset. The encoder checks capacity and record validity before writing.
This is a C representation example, not a replacement save-file system.
Use `sgdk-save` for versioning, checksums and recovery.

Character access can inspect an object's representation. `memcpy` can copy
representations between suitably sized live objects without incompatible pointer
accesses, but does not by itself define an external byte format or validate an
arbitrary bit pattern for a destination type. Do not serialize raw pointers or
assume a whole struct is a stable file format. Use `memmove` for overlapping
regions; ordinary `memcpy` requires non-overlap. Match the SDK's memory routines'
argument widths and behavior before using a desktop-library idiom on target.

## Sources

- SGDK 2.11 [types](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/types.h)
  and [memory API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/memory.h).
- WG14 [N1570 public C11 draft](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf):
  6.2.4 lifetime, 6.3.1.1 promotions, 6.3.1.3 conversions, 6.3.2.3 alignment,
  6.5 effective type/expression rules, 6.5.7 shifts, 6.7.2.1 structure layout,
  7.24.2 copying. These foundational rules also apply to the C17 baseline;
  this draft is a public reference, not the published C17 standard.
- GCC 16.2 [optimization and aliasing](https://gcc.gnu.org/onlinedocs/gcc-16.2.0/gcc/Optimize-Options.html).
