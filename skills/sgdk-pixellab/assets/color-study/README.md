# Preserve the art, plan the scene

This second study follows the artist’s feedback on contrast, sharpness and
specific colors. It used **76 of the additional 100 generations**. The complete
[requests and receipts](requests.json), [reviews](reviews.json) and
[local comparison gallery](gallery.html) are retained.

The first gallery mixed color conversions with different design candidates.
That was misleading: the favorite forest background and one robot had zero RGB
changes in their own prepared exports. The corrected first gallery labels
alternatives separately. This gallery always compares the same source asset.

## What worked

- Plan related actors, objects and UI together. The new themed examples keep
  their generated RGB colors unchanged; UI palette entry order changes only.
- Let large backgrounds propose their own material ramps rather than forcing
  them through an unrelated actor palette. Two lines can preserve more colors
  than one, with each native 8×8 tile choosing a complete line.
- Keep bright accents, highlight endpoints and deep outlines. A compliant
  palette can be vibrant. A color-count target alone does not produce good art.
- Near-color conversions or swaps are acceptable when they preserve the brief.
  Explicitly requested colors are constraints: retain them, or explain any
  hardware conversion and get the artist’s decision. Never silently restyle art.
- Dithering is optional. The first study compares none, 2×2 and 4×4; the latter
  can be too grainy. These new conversions use no added ordered dithering.

## What still needs an artist’s judgment

The floor request became a raised block. City buildings came back angled rather
than strictly top-down. Hex tiles are independent variants, not transitions.
The connected meadow sheet retains its original adjacency metadata, but still
needs a map seam review. Still portraits/actors do not prove animation quality.
Native dimensions and valid palettes cannot certify any of those things.

Pro object calls returned 64 options each. Candidate 0 is shown explicitly,
with every returned option preserved in native contact sheets. A different
candidate is an art choice, not a palette conversion.

## Four lines, per visible scene

[Scene plans](scene-palettes.json) allocate background lines 0/1, actors/items
line 2, and UI line 3 for each themed page. A level change can load another set.
There is no requirement to use one universal 16-color palette for the game.
These examples do not use HBlank palette changes.

The banked exporter requires opaque input and a common index-zero backdrop.
Hex source corners were explicitly composited onto black for this demonstration;
the raw transparent atlas remains unchanged. Unused PNG palette lines are not
permission to overwrite palettes owned by actors or UI at runtime.

## Reproduce offline

From this directory, with Python 3.10+:

```sh
python rebuild.py --check
python build_gallery.py
python build_rom.py --output /path/to/new-color-showcase --platform linux/arm64
```

The ROM builder uses `ewrogers/sgdk:latest`; pull it first. Use `linux/amd64`
for that architecture. The destination must be new. The four-page showcase
loads each scene’s palettes while display is blanked; A/Start advances and B
returns. It displays backgrounds, actors, objects and UI together using normal
VDP rendering. It is a resource demonstration, not a gameplay or performance
benchmark. Actual visual approval still requires emulator/hardware review.

`rebuild.py` intentionally rewrites derived exports unless `--check` is used;
source images and plans are never modified. It makes no paid or network calls.
