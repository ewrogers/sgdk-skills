# Importing Tiled maps

## Import the visual layer

For an orthogonal map with an external tileset, keep the TMX, TSX and image
together with their relative paths intact. ResComp's `MAP` resource names the
tile layer to import:

```text
PALETTE terrain_palette "outpost.png"
MAP outpost_map "outpost.tmx" "Ground" NONE NONE
```

For this TMX form, the two `NONE` arguments select tileset compression and
map compression. They are not the `TILESET` resource's optimization arguments.
In SGDK 2.11 this example exports `outpost_map` and
`outpost_map_tileset0`. Read the generated header rather than guessing resource
names, especially when adding more tilesets.

The lab's Tiled cells are 16x16 pixels. ResComp turns them into metatiles made
from 8x8 hardware tiles, then groups map data into 128px blocks. A 64x64-cell
map therefore covers 1024x1024 pixels and has 8x8 blocks in its MapDefinition.
That block count is not its cell count or pixel size.

Use an indexed PNG with a deliberate palette. The lab's images each have
exactly 16 palette entries. A PNG with a larger palette can accidentally
produce a multi-line palette resource even when few colors appear on screen.
The game still owns palette assignment and tile residency.

## Keep gameplay meaning separate

`MAP` imports the selected visual tile layer. It does not create gameplay
entities or collision rules from every object/property in the file. SGDK also
has an `OBJECTS` resource for typed object extraction; use that when it suits
the project. Do not treat its absence from this lab as an SDK limitation.

Optimized tile indices are graphics implementation details. A tile may be
deduplicated or flipped, so a collision rule such as "VDP tile 42 is solid"
can break when art changes. Preserve semantic data separately: solid flags,
collision shapes, material IDs, spawn records or whatever the game needs.

Level Lab uses a hidden tile layer named `Collision`. An empty cell is clear;
any nonzero, valid tileset GID is solid. It is a full-cell occupancy convention,
not a polygon solver. The visible `Ground` layer and hidden `Collision` layer
remain independently editable. If changing a wall, update both deliberately.

## Export collision for the lab

Run `tools/tiled_collision.py res/outpost.tmx` from a copied lab directory.
It writes `inc/level_data.h` and `src/level_data.c`; those generated files are
ignored by Git. The original TMX/TSX and PNGs stay in source control. Rerun the
helper after editing Collision, then build the ROM.

The helper accepts finite orthogonal maps with square 16px cells and one
top-level, unshifted Collision layer using uncompressed CSV data. External
TSX and inline tilesets are resolved using `firstgid` and `tilecount`.
Horizontal/vertical flip bits do not change full-cell occupancy. Unsupported
diagonal/hex rotation bits, unknown IDs, overlapping GID ranges, missing
tilesets, shifted/nested layers and mismatched cell counts fail clearly.

Its output is a row-major bitset, least-significant bit first within each
byte. The 64x64 lab needs 512 bytes of const collision data in ROM, with no
full collision grid copied into RAM. This helper's limits are an explicit
sample contract; other Tiled formats need an appropriate importer rather than
silently treating unsupported data as empty space.

The helper validates its collision input. ResComp separately validates the
visual import; success in one does not guarantee success in the other. Neither
checks that every visible wall matches the authored collision layer.

## Sources

- [SGDK 2.11 ResComp resource syntax](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt).
- [TMX reader](https://github.com/Stephane-D/SGDK/blob/v2.11/tools/rescomp/src/sgdk/rescomp/type/TMX.java)
  and [map resource generation](https://github.com/Stephane-D/SGDK/blob/v2.11/tools/rescomp/src/sgdk/rescomp/resource/Map.java).
- [Tiled TMX format](https://doc.mapeditor.org/en/stable/reference/tmx-map-format/)
  and [global tile IDs and transform flags](https://doc.mapeditor.org/en/stable/reference/global-tile-ids/).
