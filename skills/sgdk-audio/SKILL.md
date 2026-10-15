---
name: sgdk-audio
description: Implement SGDK music playback, sound effects, channel priorities and audio transitions, and choose a suitable sound driver. Use for runtime audio behavior and driver integration rather than source-art preparation or general build debugging.
---

# SGDK Audio

Help games play music and sound effects without losing track of driver, channel
or bus ownership. Keep the project's existing loop and audio architecture.

## Choose the driver first

Check the existing driver and resource declarations before adding calls. For a
new game, prefer XGM2 unless the music format, sample requirements or existing
code gives a reason to choose another driver. Do not mix XGM and XGM2 APIs or
switch Z80 drivers just to play an effect.

- [Driver choice and hardware](references/drivers.md): XGM2, XGM and PCM-only
  alternatives, FM/PSG/DAC sharing, Z80 ownership and performance.
- [Music and effects](references/playback.md): playback, pause, completion,
  looping, priorities, volume, fades and resource lifetime.
- [Sound Lab](assets/sound-lab/README.md): a standalone sound-test ROM with
  original PSG music and PCM effects, controls and a listening checklist.
  The optional [audio generator](scripts/make_lab_audio.py) recreates its assets
  in a new folder using only Python's standard library.

Preserve an existing toolchain pin. The tested default is:

```text
ewrogers/sgdk:2.11-gcc16.2.0-binutils2.47@sha256:7c43a41e5bfa177b59b938d56bf496419df6b55778a6b5c68c629591ccc21b34
```

It provides Linux ARM64/AMD64 tools, including through Docker Desktop. Verify
unfamiliar APIs against `/opt/sgdk/inc/snd`, `/opt/sgdk/src/snd` and
`/opt/sgdk/bin/rescomp.txt`. XGM2's format/driver notes are in
`/opt/sgdk/bin/xgm2.txt`. Prefer implementation evidence when an old header
comment contradicts the current code.

## Integrate deliberately

Give each PCM channel a purpose and decide which effects may interrupt others.
Keep music controls separate from effect controls: pausing music or fading its
FM/PSG parts is not a master pause or master volume for PCM effects.

Read cached input once per frame and issue effects on events, not every held
frame. Keep sample data resident for the full playback and satisfy the driver's
alignment, size and rate requirements through ResComp. Preserve normal
`SYS_doVBlankProcess()` service and SGDK's bus protection.

Keep game-side state for requested playback and pause. Driver commands can be
pending when status is queried. Do not announce completion merely because
`XGM2_isPlaying()` is false immediately after requesting play.

## Validate what matters

Build actual music and WAV resources and a release ROM. Test command ordering,
button edges, pause versus completion, effect rejection and transition ownership.
Use the selected SDK's real headers as well as any recording test fixture.

Then listen in an emulator or on hardware: verify loop boundaries, pause/resume,
sample priority, music percussion alongside effects, fades, and audio under heavy
graphics DMA. Check both regions when targeted. Report builds, logic tests and
listening checks separately. Compilation cannot establish sound quality, timing
or absence of clicks.
