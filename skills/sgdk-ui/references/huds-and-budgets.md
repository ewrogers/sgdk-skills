# HUDs and budgets

## Choose where the text lives

Text is background tilemap data, not a hardware sprite per character.
Drawing scores on plane A is simple if it does not scroll. For a HUD that
stays put over a moving world, the Window plane is often useful. It replaces
plane A in its configured screen region; plane B remains underneath.

For a four-row HUD at the top, clear any old horizontal boundary first:

```c
VDP_setWindowHPos(FALSE, 0);
VDP_setWindowVPos(FALSE, 4);
VDP_drawTextEx(WINDOW, "SCORE", TILE_ATTR(PAL1, TRUE, FALSE, FALSE),
               2, 1, CPU);
```

Vertical positions use 8-pixel rows; horizontal positions use 16-pixel units.
Window boundaries combine as regions, not as a general rectangular clipping
widget. Clearing both before configuring a new layout avoids an old horizontal
strip remaining visible. Menu Lab resets H and sets V explicitly.

High-priority text affects VDP priority relative to other graphics; it is not
a universal UI layer above every possible sprite/plane combination. Check the
game's composition. A transparent Window cell does not bring plane A back in
that region. Put an intended backdrop on plane B, or supply opaque UI tiles.

## Reserve tiles and colors

The normal 96-glyph font occupies 96 x 32 = 3,072 bytes of tile patterns in
VRAM. A custom font loaded into the same slots replaces those patterns; it
does not need a second 3,072-byte runtime copy. The resource also occupies ROM.
Compression may reduce ROM at the cost of unpacking work/storage.

Tilemaps, sprite patterns and VDP tables share the console's 64 KiB VRAM.
Do not budget the font as if all 2,048 tile indices were available alongside
the name tables. Use the selected SGDK layout and `TILE_FONT_INDEX` rather
than hard-coding the font address. The sprite engine and plane layout affect
available user tiles.

Text shares the four 16-entry palette lines with other graphics. Using another
palette line for highlighted text does not allocate another font. Menu Lab
uses foreground index 15 in PAL0/PAL1, a dark grid color in PAL2, and backdrop
entry 0. A game may share palette lines more tightly; those are ownership
choices, not a requirement to spend three lines on menus.

Palette fades or cycling change everything that uses the affected entries.
Keep the HUD's entries out of a world-only fade, or deliberately fade them too.
Do not promise independent per-node colors without accounting for those shared
entries. A menu transition need not allocate a fifth palette: it can fade and
replace colors within the existing four.

## Spend transfers where they help

Each text cell writes one 16-bit tilemap entry. Updating six score digits
requires six entries, not another upload of six glyph patterns. Rewriting the
entire HUD every frame adds CPU/transfer work that a changed-field check avoids.

Immediate CPU text drawing is convenient for small fields; schedule larger
updates around VBlank or blank the display during a screen rebuild. Queued DMA
has operation and byte budgets as well as source-lifetime requirements.
Do not hand a queue a stack buffer that disappears before the transfer runs.
Consult the selected helper implementation: SGDK 2.11's `VDP_drawTextEx`
uses a row helper with temporary-buffer handling. That does not make arbitrary
queued tile/palette calls copy their source data.

Do not repeatedly clear a visible field and then redraw it: the scanout can
show the empty interval as flashing, even when the final text is correct.
Prepare the replacement before publishing it. For a larger changing interface,
a RAM tilemap with changed rows queued for VBlank is one option; count the
operations and bytes, and keep its source immutable until the queue drains.
There is no need to transfer rows whose final contents did not change.

Menu Lab uses immediate CPU text and blocking font DMA while blanked. Its
recording tests check field writes and ownership, not bus contention or frame
time. Measure a busy game with its actual art, audio and transfers before
claiming the UI cost is negligible.

## Sources

- [SGDK 2.11 Window and VRAM layout](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h).
- [Text/scroll API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_bg.h)
  and [text row implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp_bg.c).
- [Font loading and tile transfers](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_tile.h).
- [Palette API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/pal.h).
