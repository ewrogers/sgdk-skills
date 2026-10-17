# Authoring music and effects for SGDK

## Begin with a sound plan

Ask about the game's direction, existing music/assets and intended driver.
Reserve musical roles before writing: lead, bass, harmony, rhythm and important
feedback. Those roles need not each occupy a dedicated voice. Count the busiest
passage together with effects; an empty intro is not the budget.

XGM2's normal mix supports five FM voices alongside DAC audio, PSG tone/noise
and three mixed PCM voices. Its runtime effect API plays PCM, not independent
FM/PSG sequences. Keep tracker FM/PSG authoring offline; do not write chip
registers underneath the driver to add an extra sound.

## Shape an FM instrument deliberately

The YM2612 has four operators per FM channel. Choose an algorithm to establish
which operators modulate and which reach the output. Algorithm 4, used by the
Lab's auditions, gives two two-operator stacks; it is an example, not the best
algorithm for every instrument.

- Carrier total level strongly affects audible level. Modulator level changes
  harmonic content; making every operator louder can make a patch harsher
  rather than simply louder. Feedback can add useful edge but needs listening.
- Multipliers/detune change harmonic relationships and beating. Compare the
  patch in its intended bass/lead range rather than judging one middle note.
- Attack, decay, sustain and release determine articulation. Check held notes,
  short gates, repeated notes and transitions to silence. A quiet release is
  still a live musical voice until the arrangement replaces it.
- Balance instruments in the arrangement. Do not normalize each solo to the
  same peak and assume the combined mix will be clear.

Sound Desk's Bell, Reed and Bass auditions use original register-defined patches
in VGM files. They are small executable explanations, not an instrument bank or
an implementation of every tracker effect. The PSG audition isolates tone; the
Drums audition isolates converted DAC streams. Music arrangements combine paths.

## PSG and sampled rhythm

PSG adds inexpensive contrasting tone/noise textures, but its volume steps and
waveforms differ from FM. Budget it as real voices rather than free extra
polyphony. Avoid leaving tone/noise enabled after the last note in an audition.

Short kick/snare samples can carry rhythm while leaving the FM arrangement room
to breathe. Balance their source gain with effects and reserve the PCM channels
the converted track actually uses. Reusing samples through a multi-track XGM2
resource can save ROM; separate song blobs do not guarantee deduplication.

## A tracker-to-ROM workflow

Furnace's official documentation supports VGM export with selectable chips,
format version and loop information. Use a normal Genesis/YM2612+PSG setup for
an initial compatibility probe. Do not assume an enhanced tracker playback mode
such as DualPCM transfers to SGDK's driver unchanged.

1. Keep the editable song and instrument definitions. Export a short phrase
   with each intended chip, a sample hit and a loop/one-shot ending.
2. Include loop information for looping music. A runtime loop count cannot add
   a missing VGM loop command. Check the export's loop-trail settings and listen
   across several repeats for state/envelope discontinuities.
3. Compile the real export through the selected image's ResComp/XGM2 tools.
   Inspect warnings and converted PCM use. A supported chip label alone does
   not prove support for every VGM opcode or tracker extension.
4. Listen in the ROM at both target refresh rates. Compare tempo, instrument
   balance, sample rate, first note, release and loop seam against the source.
5. Add the game's effects and representative graphics DMA, then compare again.
   Preserve the successful export/settings and a small reproducible test song.

```text
XGM2 stage_music "stage_music.vgm"
WAV item_pickup "item_pickup.wav" XGM2 13300
```

The Lab uses deterministic Python-generated VGM as an executable fixture. Its
builds do **not** certify a particular Furnace project, version or arbitrary
tracker export. Verify the actual artist's export rather than claiming the
fixture tested it. No tracker installation is required to build Sound Desk.

## Sources

- [Furnace export controls](https://github.com/tildearrow/furnace/blob/master/doc/2-interface/export.md),
  [YM2612 authoring and special modes](https://github.com/tildearrow/furnace/blob/master/doc/7-systems/ym2612.md)
  (official docs checked 2026-10-16; consult the installed tracker version).
- [SGDK 2.11 XGM2 format and limitations](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/xgm2.txt).
- [SGDK 2.11 ResComp syntax](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt).
