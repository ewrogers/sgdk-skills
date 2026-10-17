# Validation case 2: streaming and audio handoffs

This follows the [courier composition check](validation-milestone.md). It uses
installed skills in a disposable project to test their integration boundaries.
It is not a blind fresh-agent evaluation or evidence that every skill composes.

## The request

Build a scrolling route with two music zones across four art sections. Keep
the hero and HUD resident, stream map patterns within a fixed allowance, and
provide covered warps, fanfares and pause. Test ordinary movement, reversing at
a boundary, a cold warp, a fanfare overlapping travel and failed admission.

Controls: Left/Right walk; A warps between ends; B selects 64/256-byte pattern
uploads; C requests a fanfare; Start pauses. Pause requested during a transition
waits until the visual fade and observed music startup have finished. A second
press cancels a pending pause. Art, music and policies are illustrative.

## Installed components and ownership

The local run installed nine skills from merged main `447512e`, using
`skills@1.7.2` with project-local copies and telemetry disabled. The previous
milestone already performed the requested telemetry-enabled refresh. No global
installation or emulator settings changed.

The new fixture uses Streaming's cache, world helpers and bounded DMA adapter,
plus Audio's director. It uses their original generated art/music fixtures too.
It does not copy either Lab's complete main loop. A new coordinator and renderer
adapter join the components in `tests/fixtures/stream-audio`.

| Owner | Responsibilities |
| --- | --- |
| Coordinator | Input edges, camera proposal, pause request and transition state |
| Tile cache | Pins, staging buffer and readiness after a drained upload |
| Renderer | Persistent columns, palettes, covered redraw and scroll publication |
| Audio director | Latest requested music, fade ownership, observed startup and fanfare return |
| Frame loop | One input snapshot and normal VBlank service; drain before reusing borrowed RAM |

Project initializes/builds the standalone project. Graphics/Input/Scenes guide
the renderer and controls; Testing supplies evidence boundaries; Release checks
the actual ROM. Planning remains separate from measured runtime performance.

## Budgets and clocks

- Pattern uploads: at most 256 bytes per presented frame, or 64 in slow mode.
- Shared DMA allowance: 1,536 bytes, including map columns, palettes, HUD and SAT.
- Covered redraw: at most four 24-word columns per frame.
- Cache: 18 slots of 16 tiles; 24 catalog pages. Hero tiles and panel lie outside it.
- Four palette banks and one hardware sprite. The font/HUD remain resident.

The fixture uses 160 pixels of prefetch margin in both directions. At 64 bytes,
one area's 3,072 pattern bytes need 48 uploads. This is a tested combination of
speed, art and allowance, not a universal default or a CPU/DMA hardware limit.

Movement uses nominal 60-tick simulation on 50/60-Hz displays. The visual fade
advances with completed presentation updates; music fades run through SGDK's
VBlank service. The audio adapter measures actual elapsed `vtimer` VInts for
startup deadlines and effect leases, including skipped main-loop updates.
Music retains its source tempo. The game does not recover an arbitrary backlog
of missed simulation ticks. The tests do not measure console CPU load.

## Findings and changes

**Queue rejection must not advance the handoff.** A rejected final black-palette
upload cannot authorize cache recycling. The coordinator retains the prior
fade step and retries it. It also refuses another frame while the previous
frame still borrows RAM. Acceptance and drain are distinct events.

**Palette replacement belongs to view publication.** Entering columns are
admitted before reusing a palette bank. If admission fails, the old view stays
valid. Rejected palette admission restores its RAM colors too, so a later fade
cannot accidentally use a palette for an unpublished view. Streaming Lab now
uses this ordering and restores rejected fade steps; the streaming guide
explains the rule.

**Audio completion is observed, not inferred.** Delayed play commands do not
end a fanfare. Zone reversals during a fade coalesce to the latest request, and
travel during a fanfare changes its return destination. A pause request waits
for the owning transition rather than trying to freeze a VBlank fade task.

**Waiting needs a fallback.** If the driver never starts, the director times
out without repeatedly issuing play. A ready destination is then revealed with
silence instead of remaining black forever. This policy is explained in the
audio-management guide; it is not a guarantee about SGDK load times.

The real SDK build also exposed two fixture assumptions: a private `clamp`
function collided with an SDK macro, and the target library lacks hosted C's
`memcmp`. The fixture uses a prefixed helper and a small byte comparison. These
are integration mistakes, not compiler bugs or defects in the original skills.

## Evidence

`tests/test_stream_audio.py` runs O0/O3 host contracts with undefined-behavior
checking where available. The host checks every visible map cell and palette
owner after each frame, staged patterns after drain, rejection recovery, budget
bounds, eviction, reversal, warp, deferred pause and an unresponsive driver.
A skipped-update case verifies the audio startup deadline uses elapsed VInts.

The target probe runs the coordinator/cache/director contracts on a base M68000
and checks real SGDK A/B/C/Start mapping. It samples visible map cells instead
of repeating the host's full scan. Its explicit 200-million-instruction cap is
a test-run ceiling, not a cycle measurement; older probes retain their cap.

Both ARM64 and AMD64 builds passed using `ewrogers/sgdk:latest`: SGDK 2.11,
GCC 16.2.0 and ResComp 3.95. PAL/NTSC release/debug ROMs and both probe ROMs
match across architectures, as do the traces and target layout. `sizeof(Route)`
is 586 bytes on the target; that excludes renderer/SDK state and the stack.
CI retains
`build/stream-audio-integration/stream-audio-report.json` and logs and compares
the native-runner results.

The user confirmed traversal, transitions, music/fanfare handoffs and pause in
ares in NTSC and PAL, including the 64-byte setting. No physical hardware was
tested. Bare-M68000 execution does not emulate VDP DMA, Z80 service or audible
quality; the fault-injection backend checks protocol behavior rather than
simulating physical bus contention. Original Streaming Lab changes are built
by its existing integration tests; its complete main loop is not run by the
new fault-injection backend.

## Repeat the automated check

```sh
python -m unittest discover -s tests -p test_stream_audio.py -v
```

For container checks, set `SGDK_SKILLS_INTEGRATION=1` and `SGDK_PLATFORM` to
`linux/arm64` or `linux/amd64`, using your shell's syntax. Integration requires
Docker, a host C17 compiler and the repository's Unicorn dependency. The tests
create isolated project-local copies. CLI installation remains separately
covered by `tests/distribution.py`, with telemetry disabled in CI.
