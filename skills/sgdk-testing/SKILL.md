---
name: sgdk-testing
description: Build repeatable SGDK game tests with seeded input replay, PAL/NTSC logic checks, checkpoint comparisons and bounded soak runs. Use for regression suites and automated QA, rather than investigating a single fault or measuring performance alone.
---

# SGDK Testing

Test the game's real update code with controlled inputs. Keep failures small
enough to replay and explain what each check actually exercised. SGDK does not
provide a game-replay or regression framework; the Lab's `lab_*` functions and
JSON reports are example game code.

## Choose the evidence you need

- [Replay contracts](references/replay.md): tick-indexed input, initial state,
  RNG ownership, bounded recording and explicit state checkpoints.
- [Regression and automated QA](references/regression.md): host and M68000
  execution, report comparison, failure artifacts and reviewed baselines.
- [PAL/NTSC and presentation](references/regions.md): display schedules, input
  edges, pause, frame misses and emulator/hardware checks.
- [Test Lab](assets/test-lab/README.md): a standalone seeded courier scene with
  reference playback, live recording and a deliberate regression switch.

Fit the existing project's architecture. A small test seam around input, time,
randomness or storage is often enough; do not replace the game with the Lab or
require every game to use its 60-tick clock, JSON schema or capacities.

## Keep failures repeatable

Record the build/asset identity, replay version, initial state, seed, logical
input sequence and timing policy. Reset pools, held-input baselines and RNG
streams before each run. Keep cosmetic randomness outside gameplay state.

Use the same rule functions for live play and replay. Check ordered semantic
fields or explicitly serialized bytes, never raw C structs, pointers, allocator
addresses or renderer handles. Compare useful checkpoints and report the first
divergence; a final hash alone makes investigation unnecessarily difficult.

Reject malformed or truncated tapes and reports. Stop recording cleanly at its
chosen capacity. A mismatch must fail the command/job; generating a new baseline
from the failing result must never be part of an ordinary test run.

## Prove more than the happy path

Exercise held/released buttons, pause and restart, scene handoffs, empty/full
pools, invalid actions and repeated save/load where relevant. Add an intentional
failure to prove the checker notices a change. Use boundary cases and seeded
soaks alongside recorded playthroughs; replay alone covers only its route.

Compile the real adapters and resources with `ewrogers/sgdk:latest`, pulled
before validation. Verify unfamiliar SDK calls in that image. The references
were checked against SGDK 2.11 and GCC 16.2.0. Execute pure rules on host and
base M68000 when useful, then check presentation in an emulator and hardware
when available. Host tests, CPU probes and equal regional traces cannot prove
VDP timing, audio quality, real input protocols or frame-rate headroom.

Other installed skills are optional: `sgdk-debug` for investigating failures,
`sgdk-performance` for measured frame costs, and `sgdk-release` for packaging
the exact tested candidate. This skill and its Lab work independently.
