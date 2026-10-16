# Palette planning and native validation

## Follow the artist's palette

Inspect the project's source art, palette files and scene allocation first.
Ask when the intended palette is unclear; do not replace an existing one just
because a familiar palette looks attractive. If none exists, offer a small
selection from [Lospec](https://lospec.com/palette-list) and explain why their
ramps fit the requested mood and readability. Lospec provides PNG/HEX and other
exports; retain the palette name, author and source link when using one. Its
list includes both hardware palettes and artist-created palettes, so do not
assume a “16-color” label means Mega Drive-ready.

Plan four VDP palette lines for the whole visible scene, not four per asset.
Each line has sixteen indices. Sprite index zero is transparent, leaving
fifteen opaque indices. Background index zero is transparent to lower layers;
the selected backdrop color may show through. An opaque PNG background is not
a new hardware opacity mode. Multi-palette background art needs per-8×8-tile
palette assignment and corresponding tilemap attributes; the helper here
intentionally exports one palette line per asset.

Reduce a proposed palette to the intended RGB333 values, inspect the changed
ramps and flag merged shades. Prefer distinct values for outlines, body colors
and highlights instead of spending many entries on near-identical shades.
The helper accepts conventional full-range previews of the eight channel steps:
`0,36,73,109,146,182,219,255`. These are authoring previews, not a prediction of
the console's analog brightness curve. The tested ResComp masks high channel
bits when building its VDP words. Verify the actual linked palette rather than
assuming a runtime RGB-conversion macro uses identical rounding.

A forced palette PNG is a set of RGB colors, not a declaration of transparent
index zero. Prefer feeding just the intended opaque sprite colors to a generator.
If opaque black is required, reserve it at an opaque index too. Never remove
all black pixels as a transparency key. Our raw forced-palette robot included
opaque black; conversion without an opaque black slot changed those pixels.
A later fifteen-opaque-color generation avoided that ambiguity.

## Inspect, prepare, validate

The [helper](../scripts/art_pipeline.py) needs Python 3.10+ with its standard
library. It accepts non-interlaced, 8-bit RGB/RGBA/indexed PNGs. Other bit depths,
grayscale and interlaced inputs need an explicit lossless PNG export first.
It checks CRCs and bounded decompression; it is not a general image editor.

Copy a [study contract](../assets/prompt-study/contracts/robot-profile.json)
and edit the dimensions, kind and all sixteen palette entries. Optional
`frame_width` and `frame_height` divide a sheet into equal native frames.
Export frame dimensions must be multiples of eight. Deliberately crop empty
canvas or pad it first when needed; this helper never resizes or guesses padding.

From the installed skill folder:

```sh
python scripts/art_pipeline.py inspect /path/to/source.png /path/to/contract.json --report /path/to/new-report.json
python scripts/art_pipeline.py prepare /path/to/source.png /path/to/contract.json --output /path/to/new-indexed.png
python scripts/art_pipeline.py validate /path/to/new-indexed.png /path/to/contract.json
```

`inspect` reports deviations without rejecting off-palette art. `prepare`
requires a new output file and maps colors onto the declared palette.
`validate` rejects wrong dimensions, off-palette pixels, partial alpha and
transparent pixels in a declared opaque background. It validates decoded pixels,
not on-disk index ordering: use the prepared indexed output with ResComp.
All commands reject incorrect native dimensions. Existing outputs/reports are
not overwritten. A later I/O failure can leave a partial new export; sources
are never modified. Keep export destinations separate from source artwork.

For sprites, transparent pixels map to index zero and opaque pixels only to
indices 1–15, even when their RGB equals index zero's RGB. Partial alpha needs
an explicit `alpha_threshold` decision during preparation; a VDP sprite has no
per-pixel translucency. Preview that decision, especially around outlines.

Reports include dimensions, palette words, opaque/off-palette colors, partial
alpha, frame bounds, edge contact, isolated pixels, seam edge differences and
unique native 8×8 patterns. Counts are hints: intentional sparkles are isolated,
a valid ground tile touches its edges, and identical outer columns do not prove
a seamless repeated texture. The pattern count ignores flip reuse and includes
transparent patterns. ResComp may optimize these differently; use its linked
resources for real allocation.

## Optional dithering

Compare undithered, 2×2 and 4×4 reductions when broad gradients need blending.
The helper's `dither` contract field accepts `none` (default), `bayer2` or
`bayer4`. It projects onto the two nearest palette colors and uses a fixed
ordered threshold. This simple deterministic algorithm is not PixelLab's
implementation or perceptual color management. Exact palette colors and alpha
remain unchanged; the pattern phase repeats at each frame's origin.

Prefer selective authored transition bands when whole-image dithering makes
flat areas or outlines noisy. It may be best to use no dithering. Review at
actual display size, during scrolling and with the chosen emulator filters.
Increasing pattern variety can increase tile storage and transfers even when
the palette is unchanged. The study compares the visual result and native
pattern counts; it does not prescribe its preferred treatment for other games.
