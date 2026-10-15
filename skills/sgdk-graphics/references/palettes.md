# Palettes and frame updates

## Assign colors before effects

The VDP has four palette slots of sixteen entries. A tile or sprite piece
selects one slot. Index zero is transparent for that tile or piece; the global
backdrop shows through where nothing opaque covers it.

Write down who owns each slot. For example, Graphics Lab uses PAL0 for distant
scenery, PAL1 for cycling foreground highlights, PAL2 for the fading actor and
PAL3 for text. These are example assignments, not an SGDK convention.

`PAL_setPalette(PAL1, colors, transfer_method)` replaces all sixteen entries.
`PAL_setColors(first_index, colors, count, transfer_method)` writes a range in
the full 64-entry CRAM space. PAL1's first visible color is global index 17.
An effect changes **all** tiles and sprites using those entries.

Normal CRAM colors use three bits per channel, encoded as `0x0BGR` with even
hex digits. Prefer SGDK color macros or converted resource palettes over
inventing RGB888 register writes. Hardware output is not a linear sRGB ramp;
compare fades and dark colors in the target emulator or on hardware.

## Fade without blocking gameplay

```c
if (!PAL_isDoingFade())
    PAL_fadeOutPalette(PAL2, 30, TRUE);
```

`TRUE` starts an asynchronous fade, advanced by normal VBlank processing.
`PAL_fadeInPalette()` fades from black to a supplied target;
`PAL_fadeToPalette()` fades from the current colors. Keep the target valid as
required by the selected API; a static resource palette is a simple choice.

There is one global fade operation, not an independent timer per palette.
Coordinate requests: wait, reject a second request, or deliberately interrupt
and replace it. `PAL_interruptFade()` stops progression; it does not restore the
old colors. `PAL_isDoingFade()` can be polled for completion.

Blocking fades run VBlank processing themselves while the caller waits. They
do not run your ordinary gameplay loop. Use them for intentional transitions,
not when movement is meant to continue. Fade lengths count refresh steps, so
30 steps is about half a second at 60 Hz and 0.6 seconds at 50 Hz.

## Cycle a small range

Rotate selected colors for water, lights or a shimmer. Keep the source colors
in game state and write only the changed range. If the range is built on the
stack, copy it into SGDK's DMA queue:

```c
u16 colors[3] = {highlight_1, highlight_2, highlight_0};
PAL_setColors(17, colors, 3, DMA_QUEUE_COPY);
```

Use a tick counter rather than allocating a timer object for every palette.
Do not cycle entries another effect is fading at the same time. Choose one
owner or combine both effects into one computed output. To fade a whole scene,
stop cycling first or include its result in that scene's fade policy.

## Keep transfers inside the frame budget

A typical loop reads SGDK's cached input, updates game state, changes sprite
positions/animation requests, queues graphics changes, calls `SPR_update()`,
then calls `SYS_doVBlankProcess()`. Input and asynchronous system work are also
serviced by that VBlank path. Do not replace it with a bare busy-wait and expect
all queued work to run.

`DMA_QUEUE` keeps a pointer to the source until transfer. That source must stay
alive **and unchanged**. `DMA_QUEUE_COPY` copies it into a bounded temporary
buffer. Copies are useful, but still consume RAM and transfer space. Verify the
selected APIs' return contracts; palette setters return `void`, so they are not
recoverable capacity checks.

Budget transfers in bytes separately from queue entries and temporary buffer
capacity. Tile uploads, tilemaps, the sprite table, palettes and scroll tables
all compete for VBlank time. Increasing a queue limit cannot create more
bandwidth. Stage large initial loads with the display disabled, and test worst
case animation changes and camera jumps during gameplay.

Avoid immediate CRAM writes during active display unless intentionally using a
carefully timed raster effect. They can produce visible color dots. For routine
effects, use queued or SDK-managed VBlank updates. On scene teardown, finish
pending transfers before freeing their sources or reusing their destinations.

## Sources

- [SGDK 2.11 palette API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/pal.h)
- [Fade implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/pal.c)
- [DMA queue and temporary buffers](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h)
- [System frame processing](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c)
- [Mega Drive color ramp](https://plutiedev.com/vdp-color-ramp)
