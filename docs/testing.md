# Testing the skills

The tests exercise the helpers and compile actual SGDK code. They do not ask a
language model to judge whether the instructions sound right.

## Quick checks

Run the Python tests from the repository root:

```sh
python3 -m unittest discover -s tests -v
```

Python checks use the standard library. Input, graphics, audio, gameplay, save
performance, UI and level behavior tests also need a C17
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
source. Successful compilation proves that the examples match the selected SDK's
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
and zero-filled sample tails. Sound Lab's original music now includes FM, PSG and two sampled drums. Tests
parse its VGM stream timing, FM patch/key events and DAC sample ranges, then
inspect the converted music's FM/PSG blocks and sample table. Additional checks
cover contention/DMA switch chords, DMA-delay setup, region-aware fade durations,
four-second tempo measurements including counter wrap, and disposable PAL/NTSC
headers with custom-header protection. Audible transitions, region
tempo and DMA contention still require listening checks; these tests do not
execute the Z80 mixer or certify hardware playback.

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

UI tests execute Menu Lab's navigation, repeat delay, action edges, option
changes, pause/restart/results transitions, nominal PAL/NTSC countdowns and
bounded text/number formatting. A recording VDP fixture checks fixed Window
HUD ownership, palette/font changes, stale-text removal and avoiding unchanged
field writes. Container checks compile the real APIs, build release/debug ROMs
and compare all 96 compiled glyph tiles with the font pixels. These do not
emulate the VDP, measure frame time or prove TV readability. Follow Menu Lab's
playtest checklist for those visual checks.

Level tests parse the actual Tiled collision layer, compare every occupancy
bit, and reject unsupported geometry, transform flags and broken references.
They execute fractional collision edges, wall sliding, corridor traversal,
camera limits, nominal PAL/NTSC motion and a long movement run. A recording
SDK fixture checks map/sprite ownership, queued-work draining, loading
failures, teleport redraws and exact VRAM reservation boundaries. Container
checks build the real external TSX/TMX resources in both modes and compare the
linked collision bytes with the source. Art checks verify indexed palettes
and dimensions, including the directional robot sheet. Walk/idle phase and
pause tests exercise the simulation-driven animation. Emulator checks cover visible streaming and collision
alignment; physical-console DMA timing and PAL visuals remain separate checks.

## Release candidate checks

Release tests check SGDK's whole-file XOR checksum against an independent
16-bit-word oracle, detect changes in headers/vectors/payload/padding and show
why cancelling XOR changes need a stronger release identity hash. They exercise
the stock stack alias and fixed ROM-end warning, malformed/oversize files,
SRAM overlap, CLI failures, standalone installation, exact packaged bytes,
manifest hashes, deterministic ZIPs and no-overwrite behavior. Docker tests use
SGDK's actual `sizebnd` as an oracle and build the existing Menu Lab in both
modes. The release ROM and ELF are packaged before debug outputs replace them.
These are file/build checks, not proof of game behavior, matching source lines,
banked-cartridge compatibility or physical hardware support.

## Architectures and image versions

CI runs the Docker checks on separate native Linux ARM64 and AMD64 runners. The
same Linux image can run through Docker Desktop on macOS and Windows. Windows
Docker integration has not yet been verified; its host-only Python tests do run
in CI.

The scripts and CI use `ewrogers/sgdk:latest` by default. CI pulls it explicitly;
the project helper also pulls its default before running. Use `SGDK_IMAGE` for
an explicit local development image when testing toolchain changes:

```sh
SGDK_IMAGE=your-image:tag SGDK_PLATFORM=linux/arm64 \
  SGDK_SKILLS_INTEGRATION=1 python3 -m unittest discover -s tests -v
python3 tests/integration.py --image your-image:tag --platform linux/arm64
```

Use `linux/amd64` for the other architecture. Running an AMD64 image through
emulation on an ARM Mac is useful for a smoke test, but is not a native AMD64
test. Both architectures must pass before accepting a toolchain change.

CI also compares the initialized project's release and debug ROMs across both
native architectures. Record `doctor` output and the resolved image identity
when diagnosing differences. Use the new image's
headers/manual to recheck API claims; a version-string edit is not validation.

The published toolchain includes the GCC M68000 byte-copy workaround and base-68000
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

When switching architectures locally, explicitly pull the tag for the next
platform. This also avoids reusing a different architecture cached under that
tag. The project helper's default `--pull=always` does this automatically:

```sh
docker pull --platform linux/amd64 ewrogers/sgdk:latest
SGDK_PLATFORM=linux/amd64 SGDK_SKILLS_INTEGRATION=1 \
  python3 -m unittest discover -s tests -v
```

The integration runner writes logs and a JSON report under `build/integration/`.
The report includes ROM sizes and hashes. Generated ROMs, compiler outputs and
logs stay outside version control.
