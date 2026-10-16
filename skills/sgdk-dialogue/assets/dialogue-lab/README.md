# Dialogue Lab

Three conversations at Lantern Quay show gradual text, inline styles, animated
portraits, optional nameplates, retained top/bottom panes, paging and choices.
A quiet original 6/8 harbor shanty loops underneath finite dialogue cues.
The setting, art and music are illustrative; reuse the code patterns with your
own game's style.

## Build and run

Copy this whole folder to a new project directory. With Docker running, open a
terminal there:

```sh
docker pull ewrogers/sgdk:latest
docker run --rm --mount "type=bind,source=$PWD,target=/workspace" ewrogers/sgdk:latest make release
```

PowerShell can use `${PWD}` in place of `$PWD`. Docker Desktop must use Linux
containers. The image selects ARM64 or AMD64 for the host; no native SGDK is
needed. Load `out/rom.bin` in your Mega Drive emulator. On macOS with ares:

```sh
/Applications/ares.app/Contents/MacOS/ares --system "Mega Drive" "$PWD/out/rom.bin"
```

Use `make debug` through the same container for a debug build. Output remains
in this project's `out/` directory.

| Control | Action |
| --- | --- |
| Up / Down | Select a story or visible choice. |
| A | Open, reveal the current page, then advance/select on the next press. |
| B | Return to the story menu. |
| C | Replay the current story with fresh flags. |
| Start | Pause/resume text, portraits and music. |

## Code and authoring

- `dialogue.c` owns bounded progression, cells, choices, flags and events.
- `lab.c` maps one cached controller snapshot to edges and a nominal 50/60 Hz clock.
- `view.c` draws nine-slice panels, resident font banks and low-level portrait sprites.
- `sound.c` handles XGM2 music and prioritized, finite PCM cues.
- `res/conversations.json` is the authoring source. `dialogue_data.c` is generated.

From this folder inside the installed skill, regenerate after editing JSON:

```sh
python ../../scripts/compile_dialogue.py res/conversations.json --out . --force
```

If you copied only the Lab, invoke `compile_dialogue.py` from the installed
skill's `scripts` folder instead. The generated C is already committed; Python
is not needed for normal builds. See [authoring](../../references/authoring.md)
for schema, markup, limits and flag lifetime.

`tools/make_art.py` generates the native panel/font/world assets;
`tools/make_audio.py` generates original VGM/WAV sources. Both take an output
directory and refuse existing outputs unless `--force` is used. Portrait sheets
are committed at native size; their PixelLab provenance and palette preparation
are recorded in `res/art-provenance.json`. Repacking portraits additionally
needs the original five frames per speaker via `--portraits`.

The adapter uses three palette lines and 745 resident art tiles in the tested
SDK, plus its system/font/table reservations. `DialogueLab` is 596 bytes on
M68000; two renderer caches add 4,480 bytes before SDK buffers, stack and audio.
See [rendering](../../references/rendering.md) for the full accounting and
integration decisions. The resulting release/debug ROMs are 128 KiB each.

## Playtest

1. Open Last Ferry. Confirm once while typing: the page fills, but does not
   advance until another press. Check the arrow, speaker swap and two panes.
2. Choose both branches on separate runs. Open Compass Question, borrow the
   compass, then revisit the question and check its changed options.
3. Open Voice in Fog. Check the unnamed/no-portrait exchange, alternate face,
   colored words and deliberate delays. Verify the final-page marker.
4. Pause during a delay or portrait movement, hold A, then resume. It should
   not skip ahead or replay a stale sound. Try B and C during an exchange.
5. At the ending prompt, wait: music should continue and the ending cue should
   stop. Confirm returns to stories. Repeat several conversations.
6. Test at PAL and NTSC refresh rates and alongside your game's actual graphics
   and sound load before claiming console-ready behavior.

Local NTSC ares feedback confirmed the controls/dialogue presentation and the
music/ending-cue fix. Automated checks cover model clocks and resource contracts;
they do not emulate VDP scanlines, Z80 playback or console timing.
