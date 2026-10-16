# Make a useful replay

A replay should answer “what happened differently?” without guessing what the
player pressed. Record held button masks at the boundary where game rules
consume them. Derive pressed/released edges once from consecutive snapshots.
Do not combine recorded input with the live pad during playback; reserve a
separate transport control if the tester needs pause or restart.

## Own the starting state

Keep a small manifest: game/replay schema, level, difficulty, initial save or
checkpoint, seeds and any setting that changes the rules. Store content IDs,
not ROM addresses. In CI, keep the ROM hash, compiler/SDK versions and asset
revision next to the trace. A tape's hash identifies the inputs, not the build.

Reset counters, free-list order, pools, pending transitions, input history and
RNG state. Use a stable iteration order and define ties. Avoid unspecified C
evaluation order, signed overflow and reading uninitialized bytes. Host `int`
and target `int` need not have the same range; use explicit types and widen
intermediate arithmetic where needed.

SGDK has `setRandomSeed(u16)` and `random(void)`, but a global stream also
depends on every caller. A particle effect must not change the next enemy's
decision. Separate gameplay/cosmetic streams or record the relevant results.
Test Lab uses a small unsigned, game-owned generator; it is not an SDK API or
a required RNG algorithm.

## Use the game's update function

Keep sampling and simulation separate. A minimal seam looks like:

```c
u16 held = replaying ? replay_input(tick) : JOY_readJoypad(JOY_1);
game_step(&game, held);
```

Here `replay_input` and `game_step` belong to the game. Read `JOY_readJoypad`
from SGDK's cached state; do not repeatedly call `JOY_update` from individual
entities. Preserve separate player/device data if the game uses more than one
pad or mouse deltas. The Lab's mask tape supports one pad only; adapt the
record format for other devices and consume mouse motion exactly once.

Record enough information for the chosen time policy. Tick-indexed tapes are
good for repeatable logic. For diagnosing real input latency or missed frames,
also retain display-frame samples, region and frame-miss events. They answer
different questions. A logical tape cannot reconstruct physical sampling time.

## Bound recording and compare state

Run-length encoding saves RAM when held input stays unchanged. Its worst case
is a change every tick: impose a run count and duration, and report why recording
stopped. Never continue simulation with unrecorded inputs and later call the
partial recording a complete replay.

Hash explicit fixed-width fields in a fixed byte order. Include RNG, input
history, cooldowns and occupied pool slots if they can affect future behavior.
Leave addresses, padding and presentation handles out. Test Lab uses FNV-1a
over explicit 16-bit words, plus readable state fields at each checkpoint.
Its 32-bit digest is a quick regression signal, not a collision-free proof;
export more fields or a stronger digest for higher assurance.

A checkpoint mismatch identifies an interval, not necessarily the exact bad
instruction. Re-run with every-tick state output around that interval. Keep
expected and actual field values so the report is useful without a screenshot.

## Verified sources

- [SGDK 2.11 joypad API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/joy.h):
  cached button reads, masks and explicit refresh.
- [SGDK 2.11 frame processing](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c):
  `SYS_doVBlankProcessEx` refreshes the input snapshot.
- [SGDK 2.11 random helpers](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/tools.h):
  seed and random-value signatures.
