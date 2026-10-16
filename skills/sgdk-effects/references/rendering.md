# Rendering and budgets

## Count hardware pieces and pixels

The Mega Drive's H40 mode (320 pixels wide) allows 80 hardware sprites in a frame,
20 on a scanline and 320 sprite pixels on a scanline. H32 allows 64, 16 and 256.
A large SGDK object can use several pieces; count actual frame rectangles, not
objects or opaque pixels. [Plutiedev's sprite guide](https://plutiedev.com/sprites)
explains the limits and column-first tile arrangement.

Reserve room for gameplay and HUD sprites before admitting decoration. Effects
Lab renders at most 64 one-piece 8×8 effects and one 16×16 lantern. It admits at
most eight effect rectangles on any exact scanline: at most nine sprites and
80 sprite pixels including the lantern. Its 64 simulated slots, 64 draw slots
and eight-per-line allowance are software choices, not console limits.

The Lab counts each eight-pixel rectangle in the one or two eight-scanline
bands it touches. No band admits more than eight effects, so no individual
scanline can exceed eight either. This conservative test may reject rectangles
that would fit with exact per-line accounting. It trades a few cosmetic sprites
for cheaper admission and a 28-byte counter table; it does not relax the bound.
A separate reference verifies accepted order and positions, while tests count
actual rectangle coverage independently on every scanline.

Impact priority is processed before trails/weather; rotating equal-priority
order avoids permanently starving the same particles in a dense cluster.
A skipped particle remains simulated and may render next frame. This rotation
can produce visible flicker under pressure: choose lower density or a different
policy when that is distracting. A bound is not a promise of comfortable visuals.

## Share frames; keep one table owner

For the general Sprite engine, SGDK 2.11 provides `SPR_loadAllFrames()` and
manual VRAM indices; its returned table must be freed. Leaving auto tile upload
active on every instance defeats a manually owned shared cache. Verify flags,
frame callbacks, frame geometry and allocation failures against the selected SDK.
See the [versioned Sprite API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h).

The Lab instead uses ResComp SPRITE definitions as an offline frame format and
checks that every frame is a single piece at offset zero. It preloads those
TileSets once with `VDP_loadTileSet()` while display is disabled. The return is
a success flag, not a tile count. Tile counts come from the resource metadata.
All 24 animation tiles are shared by every rendered instance.

For this measured batch, `VDP_setSpriteFull(index, x, y, size, attributes, link)`
builds SGDK's persistent SAT cache and `VDP_updateSprites(count, DMA_QUEUE)`
queues its transfer. Entry zero is the lantern; every following link advances
through the accepted effects and the last link is zero. Submitting a shorter
chain removes previous effects without clearing every stale entry. Passing zero
to `VDP_updateSprites` hides the table. Source: [SGDK 2.11 VDP sprite implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp_spr.c).

This adapter owns the entire SAT. Do not combine it with an independent
`SPR_update()` call. A real mixed renderer needs one shared table compositor
or a single Sprite-engine owner. Keep the queued cache unchanged until VBlank;
the Lab processes it before building the next frame. Use copied transfers if
that lifetime cannot be guaranteed, accounting for queue-buffer space.

## Profile the sustained workload

Separate update, selection, sprite building, HUD and transfer costs. Long-lived
snow and a saturated pool exposed costs hidden by a short rain test in this Lab.
Snow alone reaching 60 FPS was not enough: adding bursts exceeded the budget.
Profile their combined peak, including allocation/replacement on the spawn frame,
not just steady particle movement or one priority class.
Avoid modulo/division inside every slot traversal when a bounded wrap suffices.
Give long-lived weather its own measured allowance so it cannot
occupy the whole pool. Skip empty priority classes, cache invariant frame data and avoid formatting and
uploading unchanged HUD rows. Inspect actual M68000 output before choosing a
more complicated optimization; do not globally disable language safeguards.

CI compares the optimized selector with the simple reference and reports target
instruction counts for identical inputs. It also measures maximum simulation
and selection work while moving through maximum snow and repeatedly bursting.
The optional `--stress` ROM repeats this workload with FPS and a missed-frame
counter; use it for sustained release-mode emulator checks. Instructions are a work proxy, not
cycle-accurate timing or a frame-rate certificate. Measure the release ROM with
its ordinary HUD and compare diagnostic overlays separately.

For widespread rain/fog, consider an animated/scrolled tile plane instead of
hundreds of sprites, if a plane, palette and transfer budget are available.
That trades resources; it is not free transparency. Large smoke can be a shared
animation, fewer particles or background art. Follow the game’s visual design.
