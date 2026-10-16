---
name: sgdk-localization
description: Prepare SGDK games for translated text with stable message IDs, accented or custom glyphs, font budgets, safe substitutions and translated layout checks. Use for multilingual menus, dialogue and language preferences, rather than general text rendering alone.
---

# SGDK Localization

Keep translations readable in the source project, then validate their glyphs,
substitutions and screen layouts before building the ROM. SGDK provides tile
and text drawing, not a Unicode decoder, translation catalog or plural engine.
Preserve the game's chosen UI, fonts, languages and creative direction.

## Start with the languages and screens

Find existing catalogs, fonts, save settings and text drawing before adding
another system. Establish the required languages, character repertoire,
user-entered names, maximum substitutions and actual text boxes. A small Latin
repertoire, Japanese subset and shaped right-to-left script need different
rendering work; do not promise them all from a byte-to-glyph conversion.

- [Catalogs and wording](references/catalogs.md): stable IDs, UTF-8 authoring,
  normalization, placeholders, plurals, fallback and saved preferences.
- [Fonts and rendering](references/fonts.md): native glyphs, tile budgets,
  custom mappings, font banks and SGDK's actual text behavior.
- [Layout and validation](references/layout.md): translated expansion,
  wrapping/paging, runtime names/numbers and checks at each test layer.
- [Language Lab](assets/language-lab/README.md): a standalone translated menu,
  conversation, inventory and name grid, plus an offline catalog compiler.

## Keep the contracts explicit

Use stable message/language IDs and keep translated prose separate from game
logic. Name substitutions instead of constructing sentences by concatenating
English word fragments. Check placeholder agreement and bound runtime values.
Do not use a translation as a `sprintf` format string.

Count rendered glyphs or measured advances, not source UTF-8 bytes. Normalize
authoring text consistently; reject missing glyphs and unexpected controls.
An unsupported character must produce a useful build error or a deliberately
chosen fallback, not a random VRAM tile.

Budget font pattern bytes separately from text bytes in ROM, RAM scratch,
tilemaps, palettes and transfers. Show actual glyph/atlas counts and the
reservation they use. The Lab's byte codec and 192-tile font reservation are
example choices, not hardware limits or required game architecture.

## Prove the longest cases

Check every locale with the longest allowed names/numbers, all plural variants,
explicit line breaks, narrow boxes and maximum pages. Exercise language changes
inside menus/dialogue, stale-text clearing and unsupported saved language IDs.
Keep missing translations visible in validation; report intentional fallback.

Use `ewrogers/sgdk:latest`, pulled before validation, and verify unfamiliar APIs
against its headers/source. The references were checked with SGDK 2.11 and
GCC 16.2.0. Compile the actual resources/adapters and test both optimized and
debug behavior. Host or M68000 checks cannot judge translation quality, reading
comfort or physical-console video. Ask for native-speaker review where needed.

Other installed skills are optional: `sgdk-ui` for general menus/fonts,
`sgdk-dialogue` for conversation flow, `sgdk-save` for campaign storage and
`sgdk-testing` for regression automation. This skill works independently.
