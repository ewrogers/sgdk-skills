# Asterfall: The Lantern Road

A small, playable fantasy JRPG **art catalog**, built with SGDK. It demonstrates
how PixelLab illustrations, characters, portraits, font and inventory art can
fit together on the Mega Drive. It is a design mockup, with fictional saves,
stats, equipment and quests—not a full RPG or a new game framework.

The project used **55 of the allowed 200 generations**. Exact prompts, rejected
requests and returned costs are in [requests.json](requests.json). The original
PNG images and character ZIPs remain in `raw/`. No new service calls are needed
to build, test or customize this example.

## Try it

A skippable parallax story intro leads to Start, Continue and Options. Start
lets you name the roadkeeper on a grid keyboard; Continue offers four mock save
cards. The travel map links Willowmere, Old Lantern Crypt, Mossbell Wood and
Tideglass Quay. Walk around, meet Bram and Lyra, and open the character menus.

| Control | Field | Menus |
| --- | --- | --- |
| D-pad | Walk | Choose |
| A | Talk to a nearby NPC | Confirm / reveal / advance |
| B | Open character menu | Back / erase while naming |
| C | Open travel map | — |
| Start | Open character menu | Confirm name / skip intro |

With the review machine’s ares mappings: J/K/L are A/B/C; Enter is Start.
The music and eight UI cues are original synthesized FM/PCM compositions,
with a different theme for each location. Options adjusts music and dialogue
speed. No commercial soundtrack or recording is included.

## Build

Install Docker and pull `ewrogers/sgdk:latest`. From this folder:

```sh
python tools/build.py --output /path/to/new-asterfall --platform linux/arm64
```

Use `linux/amd64` for that architecture. Add `--mode debug` when needed. The
output directory must be new; the helper never overwrites a project or source
art. The ROM is `out/rom.bin`, currently **393,216 bytes** after SGDK padding.
To inspect one screen directly, add e.g. `--preview inventory`, `--preview
naming` or `--preview field`. Normal builds always start with the intro.

Create the local original/prepared gallery next to a prepared build:

```sh
python tools/catalog.py /path/to/new-asterfall
```

Then open its `catalog.html`. Original/prepared pairs show the same asset;
character sheets retain native 48×48 frames and consistent direction order.

## Art handoff and native finishing

- Locations use two background palette lines; all three field characters and
  portraits share line 2; UI owns line 3. Menus reuse line 1 for item/spell art.
  Palette sets change between locations with display blanked.
- Backgrounds select a complete palette line per 8×8 tile. They are VDP tile
  planes, not hundreds of hardware sprites. Only current actor frames occupy
  their fixed VRAM windows; other frames stay in ROM until needed.
- Images are never smooth-downscaled. The font’s actual 8-column atlas is mapped
  to ASCII, its glyph masks are preserved and recolored, missing arrow/bracket
  glyphs are authored, and the title gets an integer 2× variant with an explicit
  shadow and color treatment. Ordinary text cells use opaque navy to prevent
  scenery bleeding through letters inside panels.
- PixelLab invented lettering in the intro sky. A recorded, native-size empty
  sky strip repair removes it; the original is retained. Two small map pins and
  the UI layout/nine-slice assembly are authored directly. The healing tonic
  returned blue despite the green brief; its liquid ramp is explicitly recolored
  to emerald before shared-palette fitting, preserving its shape and highlights.
- No full-screen shadow/highlight mask is used: reserving the scanline space for
  actors is more useful here. The separate lighting study demonstrates that
  optional technique and its cost. Nothing here prescribes another game’s style.

`asset-report.json` in each prepared build records palette plans, dimensions,
conversion hints and cleanup. `tools/prepare.py` performs no paid/network calls.
Font alpha thresholding and sprite alpha threshold 128 are explicit decisions.
The title-only font variant is loaded only for that screen.
The body artwork can overlap scenery; collision uses smaller foot boxes and
hand-authored walking regions, not colors inferred from an image.

## Verification and limits

Host C tests exercise naming bounds, all four mock saves, travel, menus, dialogue,
options, entry positions and collision. Actual M68000 probes repeat the logic
and check linked palette ownership, sprite frames and room/portrait VRAM bounds.
Release/debug ROMs are built on ARM64/AMD64 and compared. Visual/audio review in
an emulator remains separate; no real-hardware or analog-color guarantee is made.

This deliberately small example has no battles, real save persistence, real
item effects or automatic art-derived collision. The purpose is to inspect a
cohesive asset catalog in a navigable ROM and show how the assets are prepared.
