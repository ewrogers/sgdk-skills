# SGDK rendering, art and budgets

Verified against SGDK **2.11**, ResComp **3.95** and GCC **16.2.0** in the pulled
`ewrogers/sgdk:latest`. Recheck headers after an image update.

## Map an order to the actual sprite engine

`SPR_setDepth(sprite, value)` places **lower** values in front of higher values,
and immediately updates the sprite list when the depth changes. The Lab's
ordering array is far-to-near, so it assigns `-(s16)rank`, not `rank`.
It leaves those depths unchanged when ranks remain unchanged. `SPR_setZ()` is
an alias for sprite depth; it is **not** the actor's physical map elevation.

Sources: [sprite API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h),
[depth sorting implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sprite_eng.c).

Hardware priority bits choose relationships with background planes; they do not
provide arbitrary per-object Z levels. The Lab uses ordinary-priority world
sprites and a high-priority BG_A HUD. BG_B holds the static ground image and
scrolls by the camera, while BG_A stays fixed. Four palette lines belong to
world, scenery, hero and UI. Each resource uses one line; there is no HBlank
palette switching or attempt to get more colors by changing CRAM mid-frame.

Sources: [background API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_bg.h),
[sprite/VDP attributes](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_spr.h),
[palette API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/pal.h).

## Native art with consistent anchors

The original Python rasterizer draws 2:1 diamonds, step prisms and pixel clusters
directly at the working resolution. It does not downscale a larger painting.
All exported RGB values lie on the Mega Drive three-bit component grid. Sprite
index zero is transparent; opaque HUD font backgrounds use index one so world
art does not show through glyph tiles. The font is original 5×7 pixel lettering
inside 8×8 tiles, generated as typed `u32` data for `VDP_loadFontData`.

The hero has 16 poses in a 384×32 sheet. Other sprites have matching foot anchors
across their frames. ResComp dimensions are in 8-pixel tiles, for example:

```text
SPRITE hero "hero.png" 3 4 NONE 0 NONE NONE
IMAGE ground "ground.png" NONE ALL
```

These resources disable automatic sprite animation and automatic collision
metadata. `SPR_setFrame` selects the final simulation pose; the fountain has
three timed frames. A 32×64 tower is two hardware pieces, although it remains
one logical sortable Sprite. Tile cutting, transparent canvas size and sprite
animation state are all rendering concerns, not world movement.

Source: [ResComp syntax](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt).

## Budget the actual scene

The compiled Lab has 269 ground tiles and 278 tiles of maximum per-instance
sprite allocations, including the hidden debug marker. `SPR_initEx(320)` reserves
320 sprite tiles. The adapter checks background/static tiles against the actual
`TILE_SPRITE_INDEX`, keeping them away from sprite and font storage. The 17
logical sprites produce more hardware pieces because tall art is split vertically.
A large transparent canvas still counts against scanline rendering.

Tests combine compiled resource sizes with the authored placement. They count
all static rectangles on every line, then conservatively add the hero and debug
marker on every line, including transparent pixels and without off-screen credit.
This checks H40's 20-piece/320-pixel scanline limits for this layout; it is not a
promise for arbitrary additions, larger bosses or different camera/content sets.

The HUD retains six 40-word rows (480 bytes) until DMA completes. Updates queue
for VBlank under a 4,096-byte allowance. Sprite frames/SAT and other transfers
share that allowance. Initial art, palettes and the font load with display off;
four bounded sprite warm-up frames allow initial frame uploads before showing
play. Runtime palette writes and full background reloads are unnecessary.

The quiet original FM theme is generated as VGM and compiled to XGM2. The adapter
uses `XGM2_play`, `XGM2_pause` and `XGM2_resume`; it does not write directly over
the driver's FM/PSG ownership. Art, theme and tuning are only this Lab's creative
choices.

Sources: [DMA units/lifetimes](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h),
[tile API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_tile.h),
[XGM2](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/xgm2.h).

## Validation boundaries

Host O0/O3 contracts cover projection, inverse range, height ordering, stable IDs,
full capacity, support overrides, cycles, invalid input, stairs, underpasses,
collision, camera bounds and pause. A 20,000-step movement soak keeps surfaces
and ordering valid; a 1,000-step route compares full and cached order. Undefined
behavior sanitization runs where supported.

The base-M68000 probe executes the contracts with smaller route/soak counts and
a 128-step host/target trace digest. It also reads compiled resource geometry,
palettes, font words and authored placements. Both container architectures build
release/debug plus PAL/NTSC-region ROMs and compare reports/hashes. These are
logic/resource checks, not VDP/audio timing emulation or real-hardware testing.
Keep actual emulator movement, occlusion, sound and frame-rate observations
separate from what compilation and geometry tests prove.
