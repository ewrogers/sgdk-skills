# Sound quality and controlled comparisons

## Separate the causes

First compare the source and converted sample alone. Then compare music alone,
music plus one effect, a realistic burst and the same playback with bounded
DMA. Wait for the button chord's effect to finish before judging “music alone.”
A changed mix is not automatically a DMA dropout; correct source tempo does
not prove clean PCM either.

Keep gain/headroom in the source mix. Peaks near full scale in several voices
can produce a harsh mix even when each source has no clipped samples. FM/PSG
music volume APIs do not provide per-effect PCM gain. Use intentional sample
levels or prepared variants when the selected driver has no suitable mixer API.
Avoid prescribing one loudness target or processing chain for every game.

## Clicks, noise and rate

A nonzero start/end or abrupt loop seam can create a click. Short edge ramps,
zero-crossing edits or a better loop point can help; an audible transient may
also be intentional. Do not fade away an impact merely to satisfy a metric.
DC offset wastes headroom and may worsen edges. Inspect and listen after edits.

Use proper low-pass filtering when reducing sample rate. Changing a WAV header
or selecting `halfRate` on a full-rate resource changes speed/pitch; it is not
high-quality resampling. ResComp conversion must match the runtime rate flag.
Keep high-quality editable sources, and avoid repeated lossy/8-bit conversions.
Dither at final bit-depth reduction can trade distortion for noise; audition it
rather than applying it universally, especially to already noisy effects.

## Small non-destructive helper

From the installed skill:

```sh
python scripts/pcm_tool.py inspect path/to/effect.wav
python scripts/pcm_tool.py prepare path/to/effect.wav path/to/new_effect.wav --gain-percent 75 --fade-ms 3 --remove-dc
```

Inspection accepts mono/stereo 8/16-bit uncompressed WAV and reports rate, frame
count, peak/RMS level, mean offset, endpoint magnitude and samples at the numeric
extremes. Those are diagnostics, not a sound-quality score or proof of clipping.
Amplitude metrics use a signed 16-bit scale even for 8-bit input.

Preparation deliberately accepts **mono 16-bit PCM only**, preserves rate/frame
count, and writes a new file. Gain is explicit, DC removal optional, and an edge
ramp is bounded by half the sample. It clamps at the numeric range; excessive
gain can still clip, so inspect the result. It does not resample, denoise,
compress, spatialize or turn stereo into mono. Mix stereo deliberately in an
audio editor and check phase cancellation. The small SFX helper limits decoded
PCM to 8 MiB and never overwrites source/existing output.

## Sound Desk comparisons

The SFX page provides four original pairs with identical oscillator/pitch and
length, one with an attack/decay envelope and one with abrupt edges. A plays
clean; B plays abrupt. Neither is a universal sound-design prescription. They
are bounded, finite samples with headroom, useful for hearing the difference.

The UI retains its tilemap and uploads only changed rows. Holding a sound
button does not retrigger it or force a full-screen upload. This matters: an
unnecessarily busy diagnostic UI can itself change bus contention. CHECK adds
an explicit 2048-byte/frame transfer, separate from normal UI changes.

The optional SGDK short DMA delay is enabled, as in the previous Sound Lab.
It is mitigation, not proof of clean output across hardware revisions. Repeat
critical listening on actual hardware; analog output, filtering and revisions
are outside a CPU model's evidence. See [driver/bus guidance](drivers.md) and
[region checks](music-and-regions.md).

Sources: [XGM2 volume, PCM and DMA behavior](https://github.com/Stephane-D/SGDK/blob/v2.11/src/snd/xgm2.c),
[resource conversion](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt).
