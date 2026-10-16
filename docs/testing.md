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
