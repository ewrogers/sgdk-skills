# Cache ownership and visibility

## IDs are not VRAM addresses

Keep a stable content ID in the world data and translate it to the current
cache slot when building tilemap entries. An evicted slot can contain different
art later. Do not save its VRAM index as the permanent identity of a wall or
animation. Keep collision and gameplay data independent of visual tile IDs.

SGDK's `MAP_scrollTo` updates name-table entries; it does not provide an
arbitrary pattern cache for an unlimited tileset. ResComp may deduplicate and
flip tiles. A custom cache needs a mapping that respects the exported IDs,
flips and palette attributes, or a deliberately authored page format.
See [SGDK 2.11 MAP implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/map.c)
and [ResComp formats](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt).

Streaming Lab uses uncompressed 16-tile pages and a small generated world.
It does not pretend its page IDs are arbitrary ResComp MAP indices. Its
18-slot arena deliberately holds less than the 24-page catalog; the entire
catalog could fit a larger VRAM allocation. This demonstrates eviction without
claiming that 288 tiles is a console limit.

## Pin the old and next pictures

A simple cache can use an inactive marker, loading state, ready state, an LRU
timestamp and explicit pins. Choose an unpinned victim; if none exists, report
pressure instead of overwriting a visible page. Include prefetched pages in
the wanted set so loading one page does not evict another page of the same
destination. Protect an in-flight upload even if the requested destination
changes. The Lab finishes that small page before admitting another request.

Only a fully transferred page may resolve to a usable tile index. A successful
queue call means the transfer was admitted, not that its bytes are in VRAM.
For ordinary scrolling, retain old visible pages until the tilemap and scroll
updates that stop referencing them have drained. Covered teleports can release
those pins earlier because the Window hides the old picture.

The Lab has one cache owner and no heap allocation. A 24-byte page-to-slot
lookup avoids searching every cache entry for each tilemap cell. LRU ages use unsigned
differences to tolerate clock wrap during ordinary cache turnover. A game
retaining an untouched entry for an entire counter cycle needs a different age
policy. Do not assume timestamps establish unlimited lifetimes.

## Prefetch is a timing budget

One Lab area needs 6 × 16 × 32 = 3,072 pattern bytes. At 256 bytes per displayed
frame it takes 12 frames; at 64 bytes it takes 48. The Lab reserves a 128-pixel
margin in both directions and warms that margin before showing the first view.
Its 120-pixel/second walk allows about 64 NTSC or 53 PAL frames to load an area.
The chosen margins therefore cover both upload settings under the tested load.

An earlier build prefetched too late: walking could pause even while the ROM
reported 60 FPS. Distinguish waiting for content from missing a frame. Reversing
direction immediately after a teleport is another useful test. Faster players,
larger art, more consumers or lower upload budgets need more lead distance,
different working sets or a visible loading policy. Do not copy the Lab's
numbers as universal defaults.

## Circular maps and covered redraws

The Lab uses a 64-column plane as a ring. A world column writes to
`world_column & 63`. It queues the entering column before changing the scroll
position, using persistent column buffers with a 128-byte destination stride.
A partially scrolled 320-pixel view needs 41 columns, not always 40. Moving
from camera 0 to 2 already exposes a new column at the right edge even though
`camera / 8` has not changed. Tests cover every camera alignment.

Ordinary movement changes at most one entering column per frame. A teleport
instead covers the picture using resident Window tiles, warms destination art
and redraws up to four columns per frame. Reveal it only after the last queued
column has drained. The cover requires no large active-display fill or palette
write. This is a loading policy, not a universal seamless-teleport promise.

For a softer transition, Streaming Lab fades all four palettes to black over
about a quarter second, holds black while the covered redraw finishes, then
fades back in. It waits for the black palette to drain before changing the old
picture, and for the final map uploads to drain before fading in. A reset still
shows the loading panel so both approaches can be compared.

This is RGB3 brightness scaling, not palette cycling or HBlank writes. Queue
palette changes for VBlank and keep one owner during the fade; background palette
loads must update the fade's source colors instead of fighting its output.
Include the 128-byte full-palette transfer in the frame's shared DMA allowance.
The Lab holds its RAM palette buffer until the queue drains. The fade improves
presentation but does not reduce load time; a cold cache still needs a black hold.

Window geometry and plane addresses come from the selected SDK:
[VDP layout and Window API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h).
If integrating with another renderer, reconcile its plane, Window and tile
ownership first.
