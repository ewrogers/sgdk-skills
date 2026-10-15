# [Game or feature] plan

Adapt this template to the request. Remove these instructions, replace the
bracketed text, and drop sections that do not help the next decision.

## What we are making

[Describe what the player does and what makes the idea recognizable in a few
sentences. Note any existing project decisions that must be preserved.]

## First playable milestone

[Describe one small playable situation, its controls, and an observable success
condition. Include a representative crowded moment or expensive feature.]

**Assumptions:** [Region/display mode, intended update rate, players/controllers,
SDK/toolchain versions, cartridge target. Separate confirmed choices from defaults.]

## Resource outlook

Use measured / estimated / unknown for game-use figures and state their basis.
Keep hardware limits, SDK reservations and chosen project headroom distinct.
Leave cells unknown instead of inventing precision.

| Resource | Planned peak use and basis | Relevant limit or reservation | Next check |
| --- | --- | --- | --- |
| Main RAM | [Game state, SDK, buffers, heap, stack] | [Actual target layout/reservations] | [Link/map/sizeof/stack check] |
| VRAM | [Resident patterns and all tables] | [Configured layout] | [Real exports and allocation layout] |
| Palettes | [Which assets share each bank] | [Four shared banks] | [Representative art/transition] |
| Sprites | [Hardware pieces and busiest scanline] | [Selected video mode] | [Actual cutting and overlap] |
| Transfers | [Busy-frame uploads and temporary storage] | [SDK configuration/project ceiling] | [Peak frame probe] |
| ROM | [Content and padded file separately] | [Cart capacity/mapping] | [Actual linked and padded sizes] |
| CPU/audio | [Measured or unknown, with workload] | [Chosen update target/driver] | [Optimized repeatable probe] |

## Risks worth proving early

| Risk or unknown | Smallest useful test | Evidence needed before expanding |
| --- | --- | --- |
| [Specific uncertainty] | [A representative probe] | [Build, visual/audio or timing check] |

## Growth path

1. [First playable outcome.]
2. [Next feature that changes the hardware workload.]
3. [Content/polish after that risk is understood.]

**Deferred:** [Feature, reason, and the point at which to reconsider it.]

## Evidence and next action

[List only checks that actually ran, with build settings/artifact references
where useful. State what remains untested.]

**Next:** [The one implementation or feasibility step that follows this plan.]
