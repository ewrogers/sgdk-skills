# Regression checks and automated QA

Start with a failure someone could recognize: a held button fires twice, a
restart leaves an enemy active, a save loses inventory, or a level exit repeats.
Keep that case as a small executable test. Add replay coverage for a complete
route once the individual rules are understandable.

## Use layers of checks

| Check | Useful evidence | What remains outside it |
| --- | --- | --- |
| Host rules | State changes, rejected actions, bounds, replay and recovery | Target arithmetic/ABI, SDK devices |
| Compiled adapters/resources | Actual SDK signatures, linked dimensions and budgets | Correct runtime presentation |
| M68000 logic probe | Target instruction execution and deterministic state | VDP, Z80, interrupts and real-time speed |
| Emulator playthrough | Visible/audible flow and controller interaction in that emulator | Physical-console behavior |
| Hardware session | Behavior on the recorded console/cart/controller setup | Untested hardware combinations |

Keep host stubs narrow. A test double records your contract with the SDK; it
does not prove the SDK implements that contract. Compile real calls in both
release and debug. For compiler-sensitive code, use the same target options
and compare actual target execution with independent expectations.

The optional Lab probe runs with Unicorn 2.1.4 explicitly set to **M68000**.
It enters the probe function directly with a bounded stack and instruction
budget. There is no SGDK startup, live VBlank, DMA, sound or controller device.
Only use it for functions whose dependencies are explicitly initialized. Never
point this harness at an ordinary game and interpret failure as a console bug.

## Make automation fail usefully

The Lab's [QA helper](../assets/test-lab/tools/qa.py) emits a trace, validates
its identity and required checkpoints, and compares against a reviewed file.
It exits **0** for equality, **1** for a regression and **2** for invalid input,
missing tools or execution failure. Its `--fault` option should produce a
regression, proving that a green result is not an empty test.

Run commands with argument arrays and bounded execution time. Preserve the
return code. On failure, retain the input tape, expected/actual reports, build
log, ROM/ELF, tool versions, region and first differing checkpoint. Include
seed and iteration count for randomized tests. Do not hide a failing check by
retrying until a run happens to pass.

For a game's CI, run cheap host checks first, then build/execute the target
checks on the required architectures. Use `ewrogers/sgdk:latest` as the default
and record what was actually pulled. Compare outputs across ARM64 and AMD64;
matching files show toolchain reproducibility, not independent gameplay proofs.

The Lab helper can be copied and adapted into a QA job. Replace its courier
model/trace fields with the game's update seam and state contract. The schema
and runner are deliberately small examples, not a generic SGDK testing API.

## Review baseline changes

Keep expected traces in source control. A new expected file requires an
intentional behavior change, a readable explanation and review of the changed
state—not simply “the current output differs.” The helper refuses existing
output paths and never rewrites a baseline during comparison.

Use independent invariants as well: no negative stock, one reward per victory,
all active objects inside valid bounds, or a known number of ticks during a
pause. Two runs of the same incorrect function can agree perfectly.

## Presentation and agent-assisted QA

If an emulator exposes documented automation, drive it through that interface
and capture named checkpoints. Otherwise use the ROM's built-in playback;
synthetic host keypresses are susceptible to focus and mapping differences.
Do not edit the user's emulator settings merely to make automation convenient.

Keep logic traces separate from image/audio comparisons. A screenshot baseline
needs the same emulator version, region, scale, filter, overscan and capture
point. Changes in capture settings are not automatically game regressions.
Review native pixels and transitions, and listen to audio. Neither a matching
logic digest nor an agent saying “looks fine” replaces those checks.

## Sources

- [SGDK 2.11 build rules](https://github.com/Stephane-D/SGDK/blob/v2.11/makefile.gen)
  for the selected release/debug options.
- [Unicorn 2.1.4 M68K definitions](https://github.com/unicorn-engine/unicorn/blob/2.1.4/include/unicorn/m68k.h)
  for explicit CPU-model selection. Unicorn is an optional CPU test dependency,
  not the emulator used for visual acceptance.
