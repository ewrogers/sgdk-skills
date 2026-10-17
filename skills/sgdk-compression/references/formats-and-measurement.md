# Formats and measurement

## Choose by access pattern

| Path | Useful when | Cost to consider |
| --- | --- | --- |
| NONE | Hot/shared art, small data, low redundancy, or direct ROM-to-VRAM upload | Larger ROM payload; no codec staging is needed for a direct mapped-ROM transfer |
| APLIB | Dense data loaded infrequently while the screen is covered | Higher decode cost; decoded output needs storage before a staged upload |
| LZ4W | Frequent preparation or streamed graphics with adequate measured headroom | Usually less compact than APLIB; still synchronous CPU work |

These are starting points, not a universal winner. SGDK's ResComp 3.95 manual
labels APLIB slow and discourages it for streamed sprites; it recommends LZ4W
there. High-level image/sprite loaders may perform decoding and allocation on
your behalf: inspect that path before budgeting it as a cheap pointer change.
Sound drivers have their own data formats and timing constraints; do not wrap a
live XGM/PCM stream in arbitrary asset compression and expect the driver to read it.

## Resource and API contracts

Verified against the selected container: SGDK 2.11, ResComp 3.95, GCC 16.2.0.
The Lab uses:

```text
BIN room_art "room.bin" 2 2 0 LZ4W FALSE
```

Arguments after the filename are address alignment, size alignment, fill byte,
compression and far placement. Explicit BIN codecs are forced in this version,
even when compression makes data larger. Embedded graphics resources can choose
NONE instead: use their emitted compression metadata. Do not apply a presumed
codec to a resource solely because the input directive requested it.

```c
u32 unpack(u16 compression, u8 *src, u8 *dest);
u32 aplib_unpack(u8 *src, u8 *dest);
u32 lz4w_unpack(const u8 *src, u8 *dest);
```

`unpack()` dispatches APLIB/LZ4W and returns decoded byte count. NONE returns zero;
copy a known uncompressed length yourself if a RAM copy is actually needed.
Both source and destination must satisfy the selected decoder's alignment and
mapping contracts; the Lab uses even-addressed packets and word-aligned buffers.
These routines have no source-length or destination-capacity argument. They are
for valid build-generated assets, not untrusted or corrupt streams. Reject a
known decoded size larger than the destination **before** calling them; checking
the return afterward cannot prevent an out-of-bounds write. Guard-word tests
check the fixtures, not safety for arbitrary malformed input.

LZ4W packing can reference preceding resource data. Keep that dictionary reachable
and unchanged, or produce a self-contained stream. The Lab uses one BIN per
resource file for its independent packets; copying only a dependent compressed
block to RAM would lose its preceding dictionary. Inspect packer output when
changing resource layout or switching toolchain versions.

## Measure four different budgets

1. **Stored payload:** actual emitted array sizes, including codec headers. Account
   separately for alignment padding between resources. Compare the same decoded bytes. Smaller chunks add overhead and
   reduce dictionary reuse; noisy data can expand.
2. **Shipping ROM:** code, metadata, resources and final padding. Link and inspect
   an actual candidate build. A multi-format benchmark contains all candidates;
   its padded size is not a per-codec savings claim.
3. **CPU:** decode/copy duration in optimized builds, including representative
   interrupts/audio. Keep DMA submission, transfer and file/host build time out
   of a number labeled decode-only. Also measure the complete loading frame.
4. **RAM/VRAM:** decoded staging, retained caches and buffer overlap, plus any
   allocator/stack overhead. Codec output size does not shrink VRAM usage. The
   final decoded bytes still need their normal DMA budget.

The Lab estimates microseconds using `getSubTick()` (nominal 1/76,800 second),
averaged across 16 calls. SGDK's timer holds a fixed value through VBlank; endpoints
there introduce quantization. Interrupts stay enabled, so wall-time includes them.
Do not mask interrupts around a long decode to make a benchmark look better:
that changes audio, input and video behavior. The base-M68000 test executes
instructions for correctness, not cycle-accurate measurements. Recheck estimates
on the intended console/region before treating them as a deadline guarantee.

## Primary sources

- [SGDK 2.11 resource manual](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt)
- [Decoder declarations](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/tools.h)
  and [dispatcher](https://github.com/Stephane-D/SGDK/blob/v2.11/src/tools.c)
- [BIN export](https://github.com/Stephane-D/SGDK/blob/v2.11/tools/rescomp/src/sgdk/rescomp/resource/Bin.java)
  and [packing decisions](https://github.com/Stephane-D/SGDK/blob/v2.11/tools/rescomp/src/sgdk/rescomp/tool/Util.java)
- [Timer contract](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/timer.h)
  and [implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/timer.c)

Use the image's `/opt/sgdk` files to resolve differences from these versioned links.
