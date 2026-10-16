# Adventure Lab — The Last Bell

Three lighthouse keepers cross a moonlit causeway to wake a drowned observatory.
A small, playable RPG example for SGDK: prepare your party, choose an expedition,
learn a few combat interactions, and bring the last bell back to life.

The coastal setting, art, music, formation, turn rules and balance are original
choices for this Lab. They illustrate implementation patterns, not how another
RPG should look, sound or play. This is a compact example, not a general engine.

## Play

| Control | Harbor / menus | Battle |
| --- | --- | --- |
| D-pad | Walk / choose | Choose commands and targets |
| A | Interact / confirm | Commit / advance an action message |
| B | Party / back | Cancel target or submenu |
| C | Expedition route | — |
| Start | Party / confirm | Pause / resume |

In ares's review mapping, J/K/L are A/B/C and Return is Start.

Start a **New Voyage** and read the short help screen. In the harbor, walk left
to the workshop to change gear, right to the inn to rest/save, or use the bell
at the center to choose an expedition. C opens the route from anywhere.
B opens Status, Gear and Pack; Left/Right changes tabs. In Pack, Up/Down chooses
an item and C chooses the keeper who receives it. Gear swaps the selected
keeper between light and heavy equipment, changing attack, defense and speed.

Three encounters open in order. Earlier encounters remain available; you can
rest freely at the inn. The party has different strengths:

- **Mara:** Tidecut damages and soaks a foe. Beacon is a stronger direct strike.
- **Ivo:** Groundspark doubles damage against a soaked foe. Dampen weakens one.
- **Neri:** Mend heals a living ally. Wake revives a fallen one.

Strike costs no MP. Guard reduces incoming damage until your next turn. Tonics
restore HP, Ether restores MP, and Bell Seeds revive a fallen keeper. Dead or
absent actors cannot be attack targets. A bobbing arrow marks the selected
actor; the boss and its companion have separate positions.

Every ordinary action message waits for A, with a short guard against an
accidental repeated press. A victory opens the spoils automatically after the
last hit settles. Confirm the spoils to return to harbor; the final encounter
leads to the ending. Defeat restores the departure checkpoint and its item
stock. The guardian announces its charged strike one turn ahead—Guard helps.

**Continue** loads a real SRAM checkpoint, saved before departure or when
resting. It restores campaign data, not a partially presented attack. Two
40-byte journal slots preserve the older valid record if the latest is damaged.
The demo owns only 80 logical SRAM bytes. Physical retention still needs a
cartridge test; ares stores its own backing save separately.

## Build

Install Docker with Linux containers, then pull the published toolchain:

```sh
docker pull ewrogers/sgdk:latest
python tools/build.py --output /path/to/new-adventure --platform linux/arm64
```

Use `linux/amd64` on that architecture. The output directory must be new; source
art and existing projects are never overwritten. The ROM is `out/rom.bin`,
currently **262,144 bytes** after SGDK padding. Add `--mode debug` for symbols.
Optional `--preview camp`, `--preview party` or `--preview battle` starts a
review-only build at that screen; normal builds start at the title.

No API key, paid call or network asset download is needed to build this Lab.
Python helpers use the standard library. Keep all generated output out of Git.

## Art, sound and hardware budgets

The new art used **14 PixelLab generations**: three illustrations, six battle
sprites, one four-direction field character and four directional walk jobs.
Exact prompts and receipts are in [requests.json](requests.json); raw images
and the character bundle remain in `raw/`.

Preparation keeps native dimensions. The field tool returned 36×36 canvases;
all frames are padded to 40×40 at +2,+4 without rescaling. Its down-facing coat
and hands merged into a long protrusion during walking; the lower body is
explicitly re-authored on the native grid while keeping the generated head.
The battle illustration invented tiny people despite the brief; its bottom
24 rows are replaced with an authored stone floor. Originals remain intact.
`asset-report.json` records these corrections and the derived palettes.

Backgrounds use palette lines 0/1, all battlers and field frames share line 2,
and UI/effects use line 3. There is no HBlank palette trick or shadow/highlight
requirement. Battle sprites occupy fixed VRAM ranges; only changed field/effect
frames transfer during play. The actual linked-resource test checks fonts,
background banks, frame geometry, user tile reservations and conservative
sprite/scanline peaks for all three formations.

The checked M68000 build uses 270 bytes for `Game`, including its campaign,
departure copy and battle state. Conservative graphics checks reach 13 total
hardware pieces, seven on one scanline and 208 sprite pixels on that line.
These are resource bounds for the Lab's formations, not frame-time measurements.

The UI uses authored 5×7 letters and a small nine-slice border. Its current and
last submitted 40×28 tilemaps use 4,480 bytes, plus 28 row flags. Only changed
rows transfer during VBlank. Palettes and art reload at scene changes while the
display is hidden, never because the player moves a menu cursor.

Seven original FM themes and ten finite PCM cues are synthesized by
`tools/adventure_audio.py`. The generator and bitmap-font foundations are
adapted from earlier original Labs; the new melodies, game art, panel, cursor
and spell effect belong to this example. No commercial art or recordings are
included. Shared preparation utilities are copied locally so installation is
self-contained.

## Code and checks

`party.c` derives stats and applies equipment/items. `battle.c` owns validated
actions, order, seeded RNG, effects and outcomes. `checkpoint.c` encodes explicit
versioned bytes; `save.c` handles SGDK SRAM. `game.c` owns menu flow and message
pacing; `clock.c` handles nominal PAL/NTSC time. `layout.c` gives the renderer
separate battle slots. Rendering, UI and audio are separate adapters.

Host tests cover rejected actions without side effects, healing/revival, combo
rules, turn ties, fallen actors, seeded outcomes, progression, one-time spoils,
menu targeting, pause, clock cadence and checkpoint recovery. A recording SRAM
device interrupts every write boundary, checks sequence wrap and corrupt-slot
fallback. Native art preparation must reproduce exactly and refuse overwrites.

Docker checks compile release/debug and execute the same rules plus linked
resource budgets as base M68000 code. ARM64/AMD64 hashes are compared in CI.
These checks do not emulate VDP transfers, audio or physical SRAM retention.

For a visual playtest, check all menus, walking directions, target changes,
soak/lightning, weakening, Guard, healing/revival, pause, victory, defeat,
Continue, all three encounters and the ending. Confirm that attack messages
remain readable and no sprite obscures the HUD. PAL display/audio and physical
hardware remain separate from the nominal clock and build tests.
