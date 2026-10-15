# ROM banks and FAR data

A large ROM is still storage. The 68000 sees an address window, and a mapper
chooses which part of that storage appears there. Loading a stage means mapping
its bytes, then transferring the needed graphics into VRAM. It does not copy a
whole bank into the console's 64 KiB work RAM.

## Start with the cartridge

An ordinary Mega Drive cartridge has a 4 MiB linear ROM window. The Sega/SSF
mapper divides that CPU window into eight 512 KiB regions. Region zero stays
fixed. SGDK's `SYS_setBank(region, bank)` changes regions 1 through 7;
`SYS_getBank(region)` returns SGDK's remembered assignment, not a hardware
readback. Raw register writes would make that remembered state inaccurate.

The board determines available banks and SRAM behavior. A larger `.bin` and an
SSF header do not make every cartridge support this mapper. Keep the physical
board or flash-cart profile in the release plan. Bank Lab uses twelve banks
(6 MiB), so four of them lie beyond the ordinary CPU ROM window.

See [SGDK 2.11 mapper.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/mapper.h),
[mapper.c](https://github.com/Stephane-D/SGDK/blob/v2.11/src/mapper.c) and
[Plutiedev: beyond 4 MB](https://plutiedev.com/beyond-4mb).

## Keep common content reachable

Bank Lab uses this explicit layout:

| Physical ROM area | Content | CPU access |
| --- | --- | --- |
| Bank 0, below `0x080000` | Code, interrupts, initialized-data image, UI, hero, click and resource metadata | Fixed region 0 |
| Bank 8, `0x400000` | Ember palette, floor and animated relic tiles | Temporary FAR regions 6/7 |
| Bank 9, `0x480100` | Ember FM stream | Reserved region 3, mapped at `0x180100` |
| Bank 10, `0x500000` | Tidal palette, floor and animated relic tiles | Temporary FAR regions 6/7 |
| Bank 11, `0x580000` | Tidal FM stream | Reserved region 3, mapped at `0x180000` |

A 32-byte probe starts at `0x47FFF0`. It deliberately spans banks 8 and 9.
Its bytes before `0x480100` do not overlap the first song.

Regions 1/2/4/5 are unused here. Do not put directly accessed common data at
`0x180000` through `0x1FFFFF`: that CPU region belongs to audio in this layout.
SGDK's FAR helper considers addresses below `0x300000` directly accessible;
it cannot know you reassigned another region.

ResComp's `NEAR` directive puts a resource file's binaries in `.rodata_bin`,
before FAR binaries. It does not promise bank-zero placement. The lab's linker
checks the combined fixed code/assets and initialized-data load image against
512 KiB. Its stage sprite descriptors stay resident, while their tile payloads
live in the stage graphics bank.

The custom linker also adds a trailing `BYTE(0)` to standalone resource output
sections. ResComp's binary input sections lack `ALLOC`; without a loadable
output section, `objcopy` can omit the bytes despite correctly addressed
symbols. Verify section flags and actual payload bytes after linking.

See [ResComp manual](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt)
and the [SDK linker](https://github.com/Stephane-D/SGDK/blob/v2.11/md.ld).

## Build the library for banking

SGDK 2.11 defaults `ENABLE_BANK_SWITCH` to zero. `FAR`/`FAR_SAFE` become active
when that configuration is enabled. SDK code, including sprite uploads and
startup data copying, must be compiled with the same configuration as the game.
Passing a game-only define is insufficient.

The lab copies the selected SDK into its own ignored output directory, enables
banking, rebuilds its release/debug libraries and uses a checked linker adapter.
It leaves `/opt/sgdk` unchanged. Mode/toolchain changes rebuild affected objects.
If an SDK update changes the expected layout, the helper fails for review.

See [config.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/config.h),
[mapper.h FAR macros](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/mapper.h) and
[startup](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c).

## Use temporary views briefly

`FAR(data)` returns a readable CPU pointer using regions 6/7 when needed.
`FAR_SAFE(data, size)` also handles a resource crossing a 512 KiB boundary.
Use a positive byte count. The helper exposes at most two adjacent banks;
chunk larger reads rather than assuming an unlimited mapped span.

```c
u8 marker[16];
const u8 *view = FAR_SAFE(stage_marker, sizeof marker);
for (u16 i = 0; i < sizeof marker; ++i) marker[i] = view[i];
/* marker is now owned RAM. Another FAR call may invalidate view. */
```

Treat pointers above the CPU ROM window as physical resource identifiers until
mapped. Do not dereference them directly, pass them to ordinary `memcpy`, or
cache mapped pointers across unrelated FAR calls. Interrupt handlers must not
silently change a mapping a foreground reader is using. Keep their resources
fixed, or design an explicit ownership/restoration protocol.

## Queued transfers are still readers

A DMA queue keeps a source address. It does not own the mapper assignment.
Mapping another resource before VBlank can turn that address into different
bytes. This applies to SDK sprite tile uploads as well as game-created queues.

Choose one policy per transfer:

- Transfer immediately while the view is valid.
- Copy into persistent RAM and keep it alive until the queue flushes.
- Use `DMA_QUEUE_COPY` to let SGDK copy the payload into its staging buffer.
- Keep the mapping unchanged until the queued read has finished.

Bank Lab queues a copied palette, deliberately remaps FAR regions, then lets
VBlank consume the copy. It performs other FAR work before `SPR_update()` and
makes no further FAR calls until those sprite uploads flush. This is a small
scene-specific schedule, not a general-purpose bank-aware queue.

The small palette read uses 48 scratch bytes plus a 32-byte color array; the
boundary probe uses 32 bytes. SRAM readback uses two 16-byte records. Compiler
stack overhead, SDK DMA staging and the sprite engine are additional costs.
No helper allocates a whole ROM bank in work RAM.

The DMA source boundary is separately 128 KiB. Standard SDK DMA helpers can
split transfers there; their `Fast` variants require the caller to respect it.
Neither boundary guarantees that VRAM space or the frame's transfer budget fits.

See [dma.c](https://github.com/Stephane-D/SGDK/blob/v2.11/src/dma.c) and
[sprite_eng.c](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sprite_eng.c).
