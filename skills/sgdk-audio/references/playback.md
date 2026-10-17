# Music and effects

## Prepare matching resources

```text
XGM2 music "music.vgm"
WAV confirm "confirm.wav" XGM2 13300
```

ResComp converts VGM to the compiled XGM2 format and WAV to signed PCM with the
required padding/alignment. Generated headers provide the array sizes, so use
`sizeof(confirm)` where the array declaration is visible. Do not use the size
of a pointer passed into a helper.

XGM2 PCM buffers should be 256-byte aligned and have a length divisible by 256,
following the public API contract. Keep them valid and accessible throughout
playback. Static ROM resources are the normal choice. For half-rate content,
export with `XGM2 6650` and pass `halfRate = TRUE` when playing.

The resource keyword `XGM` is for the older XGM driver. It is not interchangeable
with `XGM2`, even though both begin from VGM source. Use the converted resource,
not raw WAV or VGM file bytes, in runtime playback calls.

## Start, pause, stop

```c
Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);
XGM2_setLoopNumber(0); /* Play once, even if the track contains a loop. */
XGM2_play(music);

/* In response to separate game events: */
XGM2_pause();
XGM2_resume();
XGM2_stop();
```

The calls above illustrate separate actions, not a sequence to issue together.
Commands cross to the Z80 and status may lag the request. Keep a requested
state and poll on later frames. A false `XGM2_isPlaying()` immediately after
play is not proof that the track finished.

Likewise, false can mean stopped, paused or naturally finished. Track pause and
stop requests yourself. For completion handling, observe the track playing
first, then recognize its end only while the game expects normal playback.
Sound Lab demonstrates this small state machine. It is a single-owner example,
not an acknowledgement protocol for arbitrary competing music controllers.

Music stop/pause and PCM effects are separate operations. Use
`XGM2_stopPCM(channel)` for an effect you own. A music pause menu may continue
UI sounds; a full game pause needs an explicit effect policy. There is no
generic PCM resume-at-the-old-sample-position API here.

## Loops and tracks

`XGM2_setLoopNumber(0)` disables repeat at the track's loop command; `1` allows
one repeat, and `-1` selects the SDK's default indefinite-loop setting. A track
needs a loop command for looping to take effect. It does not synthesize a loop
point in non-looping source music.

For a multi-track XGM2 resource, call `XGM2_load(blob)` once, then
`XGM2_playTrack(index)` with a valid track index. This can share embedded PCM
between tracks. Starting another track replaces the current music; it does
not create a second music player for a crossfade.

Loading a song selects its 50/60 Hz music tempo from the resource flag. The
driver adjusts for the console region. `XGM2_getElapsed()` reports **music
frames**, not milliseconds or necessarily displayed game frames. Keep normal
VBlank service and test PAL and NTSC rather than manually advancing music a
second time. The header's reference to `XGM2_setManualSync()` is stale; that
API is not present in SGDK 2.11. Do not copy the older XGM sync API into XGM2.

## Effects and priorities

```c
bool accepted = XGM2_playPCMEx(confirm, sizeof(confirm),
    SOUND_PCM_CH2, 4, FALSE, FALSE);
```

Priorities run from 0 to 15. On an explicitly chosen busy channel, a new request
can replace the old sound when its priority is equal or higher. A lower-priority
request returns false. Treat rejection as a normal result for optional sounds.
Do not retry in a busy loop until it succeeds.

`SOUND_PCM_CH_AUTO` lets the driver choose among its three channels. Use it only
when that agrees with the game's music/effect ownership policy. Explicit
channels make important sounds and music percussion easier to protect.

The bool return means the request was accepted, not that the user has heard it
or that its entire duration is guaranteed. A later accepted request can replace
it. The CHECK page sends its low-priority click and high-priority alarm to CH2.
Its optional contention mode uses CH1, alongside the track's sampled drums.
This is deliberately disruptive, not the recommended default channel policy.

`XGM2_isPlayingPCM()` takes **channel masks** such as `SOUND_PCM_CH2_MSK`, and
returns a mask of playing channels. Play and stop calls instead take the
channel enum `SOUND_PCM_CH2`. Confusing the two can query a different channel.

## Volume and transitions

`XGM2_setFMVolume()` and `XGM2_setPSGVolume()` accept 0–100. They change those
music components, not PCM sample loudness. Prepare PCM levels in the source
audio and leave mixing headroom; the XGM2 API is not a per-effect volume mixer.

`XGM2_fadeOutAndStop(frames)` fades FM/PSG and then requests music stop.
`XGM2_fadeOutAndPause()` and `XGM2_fadeTo()` support other transitions.
`XGM2_isProcessingFade()` reports the shared fade operation. These are not
independent per-track envelopes or a fade of all PCM output.

Let one system own transitions. A pending fade's final stop can otherwise stop
music started by another caller. The CHECK transport blocks new music requests and volume
changes during its fade, but still allows effects and an immediate stop. That
stop does not cancel the fade's pending final action; new music remains blocked
until it finishes. The MIX conductor demonstrates queuing/coalescing its next track until then;
see [runtime management](management.md).

Fade steps run through system VBlank processing. Sixty steps takes about one
second at 60 Hz and 1.2 seconds at 50 Hz. This is separate from music tempo
adaptation. Use positive, sensible durations and validate the target region.
Sound Lab uses the detected 50/60Hz refresh rate for its one-second fade;
it leaves the converted song's source tempo alone.

## Sources

- [SGDK 2.11 XGM2 API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/xgm2.h)
- [Status, priorities, tempo and fades in the implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/snd/xgm2.c)
- [Resource syntax](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt)
- [PCM channel enums and masks](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/sound.h)
- [System frame processing](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c)
