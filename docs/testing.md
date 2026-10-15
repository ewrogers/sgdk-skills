# Testing the skills

The tests exercise the helpers and compile actual SGDK code. They do not ask a
language model to judge whether the instructions sound right.

## Quick checks

Run the Python tests from the repository root:

```sh
python3 -m unittest discover -s tests -v
```

Python checks use the standard library. Input, graphics, audio, gameplay, save
and performance behavior tests also need a C17
compiler (`clang`, `cc` or `gcc`) to execute the actual example modules with a
fake SDK. They skip locally if no compiler exists; CI requires one. CI runs
host checks on macOS, Linux and Windows without requiring Docker. Docker tests
are skipped unless explicitly enabled.

The planning tests check supplied budget arithmetic, H32/H40 sprite limits,
RAM/VRAM boundaries, ROM padding and mapping declarations. They exercise
unknown costs, partial coverage, invalid input, CLI exit codes and a copied
standalone helper. They do not predict game feasibility, runtime memory safety
or CPU performance. The example worksheet is deliberately incomplete.

The installation job runs `python3 tests/distribution.py` with Node.js 24. It
uses the pinned Skills CLI 1.7.2 to discover and copy each skill into a scratch
project, then checks that its references, scripts and original assets arrived.
It does not install into your home directory. Relative-link checks also reject
missing resources and dependencies on sibling skills or repository-root files.

Installation tests keep telemetry disabled so repeated CI runs do not affect
directory install counts. The repository's `skills.sh.json` groups the public
directory page; a host check keeps those groups in sync with the shipped skills.

## Build real ROMs

With Docker running in Linux-container mode, enable the resource and debugging
checks, then test the project helper:

```sh
SGDK_SKILLS_INTEGRATION=1 python3 -m unittest discover -s tests -v
python3 tests/integration.py
```

In PowerShell, set `$env:SGDK_SKILLS_INTEGRATION = "1"` before running the Python
command. Use `python` if that is the Python command on your machine.

The integration runner copies `sgdk-project` into a separate folder before using
it. This catches accidental dependencies on another installed skill or this
repository. It starts with SGDK's official Hello World source, builds release
and debug ROMs, reads debug symbols with GDB, cleans outputs and verifies that
invalid C fails the build. It also checks paths with spaces, command arguments,
exit codes and host ownership of generated files where available.

The asset tests build a small resource example and check rejected input. The
debugging tests compile a profiling example and resolve a symbol back to its C
source. Successful compilation proves that the examples match the pinned SDK's
API. It does not prove correct visuals, sound, frame timing or hardware behavior.
Those still need emulator and, when available, console testing.

Input tests exercise press/hold/release, alternate action bindings, independent
player slots, mouse accumulator consumption and direction normalization,
adapter configuration and state baselines after reset/type changes. Docker
checks verify the fake SDK's constants and API signatures against real SGDK,
then compile and link Input Lab in release and debug modes. These checks do
not execute SGDK's hardware protocols. Follow the skill's device checklist for
emulator or console validation, including its documented SGDK 2.11 caveats.

Graphics tests exercise the lab's tile-budget boundaries, allocation failures,
animation request edges and completion latch, pause state, parallax wrap,
palette ownership, copied temporary transfers and teardown ordering. The fake
SDK records calls; it does not emulate SGDK's animation scheduler or the VDP.
Art tests check reproducibility, indexed pixels, PNG checksums and overwrite
protection. Docker checks compile real APIs and both build modes, then use GDB
to read the compiled sprite's dimensions, animation counts and per-frame holds.
The scrolling lab uses wrapping planes; the large-map and line/column guidance
has API compatibility checks, not an executed map-streaming test. Follow the
lab's visual checklist to check the screen and frame timing separately.

Audio tests execute Sound Lab's command edges, pending playback status,
pause/resume, completion, effect acceptance/rejection and fade ownership against
a recording SDK fixture. They do not simulate the Z80 mixer or test its priority
algorithm. Asset checks validate the original VGM's timing/loop commands, WAV
format and reproducibility, and overwrite protection. Docker checks API contracts,
builds release/debug ROMs, and verifies linked audio addresses, padded lengths
and zero-filled sample tails. The lab's music is a PSG fixture; FM instruments,
music percussion, audible transitions, PAL playback and graphics-DMA contention
still require listening checks with representative content.

Gameplay tests execute movement, fractional wall checks, half-open collision
bounds, projectile substeps, hit retirement, pool exhaustion/reuse and camera
clamping. Timing tests compare nominal simulation ticks across 50/60 Hz display
cadences and check pause behavior. Docker checks compile the SDK adapter in both
build modes. These are game-model and API checks, not emulated physics, VDP or
measured frame-rate guarantees. Follow Gameplay Lab's playtest checklist too.

Save tests execute the original lab's byte codec and storage transaction with a
bounded memory fixture. They cover CRC vectors, every single-bit slot error,
each interrupted-write step, schema/range rejection, unknown-version protection,
sequence wrap/conflicts, I/O failures and password roundtrips/typos. A recording
SRAM fixture checks logical offsets and unlock/lock pairing, including errors.
It is not an electrical SRAM simulation. Docker checks the real SDK signatures,
builds both modes, inspects the linked odd-address SRAM header and ensures this
lab stays below the 2 MiB overlap boundary. Emulator persistence, battery
retention and real power-loss behavior require separate runtime checks.

Performance tests compare exact collision-pair bitmaps from the all-pairs and
grid implementations across capacities, movement, cell borders and a dense
worst case. They also check bounded motion, deterministic restarts, animation
schedule, sample warm-up, frozen windows and skipped-refresh arithmetic. Art
checks reproduce the indexed sheet and reject overwrites. Docker checks compile
both build modes and inspect the real sprite's dimensions/frame count with GDB.
These establish behavior and API compatibility, not CPU speedups, DMA timing or
VDP scanline behavior. Follow the lab's emulator checklist and measure optimized
builds; its silent synthetic workload cannot certify whole-game headroom.

## Architectures and image versions

CI runs the Docker checks on separate native Linux ARM64 and AMD64 runners. The
same Linux image can run through Docker Desktop on macOS and Windows. Windows
Docker integration has not yet been verified; its host-only Python tests do run
in CI.

The scripts use the published image digest pinned in the skills by default.
Override it when checking an update:

```sh
SGDK_IMAGE=your-image:tag SGDK_PLATFORM=linux/arm64 \
  SGDK_SKILLS_INTEGRATION=1 python3 -m unittest discover -s tests -v
python3 tests/integration.py --image your-image:tag --platform linux/arm64
```

Use `linux/amd64` for the other architecture. Running an AMD64 image through
emulation on an ARM Mac is useful for a smoke test, but is not a native AMD64
test. Both architectures must pass before changing the default image.

CI also compares the initialized project's release and debug ROMs across both
native architectures. The pinned digest, known platform child digests and source
links must be reviewed together when updating the SDK. Use the new image's
headers/manual to recheck API claims; a version-string edit is not validation.

The current pin includes the GCC M68000 byte-copy workaround and base-68000
compiler/assembler policy. The toolchain's CI executes target-code regressions
on both native host architectures. Older digests can produce silent Z80 audio
despite matching ROM hashes; compare playback as well as generated bytes when
testing a toolchain change.

Before a release, try a realistic agent task with only the selected installed
skills in a scratch project. Check the resulting project and diagnostics rather
than scoring whether the agent repeats the instructions. This complements the
deterministic tests and can reveal confusing steps or missing context.

For planning, try an unfamiliar game idea without a running Docker daemon.
Check that the resulting plan picks a useful first playable milestone,
separates known limits from estimates, and names the probes needed to resolve
unknowns. A budget worksheet should help a decision, not become a prerequisite
for starting the game.

Docker Desktop can retain only one architecture for a given index digest in its
local image store. The project helper handles this for its pinned image. For the
opt-in unit tests, use the matching child digest if switching architectures
reports a platform mismatch. For AMD64 with the current pin:

```sh
SGDK_IMAGE=ewrogers/sgdk@sha256:82ad84637e4be801314e6feaf3066575d2af2934ab1c22832c048e74c6e9044e \
  SGDK_PLATFORM=linux/amd64 SGDK_SKILLS_INTEGRATION=1 \
  python3 -m unittest discover -s tests -v
```

The integration runner writes logs and a JSON report under `build/integration/`.
The report includes ROM sizes and hashes. Generated ROMs, compiler outputs and
logs stay outside version control.
