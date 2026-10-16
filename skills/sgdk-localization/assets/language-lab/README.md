# Language Lab — The Word Ferry

An SGDK translation example with a ferry keeper's conversation, inventory and
name-entry screen. Switch between English, French, Spanish and a small katakana
sample without changing the selected menu or losing your name. Artwork, wording
and font style are illustrative. This is not a translation service or a full
Japanese character set.

## Build and play

From this directory, with Python 3.10+ and Docker running:

```sh
docker pull ewrogers/sgdk:latest
python3 tools/build.py --output "build/language-lab"
```

Use `python` if that is your Python command. Output must be a new directory.
The helper copies a standalone SGDK project, compiles the catalog, generates
the native font/ferry art and builds `out/rom.bin`. Add `--platform linux/arm64`
or `--platform linux/amd64` to choose the container architecture, `--mode debug`
for a debug build, or `--region ntsc` / `--region pal` for a header hint.

Open the ROM in ares, or use its executable with:

```sh
ares --system "Mega Drive" "build/language-lab/out/rom.bin"
```

| Control | Action |
| --- | --- |
| Left/Right on the home screen | Change language |
| Up/Down, A | Choose/open Talk, Inventory or Name |
| C on any screen | Next language, keeping the current selection |
| A in dialogue | Next page; return home after the last page |
| A in inventory | Cycle 1, 2, 999, 12,345 and 65,535 |
| B in dialogue/inventory | Return home |
| D-pad, A in name entry | Select/add a glyph, up to eight |
| B in name entry | Erase the last glyph |
| Start in name entry | Accept; an empty name returns to Mira |

With the usual ares mappings, J/K/L are A/B/C and Return is Start. Navigation
uses press edges rather than automatic repeat. Language changes restart
reflowed dialogue at page one, while keeping the name, quantity and menu choice.

## Author and validate translations

`res/catalog.json` is UTF-8 source. Each message has an explicit stable ID,
key, cell width, rows per page and maximum pages. Each language has a stable
saved ID, tag and grouping separator. IDs survive reordering; names and tags
are for people, not persisted array positions.

Only `{name}` and `{count}` are substitutions. Formatting syntax, unknown
tokens or mismatched placeholders are rejected. Authored newlines are explicit
line breaks. Names are at most eight glyphs, and counts are unsigned 16-bit
values. The compiler checks every translation with those maximum expansions,
then wraps in the same manner as the runtime. It counts glyphs, not UTF-8 bytes.

```sh
python3 tools/catalog.py res/catalog.json --output "build/catalog-check"
```

This emits C tables, a glyph atlas, ResComp resource definition and
`catalog-report.json`. Existing output directories are refused. Unsupported
characters, missing translations, bad IDs, invalid controls, oversized words,
page overflow and font over-budget errors fail the command. NFC normalization
lets precomposed and combining-accent authoring produce the same glyph data.

Missing translations fail by default. An intentional fallback must be named
in `allow_fallback`, for example `[["es", "item"]]`, with that value omitted
from the message. The report lists every fallback. It is never a silent success.
The shipped catalog has no fallbacks. Invalid runtime language IDs fall back
to the English default; the preference decoder rejects unknown saved IDs.

French numbers use a nonbreaking space, English a comma, Spanish a period and
the katakana sample no grouping. These are example locale conventions, not
complete international number formatting. Singular/plural messages are chosen
explicitly for one versus other; other languages can require more plural forms.

## Font and layout budgets

The current catalog uses **164 glyph slots including the blank tile**, packed
into **176 tiles / 5,632 VRAM bytes**, inside a 192-tile reservation. It also
uses 2,016 encoded text bytes in ROM, before table/pointer metadata. The footer
shows the actual font allocation. The 32-byte-per-tile cost is separate from
RAM scratch space, plane tilemaps, palette lines and DMA work.

The atlas is a 16-column grid of native 8x8 tiles. ResComp uses
`TILESET ... NONE NONE ROW`: no deduplication, flips or reordering. Code zero
is the blank/end marker; printable glyph IDs and three control tokens use a
Lab-specific byte codec. Its 251-symbol ceiling is a codec choice, not a Mega
Drive language limit. Larger repertoires need wider IDs, font banks or another
renderer. Do not feed the encoded bytes to `VDP_drawText`.

`VDP_loadTileSet` loads the font at tile 128. The renderer writes explicit
tilemap entries. It does not use SGDK's 96-slot default font area for accents.
All four example repertoires remain resident, so switching language does not
replace glyphs still used by existing text. The ferry uses 48 tiles and panel
edges two more: **226 custom tiles / 7,232 pattern bytes** in total. No hardware
sprites are used. Four palette lines supply text colors and the ferry art.

The 40x28 screen buffer occupies 2,240 RAM bytes. A changed screen queues a
copied 2,240-byte tilemap update; idle frames queue nothing. Expansion and
wrapping use bounded stack buffers. Font/art uploads and palette writes happen
with display disabled at startup. These bounds are not CPU-load measurements.

## Remembering the language

The Lab stores a six-byte versioned language preference in cartridge SRAM:
magic, version, stable language ID, CRC and commit marker. It invalidates the
marker before writing and commits last. Invalid, unknown or interrupted records
fall back to English. This tiny preference format intentionally does not retain
the previous selection after an interrupted write; a campaign save should use
its existing transactional save system. Names and inventory counts are mock
RAM data and are not persisted. The original example is well below the SRAM
overlap boundary and owns only six logical SRAM bytes.

## Check it in ares

1. Check French `Été`, `îles`, `cœur`, `façade` and Spanish `¡Qué día!`, `Niño`,
   `pingüino` for sharp, distinct glyphs. Compare them at integer scale.
2. Open Talk and advance every page in every language. The final indicator must
   disappear at the end; no words should clip the panel.
3. Open Inventory and cycle every quantity. Check singular/plural wording,
   number separators and the longest formatted sentence.
4. Enter an eight-glyph name with accents. Switch languages, then check dialogue
   and inventory again. No replacement boxes or stale letters should appear.
5. Change language, reset/reload and confirm it returns. Then test a fresh SRAM
   file separately without deleting an existing game's save.
6. Check PAL and NTSC presentation separately. Layout is cell-based and has no
   time-dependent reveal; this does not establish hardware compatibility.

The Lab is intentionally silent. Automated checks validate the compiler,
formatter, controller, preference recovery and linked font tiles; they do not
judge translation quality, native-speaker readability or physical-console video.
