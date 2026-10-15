# Sound Lab

A small XGM2 sound-test ROM with an original PSG arpeggio and two original PCM
effects. No tracker installation or downloaded music is needed. The music
fixture uses PSG, not an FM patch or sampled percussion; those still need
separate listening checks with representative game music.

Copy this folder to a **new** project directory and build there:

```sh
docker run --rm --pull=always --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/sgdk:latest build
```

In PowerShell, replace `$(pwd)` with `${PWD}`. Use Docker's Linux-container
mode. The image supports ARM64 and AMD64. The ROM is `out/rom.bin`; `build debug`
adds symbols and `build clean` removes generated output.

## Controls

| Button | Action |
| --- | --- |
| A | Start or restart music |
| B | Pause or resume music |
| C | Low-priority click on PCM channel 2 |
| Up | High-priority alarm on the same channel |
| Left | Toggle music looping |
| Right | Toggle FM/PSG music volume between 50% and 100% |
| Down | Fade music out over 60 VBlanks, then stop |
| Start | Stop music and the lab's effect channel |

During a fade, play/pause, loop and volume controls are temporarily ignored so
a later play request cannot be stopped by the old fade. Start still stops the
sound, but the pending fade must finish before music controls unlock. Effects
remain available. Accepted/rejected
counts describe driver requests, not a measurement of audible output.

## Structure

`src/main.c` owns the frame loop and only redraws status fields when they change.
`src/audio.c` owns command edges, transition state and the effect channel policy.
It waits for playback status before treating a later stop as completion, and
keeps pause requests distinct from natural completion.

The resources stay in ROM for their full playback. CH2 belongs to effects;
CH1 and CH3 are unused by this tiny music fixture. A real game must account for
its music's sample use. Do not assume another song has the same channel needs.

From the installed `sgdk-audio` skill folder, optional
`python3 scripts/make_lab_audio.py NEW_DIRECTORY` recreates the audio without
third-party libraries. It refuses to overwrite existing lab files. The VGM
generator follows the [VGM specification](https://vgmrips.net/wiki/VGM_Specification)
and emits a short original PSG sequence; it is not a general music exporter.

## Listening checklist

1. Start music; verify a clean loop. Turn looping off and check that completion
   increments once. Start again, pause and resume without a false completion.
2. Hold C: it should request one click, not restart every frame.
3. Trigger Up, then C while the alarm plays: the click should be rejected.
   Trigger C, then Up: the alarm should replace the click.
4. Change music volume or fade: the PSG changes, while PCM effects remain at
   their source level. Confirm the music can restart after a completed fade.
5. Compare PAL and NTSC. Music tempo adaptation and fade duration use different
   clocks; this lab's 60-VBlank fade takes longer on PAL.

For game integration, also listen with FM music, sampled percussion and heavy
graphics DMA. Check clipping, clicks, missing percussion and unstable playback.
Automated tests compile resources and test command logic; they do not execute
the Z80 driver or establish audible quality and timing.
