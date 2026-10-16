# Asterfall: The Lantern Road

A playable art catalog for an original fantasy JRPG. The creative brief calls
for crisp native-resolution art, rich but controlled palettes, readable menus
and gentle adventurous music. These choices illustrate this project, not a
style rule for SGDK games.

## Scope

A skippable parallax story intro leads to Title: Start, Continue, Options.
Start opens a grid keyboard. Continue offers four fictional saves in a 2×2
layout. The world map links Willowmere (town), Old Lantern Crypt (dungeon),
Mossbell Wood (forest) and Tideglass Quay (coast). All four can be explored with
a moving character, wandering NPCs and short dialogue. This is an art/design
mockup: saves, stats, equipment, inventory, spells and progression use mock data.
It has no battle or persistent saving requirement.

Field controls: D-pad moves, A talks, B opens the character menu, C opens the
world map, Start opens the menu. Menus use D-pad, A confirms, B returns.
Keyboard supports letters, digits, erase and Done. Start confirms a nonempty name.

## Art and palette plan

PixelLab supplies illustrations, three field characters and walking cycles,
portraits, font and core icon art. Native pixel cleanup, nine-slice borders,
markers, cursors, glyph repairs and small effects may be authored directly.
No smooth downscaling. Retain original assets, requests and exact corrections.

Explore screens use two source-derived background palette lines, one shared
field-character line and one UI line. Palette sets may change per location.
Menus can repurpose the background lines for portraits and item/spell icons.
The intro uses separate background/foreground plane art and fading text.
The title uses a selective 256×64 shadow/highlight operator mask below its
lettering. Field scenes disable S/H to preserve actor scanline headroom. Cursor
updates reuse loaded art and queue only changed UI rows through VBlank; no
input-triggered palette uploads. Production lettering is authored at 5×7 pixels
after playtesting found the generated font uneven. Map locations use circles
with a pixel-smooth bobbing selection pin. Submenus retain their parent choice.

The generation allowance is **200 maximum**, starting at account usage 159.30.
Record requests and returned costs, including pending work, and stop before the
limit. Failed validation requests are retained. Budget is a ceiling, not a target.

## Validation

Check native image dimensions, alpha and per-tile palette assignment. Compile
through the real SGDK image on ARM64/AMD64. Test navigation, naming, menu bounds,
travel, collision and dialogue state independently from drawing. Inspect the
ROM in ares for text clipping, palette ownership, sprites and music transitions.
