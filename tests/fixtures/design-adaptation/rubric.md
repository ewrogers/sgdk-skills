# Design-adaptation assessment, frozen before the forward run

The evaluator receives `brief.md`, installed skills and an empty project folder.
It does not receive this rubric, the prior conversation or a list of suspected
failures. The run has scoped filesystem instructions, not an OS security sandbox.
Record the exact prompt, input hashes and first submission before making fixes.

## Gates

1. **Identity:** the result is a municipal dispatch desk, not an avatar game with
   a new skin. Inspect source, generated art and ROM presentation. Quiet paper /
   ink / amber presentation and sparse cues must not become fantasy or sci-fi
   adventure art, an upbeat soundtrack or a heroic plot.
2. **Flow:** title -> desk -> one shift report -> same-shift retry/title.
   No unsolicited character selection, level sequence, bosses, combat rewards,
   upgrades, inventory, currencies or narrative quest chain.
3. **Controls:** real SGDK button masks implement C dispatch, B clear selection,
   A clock hold and Start help. Closing help restores the previous hold state.
   C rejects safely; B neither dispatches nor discards a service. Inspect the
   SDK adapter as well as pure logic; a test of private fake button constants
   is insufficient. Title navigation can use ordinary menu conventions.
4. **Dispatch rules:** shared-segment conflicts and occupied bays prevent route
   admission. Completed journeys release resources. Queue overflow has an
   explicit policy; there is no unbounded allocation or silent state overwrite.
5. **Clock and retry:** 90 real seconds at PAL/NTSC, held simulation freezes all
   arrivals/journeys/waiting-time accounting, help holds without toggling the
   user's hold, retry reconstructs the same timetable and initial state.
6. **Technique transfer:** identify concrete skill guidance used for bounded
   state, time, input, VDP transfers and/or audio lifetime. Distinguish reusable
   contracts from importing a Lab's complete main loop, module layout or game
   state machine. Reusing a small suitable helper is allowed with provenance.
7. **Provenance:** compare the first submission against installed Lab source and
   assets. Exact asset hashes and long token sequences are leads for review,
   not plagiarism verdicts: SDK boilerplate and generic C patterns are expected.
   Check semantic similarities manually, including incidental names or constants.
8. **Evidence:** executable behavior tests pass, a real SGDK release build works,
   sources/APIs are inspectable and remaining emulator/hardware checks are honest.
   A successful build does not establish readability, audible quality or 60 FPS.

## Interpretation

Record each gate as pass, fail or unverified with file/line evidence. Separate
design leakage from unrelated implementation bugs and from reasonable choices
left open by the brief. Do not grade against the evaluator's preferred aesthetic
or require an arbitrary architecture. The absence of copied words is not proof
of original design; a superficially different theme is not sufficient either.

This is one descriptive forward test, not a statistical guarantee, model ranking
or controlled before/after experiment. If the first result fails, preserve it,
make only evidence-driven instruction changes, and label any retry as informed.
