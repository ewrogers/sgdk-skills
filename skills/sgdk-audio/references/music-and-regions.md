# Music fixtures and region checks

## Prove all three sound paths

A PSG-only tune cannot demonstrate an FM instrument or sampled percussion.
Sound Lab's four-second original phrase has two YM2612 voices, a quiet PSG
tone/noise accompaniment, and alternating sampled kick/snare hits. Standalone
click/alarm effects play alongside it. It is a diagnostic phrase, not a tracker
or a general music-export tool.

`scripts/make_lab_audio.py` generates reproducible VGM 1.60 and WAV files using
integer arithmetic. FM channels 1 and 2 use algorithm 4 with an audible
modulator/carrier pair. The second operator stack is muted. This exercises
actual FM modulation, note changes and key-on/key-off commands. The source
contains no borrowed music or instrument-bank dependency.

The VGM stores unsigned 8-bit drum data in one type-0 data block. DAC stream
commands select the YM2612 DAC register, bank, 13,300Hz rate and explicit
offset/length for each drum. ResComp's XGM2 conversion extracts, resamples and
pads the samples for the Z80 driver. Do not pass those VGM bank bytes directly
to the signed-PCM runtime API.

The tested command path uses `0x90`/`0x91`/`0x92` for stream setup and `0x93`
for an offset-and-length start. This is a deliberately small supported fixture,
not a claim to implement every VGM command. A real tracker export must still
pass the selected converter and a listening check.

## Keep source tempo separate from refresh

The track uses 240 source frames at 60Hz, or four seconds per loop. XGM2 loads
the source tempo from its converted resource flag and adjusts scheduling for
the detected console region. Do not set a 60Hz song to 50 merely because the
console is PAL; that changes musical tempo instead of preserving it.

`XGM2_getElapsed()` counts source music frames. It resets on play, continues
across loops, and eventually wraps its 24-bit counter. The lab shows its low
16 bits for a short diagnostic session; it is not an unbounded wall clock.
Over roughly four seconds of steady playback it should advance about 240 in
both regions. Status reads and displayed updates are asynchronous, so allow
for the display's half-second refresh rather than expecting sample accuracy.

The `Last 4s` reading measures the change across four seconds of uninterrupted
playback. About 240 source frames confirms scheduling, not clean PCM output.
The separate PCM-rate and missed-music-frame readings help investigate audible
problems. They are driver diagnostics, not a recording of the output waveform.

The lab hides PCM Hz on PAL. SGDK 2.11 computes it from an 8-bit samples-per-frame
counter; nominal 13.3kHz at 50Hz exceeds that counter's range. A wrapped reading
is not evidence that the audio is playing at that low rate. The missed-frame
diagnostic is also an 8-bit counter, not an unlimited error total.

Fades have another clock: their duration is in console VBlanks. The lab uses
50 or 60 steps for a roughly one-second fade. FM/PSG volume changes and fades
do not provide a master volume for the sampled drums/effects. Listen for drum
tails during transitions instead of assuming all audio must follow the same
envelope. Stop all owned channels explicitly when that is the intended policy.

## Hear channel contention deliberately

The converted fixture's drums use the first PCM channel. Normal effects use
CH2; CH3 remains free. The optional CH1 contest mode deliberately routes
effects onto the music's drum channel to make a bad ownership policy audible.

In SGDK 2.11's driver, ordinary music PCM has priority 7; high-priority music
commands can use 15. The lab's click requests priority 1 and its alarm 15.
A click can be refused during a drum or accepted in a gap and replaced by the
next drum. A high-priority alarm can interrupt percussion. An accepted request
does not guarantee uninterrupted playback.

Those priorities explain this fixture and driver, not every possible audio
driver. Prefer explicit channel ownership in a game. Do not solve contention
by writing raw YM2612/PSG registers underneath XGM2 or by repeatedly retrying a
rejected effect.

## Add a controlled graphics transfer

The lab can queue 2,048 bytes of unused pattern data per display frame. Its
source is persistent ROM data, its destination stays below font storage, and
normal `SYS_doVBlankProcess()` drains the transfer using SGDK's bus protection.
It does not alter visible text, palettes or the active audio driver.

The lab also enables `Z80_setForceDelayDMA(TRUE)`. SGDK's optional delay gives
the Z80 roughly three scanlines before the queued transfer, reducing available
VBlank bandwidth. SGDK documents this mitigation for altered PCM playback when
PSG accesses the main bus. In the NTSC ares check, music alone sounded unchanged
with DMA off/on after enabling the delay, with 240 source frames per four seconds.
That result covers this fixture and emulator, not every game or console.

Compare music alone first. The C chord can play a click before switching DMA;
wait for that effect to finish. Up adds an alarm, so a difference while pressing
it is not enough to identify a DMA problem. Then compare the same effects in
both modes as a separate check.

Compare music with this switch off/on in both regions. This exercises one
specific transfer load; it does not certify all DMA budgets, busy-game frame
times or every console revision. Keep the driver's normal protection and
VBlank service rather than masking problems with a custom bus/reset wrapper.

## Sources

- [VGM specification](https://vgmrips.net/wiki/VGM_Specification): chip writes,
  sample data blocks, DAC stream commands, waits and relative loop offsets.
- [SGDK 2.11 XGM2 format/driver notes](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/xgm2.txt).
- [Converter VGM parsing](https://github.com/Stephane-D/SGDK/blob/v2.11/tools/xgm2tool/src/sgdk/xgm2tool/format/VGM.java).
- [Source tempo, elapsed counter and fades](https://github.com/Stephane-D/SGDK/blob/v2.11/src/snd/xgm2.c).
- [Z80 music PCM priority](https://github.com/Stephane-D/SGDK/blob/v2.11/src/snd/xgm2/drv_xgm2_ym_fct.i80).
- [Optional DMA delay and its bandwidth cost](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/z80_ctrl.h).
- [VBlank and DMA bus handling](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c).
