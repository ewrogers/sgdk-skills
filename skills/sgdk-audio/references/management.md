# Runtime music and effect management

## One music owner

`assets/sound-lab/src/director.c` is a portable example, not a required framework.
It returns commands to the SGDK adapter instead of touching hardware. The game
requests one of three scene tracks or a fanfare; it does not directly start a
second player.

- A scene request starts from idle, or fades the existing loop for half a second.
  Further requests coalesce to the latest desired track while the SDK fade owns
  its deferred stop. Nothing starts until `XGM2_isProcessingFade()` becomes false.
- Starting has its own state. A false status immediately after play does not end
  a jingle. Observe playback on a later frame, then recognize completion.
- A fanfare temporarily replaces scene music. Scene requests during it update
  the return destination. When it finishes, that scene restarts from its beginning.
  This is not a seek/resume or two-track crossfade; XGM2 has one music transport.
- If startup is never observed within two seconds, stop and surface a timeout.
  Do not retry forever. A new explicit scene request can recover. This timeout
  is a Lab policy, not an SGDK load-time guarantee.
- Stop during a fade clears future requests but still waits for the SDK's pending
  fade action before allowing a replacement. The adapter also handles PCM tails:
  it stops music-owned CH1 when replacing a track. FM/PSG fade is not a PCM fade.

Pause holds the conductor's state and cooldown clock. Resume waits for later
observed playback too, so a paused jingle is not mistaken for a finished one. The adapter pauses music
and explicitly stops effect channels; those stopped samples are not resumed.
The Lab declines pause/page changes during an SDK fade rather than pretending
`XGM2_pause()` freezes the system's VBlank fade task. Adapt that policy to a
project's scene/pause owner instead of adding conflicting clocks.

## A bounded effects batch

Sound Desk takes up to four distinct requests per serviced game update and coalesces
duplicates. It chooses the highest-priority ordinary request for CH2 and an alert
for CH3, leaving the converted music's percussion on CH1. Lower-priority queued
requests are dropped, not replayed after they are stale. Equal priority may
replace an existing lease; lower priority cannot. Per-cue cooldowns suppress
rapid retriggers. These channel assignments/priorities are examples, not hardware
partitions or a guarantee about arbitrary converted tracks.

The adapter still checks the actual `XGM2_playPCMEx` return. Admission and driver
acceptance are separate counters; rejection releases the estimated voice lease
without a busy retry. A lease is a bounded admission estimate based on sample
length, padded tail and command-service allowance, **not** proof of audible
completion or a sample cursor. If other code shares those channels, consolidate
ownership or design an explicit feedback/priority protocol.

The effect queue lives for one update. Leases/cooldowns advance at 60 logical
ticks per second through a 50/60-Hz adapter. Music startup timeouts count display
VBlanks and fades pass console-VBlank durations to SGDK. Converted songs keep
their 60-Hz source tempo; those are different clocks with different meanings.
The adapter measures elapsed `vtimer` VInts between updates. Leases/cooldowns
advance by that elapsed time with constant bounded work, rather than slowing
when a main-loop iteration misses a display frame. Startup deadlines likewise
use elapsed VBlanks. A normal one-frame wrapper remains available for callers
that already supply exactly one tick per display frame.

## Test the policy

The executable contracts check pending starts, latest-scene coalescing, fade
ownership, one-shot observation/return, stop, timeout recovery, pause, duplicate
bursts, priority rejection, driver rejection and normalized cooldown expiry.
O0/O3 host runs include a 20,000-frame invariant trace; the base-M68000 probe runs
the same contracts and a shorter digest for both target build modes. These tests
do not emulate Z80 command service or sound; listening remains necessary.

Primary API/implementation: [SGDK 2.11 xgm2.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/xgm2.h),
[xgm2.c](https://github.com/Stephane-D/SGDK/blob/v2.11/src/snd/xgm2.c).
