# Falling blocks and other resolution phases

## Falling Lanterns' illustrative policy

The 10×18 field uses 8×8 tiles. Settled cells hold zero or a piece color ID.
The active piece has a type, four-by-four footprint, x/y and quarter-turn index;
it is not written into settled cells until it locks. There are no hidden rows.
Spawn fails if the initial footprint overlaps settled cells, producing game over.

Seven four-cell shapes come from a shuffled seven-piece bag. An unsigned 32-bit
LCG supplies a reproducible sequence from a fixed demo seed. This is gameplay
randomization, not cryptography; the tiny modulo selection is not perfectly
unbiased. Record RNG state, bag order/index and next piece for exact replay or
an undo-enabled variant. Do not reseed each frame from input or video timing.

`bl_fits` tests the complete footprint. Translation and clockwise rotation commit
only if it fits. Rotation tests these offsets in order: unchanged, one cell left,
one right, two left, two right, one up. The square keeps its footprint. This is a
small custom kick policy, **not SRS or a commercial game's exact behavior**.
Adjust pivots/offsets to the requested game and test every shape at walls/stacks.

The Lab uses 60 logic ticks/second in PAL and NTSC. Gravity is one cell per 30
ticks; Down changes it to one per three ticks. Horizontal repeat waits 15 ticks
and then five between moves. A new directional edge acts immediately. A rotates,
B hard-drops and locks on that edge. Simultaneous left/right does not move.

A grounded piece locks after 30 ticks. Sliding/rotation on the floor does not
reset that timer; becoming ungrounded does. This avoids unlimited floor stalls
but is not a mandated feel. Specify gravity acceleration, lock-reset limits,
input buffering and pause behavior for the actual game. There is no hold slot,
competitive garbage, progression curve or persistent leaderboard in the Lab.

## Lock, clear, compact, spawn

1. Write the legal active footprint into settled cells.
2. Scan rows bottom-up, skipping full ones and copying survivors downward.
3. Blank all remaining rows at the top, without shifting a row twice.
4. Award 100/300/500/800 for one/two/three/four rows; update counters once.
5. Spawn the next piece and test top-out.

The scan bounds are always 180 cells. Hard drop/ghost search tests at most the
field height; rotation tests six candidates. A landing outline uses `bl_fits`,
not a second collision approximation. Logic commits clears immediately; the UI
briefly shows “ROW CLEAR!” and plays a finite cue. A game with a clear animation
should make that a named phase, suppress incompatible input and compact once
when it ends. Test whether buffered inputs carry into the new piece.

Host/base-68000 contracts cover all four rotations' four-cell area, bag
uniqueness, failed operation immutability, row compaction, four-row clear/score,
lock deadlines under floor movement, top-out and equal 600-tick results over
ten PAL/NTSC seconds. A 20,000-tick invariant soak exercises rotation/drop/restart.
These prove rules, not the desirability of the chosen timing or score values.

## Matching, cascades and routing

The same transaction principle applies, but do not import falling-block gravity
or Sokoban pushing into another puzzle family. For a match game, use explicit
phases such as Swap → Detect → Clear → Fall → Refill → Stable. Detect against a
consistent board, merge intersecting matches before scoring, and bound each
phase's work/effect storage. Seed refill deterministically. Decide what happens
at a cascade cap: finish over later ticks or apply a documented fallback; never
silently discard valid matches or spin until a random board becomes stable.

For circuit/routing puzzles, use a bounded visited worklist to prevent loops.
Keep the worklist scratch state separate from the committed circuit result.
Test cycles, exhausted capacity, tie ordering and rejected-command rollback.
These are guidance only; neither Lab claims to implement a match-three engine.
