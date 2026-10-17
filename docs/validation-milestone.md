# Skills working together: first validation pass

This checks a new game built using installed guidance, rather than another copy
of a completed Lab. It is an integration check by the same author, not a blind
fresh-agent evaluation or proof that all 36 skills compose.

## Starting request

Build a two-room courier game: collect a parcel, open a gate, walk into another
room and press A at a delivery seal. Include collision, a HUD, short sound cues,
pause and a scene fade. Build PAL/NTSC variants, inspect the ROMs and package a
release. Use original diagnostic art and game code, without copying a Lab.

| Skills | Responsibility |
| --- | --- |
| Project | Fresh SDK project, Docker invocation and actual tool versions |
| Planning | Partial resource worksheet with explicit unknowns |
| Assets / Graphics | Indexed images, ResComp declarations, sprite and palette ownership |
| Input / Gameplay | One input snapshot, press edges, collision and simulation clock |
| UI / Audio | Event-driven HUD, finite PSG cues with no competing music transport |
| Scenes | Fade, queued-upload drain, sprite release and held-input handoff |
| Testing / Release | Host and M68000 contracts, regional builds, ROM inspection and packaging |

One game model owns simulation and pause; one adapter owns SGDK presentation.
The fixture belongs to `tests/fixtures/composition`, not an installed skill.

## Installation and isolation

On 2026-10-16, `skills@1.7.2` installed all 36 skills from merged main
(`b1ced48`) into a disposable project with telemetry enabled and local copies.
This sends installation activity; it does not establish when skills.sh updates
its cache. No global skills or emulator settings were changed.

The development run used that installed guidance. The header fix was then
applied to the disposable installation and retested. The repeatable container
test copies relevant skill directories into an isolated installation; actual
CLI discovery/copying remains covered by `tests/distribution.py`. CI keeps
telemetry disabled. Neither test invokes a fresh reasoning agent.

Each target starts from the installed project initializer, in a path with
spaces. Generated projects, installation logs and ROMs remain under ignored
`build/` directories. Only the reproducible fixture and tests are committed.

## Findings

**The stock header has a noisy compiler boundary.** GCC 16 warns about SGDK's
space-padded cartridge fields without C string terminators. These fields are
fixed-width metadata bytes. The initializer now scopes only
`-Wunterminated-string-initialization` to its copied `rom_head.c`, guarded for
GCC 16 and later. It preserves the bytes and leaves other warnings enabled.
The target regression requires a clean build and exact padded console/region
fields. Names must still fit their field widths.

**Clocks and input need one owner.** Display-frame input and tick-indexed
replay are compatible but different sources. This fixture applies pause once
before simulation substeps, retains previous input through travel and consumes
delivery once. Contracts cover PAL double steps, held pause, held confirmation
and equal simulated time at nominal 50/60 Hz. They do not prove identical
physical input sampling or recovery from missed VBlanks.

**The final fade upload still needs draining.** The adapter follows existing
Scenes guidance: service the asynchronous fade, then one more VBlank before
releasing resources. It removes the old sprite entry before allocating the
next room's sprite. No conflicting scene/graphics ownership rule was found.

**Planning is not release certification.** The worksheet declares two palette
banks, one 16-pixel hardware sprite, five background tiles and a 16-tile sprite
reservation. It does not count the reservation and its occupants twice. RAM,
SDK table/font storage and DMA remain partial, so it correctly says
`incomplete`. Release inspection separately checks actual ROM padding, range
and checksum. Neither predicts game performance.

**Some mistakes were in the harness.** Incorrect release CLI arguments and a
debug-output-path assumption were resolved using installed help and actual
build output. LTO renamed the direct-entry probe; the probe now retains its
entry symbol explicitly. These are not presented as skill or runtime bugs.

**Model tests missed a physical binding error.** The first playtest delivered
only with the wrong button: raw SGDK masks had been passed into a model whose
logical A bit had a different value. The fix translates using actual SDK
`BUTTON_*` constants. The M68000 probe now tests that A delivers and B/C do not,
using the real header. Input's testing guide now calls out this seam explicitly.
The playtest also exposed unclear navigation: the fixture adds an exit arrow,
a labeled seal, a nearby delivery prompt and a larger interaction area.
Those are usability fixes to this test game, not prescribed game design.

## Evidence and reproduction

```sh
python -m unittest discover -s tests -p test_composition.py -v
```

For container checks, set `SGDK_SKILLS_INTEGRATION=1` and `SGDK_PLATFORM` to
`linux/arm64` or `linux/amd64`, using your shell's environment syntax, then run
the same command. Integration needs Docker, a C17 host compiler and the
repository's existing Unicorn dependency.

Both targets passed with `ewrogers/sgdk:latest`: SGDK 2.11, GCC 16.2.0 and
ResComp 3.95. Four playable region/build variants, two target probes, a
12,000-update trace and the release ZIP matched across architectures. CI
retains `build/composition-integration/composition-report.json` and build logs
and compares the native target reports.

Host contracts run at O0/O3 with undefined-behavior checks where available.
They cover collision at the locked gate, pickup/travel/delivery, one delivery
per press, pause and regional schedules. The M68000 probe runs those model
contracts and real-header button translation; it does not emulate SDK startup,
the VDP, physical controllers or audio.

The ares playtest reached both rooms and delivery. The user confirmed that
the corrected A binding works and the route is clearer. Pause and sound are
not separately certified by that confirmation. Physical hardware is not
tested; matching builds cannot establish its runtime behavior.

## Next cases

Retain this small case as a regression. The [second validation case](validation-streaming-audio.md)
combines bounded map uploads with audio handoff. A later case can exercise
joining/respawns with two-pad input and scene cleanup. A fresh-agent pass
should receive the request and installed skills without this report's suspected failures or desired fixes. Amend
specialist guidance only when an observed failure supports it.
