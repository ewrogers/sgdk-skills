# Build the wave before the frame

## A table is often enough

Select `VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE)` and supply one signed
horizontal offset per visible line. For water, keep the normal camera offset
above the surface and add a small periodic displacement below it. A road can
use a different offset for each depth; heat haze can vary the displacement
over height. These are scroll-table designs, not extra sprite objects.

The lab uses a 32-entry integer wave with amplitude four pixels, sampled into
a persistent 224-word table. No floating point or per-line interrupt is needed.
The small lookup is an example, not a requirement to avoid the SDK's fixed-point
math. Keep displacement bounded so the camera's prepared map area covers it.

```c
static s16 scroll[224];

/* Fill scroll with -camera_x plus the chosen per-line effect. */
VDP_setHorizontalScrollLine(BG_A, 0, scroll, 224, DMA_QUEUE);
SYS_doVBlankProcess();
```

The queued source must survive until that flush. Do not queue a local array
that expires or rewrite a buffer before its previous transfer finishes. The
lab builds once, flushes once, then reuses the buffer. A producer that runs
independently needs an appropriate buffer handoff.

## Include the camera, do not replace it

The wave offset is added to `-camera_x`. Replacing camera scroll with the wave
alone makes the background detach from its world coordinates. Waterline and
table indexes are screen-space here; converting a world-space water height
also needs the camera's vertical offset.

SGDK's `MAP_scrollTo()` handles map streaming and can queue its own flat line
table when the camera moves in line-scroll mode. The lab queues its waved table
after MAP's update so the final transfer wins. This costs an extra baseline
table upload on camera moves; do not hide that work in a budget. For a production
renderer, choose a deliberate owner/composition path instead of letting systems
overwrite each other's scroll values accidentally.

## Count the right resources

| Item | Cost for this lab |
| --- | --- |
| User line-scroll buffer | 224 signed words, 448 bytes RAM |
| User line-scroll upload | 448 bytes per displayed frame |
| Both planes, if both use independent tables | 896 bytes per displayed frame |
| MAP's own line buffer | Additional allocation; its flat upload occurs on camera updates |
| Water palette storage | 32 dry colors plus 16 packed wet pairs, 128 bytes RAM |

The VDP scroll table interleaves A/B words with a four-byte stride. Its reserved
VRAM table space is separate from the RAM buffer and transfer-byte count.
Streaming map updates, sprite frames and other DMA still add their own costs.
Raster effects do not increase sprite scanline limits or the number of palette
banks available to a pixel.

Sources: SGDK 2.11 [scroll API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_bg.h),
[table destination/stride](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp_bg.c),
[MAP scroll composition](https://github.com/Stephane-D/SGDK/blob/v2.11/src/map.c).
