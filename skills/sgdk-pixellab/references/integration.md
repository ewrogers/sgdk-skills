# SGDK integration and evidence

Pull `ewrogers/sgdk:latest`, inspect the selected SDK version and read its
`bin/rescomp.txt` before writing unfamiliar resources. The study's integration
checks use SGDK 2.11. Original PNG pixels, palette exports and dimensions are
separate from the generated resource layout.

- A VDP tile is 8×8 at four bits per pixel: 32 bytes of patterns. Authored
  16×16 or 32×32 terrain cells contain multiple hardware tiles.
- A logical sprite can need several hardware pieces. Inspect actual
  `SpriteDefinition` animations, `AnimationFrame` pieces and tile counts.
- Frame order, native canvas and anchors must agree with the `.res` dimensions.
  Our five-frame robot sheet is 160×32 and declares `4 4` tiles per frame.
- Reserve VRAM, palette lines and DMA alongside existing scene art. H40's
  80 sprites/frame, 20/scanline and 320 sprite pixels/scanline are independent
  constraints. Successful resource compilation does not prove a busy scene fits.
- Inspect converted sprites on their real backdrops. Readability, repeat seams,
  scrolling shimmer, outline quality and motion need visual review.

## Tilesets and maps

Keep the provider's metadata beside the raw sheet. In the tested top-down
export, each tile's `bounding_box` locates its pixels, and its `corners`
(NW/NE/SW/SE) identify a compatible terrain configuration. `wang_N` and
`original_position` are not reliable sheet coordinates. Validate every required
configuration and the atlas bounds before painting. No matching tile should
produce an authoring error rather than a silent black square.

Our first PixelLab map reported four missing shapes at path/empty boundaries.
We changed the path to axis-aligned rectangles surrounded by grass and verified
that the warnings disappeared. The study's final 320×192 map is reconstructed
at native size from those corner descriptors and actual atlas rectangles.
This demonstrates art placement; it is not a Tiled importer, gameplay collision
map or claim that PixelLab's editor data is an SGDK runtime format. Add collision,
objects and streaming through the game's chosen level pipeline.

## Executable checks

Repository host tests cover native dimensions/frame contracts, all PNG filters,
transparency versus opaque black, palette mapping against an independent
nearest-distance oracle, deterministic ordered dithering, frame phase, malformed
PNG inputs, no-overwrite behavior and paths containing spaces. Committed final
assets reproduce offline. The native chest crop and five-frame sheet are
compared pixel by pixel with their source regions/frames.

Opt-in Docker tests compile representative native 8×8 tiles, 16×16 icons,
32×16 props, a five-frame 32×32 actor and large images in release/debug. A
Unicorn 2.1.4 base-M68000 probe checks actual ResComp tile nibbles, linked palette
words, frame geometry and resource counts. ARM64/AMD64 CI compares hashes and
budgets. The harness bypasses console startup and does not emulate VDP timing,
rendering or Z80 audio. No paid service calls run in CI.

The gallery is a visual review artifact, not an emulator playtest. The study
makes no claim that every returned candidate is usable; rejected pose, layout
and wide-reference results remain visible to explain the selection.

## Primary sources

- [PixelLab MCP docs](https://api.pixellab.ai/mcp/docs): current tools, sizes,
  palette/reference controls, asynchronous jobs and generation costs.
- [Lospec Palette List](https://lospec.com/palette-list): artist/hardware
  palettes, attribution and download formats.
- [SGDK ResComp manual](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/bin/rescomp.txt): resource syntax and indexed inputs.
- [SGDK ImageUtil](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/tools/commons/src/sgdk/tool/ImageUtil.java): `convertRGBA8888toRGBA4444` and palette masks.
- [SGDK palette API](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/inc/pal.h): VDP color fields and runtime conversions.
- [Plutiedev sprites](https://plutiedev.com/sprites): arrangement and scanline limits.
