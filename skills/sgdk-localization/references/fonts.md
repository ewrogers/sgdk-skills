# Fonts and Mega Drive budgets

An accented letter should be an intentional glyph, not a substitute that happens
to occupy a nearby tile. Start with the text repertoire and the game's existing
font style. Draw or select native glyphs with enough room for accents and
descenders, then check them at integer scale and on the intended display.

## Know what SGDK's text helpers do

SGDK 2.11's `VDP_drawTextEx` maps each input byte to
`TILE_FONT_INDEX + byte - 32`. It does not decode UTF-8, wrap words or interpret
newlines. An `é` in a UTF-8 C string occupies multiple bytes, so handing that
string to the default text helper does not draw one accented glyph.

`VDP_loadFont` loads at the SDK's current font tile index. The standard font
area has 96 tiles. Loading another font there changes the appearance of text
already pointing at those slots. Finish the VDP layout before loading custom
fonts, since SDK layout changes can relocate/reload its default font.

For a different repertoire, reserve a tile range and map glyph IDs to explicit
tile attributes. Language Lab loads its atlas with `VDP_loadTileSet` and writes
tilemap rows. That function returns nonzero on success, **not the number of
tiles loaded**; inspect `TileSet.numTile` for the count.

## Preserve the atlas order

Each font tile is an 8x8, 4-bit-per-pixel pattern: 32 VRAM bytes. A glyph that
uses more than one tile costs more. Palette selection belongs to each tilemap
entry; a recolored font can share the same patterns.

The example emits a 16-column indexed PNG and this ResComp declaration:

```text
TILESET language_font "font.png" NONE NONE ROW
```

Disable tile optimization/reordering when the glyph-to-tile mapping depends
on atlas order. Identical blanks and mirrored shapes still need their assigned
slots. The atlas's padded final row consumes tiles too; report that count,
not just the number of visible letters.

Language Lab's 164 glyph slots occupy a 176-tile atlas: 5,632 VRAM bytes.
Its 192-tile reservation leaves room for more glyphs without moving the next
range. Text bytes in ROM, pointer tables, tilemaps, stack buffers and transfer
queues are separate costs. Saving RAM does not automatically save VRAM, or the
other way around.

## Choose resident glyphs or banks

Small repertoires can share one resident atlas. The Lab keeps its Latin accents
and katakana sample together so a name survives language changes without a font
upload. Its byte codec reserves control values and allows 251 printable symbols;
that is an example encoding limit, not the console's maximum glyph count.

For a larger repertoire, consider 16-bit IDs, locale/scene font banks or a
bounded glyph cache. Account for simultaneous fonts, portraits, sprites and
background patterns before choosing. Banked fonts need a lifetime rule: drain
queued work, replace or clear tilemap entries that refer to the old bank,
upload the new patterns at a safe time, then rebuild the affected text.

A cache also needs eviction/pinning rules. Never overwrite a glyph tile still
referenced by visible text. If a bank changes while a player's name remains
onscreen, that name's repertoire must remain available or be redrawn correctly.

## Repertoires are not shaping engines

The katakana example is deliberately small and hand-authored. It does not
cover all Japanese text, kanji, arbitrary combining marks or contextual glyph
shaping. Right-to-left direction, joined letters, ligatures and bidirectional
numbers need their own layout/shaping decisions; more font tiles alone do not
solve them. Keep unsupported capabilities explicit.

## Verified sources

- [SGDK 2.11 text mapping](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp_bg.c).
- [Font/tile loading declarations](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_tile.h)
  and [return/transfer behavior](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp_tile.c).
- [VDP layout and font reload](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp.c).
- [ResComp TILESET options](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt).
