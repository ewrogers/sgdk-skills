# Menu Lab

An original, silent UI example: title screen, options, credits, a 12-second
score demo, pause menu and results. Switch between an original pixel font and
SGDK's default font, then watch the HUD stay fixed over a scrolling grid.

Copy this folder to a **new** project directory, then build there:

```sh
docker run --rm --pull=always --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/sgdk:latest build
```

In PowerShell, replace `$(pwd)` with `${PWD}` and use Docker's Linux-container
mode. The image supports ARM64 and AMD64. Load `out/rom.bin` in a Mega Drive
emulator. `build debug` adds symbols; `build clean` removes generated output.

## Controls

| Screen | Controls |
| --- | --- |
| Title | Up/Down choose; A or Start selects |
| Options | Up/Down choose; Left/Right or A changes; B returns |
| Credits | A, B or Start returns |
| Score demo | D-pad moves the `+` by cells; A adds points; B clears score; Start pauses |
| Pause | Up/Down choose; A selects; B or Start resumes |
| Results | Up/Down choose; A or Start selects |

In ares with the usual project mapping, A/B are J/K and Start is Return.
The lab does not change emulator settings. It uses P1's three-button controls.

## What it demonstrates

- The cursor wraps. Hold a direction for 300 ms to start repeating every 100 ms.
- Difficulty changes the score award: 50, 100 or 150 points per A press.
- Font and difficulty settings survive returning to the title, until reset.
- Scores are six digits. Clearing a score overwrites all six cells, without
  leaving old digits behind. Shorter option names also erase the old ending.
- Pause freezes the round, scroll and gameplay controls. Restart clears the
  score and restores 12 seconds; results offer another round or the title.

`src/menu.c` owns state, repeat and bounded formatting. `src/view.c` owns VDP
drawing and only rewrites changed fields. `src/main.c` samples SGDK's cached
joypad state once per frame and supplies nominal 50/60 Hz elapsed milliseconds.
These modules are a small example, not a required game framework.

The HUD uses Window's top four rows. Menus and the marker use plane A; the
grid uses plane B. Layout is 320x224 in both regions. PAL timing normalization
does not add extra display rows or recover missed frames.

The original font sheet has 96 glyph slots in ASCII order. Lowercase uses the
uppercase designs; missing designs use `?`. From the installed skill folder,
`python3 scripts/make_lab_font.py NEW_FONT.png` regenerates the sheet using
Python's standard library and refuses to overwrite existing art. Its resource
disables tile deduplication so blank/repeated glyphs keep their positions.

## Playtest checklist

1. Hold Down/Up to check repeat and wrapping. Hold A when entering Play and
   confirm the score stays zero until a new press.
2. Open Options. Change Normal to Easy and check that no letters remain.
   Switch fonts, return to title, then reopen Options: choices should persist.
3. Start Play. Check the background scrolls while score/time stay fixed.
   Add points, then clear them. Check the six score digits and countdown.
4. Pause near the end of a round, wait, then resume. Remaining time should
   be preserved. Test Restart and Title Screen as well.
5. Let a round finish. Verify final score, Play Again, and Title Screen.
   Visit Credits and return without accidental double selections.
6. Repeat in NTSC and PAL. Compare elapsed real time, not display-frame counts.
   Check readability on a TV/console when available; the lab leaves margins
   but does not certify every display's overscan or hardware timing.

Host tests execute the model and record renderer calls. Container tests build
release/debug ROMs and compare every compiled font tile against the source
pixels. Those checks do not emulate the VDP or replace this visual checklist.
