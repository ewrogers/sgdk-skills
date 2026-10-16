---
name: sgdk-streaming
description: Stream SGDK graphics through bounded VRAM tile caches, with resident shared art, prefetch, staged DMA and safe FAR reads. Use when an area's patterns cannot all remain resident or new art must load during play, rather than ordinary MAP scrolling or cartridge mapper setup alone.
---

# SGDK Streaming

Keep the art needed for the current picture valid while preparing the next one.
Start with the game's renderer, cartridge layout, movement speed and frame
budget. The Lab's scenery, page size and loading policy are examples; follow
the project's art direction and requirements.

## Separate the resources

Distinguish ROM storage, decompression RAM, VRAM tile patterns, tilemap entries,
palette banks and queued transfers. Scrolling a tilemap does not automatically
make its pattern data resident. Deduplicate shared art before introducing a
cache; do not stream identical data separately just because two areas use it.

Reserve the hero, HUD and common effects separately from replaceable area art.
Plan a VRAM range that avoids plane tables, fonts and any Sprite-engine arena.
A shared ROM definition does not establish shared VRAM ownership.

- [Cache and visibility](references/cache-and-visibility.md): page IDs, pinning,
  eviction, circular tilemaps, lookahead and teleports.
- [Uploads and ROM banks](references/uploads-and-banks.md): staging lifetime,
  admission, compression and temporary FAR mappings.
- [Streaming Lab](assets/streaming-lab/README.md): standalone courier demo,
  native art, explicit budgets, automated walking and validation limits.

## Publish only complete work

Treat a cache entry as missing, loading or ready. Never publish its VRAM index
to a visible tilemap while only part of the entry has arrived. Pin old visible
art through the VBlank that replaces its last references. A pending upload
also owns its destination and source until the transfer has finished.

Choose chunk sizes from the remaining frame allowance after gameplay, sprites,
HUD, palettes and scroll work. Handle read, allocation and queue failures without
advancing progress. A configured DMA limit alone is not a completion protocol.
When the working set cannot fit, retain the old picture or cover it and perform
a staged redraw; do not silently evict something still visible.

## Prove the lead time

Prefetch for both movement directions, including the first step after loading.
Compare bytes to load / bytes per displayed frame with the time until that art
becomes visible. PAL has fewer upload opportunities per second even if movement
uses the same simulation rate. Test reversals, seams, cache exhaustion, cold and
warm teleports, frame-budget reductions and cleanup with pending DMA.

Pull `ewrogers/sgdk:latest` and verify the selected SDK's API and behavior before
writing an adapter. The Lab is checked against SGDK 2.11 / GCC 16.2.0. Host
oracles and executed M68000 probes check contracts and linked bytes; emulator
and hardware testing establish presentation and timing.

`sgdk-levels` covers Tiled/MAP and collision data; `sgdk-cartridge` covers mapper
configuration, banked audio and SRAM. Those skills are optional complements.
This skill installs independently, and its ordinary Lab ROM uses linear ROM.
