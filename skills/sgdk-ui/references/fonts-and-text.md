# Fonts and text

## Start with the built-in font

`VDP_drawText("READY", 2, 3)` draws at column 2, row 3 in the current text
plane. `VDP_setTextPlane`, `VDP_setTextPalette` and `VDP_setTextPriority`
change shared drawing state. Set them deliberately, or use the explicit form:

```c
VDP_drawTextEx(WINDOW, "READY", TILE_ATTR(PAL1, TRUE, FALSE, FALSE),
               2, 1, CPU);
```

That palette number selects a line of 16 colors, not an individual color.
The font's pixel indices select colors within the line. The default font and
Menu Lab's font use index 15 for the foreground; changing palette entry 31
therefore changes text drawn with `PAL1`. Keep index 0 transparent.

The text API maps each byte to `TILE_FONT_INDEX + byte - 32`. It does not
interpret newlines, wrap words or decode UTF-8. Use printable ASCII bytes
32 through 126 unless you implement a different glyph mapping. An accented
UTF-8 character is multiple bytes, not one supported glyph. Replacing invalid
bytes with `?`, as the lab does, is a fallback, not Unicode support.

## Import a custom font

Arrange 96 tiles in ASCII order, starting with space (32), through slot 127.
Each glyph occupies an 8x8 tile. A 128x48 sheet gives 16 columns and 6 rows.
Do not deduplicate or reorder these tiles, even if some glyphs are blank:

```text
TILESET menu_font "font.png" NONE NONE ROW
```

The first `NONE` disables compression. The second disables tile optimization;
its default would remove duplicate/flipped tiles and break the glyph indices.
After setting the VDP memory layout, load the resource:

```c
if (!VDP_loadFont(&menu_font, DMA)) {
    /* Handle resource loading failure using your project's error path. */
}
```

Loading another font replaces the same font tiles, so existing text also
changes appearance. To show two fonts at once, reserve a second tile range
and supply your own text-to-tile mapping. Do not expect the standard text
API to manage multiple typefaces. Later VRAM-layout changes can move font
slots and cause SGDK to reload its default font; finish layout setup first.

Uncompressed font data in ROM works with immediate DMA. For compressed
resources, check unpacking and buffer lifetime: `DMA_QUEUE` can outlive a
temporary unpacked buffer. Use an appropriate copied transfer or keep the
buffer alive. Menu Lab loads fonts with display disabled during setup/changes.

## Replace a field safely

Drawing `"9"` over `"100"` leaves `"900"`. Write a padded fixed-width string,
or clear the old field with `VDP_clearTextBG` before drawing the new text.
Keep the width within the visible row. A trailing `\0` still needs buffer space.

Menu Lab's `menu_field` clips text to the field, replaces unsupported bytes
and pads with spaces. Its `menu_number` writes zero-padded decimal digits;
overflow displays `#` characters instead of silently dropping leading digits.
These are lab helpers, not SGDK APIs. A six-cell score needs seven bytes.

SGDK's `uintToStr` and `intToStr` accept a minimum digit count, not a buffer
capacity or maximum width. Their documented range is also narrower than every
possible 32-bit value. Size and range-check the destination yourself, or use a
bounded converter such as the lab's. `sprintf` likewise has no buffer capacity;
verify availability and semantics before assuming a host formatting function
exists in the target library.

Avoid SGDK 2.11's `VDP_drawTextFill` / `VDP_drawTextBGFill` for unbounded input:
their implementation copies the whole string into a field-sized temporary
before padding. An input longer than that field can overrun the buffer.
The lab bounds its text before calling `VDP_drawTextEx` instead.

## Sources

- [SGDK 2.11 text API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_bg.h)
  and [implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp_bg.c).
- [Font loading](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_tile.h)
  and [VDP layout/default font reload](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp.c).
- [ResComp TILESET syntax](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt).
- [String conversion contracts](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/string.h)
  and [implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/string.c).
