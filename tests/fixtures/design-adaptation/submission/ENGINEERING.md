# Engineering handoff

## References and provenance

Consulted the workspace-local `.agents/skills` versions of `sgdk-project`,
`sgdk-input`, `sgdk-ui`, `sgdk-audio`, `sgdk-testing`, and `sgdk-release`.
Relevant supporting references inspected: project `docker-workflow.md`, input
`input-patterns.md`, audio `drivers.md`, testing `regions.md`, and release
`rom-validation.md`. They informed actual API checks, input ownership, bounded
presentation, PSG ownership, region scheduling, and SGDK-specific ROM checksum.
No genre template, combat system, external art or generated media was used.

The project helper initialized the stock SGDK Hello World project. Its boot
startup `src/boot/sega.s` and ROM-header scaffold remain; the cartridge header
was customized. The game simulation, renderer, ten 8x8 tile patterns, PSG cues,
tests and local verifier were written for Last Service. The crisp bitmap font,
startup/runtime and linked library are SGDK assets/code, not original game art.
SGDK's MIT notice is in LICENSE-SGDK.txt. No skill supporting game assets or lab
code were copied. Original game files use the included MIT license.

## Technical contracts

- Simulation owns one fixed 18-service table. States are future, waiting,
  transit, delivered. Admission is timetable order with at most 3 waiting.
  A due service outside the desk is a backlog entry, never lost or overwritten.
- Every journey reserves one bay plus X, Y or both. Validation is read-only;
  the state changes only after validation succeeds. Locks are derived from the
  transit table, so release has no stale cached lock to clear. At most two
  journeys coexist. A journey's 360th logical tick releases everything.
- `selected` is a service ID, not a row. It changes only on navigation or when
  that service leaves waiting. Admission preserves the selection otherwise.
- `main.c` reads cached JOY_1 once per outer frame, forms press edges with
  `now & ~previous`, then maps to desk actions. No callbacks, repeat timer or
  event queue. One input batch is consumed before simulation each frame.
- A 60 Hz logical clock accumulates 60 units per displayed frame and consumes
  50 or 60 units per tick, selected by verified `SYS_isPAL`. At 50 Hz there are
  at most two ticks per frame; at 60 Hz one. The shift is 5,400 ticks: exactly
  4,500 PAL frames or 5,400 NTSC frames, assuming each loop meets its vblank.
- Held/help/finished frames do not advance tick, fractional phase, arrivals,
  journeys or waiting time. Help is independent of held, so prior hold survives.
  Start takes priority over other simultaneous desk inputs. B takes priority
  over Left/Right; Up takes priority over Down; Left takes priority over Right.
- Waiting totals fit in 32 bits (at most 97,200 service-ticks). No heap, file
  access, unbounded queues, animation objects or runtime random numbers.
- The adapter alone owns VDP and PSG. BG_A holds text, BG_B the schematic.
  Ten original patterns start at TILE_USER_INDEX; the SDK font retains its
  own tiles. PAL0 is paper/ink/amber, PAL1 is the restrained warning text.
  Scene changes clear planes; live updates occur on input or at 10 Hz, with
  a bounded 40x11 schematic clear and bounded text fields. No sprites/DMA
  queue ownership is needed; static tile upload uses CPU transfer.
- Audio has one current cue and one bounded frame timer. New cues replace old
  ones. PSG channel 0 alone is audible; all others start muted. The NULL Z80
  driver is explicitly loaded, avoiding a concurrent music driver's PSG writes.
  Acknowledgement: short 660 Hz; rejection: 155 Hz; close: 440 to 330 Hz.
- Source declarations were verified inside the selected image, not invented:
  JOY_readJoypad, SYS_isPAL, PSG setters, Z80_loadDriver, VDP tile/text APIs and
  PAL_setColor. The real adapter compiles and links with that SDK.

## Evidence and limits

`logs/tests.log`: executable C host tests pass with warnings treated as errors.
They exercise all 144 ordered route pairs, exact release/reuse, full queue and
backlog, selection identity, hold/help including frozen fractional phase,
deterministic replay/retry, exact frame counts and PAL/NTSC replay equality at
second boundaries. They do not emulate VDP, PSG, input hardware or 68000 timing.

`logs/build.log`: release compiles without warnings. `logs/rom-inspect.json`:
selected skill's release inspector reports valid, no findings. Local verification
checks title, regions, controller declaration, ROM extent, reset/stack vectors,
no SRAM declaration, 128 KiB padding and SGDK's XOR-folded checksum. SHA-256
identifies exact release bytes; it is not an authenticity signature.

Playtest checklist for caller: title guide and direct Begin; held C must dispatch
only once; B must preserve selected car; try a red route; run disjoint X/Y
journeys; observe release and last-second transit report; hold before an arrival;
open/close guide from running and held; complete/retry/title in both regions;
listen for three cues at normal volume; inspect readability and verify each
loop meets vblank (no measured performance claim). No apps or emulator settings
were opened/changed. No emulator or hardware claim is made.
