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
