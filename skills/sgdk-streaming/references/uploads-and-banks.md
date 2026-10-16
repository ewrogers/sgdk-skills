# Bounded uploads and banked sources

## Own the source until the reader finishes

The cache's 256-byte, word-aligned staging array stays unchanged while a queued
DMA borrows it. It accepts at most eight tiles per step, and a page remains
loading until both halves have drained. Failed reads or queue submissions do
not advance the completed tile count. Reset refuses to release in-flight data.
An aborted page between transfers can be discarded while it is still hidden.

The adapter uses `DMA_queueDma(location, source, destination, words, step)`.
Its length is in 16-bit words, its destination is a byte address, and tiles are
32 bytes each. The ordinary helper handles a DMA source crossing a 128 KiB
boundary; the `Fast` variant skips that check. This boundary is different from
a cartridge mapper's bank size. Keep destination ranges and source alignment
valid as well as the transfer size.

The Lab configures a 1,536-byte allowance, checks both its own remaining byte
budget and SDK queue capacity, then charges successful work. Pattern streaming
uses at most 256 bytes; column, palette, sprite and HUD work share the rest.
Scroll-register updates and CPU work also cost time. These are conservative
Lab choices, not a promise that every game can spend that much DMA every frame.

`DMA_setMaxTransferSize()` is not a scheduler that reliably defers arbitrary
over-budget work. In SGDK 2.11, overflow handling depends on queue settings;
transfers can be flushed beyond the limit or ignored. `DMA_canQueue()` checks
transfer capacity, not free queue entries. Check both, prevent overflow before
admission, and acknowledge only transfers your ownership protocol knows were
processed. A newly empty queue alone does not prove nobody dropped its work.

See [DMA API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h) and
[implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/dma.c).
The Lab has no competing renderer; its explicit producers stay below the
allowance, drain each frame and only then reuse their source buffers.

## Larger ROMs do not enlarge VRAM

Bank switching changes which ROM bytes the CPU can see. It does not increase
RAM, VRAM, palette slots or VBlank bandwidth. Keep two independent plans:
where an asset lives in the cartridge and where its current working data lives.
Common code, interrupts, metadata and active audio need stable ROM access;
hero/HUD tiles need stable VRAM access. Those are different kinds of residency.

For Sega/SSF-style banking, the mapper uses 512 KiB regions. SGDK's FAR helpers
use temporary mappings; another FAR call can invalidate a previously returned
pointer. The Lab's `sc_rom_read` uses `FAR_SAFE(source, bytes)` and immediately
copies into the cache's owned RAM. It queues that RAM, never the borrowed ROM
view. A test remaps the view before the simulated VBlank and verifies that the
queued bytes remain intact.

The ordinary Streaming Lab ROM is small and linear. FAR macros are no-ops when
`ENABLE_BANK_SWITCH` is disabled. A banked game must rebuild the SDK with the
matching configuration, establish checked linker placement and test its actual
cartridge profile. A game-only define does not fix a prebuilt linear SDK.
Keep each FAR read bounded; `FAR_SAFE` exposes at most two adjacent banks, not
an unlimited contiguous ROM. Do not let an interrupt or audio driver change a
mapping while a foreground reader depends on it.

See [SGDK mapper API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/mapper.h),
[mapper implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/mapper.c)
and [Plutiedev: beyond 4 MB](https://plutiedev.com/beyond-4mb).
For board support, banked XGM2/sample residency and SRAM overlays, use
`sgdk-cartridge` when available. Its Bank Lab supplies the actual large-ROM
example; Streaming Lab's recording test is not a physical-mapper emulation.

## Compression needs its own work budget

The Lab stores pages uncompressed so reads and upload sizes are easy to audit.
For compressed art, author independently decodable blocks and budget their
worst-case decoded size, scratch memory and CPU time separately from DMA.
Decoding a whole tileset in one frame can stall a game even if uploads are tiny.

One practical backend can decode a complete small page into a bounded buffer,
then upload it across several frames. Keep that decoded page alive until the
last chunk finishes; the Lab's 256-byte chunk buffer cannot hold an entire
512-byte page. Do not assume an arbitrary offset in an APLIB/LZ4W stream can be
decoded independently. Verify the selected SDK's resource metadata, decoder
and allocation behavior before substituting compressed data for raw pages.
See [ResComp compression formats](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt)
and [SGDK unpacking tools](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/tools.h).

An enabled SRAM overlay can hide ROM even when a FAR pointer would otherwise
be directly readable. Finish source reads before enabling that overlay, or
schedule them after it is disabled. Keep shared code and active audio reachable
throughout. Mapper setup and save operations need one agreed ownership plan.
