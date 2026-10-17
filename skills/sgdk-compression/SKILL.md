---
name: sgdk-compression
description: Choose and validate SGDK asset compression, measure ROM/CPU/RAM trade-offs, and schedule decoding for loading or preloading. Use for packed graphics or data, not general archive formats or replacing SGDK's renderer.
---

# SGDK compression

Choose compression for the asset's access pattern and the game's frame budget.
Smaller ROM data can cost more CPU and staging RAM; decoded graphics still use
the same VRAM and transfer bytes. A game need not compress every asset.

Start from the existing project and its loading requirements. Preserve its design,
controls and resource ownership. Compression Lab is optional executable evidence,
not a game template or required loading architecture.

## Workflow

1. Inspect the selected `ewrogers/sgdk:latest` container's resource manual,
   headers and decoder source. Pull it before validation. Record SDK/compiler
   versions for measurements; do not assume online master matches the image.
2. Identify when each asset is needed, how long it stays resident, its decoded
   bytes and whether it must support random access. Compare representative art
   and a low-redundancy control, not just repeated zeroes.
3. Try NONE, APLIB and LZ4W where supported. Check actual emitted format/size,
   alignment, dictionary dependencies and final padded ROM. Do not interpret
   AUTO/BEST as the fastest runtime choice.
4. Measure release decoding with interrupts and normal audio behavior. Account
   for staging lifetime and upload bandwidth separately. Use direct mapped ROM
   uploads for suitable uncompressed data instead of adding an unnecessary copy.
5. Pick an explicit loading strategy: synchronous work while covered, preload
   before a deadline, or independently packed chunks. Ordinary `unpack()` is
   synchronous and cannot be resumed at an arbitrary byte boundary.
6. Check decoded byte equality, output guards, insufficient-capacity rejection
   before decoding, queue rejection/retry, and publication after completion.
   Verify PAL/NTSC display behavior in an emulator; console timing needs hardware.

Read [formats and measurement](references/formats-and-measurement.md) when
choosing a codec, preparing resources or interpreting size/time numbers.
Read [loading and preloading](references/loading-and-preloading.md) when assigning
buffers, chunking an asset or combining decoding with DMA and banked ROM.

[Compression Lab](assets/compression-lab/README.md) includes a standalone generator,
RAW/APLIB/LZ4W comparison, fade-covered loading and independent-chunk preloading.
Its host contracts and real-SDK base-M68000 tests live in repository tests; the
installed example does not depend on those tests or another installed skill.

## Boundaries

- `sgdk-assets`: source art/audio preparation, palettes and ResComp import errors.
  This skill owns codec selection, decode correctness and decoding cost.
- `sgdk-streaming`: tile caches, residency, visibility and bounded uploads.
  This skill prepares decoded bytes; a streaming system decides where they live.
- `sgdk-memory` / `sgdk-performance`: whole-game RAM accounting and profiling.
  Supply codec measurements without replacing those broader budgets.
- `sgdk-cartridge`: bank mappings, far pointers and audio reachability.
  Compression does not make an invalid banked pointer safe.

These are responsibility links, not required installations. The references explain
this skill's essential contracts directly.
