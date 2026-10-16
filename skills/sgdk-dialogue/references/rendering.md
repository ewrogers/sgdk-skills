# Panels, text and portraits on the VDP

## Choose ownership first

A 320×224 screen has 40×28 visible 8×8 cells. Tile coordinates are not pixels.
The Lab uses a stationary BG_B harbor and high-priority BG_A UI with two
optional conversation panes. Window is disabled: Window replaces part of
plane A, it is not a third independent background. In a scrolling game, choose
a fixed Window region, a dedicated plane, or coordinated scroll rows before
placing dialogue. Top and bottom retained panes do not create extra planes.

The [renderer](../assets/dialogue-lab/src/view.c) is an example adapter. Keep
its tile ranges, palette roles, sprite owner and queue scheduling explicit when
integrating it with a game renderer. It uses low-level VDP sprite entries and
does not initialize the high-level sprite engine. Do not let both systems
write the same sprite table.

## Nine-slice panels and inline styles

The panel sheet is nine native 8×8 tiles in row-major order: corners, edges,
center. Corners stay fixed and edges/center repeat in whole cells. They are not
scaled. `panel_fits` checks visible-screen bounds and a two-cell minimum before
writing; `panel_piece` returns the corresponding tile. Nameplates and choice
boxes use the same helper. Opaque panel/text backgrounds share a color index,
avoiding holes through letters or mismatched rectangles.

The Lab reserves six 96-glyph banks: two faces × three ink colors = 576 tiles,
or 18,432 bytes of VRAM. A character's style selects a tile bank. All six use
PAL1, leaving other palette lines available. This trades VRAM for inline color
and font changes without per-character palette uploads. A game with less text
styling can reserve fewer banks or use per-tile palette attributes where its
palette plan allows. A palette attribute applies to the whole tile, not a
substring of a proportional glyph.

Prepare fonts at native resolution and retain the expected glyph order. The
Lab maps printable ASCII slots and substitutes `?` for unexpected display
bytes. It has no proportional shaping or dynamic font cache.

## Portraits and transfer lifetime

Each portrait has five 32×32 frames, already indexed to a shared palette with
index zero transparent. ResComp may split an image into several hardware
sprites and omit empty tiles. Use the linked frame's piece offsets, size and
`numTile`; do not assume every frame is a single rectangle or calculate tile
addresses from nominal width alone. Talking and idle frames use the paused
conversation clock. Two retained panes can display different speakers at once.

The Lab preloads every portrait frame and changes sprite tile references at
runtime. It reserves at most eight hardware entries across two panes, with
actual counts coming from linked resources. Larger portraits or more speakers
need their own RAM/VRAM/scanline and transfer budgets. The H40 limits of 80
sprites/frame, 20/scanline and 320 sprite pixels/scanline still apply alongside
the rest of the game. A successful allocation does not guarantee visibility
under scanline pressure.

A 40×28 cell canvas and previous image consume 4,480 bytes of main RAM. Only
changed rows are queued, using `DMA_QUEUE_COPY` so subsequent canvas edits
cannot change a pending transfer. Portrait sprite cache storage stays valid
until the next VBlank consumes it, before the foreground loop changes it.
Drain queued transfers before leaving the scene or freeing their sources.
Compose on content/reveal/indicator changes rather than formatting and sending
all text every display frame. Queue/buffer exhaustion still needs a policy
when sharing this adapter with a busier game.

## Measured example budget

The SGDK 2.11 ARM64/AMD64 probes agree on an exclusive tile end of 761 from
`TILE_USER_INDEX` 16: 745 resident tiles, 23,840 bytes. That includes 576 font,
nine panel, four indicator, 16 world and 140 actual portrait-frame tiles.
It excludes SGDK's system tiles, default font and tilemap/sprite/scroll tables.
The adapter checks its end against the selected SDK's `TILE_FONT_INDEX`.
Changing the VDP table layout changes the available tile space.

On the actual M68000 build, `Dialogue` is 582 bytes and `DialogueLab` is 596
bytes, including the two retained text panes. Add the 4,480-byte rendering
caches, renderer state, SDK buffers, stack and audio driver costs. These are
example costs, not a universal per-dialogue overhead. Both normal release and
debug Lab ROMs currently pad to 128 KiB. No CPU percentage is inferred from
these sizes or from the target instruction tests.

## Verified SDK sources

Checked against SGDK 2.11, commit `ef9292c03fe33a2f8af3a2589ab856a53dcef35c`:

- [Tile loading and row transfers](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/inc/vdp_tile.h): `VDP_loadTileSet`, `VDP_setTileMapDataRowEx`.
- [Sprite table API](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/inc/vdp_spr.h): `VDP_setSpriteFull`, `VDP_setSpriteLink`, `VDP_updateSprites`.
- [Tile space and VDP layout](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/inc/vdp.h) and [startup defaults](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/src/vdp.c).
- [ResComp manual](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/bin/rescomp.txt): `TILESET`, `SPRITE`, `PALETTE`, `XGM2`, `WAV`.
- [XGM2 API](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/inc/snd/xgm2.h): track looping, pause/resume and finite PCM cues.
- [VDP sprites on Plutiedev](https://plutiedev.com/sprites): hardware arrangement and limits.
