---
name: sgdk-audio
description: Author and integrate SGDK FM/PSG music and sampled effects, improve audio quality, and manage channel priorities, jingles and music transitions. Use for tracker/VGM workflows, audible sample problems, runtime audio ownership and driver integration.
---

# SGDK Audio

Help games play music and sound effects without losing track of driver, channel
or bus ownership. Keep the project's existing loop and audio architecture.

Follow the game's requested sound direction. Lab instrumentation, melodies,
visuals, loudness and timing are examples, not a required genre or aesthetic.
Read [boundaries](references/boundaries.md) when crossing asset importing,
scene flow, banking or hardware testing. Reuse the game's input, scene and clock
systems rather than introducing another framework.

## Author for the target

Keep editable tracker/session sources alongside exports. Start with a short
phrase that exercises the intended FM instruments, PSG and percussion; compile
it through the selected converter before producing a full soundtrack. Check
loop boundaries, held/released notes, register range and sample needs. A tracker
preview and exported VGM may not use identical mixing behavior.

- [Authoring and export](references/authoring.md): FM patch decisions, PSG,
  percussion, tracker export and instrument/layer auditions.
- [Sound quality](references/quality.md): headroom, edge clicks, sample rate,
  DC offset and a non-destructive WAV inspection/preparation helper.

Reserve channels according to converted content, not only a planned chart.
Audition music alone, effects alone and their combined mix before blaming DMA.

## Choose the driver first

Check the existing driver and resource declarations before adding calls. For a
new game, prefer XGM2 unless the music format, sample requirements or existing
code gives a reason to choose another driver. Do not mix XGM and XGM2 APIs or
switch Z80 drivers just to play an effect.

- [Driver choice and hardware](references/drivers.md): XGM2, XGM and PCM-only
  alternatives, FM/PSG/DAC sharing, Z80 ownership and performance.
- [Music and effects](references/playback.md): playback, pause, completion,
  looping, priorities, volume, fades and resource lifetime.
- [Sound Lab / Sound Desk](assets/sound-lab/README.md): arrangements, instrument
  auditions, sample-edge comparisons, a scene conductor and region/DMA checks.
  The optional [audio generator](scripts/make_lab_audio.py) recreates its assets
  in a new folder using only Python's standard library.
- [Music fixtures and region checks](references/music-and-regions.md): the
  tested VGM path, source tempo, percussion ownership and PAL/NTSC comparison.

Use the latest published SGDK toolchain:

```text
ewrogers/sgdk:latest
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

## Manage requests, not just API calls

Use one owner for music transitions. Queue/coalesce scene requests while a fade
owns its deferred stop. Recognize a jingle finishing only after observed playback,
then restore the latest requested scene track. State whether restoration restarts
or resumes; XGM2 does not provide arbitrary playback-position seeking here.

Bound effect bursts and queues, coalesce duplicate events, apply retrigger
cooldowns and define channel-stealing priorities. Treat a rejected optional cue
as a normal result. Do not replay stale sounds later just because a channel freed.
[Runtime management](references/management.md) explains the tested conductor,
its timeout, channel leases, pause rules and deliberately small scope.

## Validate what matters

Build actual music and WAV resources and a release ROM. Test command ordering,
button edges, pause versus completion, effect rejection and transition ownership.
Use the selected SDK's real headers as well as any recording test fixture.

Then listen in an emulator or on hardware: verify loop boundaries, pause/resume,
sample priority, music percussion alongside effects, fades, and audio under heavy
graphics DMA. Check both regions when targeted. Report builds, logic tests and
listening checks separately. Compilation cannot establish sound quality, timing
or absence of clicks.
