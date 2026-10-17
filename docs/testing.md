# Testing the skills

The tests exercise the helpers and compile actual SGDK code. They do not ask a
language model to judge whether the instructions sound right.

## Memory accounting and lifetimes

`tests/test_memory.py` executes Memory Lab's ownership module at `-O0`/`-O3`,
with Unix undefined-behavior checks and an independent first-fit heap oracle.
Checks cover fragmentation with sufficient total free bytes, pointer/content
preservation, failure rollback at every construction step, delayed DMA ownership,
idempotent cleanup and 500 complete allocation/room cycles. ELF accounting tests
reject missing symbols and overlapping reservations and account for exactly
65,536 bytes without charging ROM/debug sections to RAM.

Opt-in container tests build release/debug and execute the actual linked SGDK
allocator on base M68000, including zero/odd requests, existing free holes,
packing, payload preservation and 32 cleanup cycles. CI compares both architecture
reports, ROM hashes and linked accounting. The probe bypasses SDK startup and
initializes the heap explicitly; the interactive Lab uses normal startup.
Run the focused check with `python -m unittest discover -s tests -p test_memory.py`.
Reports/logs are in `build/memory-integration`. These are allocator and ownership
checks, not VDP timing, stack-peak measurement or physical-hardware validation.

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

## Deterministic testing and Test Lab

The installed `sgdk-testing` skill provides a standalone Test Lab and portable
QA commands. `tests/test_testing.py` executes its real C model, checking input
edges, pause, wall bounds, pool exhaustion/reuse and recording limits. A
100,000-tick run checks repeatability and bounded state. The same logical tape
must match its reviewed checkpoints under nominal 50/60 Hz render schedules.
An injected movement bug must fail at tick 60; malformed, missing, reordered
or foreign reports must not pass. Existing outputs cannot be overwritten.

Docker tests build the ordinary ROM and a dedicated CPU probe in release/debug.
Unicorn 2.1.4 executes the probe as base M68000 and compares every checkpoint
with the host baseline, including actual loop counts for both schedules.
Native ARM64/AMD64 CI compares the four ROM hashes and reports. Probe execution
bypasses startup and does not emulate VDP, interrupts, input hardware or audio.
The ordinary ROM has built-in playback for separate ares checks; its deliberate
bug toggle demonstrates a visible regression. Live RAM takes can be replayed
but are not exported to host files in this slice.

These checks support automated logic QA. They do not prove equal PAL/NTSC
physical input latency, real-time performance, visual correctness or hardware
compatibility. The Lab README lists separate emulator acceptance checks.

## Build real ROMs

With Docker running in Linux-container mode, enable the resource and debugging
checks, then test the project helper:

```sh
python3 -m pip install -r tests/requirements-math.txt
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

## Raster checks

Raster tests execute the wave model across all phases, camera/waterline bounds,
pause/input edges, 50/60Hz cadence and all 512 quantized colors. Fake ports record
dry/wet palette ownership, one-shot arming, pending VBlank interrupts, late events,
bounded edge/FIFO waits, unchanged-pair filtering and autoincrement restoration.
They also reject forced-blank commands: the surface sprites must stay visible. The host
edge simulation checks protocol and failure handling, not scanline timing.
Container tests compile the real APIs/resources in release/debug and U/E headers,
then inspect the handler for `RTE`, bounded assembly waits and preloaded
register-to-port writes without helper calls. Emulator checks separately cover
the intentional masked band, CRAM dots, controls and music with scrolling DMA.
Physical-console timing and other VDP modes remain unverified.

Water frame tests invoke the registered early V interrupt, inspect dry CRAM
before simulated late main-loop work, sweep both directions and verify that a
later published setting cannot change the armed frame. Teardown removes both
interrupt callbacks. These test ownership and ordering, not actual DMA duration;
continuous emulator sweeps are a separate timing check.

Asset tests reproduce the original water sprites/music, check PNG checksums and
opaque surface coverage, and parse the VGM's twelve-second loop, pad envelopes
and spaced bell events. Bubble/splash tests cover entry/exit crossings, rising
and retirement, pool exhaustion, pause and matching 50/60Hz model schedules.

The separate Split Lab tests cover independent cameras, viewport-origin offsets,
50/60Hz cadence, CPU/manual control, world bounds and whole-sprite culling at
HUD/divider edges. Fake ports record VSRAM packing, Window handoff, pending/late
interrupts, bounded HBlank failures and stop/rearm behavior. Both labs build in
release/debug with U/E headers and undergo interrupt disassembly checks. These
do not emulate the VDP or establish interlace, streaming-cache or hardware
support. Split Lab deliberately uses noninterlaced output and a resident world.

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

## Cartridge banking

`tests/test_cartridge.py` compiles the actual lab model and mapper/SRAM adapter
with a C17 host compiler. It tests region timing, input edges, bounded reads,
cross-bank copies, view invalidation, audio handoff order, corruption rejection,
and SRAM closing/preserved mappings. The fake mapper models alias invalidation;
it does not emulate the Z80, DMA hardware or SRAM electrical behavior.

With `SGDK_SKILLS_INTEGRATION=1`, the test copies Bank Lab into a path with spaces,
checks adapter declarations against the real SDK and rejects a normal bank-disabled
build. It builds release/debug/release with private bank-enabled SDK libraries,
checks loadable ELF sections and actual payload bytes beyond 4 MiB, fixed-bank
budgets, the cross-bank probe, music placement, SRAM header and the SDK's
XOR-folded checksum. A deliberately oversized common section must fail linking.
Asset tests reproduce the original stage images, palette words and FM sources,
and check that regeneration refuses to overwrite existing files.

CI runs the container checks on native ARM64 and AMD64 and compares the banked
release/debug ROM hashes across them. The normal host matrix
also covers Windows, macOS and Linux. Local ares testing confirmed both rooms,
shared hero/UI, stage music, SRAM save/load and the in-ROM checks. That is not a
physical-board test. Actual cartridge/flash-cart mapper and SRAM compatibility,
power-loss behavior and hardware audio timing remain separate checks.

Bank Lab intentionally uses a sparse 6 MiB image. The existing release helper's
linear profile continues to reject banked cartridges; do not use it to certify
this mapper or replace the selected SDK checksum with a generic word sum.

## Scene flow and cleanup

Scene tests execute the Transition Lab model, byte codec and recording SGDK
adapter. They cover either-player joining, countdown skipping, held input
handoffs, pause, independent coin ownership, radial scatter and visual hop,
bounded eight-way lasers, boss defeat and level-start checkpoint roundtrips.
A flood fill checks that every authored coin can be reached around solid
blocks. A separate chase test routes the boss across the blocked arena and
checks its full footprint on every tick. Save tests check every single-bit corruption and interrupted-write
position. Fade tests model SGDK's final queued palette transfer and check the
extra drain before cleanup. Repeated partial/full loads must return to the
same warmed-up heap baseline.

Container checks build the standalone copy in release/debug, verify adapter
signatures against SGDK, and check the ROM header and linked art/audio.
CI compares the two ROM hashes across native ARM64 and AMD64. These fixtures
do not emulate VDP timing, the Z80 or electrical SRAM. Local NTSC ares testing
covers the full game flow, music and boss encounter; PAL visual testing and
physical-console checks remain separate.

## Enemy AI

Enemy Lab tests execute its actual C state, sight and navigation code. Every
walkable start/goal pair in all three maps is compared with an independent
shortest-path oracle. Checks cover symmetric supercover sight, cone/range edges,
stable target choice, tracking hysteresis, bounded/fair search work, stale
result cancellation, off-grid stun recovery, projectile exhaustion and a long
collision-safe run. Level tests check exit reachability, retries and Game Over.
They also check sheltered entries for both players, transitions from the previous
exit, and the three-second stun cooldown with pause and PAL/NTSC timing.
Doorway checks move both players through the entries with small alignment
offsets. Tests walk complete exit routes with the actual collision code and
reject paths confined to the outer two rows/columns. The recording renderer
also checks that the arena's solid edges have visible walls.
Combat tests check contact in every enemy state, inactive and invulnerable
players, player/enemy movement into contact, thin/thick cover, bomb flight/fuse/
cross-shaped damage, directional tasers, cooldowns and stun interruption.
Help tests cover browsing all three cards, press edges, beginning play and
frozen simulation with no reading timeout at either refresh rate.

A recording SDK checks HUD text/palette roles, alert and stun icons, last-second
warning flashes, facing, try icons and removal of old level messages. Recording
fixtures also check solid foreground panels, restoration of the playfield and
the recharge bar, enemy portraits, bomb landing markers, blasts and tasers.
The real adapter loop runs through a ten-minute-or-longer
wait at Game Over and completion, then restarts, checking input reads, music
restoration and a single clear jingle per exit. Container
checks verify the adapter declarations against real SGDK, keep the local
makefile's aliasing compatibility flag, and build release/debug ROMs. CI compares
their hashes on native ARM64 and AMD64. Host timing checks cover nominal PAL/NTSC
model clocks; PAL visuals, search latency under real workloads, measured CPU
cost and physical hardware remain separate checks.

## Fixed-point math

Math Lab tests compare projection with an independent rational oracle, verify
all three formations over full cycles, and exercise interpolation endpoints,
full-range squared distances, input edges, pause, nominal 50/60 Hz cadence and
a 100,000-update bounded run. Generated indexed art must reproduce exactly and
refuse overwrites. Host C17 checks run on macOS, Windows and Linux.

Docker integration builds the standalone ROM in release/debug, then executes
actual SGDK math on Unicorn 2.1.4 configured for the base 68000. The test checks
negative conversion/rounding, fixed-point multiply/divide, angle wrapping and
the entire degree-based sine cycle against a higher-precision oracle. It also
checks the SDK's intentional low-bit loss in `F32_mul`. No host replacement is
used to establish these SDK behaviors. Install `tests/requirements-math.txt`
in a virtual environment before running integration locally; ordinary host
tests do not need Unicorn.

CI compares Math Lab release/debug ROM hashes on native ARM64 and AMD64.
Executed CPU math is not VDP emulation, a frame-time measurement or a guarantee
outside the documented operand ranges. Follow the lab's emulator checklist for
visual motion, pause, diagnostics and sprite scanline pressure.

## Platformer feel and collision

Platform Lab tests execute its actual C model for half-open walls, ascending
and descending floor slopes, one-way crossing/drop-through, coyote/buffer
windows, variable-height jumps, wall/double jumps, moving-platform support,
charge thresholds, breakable terrain, stomps, armor and damage invulnerability.
They exercise all five enemy behaviors, powers, secrets, checkpoints, completion,
nominal PAL/NTSC cadence and a 100,000-step mixed-input collision-safe run.
Native art tests check exact reproduction, indexed palettes, PNG integrity and
no-overwrite behavior. Every Tiled cell is compared with the collision source. Audio tests parse VGM
loop/wait commands and PCM formats, reproduce sources, and execute the sound
adapter's priorities, charge-ready edge, pause and one-shot fanfare transition
against a recording SDK. They do not execute the Z80 mixer or judge the music.

Container tests build the standalone ROM in release/debug, inspect the header
and linked resources, then execute selected movement/terrain/clock checks as
actual M68000 instructions on Unicorn. CI compares release/debug ROM hashes on
native ARM64 and AMD64. Host models and target CPU execution do not emulate
VDP streaming, sound playback, TV readability or real frame-time headroom.
Follow the lab's playtest checklist in ares and, when available, on hardware.

## Shoot-em-ups

Shooter Lab tests execute its actual pattern, pool, movement, collision, wave,
scoring and boss code. They cover symmetry and aimed directions, complete-volley
exhaustion, substep contacts, immunity, weapon/bomb limits, score saturation,
phase changes, one defeat bonus, restart, nominal PAL/NTSC cadence and pause.
A 100,000-step run checks bounds. Authored attack scenarios count sprite and
pixel occupancy per scanline; these do not prove arbitrary patterns are safe.
Art/audio tests reproduce native indexed sheets and VGM/WAV sources, inspect
palettes, dimensions, PNG integrity, PCM format and music waits/loops, and reject
overwrites. A recording sound fixture checks priorities, pause and one fanfare.

Container tests compile standalone release/debug ROMs and execute selected
pattern, movement, collision, clock and resource checks as actual M68000 code
on Unicorn. CI compares ROM hashes on native ARM64 and AMD64. These do not
emulate VDP scanline evaluation, measure CPU headroom or establish sound quality.
The playtest checklist covers the vertical ROM; horizontal layout guidance is
documented separately and is not claimed as a second runtime demo.

Shooter Lab also compares 12,000 randomized projectile updates against the
pre-optimization substep routine, including full game-state side effects.
Its M68000 probe reports instruction counts for an identical full-pool workload
before and after broad-phase rejection. Those counts are a CPU-work proxy,
not emulated VDP timings or a 60 FPS claim. Boss tests cover the protected
warning/entrance/bar reveal, three attack phrases, music handoff and pause.

Shooter Lab's second stage checks carrying state between stages, automatic and
manual advance, timed boss-guard windows, opposite ship-relative Twin orbits,
shared firing capacity and full upgrade loss on damage. Shield tests cover
single absorption and expiry. Bomb tests cover arming delay, outward clearing,
one-time nonlethal boss damage, empty-stock feedback and pause. The compiled
font keeps 96 ASCII slots, and sprite-definition probes verify actual frame and
hardware-piece counts for the sparse boss dissolve and utility sprites.

## Carpet Lab

`tests/test_carpet.py` compiles the horizontal game's portable C model from a
standalone copy, with strict warnings. It covers three-stage/credits flow,
PAL/NTSC input edges and pause, all combined weapon states, gem purchase caps,
fixed boomerang trajectories, bounded chain targeting, Ribbon damage cadence,
releasing/repressing fire, pool exhaustion, shield loss and sparse gem drops.
A 12,000-case full-state comparison checks broad-phase projectile collision
against a reference substep walk; separate golden cases check the shared weapon
policies. A 100,000-step deterministic soak checks bounds and pool limits.

Adapter regressions execute HUD placement/selection, disabled selection,
credits panel bounds, multi-glyph banner packing and caching, shield mirroring,
and beam PCM start/refresh/release/pause behavior. Asset tests validate native
size, palette, PNG CRCs, connected gem outlines, reproducible UI/audio and VGM
wait/loop/PCM formats. Synthetic linked resources reproduce the former later-stage
VRAM overflow and reject oversized actor/banner and menu reservations. Header
tests check the final padded extent, preserve the checksum and reject malformed
input without changing the file.

With `SGDK_SKILLS_INTEGRATION=1`, CI builds release and debug ROMs in a disposable
path containing spaces, checks their headers and actual linked VRAM usage,
then builds a direct-entry probe against that same SDK. Unicorn 2.1.4 executes
the probe as M68000, checking resource geometry, mirrored sprite placement,
clock behavior, combined upgrades, beam/boomerang/chain rules and direction
vectors. This does not emulate startup, VDP timing or the audio hardware.

The native AMD64/ARM64 jobs publish `build/carpet-integration/carpet-report.json`
and build logs. The comparison job requires matching release/debug ROM hashes
and resource budgets. Example installation remains self-contained, and the
skills installer test continues to disable telemetry. The PixelLab production
notes in `docs/pixellab-sgdk-notes.md` are observations for a future skill,
not a requirement to use generated art in every shooter.


## Best-practices examples

`tests/test_best_practices.py` copies the examples into an independent folder
and builds the same C modules at `-O0` and `-O3` with strict aliasing enabled.
Host tests cover all 65,536 input values for the widened square, saturation
against a wider oracle, wrapped elapsed counters, invalid shift indices,
4,000 byte-format round trips, malformed records and unchanged outputs on
failure. Header-first/double inclusion is exercised by the adapter builds.

The SDK test doubles inject sprite failure on either acquisition, reject
reentry, repeat cleanup and verify release order. DMA adapter checks cover
copied source contents, queue rejection/retry and balanced nested foreground
critical sections. Unix host jobs additionally run these corrected examples
with undefined-behavior sanitization; Windows runs the ordinary strict builds.
Undefined examples discussed in the reference are not executed as benchmarks.

With `SGDK_SKILLS_INTEGRATION=1`, the standalone SGDK project compiles all
adapters in release/debug. A separate direct-entry M68000 probe executes the
actual SDK's DMA copy/queue and interrupt-mask routines, checks copied words
after source mutation, fills the queue and temporary storage independently,
and verifies rejection, retry and previous-mask restoration. It explicitly
initializes the SDK allocator/queue because the harness bypasses startup.
The test's internal buffer inspection is not an application-facing SDK API.

Native ARM64/AMD64 CI requires matching release/debug hashes for the ordinary
example and SDK probe, reported in
`build/best-practices-integration/best-practices-report.json`. This validates
code and queue contracts, not live sprite allocation, interrupt scheduling,
VRAM/CRAM transfer timing or emulator/console presentation. The examples are
small build/execution probes rather than a playable Lab ROM.


## Dialogue

`tests/test_dialogue.py` executes the standalone C model at `-O0` and `-O3`,
with UBSan on Unix. Tests cover timed reveal, delays, styles, wrapping/paging,
speaker/pane retention, flag-filtered choices, all-hidden fallback, invalid
programs, work limits, nine-slice geometry and 30,000 bounded authored steps.
Nominal PAL/NTSC clock, pause, held input and single-use confirmation checks
share the actual Lab controller code. A recording sound adapter checks cue
priority, pause/resume and that completion never repeats its PCM cue on idle
frames. Art/audio tests reproduce native indexed UI assets and VGM/WAV sources,
check shared palette/portrait dimensions, finite sample endpoints and loop
commands. Compiler checks include generated C escaping and no-overwrite errors.

Docker integration builds normal release/debug ROMs and a direct-entry probe
against the selected SDK. Unicorn executes it as M68000 to check linked font,
portrait and palette resources, progression and target structure sizes. The
probe supplies SGDK 2.11's default tilemap boundary because it bypasses startup;
it does not execute the VDP initialization or emulate transfers/audio. Native
ARM64/AMD64 CI compares all four ROM hashes and the resource/structure reports
in `build/dialogue-integration/dialogue-report.json`. Host helpers run on all
three OSes, and the distribution check copies the whole installed skill with
telemetry disabled.

Local NTSC ares feedback confirmed dialogue/controls and the original harbor
music with the ending-cue fix. PAL presentation, VDP timing under gameplay load
and physical-console sound/input remain separate checks. Follow Dialogue Lab's
playtest checklist when adapting it to a game.


## PixelLab art pipeline

`tests/test_pixellab.py` checks native/frame dimensions, palette contracts,
transparent versus opaque black, nearest-color conversion against a distance
oracle, deterministic ordered dithering and frame phase, all five PNG filters,
checksums/truncation/invalid palette indices, bounded decompression, paths with
spaces and no-overwrite errors. Committed prepared artwork reproduces offline;
the 32x16 chest crop and five-frame sheet are compared with their exact source
pixels. Tests do not prescribe art style or assert that a metric is beauty.

Docker checks build actual 8x8 tile, 16x16 icon, 32x16 prop, five-frame 32x32
actor and large-image resources in release/debug. A direct-entry Unicorn
M68000 probe checks tile nibbles, linked palettes, sprite/frame geometry and
resource counts. Native ARM64/AMD64 jobs compare hashes and budgets in
`build/pixellab-integration/pixellab-report.json`. This bypasses startup and is
not a playable ROM or VDP/audio/timing emulation.

No paid PixelLab calls run in CI. The 2026-10-15 manual study preserves exact
requests, originals, candidate overviews, assessments, cleanup and final indexed
assets in the independently installed skill. Its local HTML gallery supports
native/integer-zoom visual review. Those findings do not prove arbitrary
seamlessness, scene readability or console performance. Dithering remains an
optional visual choice with possible texture and tile-budget costs.

The PixelLab color study also reproduces same-source conversions offline, tests
protected palette colors and per-8×8-tile line assignment, and checks real
ResComp banked IMAGE palette words/tilemap attributes. Its four-page native
showcase is compiled in both modes as part of the PixelLab integration check;
paid generation calls are never made by tests or CI.

The optional PixelLab shadow/highlight experiment tests per-pixel brightness
selection against an independent oracle, reproduces committed modeled previews,
and decodes samples from every linked lighting-mask sprite in real M68000 code.
CI builds both the normal comparison ROM and the executable probe in release/debug
on ARM64 and AMD64 and compares their hashes. Emulator review is separate from
these resource checks; the ares preview model is not an analog hardware guarantee.

Asterfall, the PixelLab JRPG catalog, adds host/M68000 tests for naming bounds,
mock saves, menu navigation, travel, dialogue and foot-box collision. Its native
preparation checks preserve native dimensions and verify authored production glyphs;
the generated font remains as a reference after readability testing. Linked
resource checks cover palette ownership, every direction/frame and conservative
room/dialogue VRAM reservations. CI compares release/debug catalog and logic-probe
ROM hashes across ARM64/AMD64; visual/audio approval remains a separate playtest.


## RPG rules and Adventure Lab

`tests/test_rpg.py` executes the actual party, battle, checkpoint, clock, layout
and controller modules as host C. It covers stat recomputation, capped healing,
revival, rejected actions without MP/stock/RNG changes, seeded damage, ties,
fallen-actor skipping, status/combo rules, three-encounter progression and
exactly-once rewards. Controller checks cover live target cycling in the boss
encounter, pause, manual action messages and automatic victory-to-spoils timing.
Formation rectangles must remain separate and inside the battle/HUD boundary.
Nominal 50/60 Hz clocks produce the same elapsed simulation units.

The SRAM spy executes the actual adapter with interrupted writes at every byte
boundary. It checks older-slot recovery, corruption, sequence wrap, invalid
records and balanced read/write enable calls. Art checks reproduce the full
standalone project offline, preserve source dimensions and sprite alpha, pad
field frames without rescaling, and refuse existing output directories. Paid
PixelLab calls never run in CI.

Docker tests build normal release/debug ROMs, then execute an independent entry
point as base M68000 instructions. It runs the same rule tests and checks linked
palette ownership, actor/frame dimensions, conservative VRAM reservations and
sprite/scanline peaks for all three actual formations. The report records
campaign/battle/controller structure sizes and graphics bounds. Native
ARM64/AMD64 CI compares normal/probe hashes and budgets in
`build/rpg-integration/rpg-report.json`. This does not emulate VDP/audio services,
measure frame-time headroom or prove power-off SRAM retention.

Local ares playtesting confirmed harbor movement, music, menus and battles;
feedback improved formations, overhead targeting, down-facing artwork and
message readability. Regional display/audio and physical hardware remain
separate checks. The original Lab's setting, art, music and combat tuning are
illustrative rather than requirements of the installed genre skill.


## Localization

`tests/test_localization.py` validates UTF-8/NFC catalogs, stable IDs, required
translations, explicit fallback, placeholder agreement, glyph coverage and
worst-case wrapping/paging. Native font pixels and compiler output reproduce
exactly. Host C checks at `-O0` and `-O3` execute formatting boundaries, accented
name limits, menu retention, language changes and preference corruption. A
recording SDK checks tile ownership, copied screen transfers and interrupted
preference writes at every byte boundary.

Container checks build standalone release/debug ROMs and execute the actual
rules as base M68000 instructions. All 72 translated layouts and every compiled
font row are compared with generated source data. Native ARM64/AMD64 CI compares
normal/probe ROM hashes, font budgets and target structure sizes in
`build/localization-integration/localization-report.json`.

Local ares playtesting confirmed language switching, accents, dialogue paging,
large quantities and eight-glyph names. The Japanese sample is a small katakana
repertoire, not general Japanese support. Tests do not judge translation quality,
emulate VDP/SRAM hardware or establish PAL/physical-console presentation.


## Cosmetic effects

`tests/test_effects.py` executes Effects Lab's actual C at `-O0`/`-O3` and runs
Unix builds with undefined-behavior sanitization. It covers full pools,
priority replacement, bounded/retriggerable burst ownership, separate weather allowance, cached counts, lifetime,
world-space trails, pause/input edges, nominal PAL/NTSC clocks and camera limits.
A 100,000-frame mixed run checks state bounds and exact scanline accounting;
5,000 selections must match an independent byte-counter reference. Rendering
quality/budget changes cannot mutate simulation or its cosmetic RNG.

A recording SDK exercises every frame-upload failure, cached tile ownership,
SAT chain termination/shortening, hidden-table cleanup and unchanged HUD rows.
The adapter owns the low-level SAT rather than mixing it with `SPR_update()`.
Native art reproduces offline and standalone preparation refuses overwrites.

Container checks build release/debug and execute core contracts as real base
M68000 instructions. A target trace must match the host trace. Every linked
frame/palette is compared with its native PNG, and one identical full-pool
selection workload reports optimized/reference instruction counts. A mixed
maximum-snow/movement/burst workload checks maximum simulation and selection
instruction work too, including spawn frames. The optional `--stress` release
ROM automates the emulator workload and counts missed rendered frames. These are
work estimates, not cycle-accurate timing. Native ARM64/AMD64 CI compares hashes,
geometry, trace, budgets and instruction counts in
`build/effects-integration/effects-report.json`.

Local NTSC ares review covers controls, weather, bursts, trails and shake.
Maximum snow exposed early frame-rate problems and is retained as a sustained
playtest case. The measured weather allowance is separate from the 64-slot
correctness stress case. PAL presentation, arbitrary effect mixes, full-game
headroom and physical-console results require their own playtests.


## Pattern streaming

`tests/test_streaming.py` runs actual cache/world C at `-O0`/`-O3` with Unix
undefined-behavior checks. It tests bounded chunks, failed reads/queues, partial
completion, pinned exhaustion, LRU reuse/wrap and 10,000 changing requests
against retained VRAM bytes. Forward/reverse walks at PAL/NTSC rates must never
wait for art under either budget. Every camera alignment checks visible pins.
Transition checks exercise PAL/NTSC fade duration, holding black for delayed
loads, and exact restoration/monotonic scaling of all 512 RGB3 colors. These
check the transition model; emulator review checks for visible loading flashes.

The recording adapter exercises actual FAR-copy/queue code: invalidate the
mapped source before simulated VBlank, then verify owned RAM bytes, units,
destination and frame/queue limits. This is not physical mapper emulation;
Cartridge Lab separately covers actual banked builds.

Container tests build release/debug, execute contracts as base M68000 code and
compare all linked scenery/courier bytes with native art. CI compares native
ARM64/AMD64 hashes, geometry and budgets in
`build/streaming-integration/streaming-report.json`. It also builds `--demo`:
automatic bidirectional walking at the slow budget, with readiness-wait and
missed-frame counters. PAL presentation and physical hardware remain separate
from nominal-clock and CPU/resource checks.


## Gameplay animation checks

`tests/test_animation.py` executes Animation Lab's actual clock/action model at
`-O0`/`-O3` with Unix undefined-behavior checks. It covers exact holds, first/last
active ticks, one hit per attack, mirrored contacts, restart, interruption,
buffered recovery cancels, completion once, pause/slow input and nominal PAL/NTSC
progress. A 100,000-frame mixed run checks state and event-batch bounds.
Container checks build release/debug, execute base-M68000 contracts, compare a
2,000-frame host/target trace and reconstruct every linked pose's pixels and
palette from the target resource data. Reports are in `build/animation-integration`.
Native ARM64/AMD64 CI compares hashes, geometry and budgets. These are model and
resource checks; visible alignment, audio and actual frame timing need playtests.

## Collision queries

`tests/test_collision.py` compares 12,000 deterministic box/segment queries with
an independent Python rational oracle at O0/O3 (UBSan where supported). Executable
contracts cover touching, parallel motion, finite endpoints, relative target
motion, grazing, simultaneous corners, nearest hits, static sliding, invalid
input and candidate limits. PAL/NTSC schedules and pause preserve logical state.

With `SGDK_SKILLS_INTEGRATION=1`, the selected container builds standalone
Collision Lab release/debug and PAL/NTSC region variants. The base-M68000 probe
executes the contracts and a 1,000-query trace digest, compared to host C. CI
compares `build/collision-integration/collision-report.json` across native ARM64
and AMD64, including hashes, type sizes and budgets. This does not emulate VDP
or audio timing, prove frame rate, or implement moving-wall response. Record
emulator/hardware observations separately in the Lab README.

## Isometric scenes

`tests/test_isometric.py` compares 6,000 projected-box pairs to an independent
convex-hull oracle. Host O0/O3 contracts cover negative projection rounding,
known-height inverse projection, bridge occlusion, stable topological order,
support links, full capacity, cycles and invalid input. Movement tests exercise
stairs, lower/upper surfaces, ground collision, camera bounds and a 20,000-step
soak; a 1,000-step route compares cached and full ordering. PAL/NTSC schedules
and pause preserve nominal logical time.

Opt-in container checks execute the same contracts at bounded smaller route/soak
counts on a base M68000, compare a 128-step host/target trace, and inspect compiled
sprite geometry/palettes, opaque font data and authored placements. Scanline
bounds conservatively include every static rectangle plus hero and marker.
Release/debug and PAL/NTSC-region ROM hashes, target type sizes and budgets are
compared across native ARM64/AMD64 in `isometric-report.json`. These checks do
not emulate VDP/audio timing or certify real hardware; record actual ares tests
separately in the Isometric Lab README.

## Hardware Lab

`tests/test_hardware.py` compiles the portable C helper at `-O0` and `-O3`
(with undefined-behavior checks on Unix). It checks PAL/NTSC nominal scheduling,
unsigned VInt wrap, missed service and bounded catch-up, both H40/H32 scanline
limits, VRAM upload boundaries, record round trips and every single-bit record
corruption. Padded field replacements cover shrinking numbers, stable labels,
exact width, null inputs and rejection without a partial write. An independent
Python CRC oracle checks the serialized bytes.
Standalone preparation is exercised through a second copied project. The PCM
reference is a native 256-sample, zero-DC waveform with bounded adjacent sample
steps, including its loop boundary.

With `SGDK_SKILLS_INTEGRATION=1`, both container architectures build release/debug
Hardware Lab and helper probes, plus PAL/NTSC header variants. Tests inspect the
compiled SRAM header and font, then execute the helper contracts on base M68000.
CI compares hashes and reports across architectures. These probes call `main`
directly; they do **not** execute the SGDK startup or emulate VDP, Z80, controller
or SRAM behavior. Header variants alone do not constitute PAL/NTSC runtime tests.

The [console checklist](../skills/sgdk-hardware/references/checklist.md) separates
emulator observations from real hardware checks. Physical cold boot/reset,
video output, DMA/audio bus timing, peripheral compatibility and power-loss save
retention remain unverified until tested on a console/flashcart.


## Co-op Lab

`tests/test_multiplayer.py` executes the actual portable C session model at O0
and O3 with undefined-behavior checks. It covers adapter route plans, input-edge
seeding, eight logical devices mapped into four participant slots, capacity,
simultaneous pause requests, joins during pause, consumed Leave edges, shared
pickups, independent and all-down returns, wall/dash movement and joint camera
constraints. Stun tests verify stopped patrols, damage prevention, pause,
recharge boundaries, fresh-press behavior and recovery. A 20,000-frame seeded soak checks invariants. One thousand camera
cases are compared with an independent Python bounding-box oracle. PAL/NTSC
schedules must produce the same idle simulation duration and state.

Container tests compile the real Lab in release/debug, plus PAL/NTSC header
variants, and run model contracts in a base-M68000 probe. Compiled checks inspect
map dimensions, sprite animations, palette words, glyphs, hardware pieces and
VRAM reservations. Both native architectures must match ROM hashes and
`build/multiplayer-integration/multiplayer-report.json`. Standalone copying and
native indexed asset generation run on Windows, macOS and Linux host jobs.

These checks do not run SDK startup, the VDP, audio or controller protocols.
The CPU tour supplies synthetic pads to demonstrate four participants; it cannot
validate a Team Player or EA adapter. Actual presentation, timing, direct pads,
adapter combinations and unplug/replug behavior need emulator and physical
hardware observations. Read the Lab checklist before claiming compatibility.


## Combat Lab

`tests/test_combat.py` executes the actual C resolver at O0/O3 with
undefined-behavior checks and 20,000 seeded frames. Contracts cover damage and
immunity, team filtering, front/back/side guard, unblockable attacks, simultaneous
killing blows, damage aggregation and stable knockback ties, repeated contacts,
reused targets and bolt owners, action interruption, input-buffer expiry,
shot-pool exhaustion, pause/hit-stop and nominal PAL/NTSC schedules. Extended
sword reach is exercised in all four directions. Enemy guard tests require both
attempts and misses, an actual reaction delay, and cooldown gaps.

A separate art/geometry check compares leading sword pixels with the C damage
box for every pose/direction. It verifies native quarter-turns and their world
orientation; it does not claim pixel-perfect collision. Standalone asset
creation/copying, indexed palettes and native sheet dimensions are tested on
Windows, macOS and Linux. Lab art and combat balance remain illustrative.

Native ARM64/AMD64 container builds compile release/debug and region variants,
execute the same contracts on a base-M68000 CPU, and inspect actual linked sprite
shapes, pieces, frames, palettes and font data. CI compares all six ROM hashes
and `build/combat-integration/combat-report.json`. The 40-contact batch is checked
against a mathematical maximum of 34; core sprites fit 16 pieces / 320 pixels
per scanline before cosmetic admission. This does not measure CPU deadlines or
emulate VDP, Z80/audio, controllers or SDK startup. Optional two-pad co-op,
actual PAL/NTSC presentation and physical-hardware behavior need playtests.


## Puzzle rules and the two Puzzle Labs

`tests/test_puzzle.py` compiles the actual portable C rules at O0/O3 with strict
warnings and UB checks where available. Clockwork Garden checks rejected moves
leave board/history unchanged, one-crate pushes, gap filling and undo, pressure
plates and occupied-gate holds, malformed boards, ring eviction and restart.
An independent set-based Python BFS finds a solution for each authored room;
7,200 seeded commands compare C state with that oracle, including undo/restart.
A further 20,000-step C soak checks invariants.

Falling Lanterns checks four-cell rotation footprints, seven-piece bag uniqueness,
blocked movement/rotation, simultaneous four-row compaction/scoring, lock delay
under floor movement, top-out and equal PAL/NTSC logic counts. A 20,000-tick C soak
checks bounded state while dropping and rotating pieces across restarted rounds.
The Lab's art, music, custom kick policy and tuning are examples, not genre rules.

With `SGDK_SKILLS_INTEGRATION=1`, both Labs build release/debug and PAL/NTSC ROMs
through `ewrogers/sgdk:latest`. A base-M68000 probe executes both C contracts and
shorter traces, checks their host digest, and reads real compiled art geometry,
palettes and font. CI compares all ten ROM hashes and reports across native
ARM64/AMD64. Logs and `build/puzzle-integration/puzzle-report.json` are artifacts.
The standard installation test copies the full skill, including both variants.

Build either fresh review project:

```sh
python skills/sgdk-puzzle/assets/puzzle-lab/tools/build.py --output build/garden-review
python skills/sgdk-puzzle/assets/puzzle-lab/tools/build.py --lab blocks --output build/blocks-review
```

The user confirmed both ROMs work in ares, including the revised Garden title and
Block Lab's controls/preview/clear/restart. Emulator playtests remain separate
from deterministic CPU tests; physical hardware has not been tested.


## Expanded audio authoring and Sound Desk

`tests/test_audio.py` preserves the transport/SDK resource checks and adds input
seeding on page entry and VInt-based measurement despite skipped updates or
timer wrap. `tests/test_audio_workshop.py` adds O0/O3 C conductor
contracts and a 20,000-frame trace: pending starts, coalesced scene requests,
fade ownership, jingle return, stop, timeout recovery, pause, duplicate/cooldown
suppression, channel admission and driver rejection. WAV tests check gain/DC
removal/edge ramps, preserved rate/frame count, malformed input and refusal to
overwrite. VGM tests independently decode command boundaries, wait totals, loops,
one-shot release, FM/PSG/DAC layers and reproducible source assets.

The new standalone export is generated in paths with spaces and exported again
without the repository. Tests inspect native indexed art, RGB3 palette and font.
Container checks build real release/debug plus PAL/NTSC ROMs through the selected
`ewrogers/sgdk:latest`; a base-M68000 probe runs the conductor and reads compiled
art/font. Existing audio checks still inspect XGM2 alignment, embedded drums,
PCM padding and actual API signatures. CI compares both architectures' six new
ROM hashes and reports in `build/audio-workshop-integration/`.

Sound Desk's five pages separate music, instrument auditions, clean/abrupt sample
pairs, runtime mixing and the original diagnostics. Listening checks must cover
both detected video regions, fanfare restoration, held input, loop seams and
music/effects with controlled DMA. Building a region header or running C rules
is not evidence of Z80 audio quality or physical hardware compatibility. Tracker
workflow references are sourced, but the generated fixtures do not certify an
arbitrary Furnace project/export.

The 2026-10-16 ares check showed 240 source music frames over 200 VBlanks
in PAL and 240 over 240 in NTSC, with no missed music frames. The PAL
measurement, held-button fix and MIX fanfare return were confirmed by the
user. ARM64 and
AMD64 builds produced matching audio reports. These checks do not replace
listening on real hardware, especially when testing PCM with graphics DMA.

## Cross-skill composition milestone

The [validation report](validation-milestone.md) records the request, installed
skills, findings and evidence limits for a new two-room courier fixture.
`tests/test_composition.py` exercises input, collision, pause, regional schedules,
installed project/planning/release helpers, real assets and release packaging.
Host contracts run at O0/O3; integration builds PAL/NTSC release/debug ROMs and
executes the model on a base M68000. CI retains the composition report and logs,
and compares native ARM64/AMD64 results. This is integration coverage, not a
blind evaluation of an independent agent or proof of all skills working together.
The regression also checks that the copied SDK header builds without GCC 16
terminator warnings while preserving its fixed-width metadata bytes.

## Streaming and audio composition

The [second validation report](validation-streaming-audio.md) records the
installed components, shared budgets, failure injection and ares checks.
`tests/test_stream_audio.py` composes the installed cache/world, DMA adapter and
audio director with a new route coordinator. Host O0/O3 contracts check every
visible map cell, palette ownership, borrowed RAM, bounded uploads, queue/read
rejection, rapid reversal, fanfare/warp overlap, deferred pause and startup
timeouts. The base-M68000 probe runs those contracts with sampled map checks,
real SDK button constants and an explicit 200-million-instruction ceiling.
It does not emulate VDP/Z80/audio timing. CI compares all six region/build/probe
ROMs and the reports from native ARM64/AMD64 runners. The original Streaming Lab
is still covered by its existing build and cache/adapter tests; the new fake
backend does not execute that Lab's entire main loop.

## Design adaptation

The [design-adaptation report](validation-design-adaptation.md) records a frozen
municipal tram-dispatch brief, a hidden prewritten rubric and one independent
agent submission using project-local copies of all 36 skills. Manual review
checks whether the requested design survived; source similarity is a review aid,
not an automated originality score.

`tests/test_design_adaptation.py` preserves behavior coverage for that exact
submission: the agent's tests and independent route/hold/retry/regional contracts
run at O0/O3. Container checks rebuild the release with JUE/U/E headers and
execute a base-M68000 probe including the submitted SDK button adapter. Deliberately
mapping C to A must be detected. CI retains logs/reports and compares native
ARM64/AMD64 results. Those tests do not run a fresh agent in CI or establish
visual quality, audible quality, controller feel or originality for future games.

User playtesting subsequently found flashing and an overwhelming interface in
that frozen submission. The separately labeled `revision/` is an informed
correction, not a replacement blind result. `tests/test_design_adaptation_ui.py`
checks bounded layouts, paged help/hold restoration and a RAM-staged presenter:
borrowed rows cannot change before drain; unchanged rows are not republished.
Real SDK builds validate the API use, while ares/hardware remain necessary to
check actual flicker and readability. CI compares revised PAL/NTSC ROM hashes too.


## Compression

`tests/test_compression.py` checks the standalone generator, native 128x64 art,
independent packet concatenation and loader ownership at O0/O3 with UBSan where
supported. Rejected uploads retain their decoded buffer; pending transfers block
reuse/cancellation; no picture is ready before all eight transfers complete.
The checks include failure and 1,000 repeated cancellation cycles.

Opt-in integration builds release PAL/NTSC and debug NTSC ROMs, then runs actual
SGDK RAW/APLIB/LZ4W decoding in release/debug base-M68000 probes. Every output byte
must match the raw source, undersized destinations must be rejected before writes,
and guards must stay intact. The low-redundancy control is deliberately allowed
to grow when an explicit codec is forced. CI compares native ARM64/AMD64 resource
sizes, loader layout and all five ROM hashes, retaining reports/logs under
`build/compression-integration/`.

The comparison ROM measures wall time with the SDK timer across 16 operations;
its VBlank quantization and interrupts make this an estimate, not a cycle count.
Host/target contracts do not validate screen fades, VDP timing, audio contention
or console performance. The Load page demonstrates synchronous decode while
covered; Preload uses independent chunks, not a resumable SGDK decoder. Real
hardware checks should include worst-case foreground work and other DMA producers.

The first playtest mistook the random-noise control for broken APLIB output.
The Lab now labels that sample as intentional, names the next picture before
loading, and checks decoded bytes against the original in the running ROM.
Benchmark verification runs outside the timed batch. The target probe also flips
one decoded bit and requires that comparison to reject it. This separates an
expected noisy image from a genuine decoder/output error without treating noisy
assets as a universal game-art convention.
