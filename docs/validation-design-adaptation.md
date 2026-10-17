# Design adaptation: Last Service

This forward test asks whether the installed skills help implement a different
game, rather than turn their playable Labs into an implicit game template.
It assesses one independent agent submission, not the skills' general success
rate or a controlled comparison between models or instruction versions.

## The request

[Last Service's brief](../tests/fixtures/design-adaptation/brief.md) describes a
90-second municipal tram-dispatch shift. The player operates a fixed schematic
desk, allocating shared track segments and destination bays. It specifies quiet
paper/ink/amber presentation, sparse cues and no continuous music requirement.

The contrast is structural as well as cosmetic:

| Existing examples | What this brief requires instead |
| --- | --- |
| Walking characters, exploration, combat and collectibles | An operator selecting services and routes on a fixed desk |
| Fantasy quests, space shooters, platform courses and stealth encounters | An ordinary city transport night shift |
| Bosses, stages, equipment, upgrades or a collectible economy | One timed shift and a service report; identical timetable on retry |
| Push-block or falling-block puzzle boards | Concurrent route admission with shared resources and waiting time |
| A as the usual gameplay action, Start as pause | C dispatches; B clears the bay; A holds the clock; Start opens help |
| Prominent music and character art | Restrained original schematic art and sparse audio cues |

These differences do not imply that recurring menus, fixed-size arrays or simple
state machines are copying. Those can fit many games. The question is whether
their rules and responsibilities serve this brief.

## Procedure

1. Freeze the brief and [assessment rubric](../tests/fixtures/design-adaptation/rubric.md)
   before launching the agent.
2. Install all 36 skills into a disposable project using the actual Skills CLI,
   with telemetry disabled. This includes the project/planning/best-practices
   guidance introduced in PR #46, not just instructions from repository AGENTS.
3. Give an independent agent only the brief, copied skills and a scoped workspace.
   It has no parent conversation, rubric or instructions naming suspected Lab
   copying. It chooses the useful skills and references itself.
4. Preserve its first submission before assessment or corrections. Review its
   source, tests, provenance and ROM against the frozen gates. Check actual SGDK
   button masks, not just private test inputs.
5. Record failures separately: design leakage, implementation defects and
   unverified presentation are different findings. Do not revise the first
   submission into a passing result and call the original run successful.

The workspace restrictions are instructions, not an OS isolation boundary. All
installed resources are available; the run does not force the agent to read
every Lab. Having examples installed is different from exposing their complete
source in the model's context.

## First-submission assessment

The first submission preserves the requested design in the reviewed source and
visible title. No design leakage was found, but the user's playtest found a
usability failure: flashing text, an overwhelming desk and a dense guide that
did not explain the game clearly. Adaptation and usable presentation are
separate checks. The source was frozen before review;
the 13 retained files in `submission/` are byte-identical to that handoff. The
reviewer added separate checks without rewriting the submitted game.

| Gate | Finding | Evidence |
| --- | --- | --- |
| Identity | Design preserved; live presentation failed usability review | `submission/src/main.c` draws the requested schematic. The title displayed correctly, but the user found the desk overwhelming and reported flashing text. |
| Flow | Pass in source | `main.c` has title, desk and report states, with a help overlay. Begin enters the desk directly; retry resets the same shift. No character selection, stages, combat or reward economy. |
| Controls | Pass in source and target execution | `main.c:103–112` maps actual SDK masks to desk actions; `desk.c:38–54` implements C dispatch, B clear, A hold and Start help. A temporary C-to-A mutation is rejected by the target probe. |
| Dispatch | Pass | The submitted test checks 144 ordered route pairs, capacity, backlog and release. Reviewer checks independently specify the X/Y route table, rejected-state preservation and release. |
| Clock/retry | Pass in model | Exactly 4,500 PAL frames or 5,400 NTSC frames complete the shift; matching second-boundary actions produce equal state. Help preserves both running and held states. Retry restores the initial timetable. |
| Technique transfer | Pass in reviewed implementation | Fixed service storage, admission before mutation, stable service IDs, press edges, a regional phase accumulator and exclusive PSG ownership serve this game's rules. |
| Provenance | No evidence of Lab game copying | No exact matches for the three submitted game C/H files against installed assets; comparison covered 257 asset C/H files. Longest shared sequences were 16 tokens in the clock, 15 in the renderer and 5 in the header: ordinary loop/clock/VDP idioms. Manual review found no imported Lab game flow or incidental content. |
| Evidence limits | Runtime usability failure found | Build/model/mapping checks passed, but did not catch active-display clear/redraw flashing or establish that a new player understood the rules. Audio balance, frame cost and physical hardware remain unverified. |

The agent's engineering handoff reports using `sgdk-project`, `sgdk-input`,
`sgdk-ui`, `sgdk-audio`, `sgdk-testing` and `sgdk-release`, with specific supporting
references. The code is consistent with those contracts. It did not report
consulting genre Labs, planning or best-practices, so this does not establish
that every safeguard or example was read. Correctly avoiding an irrelevant
genre template is itself useful behavior; it is not an example-exposure stress
test. The project skill's new guidance was among the selected entry points.

There are no imported game images or songs: ten small tile patterns are written
in `main.c`, and audio is three short PSG cues. The SDK Hello World initializer
supplies boot/runtime scaffolding and the default font, with SGDK's MIT notice
retained. That is appropriate toolchain reuse, not a renamed Lab.

## Reproduction and limits

[The fixture notes](../tests/fixtures/design-adaptation/README.md) explain the
retained source and how to run `tests/test_design_adaptation.py`. Portable tests
run both the submitted suite and reviewer-owned contracts at O0/O3, with UBSan
where supported. Docker checks reconstruct a fresh project and build JUE, US
and European release headers. The base-M68000 probe includes the actual
submitted SDK adapter and checks its button masks alongside the model contracts.
Its wrong-mapping negative control must fail. This does not emulate VDP, PSG,
Z80 service, controller polling or display timing.

The initial submitted ROM's SHA-256 is
`dd761f9c079423a7c49d9a150046e2e2875049a0554a33806944a80ab4b5df35`.
It rebuilt byte-for-byte with the selected SGDK 2.11 / GCC 16.2.0 container.
CI compares current ARM64/AMD64 output rather than permanently pinning that hash:
a future legitimate toolchain update can change the ROM bytes.

The local ARM64 run and AMD64 run both passed all three case tests; their three
release hashes, model trace and target layout match. ARM64 ran natively on the
Mac and AMD64 through Docker emulation. The target model occupies 122 bytes,
excluding SDK, renderer and stack state. The full portable suite passed 502
tests with 46 opt-in integration skips. CI now retains this case's reports/logs
and compares native Linux runners for both architectures.

The 90-second duration assumes the display loop meets each VBlank; the tests
verify regional scheduling, not load-independent elapsed-time compensation.
The reviewer target wrapper needed a retained entry point because release LTO
can eliminate a constant-return negative-control main. That harness change did
not alter the frozen game or skill instructions.

This is a strongly specified counter-example brief and one independent run.
It supports adaptation under these conditions, not a guarantee of originality
with a vague brief or after reading a whole genre Lab. A useful later evaluation
would use a nearby genre with different rules and deliberate example exposure.
The initial source review did not justify another creative-direction rule. The
subsequent playtest did justify the focused UI corrections below.

## Informed usability revision

`revision/` is an author correction after user feedback, not another blind agent
pass. The original 13-file handoff and its hashes remain unchanged. Both versions
use the same frozen dispatch model; the revision changes presentation and guide
navigation, keeping C dispatch, B clear, A hold and Start help.

The desk now groups waiting trams, bay choices and moving trams into panels.
The selected tram/bay have a clear background highlight; a persistent prompt
names the next action, such as sending tram 101 to bay 1. The title explains the
job before the player begins. Three guide pages cover the goal, controls and
shared-track waits. Left/Right changes pages, C advances/returns, and Start closes
the guide while preserving the previous held/running state.

The old renderer cleared visible text and track areas before redrawing them.
The revision composes both plane maps in RAM, queues only changed complete rows,
and leaves their source immutable until `SYS_doVBlankProcess` drains the queue.
It has no active-display clears or text writes. Maximum publication is 56 row
operations and 4,480 bytes per frame, below the selected SDK's defaults of 80
operations and 7,200 bytes in NTSC. This is the sole queue producer. Two staged /
displayed plane maps consume 8,960 bytes plus small metadata; no runtime heap is
needed. The SDK font slots are retained and seven 8x8 patterns are used for panels,
rails and trams. These are choices for this desk, not a prescribed UI framework.

`tests/test_design_adaptation_ui.py` covers the paged guide, hold restoration,
input consumption on close, visible bounds, a full shift/report/retry and queued
row ownership. Its fake VRAM never receives an intermediate blank frame, and an
unchanged final screen publishes no rows. The real ROM builds against the selected
SDK for PAL/NTSC on ARM64 and AMD64 with matching hashes. Those tests do not
certify physical scanout or whether a new player finds it understandable; the
user confirmed the panelled desk was clearer and no longer flashed. They then
found the guide's “linked bays” explanation misleading: a route depends on both
its origin and destination, so a bay alone cannot predict which track it occupies.
The full host suite now runs 504 tests with 47 opt-in integration skips; the
revised UI's affected contracts also pass after the final guide-close check.

A further informed correction exposes that dependency without changing the model.
Each bay shows the selected tram's U/L requirements; blocked letters turn red.
Blue dotted rails preview the proposed route, while amber rails show traffic
already in motion. The action panel names the blocking line and remaining wait.
The guide gives the exact North/Canal/Market route table instead of grouping bays
as though their dependencies were fixed. Any Canal or cross-over route needs both
lines; North to bays 1/2 and Market to bays 3/4 can run concurrently.

Host checks independently enumerate all 144 candidate/active route pairs and
verify the displayed requirements, blocking colors, rail previews and occupied
bays. They also check staggered release of two conflicting trams and the wait
countdown. This adds presentation coverage to the unchanged dispatch checks;
fresh player confirmation of these route hints remains pending.

The UI skill references now explain the observed clear/redraw failure and the
need to teach an unfamiliar game's goal/first action. They do not require paneling,
paged tutorials or this desk's visual style in other games.
