# Where combat fits

Use the smallest set of skills that matches the task. These are responsibilities,
not mandatory modules or a dependency graph. Existing games can organize them
differently without adopting the Lab's structs or architecture.

| Area | Owner | Combat's integration point |
| --- | --- | --- |
| Startup, active/recovery timing, animation transitions, timed events | sgdk-animation | Receive valid hit windows and interrupt an action after an accepted reaction. Do not create a competing sprite/action clock. |
| Sweeps, rays, overlap/contact semantics, wall sliding | sgdk-collision | Consume candidates; decide damage eligibility and response policy. Do not replace geometric queries with sprite pixels. |
| Movement, fixed-point positions, update loop, reusable pools | sgdk-gameplay | Apply bounded knockback through the existing mover and preserve pooled identity. |
| Enemy detection, intent, target choice, bounded navigation | sgdk-ai | Resolve the attack an enemy requests. Damage policy should not perform a second AI decision. |
| Controller readers and action snapshots | sgdk-input | Consume named actions or the game's existing snapshot. |
| Joining, player ownership, shared cameras and respawns | sgdk-multiplayer | Treat players as combat participants without replacing session ownership. |
| Turn order, party stats, equipment, progression, rewards | sgdk-rpg | Use its stat/damage rules when applicable; real-time contacts are not a replacement for turn-based validation. |
| Sprite sheets, playback and basic effects | sgdk-graphics / sgdk-effects | Present the resulting action and contacts within rendering budgets. |
| Driver choice, mixing and channel priorities | sgdk-audio | Submit bounded semantic cues, rather than playing one sound per overlapping pair every tick. |

Some concepts meet at a seam. Animation already documents attack identity and
stale hitboxes because they affect event delivery. Combat extends that identity
through team/immunity checks, blocked contacts, target reuse and damage batches.
Keep one record authoritative; do not add independent animation and combat
lists that disagree about whether the same swing has hit a target.

Likewise, collision reports that a contact occurred. It does not decide whether
the target can be damaged, whether a shield blocks it, or which reaction wins.
A contact rejected by combat is still a valid geometric result.

## Why the Lab includes adjacent code

Installed skills must work on their own. Copper Court therefore contains a
small action controller, axis-step mover, two direct-pad inputs and illustrative
enemy intent. These make the resolver executable without installing another
skill; they are not alternate replacements for the more capable examples in
those skills. The Lab does not implement general fast-projectile sweeps,
multitaps, scrolling cameras, advanced pathfinding or an RPG stat framework.

All capacities and rules are local choices. Six health points, two damage per
sword hit, an eight-tick buffer, short hit-stop and an arena are examples. Do
not apply them to an unrelated game's style, genre or balance.
