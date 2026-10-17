# Verified SGDK presentation contracts

Checked with SGDK 2.11, GCC 16.2.0 and ResComp 3.95 from
`ewrogers/sgdk:latest`. Pull before building and inspect that image's headers
when an API or output changes. The Lab is ordinary application C, not an SGDK
built-in combat system.

- `SPR_initEx(u16 vramSize)` reserves sprite pattern tiles.
  `SPR_addSprite(...)` can fail; the renderer checks all allocations.
- `SPR_setAutoAnimation(sprite, FALSE)` establishes manual playback. The model
  selects action poses with `SPR_setAnim` / `SPR_setFrame`; automatic playback
  does not advance the same action independently.
- Lower `SPR_setDepth` values render in front. The Lab uses negative foot Y for
  top-down ordering and places feedback above actors after capacity checks.
- `SPRITE` resources use tile-sized frame dimensions: fighters 3x4, bolts 1x1,
  sparks 2x2, swords 3x3. Rows encode animations. Compiled definitions, frame counts and
  palette words are checked rather than assumed from PNG filenames.
- `SYS_doVBlankProcess()` services normal frame/input/DMA work. Read cached
  `JOY_readJoypad(JOY_1/JOY_2)` once per display here; do not poll separately
  for each collision pair. Seed button history after input setup.
- `DMA_queueDma` source buffers must remain valid until service completes.
  The 320-byte HUD buffer is retained and drained before rewriting.
  `DMA_canQueue` lengths are words; the transfer-size setting is bytes.
- A full-screen Window replaces plane A. Transparent Window cells expose B,
  so title/world art lives on B and text panels use opaque Window cells.
- `XGM2_playPCMEx` takes sample bytes, byte length, channel, priority, half-rate
  and loop. One finite semantic cue is selected per display, with important
  impacts/blocks/defeat ahead of swing noise. No PCM polling loop runs per hit.

Primary references:

- [SGDK 2.11 sprite engine](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h)
- [ResComp resource formats](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt)
- [Controller API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/joy.h)
- [Frame services](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h)
- [DMA](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h)
- [XGM2](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/xgm2.h)

## Budgets and evidence

H40 hardware permits 80 sprite pieces overall, 20 on a scanline and 320 sprite
pixels on a scanline. A logical SGDK sprite can contain multiple pieces; actual
compiled Lab frames each contain one. The maximum core set is six 24-pixel
actors, six 24-pixel swords and four 8-pixel bolts: 16 pieces / 320 pixels if all
share one line.
Six 16-pixel effects are admitted afterward against remaining line space.
The model continues to resolve combat if a cosmetic effect cannot be shown.

The example reserves 160 tiles for 154 live sprite tiles, uses four role-based
palettes and bounded CPU-side actors/shots/effects. These capacities are game
choices. The 4-palette limit and line limits are hardware properties. Measure
CPU, DMA/upload cost, main RAM and VRAM separately; a low logical actor count is
not a performance measurement.

Host tests cover C behavior and copied asset generation. Native ARM64/AMD64
container builds check actual SDK compatibility, region headers and resources.
The base-M68000 execution probe bypasses startup and does not emulate VDP, Z80,
input protocols or sound. Actual PAL/NTSC presentation, dense combat and real
controller/hardware behavior remain playtest checks.
