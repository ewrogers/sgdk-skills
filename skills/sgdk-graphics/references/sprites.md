# Sprites and animations

Use SGDK's sprite engine for moving objects. It builds the hardware sprite list
and can upload new frame tiles as animations advance. It does not provide game
collision responses or guarantee that every object fits on every scanline.

## Allocate deliberately

Set the VDP plane layout first, then call `SPR_init()` once. SGDK 2.11 reserves
420 tiles by default for sprite graphics. `SPR_initEx(tile_count)` changes that
reservation; zero selects the default, not an empty pool.

Create an object with `SPR_addSprite(&definition, x, y, attributes)` and check
for `NULL`. `SPR_releaseSprite(sprite)` releases that instance. Do not retain
the pointer afterward. `SPR_end()` ends the entire sprite engine, so a single
object's cleanup must not call it.

The normal automatic allocator reserves each sprite definition's `maxNumTile`
per instance. An image shared by twenty objects can still require twenty frame
allocations. Inspect `SPR_getFreeVRAM()` and `SPR_getLargestFreeVRAMBlock()` when
allocation fails; enough total free space does not imply a large enough block.
`SPR_addSpriteSafe()` can try defragmentation, but that extra work is not free.

Manual shared tile placement can help repeated objects with identical frames.
Only use it after verifying tile upload ownership and synchronization. Do not
let independently animated instances overwrite each other's tiles.

## Prepare a sheet

In ResComp, frame dimensions are in **8-pixel tiles**, not pixels. Each sheet
row is an animation; columns are frames. This declares 16x16 frames, two rows,
and separate hold times:

```text
SPRITE hero "hero.png" 2 2 NONE [[18,4,4,4][5,5,5,10]] NONE
```

The nested timing lists have no spaces or comma between rows. The last `NONE`
disables generated collision data. Times are sprite-engine update counts; zero
disables automatic advancement. Avoid duplicate frames just to extend a hold.

Keep art within its palette and frame grid. Compression is a tradeoff: `NONE`
avoids frame decompression, while `FAST`/`LZ4W` can reduce ROM at a runtime cost.
Measure frame uploads and CPU before choosing a more expensive format.

## Play and finish an animation

```c
SPR_setAnim(sprite, ANIM_WALK);
SPR_setPosition(sprite, screen_x, screen_y);

/* Start a one-shot. These indices must exist in the resource. */
SPR_setAnimAndFrame(sprite, ANIM_EXPLODE, 0);
SPR_setAnimationLoop(sprite, FALSE);
SPR_setAutoAnimation(sprite, TRUE);
explosion_pending = TRUE;

/* Once per frame, after the initial frame has been established: */
if (explosion_pending && SPR_isAnimationDone(sprite))
{
    explosion_pending = FALSE;
    /* Award points, hide the object, or request its removal. */
}
```

`SPR_setAnim()` keeps the current frame when you pass the current animation.
`SPR_setAnimAndFrame()` only changes state when the animation or frame differs.
It is not an unconditional restart command, especially for a one-frame clip.
Choose an explicit gameplay state for replay rather than repeatedly issuing
the same call and expecting the timer to reset.

`SPR_isAnimationDone()` is a predicate, not a queued completion event. With
automatic animation it becomes true at the end of the last frame; with manual
animation it can be true as soon as that frame is selected. Latch your response
so it happens once. Establish a newly added sprite's initial frame with the
normal `SPR_update()` before inspecting frame-dependent state.

## Stop, resume and timing

`SPR_setAutoAnimation(sprite, FALSE)` keeps the current frame. Enabling it again
restarts that frame's full hold time; it does not preserve the remaining delay.
Use your own remaining-time state if an exact pause/resume is important.

`SPR_update()` advances automatic animation once per call. At one call per
VBlank, the same holds last longer at 50 Hz than at 60 Hz. Decide whether your
game adapts its timings or uses explicit animation control; do not claim SGDK
automatically normalizes them. Avoid extra calls to compensate without also
accounting for sprite uploads and the rest of the frame loop.

For manual playback, disable automatic animation and use `SPR_setFrame()` or
`SPR_nextFrame()` at your chosen times. World movement, animation clocks and
render submission can share a tick without being the same operation.

## Hardware limits

In H40/320-pixel mode the VDP supports 80 hardware sprites overall, with up to
20 sprites and 320 sprite pixels on a scanline. H32/256 mode allows 64, 16 and
256 respectively. Transparent portions still consume hardware evaluation
budget; a nominally empty part of a sprite is not free.

A hardware sprite is at most 32x32 pixels. SGDK splits larger frames into
pieces, so count generated pieces, not game objects. The resource compiler has
its own limits too, including 16 pieces per exported frame and 255 frames per
animation in this version.

Enable or inspect SGDK's VDP sprite checking when investigating total overflow,
but still test crowded scanlines. `SPR_getUsedVDPSprite()` is not a proof that
all rows render. Plan which objects may disappear first under pressure, and
measure changes with an optimized ROM.

## Sources

- [SGDK 2.11 sprite API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h)
- [Sprite implementation, timers and allocation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sprite_eng.c)
- [ResComp SPRITE syntax and limits](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt)
- [Hardware sprite definitions](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_spr.h)
