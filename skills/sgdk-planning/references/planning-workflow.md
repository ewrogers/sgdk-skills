# From an idea to a playable proof

The plan should help someone make their game. Start with what the player does,
then explain the few hardware decisions that affect the next build. Move the
calculations below that summary so they do not obscure the goal.

## Scope the first milestone

Keep the main interaction and one representative challenge. For a platformer,
that could be running and jumping through one short scrolling area with a few
enemies. For a shooter, it could be one enemy formation, player shots and a
single enemy attack. Menus, level counts and extra effects can grow afterward.

Inspect an existing game's source, resources and build settings before choosing
its architecture. Prefer SGDK's established subsystem and sample patterns
where they fit. Neither a scene graph nor a heapless runtime is a prerequisite.

State assumptions briefly: for example, "First proof: one player, NTSC,
320 by 224, optimized release build, standard cartridge mapping." These are
scope choices, not SGDK limitations. If PAL or multiplayer is a requirement,
keep it visible and plan a representative early check rather than quietly
dropping it. Do not infer a project's region or controller requirements from
an unrelated example.

## Plan what can exist together

Budget a busy moment, not the sum of every asset in the game. A whole level can
have more art in ROM than fits in VRAM at once. A boss may replace regular enemy
graphics, or it may overlap them during its entrance. Those are different plans.

Describe the important simultaneous combinations: active entities, animation
frames, background areas, HUD, particles and sound effects. Include a transition
or spawn burst if it temporarily needs both the old and new resources.

Use ranges or mark a value unknown when the source material is incomplete.
Record where a number came from: an actual export, a linker section, a planned
struct layout, or a rough art allowance. An estimate should not look like a
measurement merely because it has many digits.

### Example: a small vertical shooter

An initial proof might have one moving ship, one enemy formation, shots, a hit
effect and a score. Before adding a full level, test this representative peak:

| Object | Simultaneous count | Assumed hardware pieces each | Width of each piece |
| --- | ---: | ---: | ---: |
| Ship | 1 | 1 | 16 pixels |
| Enemy | 8 | 1 | 16 pixels |
| Shot | 12 | 1 | 8 pixels |
| Hit effect | 4 | 1 | 16 pixels |

These are example design choices, not recommended universal limits. Check
ResComp's actual cutting before relying on one piece per image.

The total is 25 hardware pieces, below H40's whole-screen limit of 80. However,
if all 25 cross the same scanline, they exceed its 20-piece limit. Their combined
width is 304 pixels, so the separate 320-pixel sprite-width limit would not be
the first failure in this particular arrangement.

A useful proof deliberately creates that overlap, then tests a revised
formation or effect policy. Simply raising a software object pool cannot fix
it. Also check the busiest animation/upload frame; a sprite count does not
predict transfer cost or CPU load.

## Separate the decisions from the evidence

Use three labels consistently:

| Label | Example | What to do next |
| --- | --- | --- |
| Measured | The current release ELF has a recorded `.bss` size. | Keep the build settings and artifact reference. |
| Estimated | Twenty enemies will each need the planned state structure. | Confirm target layout once the structure exists. |
| Unknown | A new animated boss has not been exported yet. | Export a representative frame set before promising its budget. |

Hardware ceilings are facts, not measured game use. SGDK reservations are
version/configuration facts. Project targets, such as leaving room for another
enemy type, are choices. Keep them in separate columns or explain them in the
text instead of presenting all three as one official SDK limit.

The optional [static worksheet](budget-format.md) helps catch arithmetic and
declared-limit problems. It cannot discover omitted allocations, infer sprite
positions, or decide whether an update will finish on time.

## Choose proofs that resolve real risks

| Risk | Useful check | What it does not prove |
| --- | --- | --- |
| Unfamiliar SDK/resource behavior | Inspect the installed declarations/manual and compile a small example. | Correct display, audio or timing. |
| Large or animated art | Export real frames, inspect generated resources, preview in a ROM. | That every future formation fits. |
| Crowded sprites | Test representative positions and worst-case overlap. | DMA and CPU headroom. |
| Scrolling or heavy frame changes | Measure peak transfer/decompression work in the intended video mode. | The rest of the game is free. |
| Collision-heavy gameplay | Profile a repeatable busy case in an optimized build. | Every possible level will run at the same rate. |
| Large physical cartridge | Check linked content, padding, mapping and the actual board's support. | Compatibility with unrelated boards or flashcarts. |

Use a fixed seed or repeatable inputs when comparing builds. Record the update
rate and what the measurement counts. Host CPU usage and emulator presentation
FPS do not by themselves measure 68000 game work. Compare an overlay on and off
when instrumentation could influence a close result.

If a probe fails, explain the limiting resource and offer a small number of
meaningful choices. Sharing tile art helps VRAM; staggering effects helps
scanlines; reducing repeated uploads helps transfers. Lowering the update rate
does not increase palette space, ROM address space or the VDP's per-scanline
sprite limit. Do not reach for assembly before identifying a real hot path.

## Grow from the proof

Write milestones as observable outcomes. "One room scrolls correctly, controls
work, and the crowded encounter stays within the chosen timing target" is more
useful than "finish rendering system."

After the first proof, add the feature most likely to change the budget: more
simultaneous enemies, a second player, a large boss, streamed map content or a
new audio combination. Recheck the relevant budgets rather than demanding a
full hardware audit for every small content change.

Leave deferred features with a reason and a revisit condition. For example,
"Mapper support is deferred while the padded ROM remains within the standard
4 MiB window; revisit when exported content approaches that limit." Do not
promise a later feature is trivial merely because it is not in the first build.
