# Transfers and rendering budgets

## CPU and DMA are different measurements

DMA can halt the 68000 while transferring data. Count queued bytes, queue entries
and temporary-buffer use separately from simulation work and retained VRAM.
Sampling `DMA_getQueueTransferSize()` before `SYS_doVBlankProcess()` shows the
pending queue, including sprite uploads. It does not include earlier immediate
transfers or prove that everything will fit in VBlank.

SGDK APIs use different units: `DMA_getQueueTransferSize()` returns **bytes**;
`DMA_transfer()` takes a length in **words**. A 2,048-byte test transfer therefore
uses a length of 1,024. Verify destination bounds and stride, and preserve a
deferred transfer's source until it finishes. Use a copied queue transfer when
a temporary source cannot outlive the call, accounting for its buffer cost.

`DMA_getMaxQueueSize()` is entry capacity. `DMA_getBufferSize()` is temporary
storage in bytes. `DMA_getMaxTransferSize()` is SGDK's per-flush byte allowance,
not a hardware promise. Changing queue/buffer capacities clears queued work;
configure them during loading. Check boolean transfer results and handle refusal
without corrupting an asset's partially updated state.

SGDK's maximum-transfer and ignore-over-capacity settings influence whether
excess work is processed or omitted. They do not create bandwidth. Keep its
conservative defaults until a measured transfer schedule justifies changing
them. Header estimates vary by mode and assumptions; do not turn one quoted
KB/frame figure into a guaranteed budget for every NTSC or PAL scene.

## Stream what changed

Share static frame tiles when useful, update dirty map cells, and avoid uploading
whole planes or palettes for a small change. Stagger optional work if the game
can tolerate it. Test peak animation changes and scrolling boundaries, not only
frames where nothing changes. Verify that an optimization preserves the intended
frame, scroll position, palette and timing.

The lab's optional transfer repeatedly clears an unused BG_B tilemap from a
permanent zero-filled ROM array. It has a bounded 4,096-byte destination and no
visible art change. This isolates extra transfer work; it is not an example of
efficient map streaming. Its two-frame orbs use SGDK's automatic tile upload,
so synchronized frame changes can produce transfer spikes.

Schedule ordinary palette changes in VBlank. Active-display palette writes can
produce CRAM dots. Avoid hiding timing problems with broad interrupt masking;
interrupts, input and sound also need time. Coordinate larger transfers with
the chosen audio driver's Z80/DMA behavior and representative sample playback.
The performance lab is silent and does not establish audio-contention headroom.

## Count hardware pieces and scanlines

| Display mode | Hardware sprites/frame | Hardware sprites/scanline | Sprite width/scanline |
| --- | --- | --- | --- |
| H40, 320 pixels | 80 | 20 | 320 pixels |
| H32, 256 pixels | 64 | 16 | 256 pixels |

A logical SGDK sprite may use multiple hardware pieces. Transparent pixels
still contribute to a piece's scanline width cost. Sprites can disappear on a
crowded line well below the per-frame total; raising an object pool will not
fix that. Check ordering and masking behavior separately from CPU overload.

The lab allocates 64 one-piece 8x8 sprites. Its one-scanline case deliberately
exceeds H40's 20-piece line limit at larger counts, while retaining the same
simulated bodies and grid queries. It compresses only their rendered Y positions.
This is a display-limit demonstration, not an equivalent visual optimization.

## Keep memory budgets separate

VRAM's 64 KiB also holds maps, the sprite table and scroll tables. Plan the
layout before allocating tiles. Four 16-entry palettes are shared across planes
and sprites. Main RAM caching cannot raise these VDP limits, although it can
make decompression, state updates or transfer preparation cheaper.

## Sources

- [DMA units, capacities and ownership](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h)
- [DMA implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/dma.c)
- [Logical sprite allocation and animation](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h)
- [Hardware sprite interface](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_spr.h)
- [VDP tile layout](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h)
