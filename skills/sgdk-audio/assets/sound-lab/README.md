# Sound Lab

An original XGM2 sound-test ROM with an FM melody/bass, quiet PSG tone/noise
accompaniment and sampled kick/snare percussion. A short four-second phrase
loops while you try effects, pause, volume, fades and graphics transfers.
No tracker or downloaded music is needed.

Copy this folder to a **new** project directory, then build there:

```sh
docker run --rm --pull=always --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/sgdk:latest build
```

In PowerShell, replace `$(pwd)` with `${PWD}` and use Docker's Linux-container
mode. The image supports ARM64 and AMD64. Load `out/rom.bin` in a Mega Drive
emulator. `build debug` adds symbols; `build clean` removes generated output.

## Controls

| Button | Action |
| --- | --- |
| A | Start or restart music |
| B | Pause or resume music |
| C | Low-priority click on the selected PCM channel |
| Up | High-priority alarm on that channel |
| Left | Toggle music looping |
| Right | Toggle FM/PSG music volume between 50% and 100% |
| Down | Fade FM/PSG for about one second, then stop music |
| Start | Stop music and both lab-owned PCM channels |
| Hold C, press Left | Switch effects between normal CH2 and contested music CH1 |
| Hold C, press Right | Toggle 2 KiB of graphics DMA per display frame |

In the usual ares mapping, A/B/C are J/K/L, Start is Return, and the D-pad
uses arrows/WASD. The lab does not modify emulator settings. Hold C before
pressing the chord direction; the new Left/Right press performs the switch.
Pressing both together also works. Release before playing the next effect.

## What to listen for

Normal mode puts drums on CH1 and effects on CH2, leaving CH3 free. The click
has priority 1 and the alarm 15. On CH2, an alarm should replace a click; a
click requested during an alarm should be rejected. The counters show accepted
and rejected requests, not how many samples played to completion.

CH1 contest mode deliberately breaks that separation. A click may be rejected
during a drum or play in a gap and be cut off by the next beat. The alarm can
interrupt percussion. This is a teaching switch, not the recommended channel
policy for a game. Return to CH2 for clean percussion alongside effects.

Music volume and fades affect FM/PSG, not PCM loudness. Drum/effect tails may
remain audible during a fade. Music pause is also separate from effect control;
it does not provide a general resume-at-the-old-sample-position API for effects.
Start explicitly stops the lab's owned channels.

The DMA switch uploads persistent ROM data into 64 unused pattern tiles. It
adds 2,048 bytes per display frame without changing visible graphics. Compare
playback with it off/on. This is a controlled audio/graphics interaction check,
not a worst-case frame-rate benchmark or proof for every hardware revision.

The lab enables SGDK's optional short DMA delay to give the Z80 time to access
the main bus before transfers. This costs roughly three VBlank scanlines.
In the NTSC ares check, music alone sounded unchanged with DMA off/on after
enabling the delay. This does not guarantee clean audio for every workload.
Wait for the chord's click to finish before comparing music, and test the alarm
separately: adding an effect changes the mix even without a DMA problem.

## PAL and NTSC checks

The screen reports the detected console refresh rate. The song remains a
60Hz source and XGM2 adjusts scheduling for the region. One musical loop
should stay roughly four seconds in either mode. The elapsed counter shows
source music frames, increasing about 240 per four seconds of playback; the
display refreshes twice a second and shows only the counter's low 16 bits.

`Last 4s` measures the actual source-frame change across four seconds of steady
playback. Expect about 240. PCM Hz and missed music frames are separate driver
diagnostics; correct tempo alone does not prove that samples sound clean.
PCM Hz displays `N/A` on PAL because SGDK 2.11's byte-sized per-frame sample
counter cannot represent the nominal 13.3kHz rate at 50Hz. Missed frames is
also a wrapping byte counter; use these readings alongside listening checks.

Fades use the detected 50/60Hz display rate so they last about one second.
This is separate from song-tempo adaptation. Do not change the song to 50Hz
just because the hardware is PAL.

To prepare a region-specific header **inside a disposable lab copy**:

```sh
docker run --rm --pull=always --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/sgdk:latest python3 tools/region_header.py pal
docker run --rm --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/sgdk:latest build clean
docker run --rm --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/sgdk:latest build
```

Use `ntsc` for a US-only header, or `auto` for the SDK's usual JUE header.
The helper copies the selected SDK's header and changes only its region field.
It replaces its own generated header or an unchanged SDK copy, and refuses
to overwrite a custom header. Generated headers stay out of Git.

Reload/power-cycle the ROM after changing region. Some emulators can force a
region despite the cartridge header, so verify **DETECTED PAL / 50 HZ** or
**DETECTED NTSC / 60 HZ** on screen. A region-header build alone does not prove
which timing the emulator used.

## Listening checklist

1. Start music. Listen for FM notes/bass, quieter PSG and alternating drums.
   Check the loop boundary, then disable looping and confirm completion counts once.
2. Pause/resume without a false completion. Restart and check the elapsed
   counter resets. It should keep counting across loops.
3. Play click/alarm in normal mode, then try CH1 contest mode. Confirm that
   contention is audible and returning to CH2 restores the normal separation.
4. Change volume and fade. FM/PSG should change; PCM is not a master fade.
   Start should stop the owned audio, and restarting should work afterward.
5. Enable graphics DMA and repeat. Listen for dropouts, clicks or unstable
   percussion rather than assuming a successful build proves playback.
6. Repeat with detected PAL and NTSC timing. Compare the four-second loop and
   one-second fade with DMA off/on. Physical-console testing remains separate.

## Assets and tests

From the installed skill folder, `python3 scripts/make_lab_audio.py NEW_DIRECTORY`
recreates the original VGM and WAV assets using Python's standard library. It
refuses to overwrite existing files. The generator is a small diagnostic
fixture, not a tracker exporter. See the skill's music/region reference for its
VGM stream and FM instrument choices.

Host tests execute command handling, priorities, chords, region-aware fades
and generated VGM timing/structure. Container tests compile the real SDK APIs,
build release/debug plus PAL/NTSC ROM headers, and inspect the linked music
blocks, two embedded drum samples, resource alignment and sample padding.
They do not execute the Z80 mixer or replace listening checks.
