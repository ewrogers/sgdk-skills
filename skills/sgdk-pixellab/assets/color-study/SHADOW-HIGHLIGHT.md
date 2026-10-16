# Same palettes, extra shades

This optional experiment compares the same source art and two unchanged
background palette lines, with and without hardware shadow/highlight.
[Open the comparison gallery](shadow-gallery.html).

The four themes gain useful shading without another PixelLab generation or a
new CRAM line. The artist confirmed the toggled comparison looks impressive in
ares. Our captured Colonial frame also showed the effect; automated input/capture
was unreliable afterward. This is an emulator observation, not a hardware test.

## What the export contains

- `normal.png` and `lit.png`: ordinary indexed background images. Each 8×8 tile
  uses one of the same two palette lines; pixel indices may differ between versions.
- `mask-0.png` and `mask-1.png`: two native 128×128 sprite masks. Transparent
  pixels leave brightness alone; indices 14/15 request highlight/shadow when
  rendered with sprite palette 3. These are control indices, not painted colors.
- `normal-preview.png` and `lit-preview.png`: modeled final RGB appearance using
  ares’s published color lookup. A regular image viewer cannot interpret the
  base image plus lighting mask as a VDP would.
- `report.json`: palette choices per tile, brightness counts and model error.
  RGB error is useful for comparing these encodings, not an artistic rating.

Both versions choose colors against the same ares lookup, from the same base
palettes. The lit version additionally chooses shadow/normal/highlight per
pixel. Original source pixels, dimensions and palette plans remain untouched.
No extra dithering is applied.

| Theme | Normal shades | With S/H | Model RGB error reduction |
| --- | ---: | ---: | ---: |
| Modern | 24 | 54 | 51.36% |
| Fantasy | 22 | 48 | 63.04% |
| Sci-fi | 17 | 26 | 54.73% |
| Colonial | 22 | 47 | 61.66% |

These are shades actually used in these images, not a new hardware palette
limit or a promise that every image improves this much.

## Rendering contract and cost

[The ROM source](shadow-rom/src/main.c) enables `VDP_setHilightShadow(TRUE)`
for the lit view. The background has high priority, so unmasked pixels remain
normal. High-priority mask sprites on PAL3 use color 14 to brighten the
underlying background and color 15 to darken it. Actor/item sprites precede
the mask in the sprite table, keeping them in front and normally colored.
Palette sets and tile data change only with display blanked; no raster ISR or
HBlank CRAM writes are used.

Backgrounds own PAL0/1, actors/items PAL2. PAL3 indices 14/15 are reserved for
sprite operators. Ordinary sprite artwork must not use them for visible colors.
Background text can still use PAL3 index 15 normally; the special meaning applies
to sprite pixels. The previous gallery’s PAL3 sprite panel is omitted here.

The full mask consumes at most 32 hardware sprite pieces, eight pieces and 256
pixels on a covered scanline, and 512 pattern tiles (16 KiB of VRAM). ResComp
trims some transparent areas; the ROM displays actual linked piece/tile counts.
This is substantial headroom on H40’s 80-sprite total, 20-sprite scanline and
320-sprite-pixel scanline limits. The two small foreground examples still fit,
but that does not prove a projectile-heavy scene would. Transparent portions
of a sprite do not make its reserved scanline width free.

Treat a full-screen mask as a demonstration, not a general default. Selective
masks over a small painting, title illustration or light source can be cheaper.
Scroll them with the art they affect and preserve anchors, priority and clipping.
Tile-priority-based shading without a mask is another approach, with coarser
brightness control; this helper does not implement it. Masks affect what is
underneath them, so overlapping objects need explicit testing.

## Reproduce and try it

From this directory, with Python 3.10+ and the latest Docker image pulled:

```sh
python rebuild_shadow_study.py --check
python build_shadow_gallery.py
python build_shadow_rom.py --output /path/to/new-lighting-demo --platform linux/arm64
```

Use `linux/amd64` for that architecture. The destination must be new. J/A or
Enter/Start toggles the effect; K/B advances the theme; L/C goes back.

The [experimental converter](../../scripts/shadow_highlight.py) is deliberately
limited to an opaque 256×128 background with planned slots 0/1 for this showcase.
It exports a package that needs the matching runtime renderer, not a drop-in
replacement for an ordinary PNG. Its generic pixel assignment is tested against
an independent cost oracle. Real release/debug ResComp checks decode samples
from every compiled mask, verify matching palettes and enforce piece bounds;
normal ROM and probe hashes match across ARM64 and AMD64.

The preview lookup and hardware composition were checked against
[ares color.cpp](https://github.com/ares-emulator/ares/blob/9408cb43d4948fc3ea6e152a307a34348df3fe04/ares/md/vdp/color.cpp)
and [ares dac.cpp](https://github.com/ares-emulator/ares/blob/9408cb43d4948fc3ea6e152a307a34348df3fe04/ares/md/vdp/dac.cpp).
See [Plutiedev](https://plutiedev.com/shadow-highlight) for the hardware rules.
Analog displays and other emulator color models can differ from these previews.
