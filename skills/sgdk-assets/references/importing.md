# Importing assets

This reference targets SGDK 2.11. The container's installed files are the authority for the project you are changing.

## Choose the resource

| Use | Resource | Important distinction |
| --- | --- | --- |
| Moving or animated object | `SPRITE` | Produces `SpriteDefinition`; the sprite engine handles its hardware pieces. |
| Small static background or picture | `IMAGE` | Contains a palette, tileset and tilemap. It is not a hardware sprite. |
| Shared pattern graphics | `TILESET` | Tile pixels; `TILEMAP` or `MAP` can refer to the same tileset. |
| Small tile layout | `TILEMAP` | Raw tilemap data, distinct from tile pixels. |
| Large scrolling level | `MAP` | SGDK map structures and streaming workflow; inspect `inc/map.h` and the matching sample. |
| Palette | `PALETTE` | Converts a palette file or image to a `Palette`. Loading it into a CRAM slot is a runtime decision. |
| Font | Inspect the text/font APIs | A font is tile graphics plus the expected character layout, not arbitrary Unicode text. |
| Music / sampled sound | `XGM`, `XGM2`, `WAV` | Match the converter and playback driver; read the installed manual for formats. |

`BITMAP` targets SGDK's bitmap engine. It is not interchangeable with tiled background resources.

## Minimal declarations

```text
PALETTE sprite_colors "gfx/marble.png"
SPRITE marble "gfx/marble.png" 2 2 NONE 8
IMAGE backdrop "gfx/backdrop.png" NONE
```

`2 2` means a **16 by 16 pixel frame**, because SPRITE dimensions are in 8 by 8 tiles. Each row of the image is an animation; cells across it are frames. `8` is the documented frame duration in units of 1/60 second. A duration of `0` disables automatic animation.

Use the installed manual when changing compression, collision data or cutting options. In SGDK 2.11, sprite frames can be composed of up to 16 hardware sprites. Explicit cutting optimization options can also produce an `_opt.png` beside the input. Run such experiments on a copy and inspect the result.

`NONE` keeps this example easy to inspect. The manual recommends `FAST`/`LZ4W` for streamed sprites and warns against slow `APLIB` decompression there. Choose after measuring ROM size and upload/decompression cost.

## Prepare the art

For a straightforward first import, export indexed PNG with a deliberately ordered palette and index 0 as transparent. Keep the editable source, such as an Aseprite file, alongside the exported PNG or in the project's source-art folder.

Index transparency matters even if a paint application previews alpha differently. Do not assume alpha blending exists in the VDP. For RGB input, ResComp expects its special palette strip layout; read the image-format section instead of guessing or converting the file blindly.

Check these before changing a palette:

- Are multiple sprites or backgrounds sharing it?
- Does each 8 by 8 background tile use one palette bank?
- Is the same color stored at the expected index throughout the frames?
- Are transparent pixels using index 0?
- Are colors quantized consistently to the Mega Drive's 3 bits per channel?

Keep palette sharing explicit. Two images having similar colors does not mean their pixel indices are compatible. Conversely, a PNG with a 256-entry palette and only low indices used is not automatically an over-budget image.

## Know which budget is involved

| Kind | Limit or cost | What to account for |
| --- | --- | --- |
| Hardware | 64 KiB VRAM | Pattern graphics, plane tables, sprite table and other VDP allocations share it. |
| Hardware | Four 16-entry palette banks | A tile or hardware sprite selects one bank, with 15 opaque indices and transparent index 0. The backdrop color is selected separately. |
| Hardware | Up to 2048 standard 8 by 8 pattern addresses | Each 4 bpp pattern is 32 bytes. This is the theoretical 64 KiB capacity, before reserving tables and other data. |
| Hardware, H40 / 320-wide | 80 hardware sprites total, 20 per scanline, 320 sprite pixels per scanline | Count hardware pieces and their scanline coverage, not just game objects. Transparent areas can still consume sprite processing budget. |
| Hardware, H32 / 256-wide | 64 hardware sprites total, 16 per scanline, 256 sprite pixels per scanline | The mode changes the limits. H40 figures are not universal. |
| SGDK | Sprite engine allocations and resource layout | Read the configured engine and VDP layout; not all 64 KiB is available to your art. Check `SPR_addSprite` for `NULL`. |
| Game policy | Chosen upload, palette and object budgets | State them as project choices. Test on crowded frames instead of promising every composition fits. |

ROM data, decoded RAM, VRAM residency and per-frame DMA traffic are different totals. Do not add compressed file sizes and call the result VRAM use. Palette colors may change over time; raster effects and shadow/highlight have additional behavior beyond this basic budget.

### Tile count is about what is loaded

In the usual non-interlaced mode, a tile pattern is 8 by 8 pixels at 4 bits per pixel. That takes 32 bytes, so 64 KiB of VRAM has **2048 possible pattern addresses, numbered 0 through 2047**. Backgrounds and sprites share that memory.

You cannot use all 2048 for artwork. Plane tilemaps, the sprite attribute table and the horizontal scroll table need VRAM too. SGDK also reserves system tiles, fonts and, when initialized, a sprite graphics region. Check the current layout in `inc/vdp.h`: `TILE_USER_INDEX`, `TILE_USER_LENGTH` and `TILE_USER_MAX_INDEX` describe the range available to user-managed graphics. Changing plane sizes or sprite reservations changes that range. Do not turn a rough percentage into a fixed project limit.

A large spritesheet or level can contain far more tiles in ROM than fit in VRAM at once. What matters is which patterns are resident together, how they are shared and how much replacement data is uploaded each frame. A 16 by 16 frame covers four tile cells; its full sheet dimensions do not describe its current VRAM use. ResComp's deduplication, sprite cutting and the runtime loading strategy affect the actual count.

### Keep uploads inside the allocated range

Tile attributes have an 11-bit pattern index, so valid indices are 0 through 2047. Also validate that the entire upload fits the **part of VRAM you own**, using a wide type to calculate its end. An index that fits 11 bits can still point into a table, font or another object's graphics.

SGDK's `TILE_ATTR_FULL` combines the fields without checking the index; an out-of-range value can alter attribute bits. `VDP_loadTileData` forwards the transfer at `index * 32` without checking your allocation. A write beyond 64 KiB can wrap around VRAM and overwrite earlier graphics or tables. Do not rely on these low-level calls to report a capacity failure. Prevent the overlap before submitting a CPU or DMA transfer.

[Plutiedev's tiles and palettes guide](https://plutiedev.com/tiles-and-palettes#loading-tiles) explains the pattern addressing and shared VRAM budget. Its assembly examples are useful for understanding the hardware; use SGDK's verified loading APIs in an SGDK project.

## Diagnose the first failure

| Symptom | First checks |
| --- | --- |
| Resource file or image not found | Path is relative to the `.res` file; Linux paths are case sensitive. Quote filenames with spaces. |
| Bad dimensions or frame layout | Image dimensions are multiples of 8 and divisible by the chosen frame size. Frame dimensions in `SPRITE` are tiles, not pixels. |
| Wrong colors or opaque boxes | Indexed pixel values, transparent index 0, loaded palette bank and tile attributes agree. |
| Missing sprite | Allocation returned `NULL`, visibility, position, VRAM allocation and hardware/scanline limits. |
| Wrong animation order | Rows define animations; columns define frames. Inspect blank trailing frames and duplicate-frame options. |
| Frame spikes | Resource upload or decompression frequency, DMA load and sprite cutting; measure before rewriting in assembly. |

For diagnosis without a full ROM build, run ResComp in a writable scratch copy:

```text
docker run --rm --mount "type=bind,source=PROJECT,target=/workspace" IMAGE java -jar /opt/sgdk/bin/rescomp.jar res/resources.res out/resources.s
```

Create `out` first. Conversion may create intermediate files, so do not treat it as a read-only validator. A full build additionally catches generated symbol/type mistakes and link failures.

## Sources and verification

- [SGDK 2.11 ResComp manual](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt): resource syntax, indexed/RGB images, frame layout, compression and cutting limits.
- [SGDK sprite engine header](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h): allocation, animation and upload behavior.
- [SGDK VDP sprite header](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_spr.h) and [palette header](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/pal.h): hardware-facing structures and palette API.
- [SGDK VDP layout header](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h), [tile header](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_tile.h) and [tile implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp_tile.c): reserved tile ranges, attribute construction and transfer behavior.
- [Plutiedev: tiles and palettes](https://plutiedev.com/tiles-and-palettes): hardware pattern format, 2048 theoretical tile addresses and palette loading.
- [Sega Genesis Software Manual](https://segaretro.org/images/a/a2/Genesis_Software_Manual.pdf), VDP chapters: mode-dependent sprite limits, CRAM and VRAM. Consult the hardware manual before extending the basic assumptions to raster or interlace modes.

The bundled example is compiled in the published ARM64 and AMD64 Docker images by the repository's integration tests. Those tests exercise actual ResComp and headers, including rejected input and a PNG with unused palette entries. They do not replace emulator or hardware review.
