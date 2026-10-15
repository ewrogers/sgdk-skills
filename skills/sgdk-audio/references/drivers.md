# Choose a driver

## A useful default

Start a new SGDK game with **XGM2** when its music can be converted from Mega
Drive VGM. It plays FM/PSG music and mixes up to three PCM channels on the Z80.
It also provides FM/PSG volume envelopes and effect priorities.

| Driver | When it fits | Important constraint |
| --- | --- | --- |
| XGM2 | New games with music and sampled effects | Three PCM channels at about 13.3 kHz or 6.65 kHz; runtime SFX are PCM, not an FM/PSG effect sequencer. |
| XGM | Existing XGM projects or content | Four PCM channels at 14 kHz; different resource format and API names. |
| PCM | A single sample stream with selectable rate | One PCM channel, 8–32 kHz supported rates; no accompanying XGM music engine. |
| DPCM2 | Two compressed sample voices | Two 4-bit DPCM channels at 22.05 kHz, with quality/storage tradeoffs. |
| PCM4 | Four sampled voices with individual levels | Four 8-bit channels at 16 kHz; sixteen volume levels. |

These are alternatives, not layers to combine. Playback APIs can automatically
load their driver, replacing the previous one. Calling a PCM-only driver while
XGM2 music is running can disrupt that music. Changing drivers should be a
deliberate transition with audio stopped and any pending game-side fade handled.

For XGM2 initialization, use `Z80_loadDriver(Z80_DRIVER_XGM2, TRUE)`, not the
driver's internal `XGM2_loadDriver()` entry point. Several XGM2 setters return
without doing anything if the driver is not already loaded.

## What the hardware shares

The YM2612 provides six FM channels. Enabling its DAC replaces the sixth FM
channel; XGM2 mixes its PCM voices into that one DAC output. Its documented
maximum combination is five FM voices, three PCM voices and four PSG channels.
The PSG provides three tone channels and one noise channel. These are not
unlimited interchangeable voices.

The first XGM2 PCM channel is commonly used by music samples. Prefer channels
2 and 3 for effects, but check the actual track: this is a channel policy, not
a guarantee that music never uses the others. The shared SDK enum includes
`SOUND_PCM_CH4`; **XGM2 does not have a fourth PCM channel**. Some comments in
its 2.11 header still mention four, but the driver and format support three.

XGM2 does not provide a runtime FM/PSG SFX API. Use sampled effects, prepare
music content accordingly, or deliberately design another audio solution.
Writing FM/PSG registers underneath an active driver can corrupt its state or
be overwritten on its next update. Do not suggest raw register writes as a
drop-in extra sound channel.

## Z80, DMA and performance

The Z80 does the sequencing and mixing, but commands still require 68000 work
and shared-bus access. Holding the Z80 bus pauses its work. Avoid long critical
sections, repeated status reads and manual bus requests around public sound
calls that already manage access.

XGM2 installs SGDK's bus-protection signal when loaded. Keep normal SGDK DMA
and VBlank processing intact. Heavy graphics transfers can still reduce audio
headroom. Measure a demanding combination of music, effects and tile uploads,
not just an idle sound test.

`XGM2_getCPULoad(TRUE)` estimates **Z80 driver** load, not whole-game 68000
utilization. `XGM2_getDMAWaitTime(TRUE)` estimates Z80 time waiting for DMA.
Their `TRUE` mode averages eight frames. Use occasional debug sampling and
listening comparisons; adding a busy diagnostic display can change the workload.

## ROM, RAM and banking

PCM storage grows roughly with sample rate and duration: signed 8-bit mono at
13,300 samples/second needs about 13.3 KB per second before alignment. Half-rate
content can save space when its quality is sufficient. Merely playing a
full-rate sample with `halfRate = TRUE` slows and lowers it; it does not create
a smaller stored sample.

Count converted music, embedded samples, external effects and padding from the
built resources. XGM2's multi-track resource form can share samples across
tracks. Do not assume separate blobs automatically deduplicate everything.
Budget the driver's Z80 RAM separately from 68000 game RAM and ROM.

In banked cartridges, resources must remain accessible for the driver's whole
playback. `XGM2_load_FAR()` and `XGM2_play_FAR()` support SGDK's bank-aware music
path; this is not permission to remap arbitrary playing PCM data. Inspect the
chosen mapper and actual access path before promising support. The lab uses a
small, unbanked ROM.

## Sources

- [SGDK 2.11 XGM2 driver and format](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/xgm2.txt)
- [XGM2 API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/xgm2.h)
- [XGM2 implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/snd/xgm2.c)
- [XGM API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/xgm.h)
- [PCM](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/pcm/snd_pcm.h),
  [DPCM2](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/pcm/snd_dpcm2.h),
  [PCM4](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/pcm/snd_pcm4.h)
- [Z80 bus and driver API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/z80_ctrl.h)
