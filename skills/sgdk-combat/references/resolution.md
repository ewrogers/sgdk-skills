# Resolution, identity and reactions

## The Lab's sequence

The frame wrapper snapshots fresh controls once per display and runs a 60-tick
logical schedule on 50/60 Hz. A normal logical tick:

1. Advances cooldowns, actions, movement and enemy requests.
2. Collects active melee contacts and each bolt's first target.
3. Filters against a pre-damage snapshot, then aggregates damage per target.
4. Applies defeat or hurt, knockback, immunity and bounded feedback.
5. Evaluates all-down and round-clear state after damage has settled.

This allows simultaneous attacks to trade. A later candidate is not rejected
because an earlier one just set immunity or defeated its source. Distinct
contacts in that same batch add damage; invulnerability protects against later
batches. If the desired game instead gives parries, throws or attacks priority,
resolve that policy explicitly before damage rather than relying on array order.

Knockback chooses the strongest hit; ties use the lower source slot. Thus
reordering equal-tick candidate delivery does not change that reaction. The
resolver caps authored damage per contact and subtracts with a floor at zero;
unsigned health must not wrap into a large positive value after a killing blow.

## Pose-aligned contact geometry

The Lab uses four sword poses, each held for two active ticks. A local box is
authored around each leading blade pose, then quarter-turned around the same
pivot and forward offset as the native weapon pixels. A short forearm region
bridges body to blade, preserving close hits while extending reach. Rendering and contact selection read the
same pose index. Tests verify leading blade pixels remain inside the damage
box in every orientation and that up/down are not accidentally reversed.
These are conservative rectangular approximations, not pixel-perfect collision.
Use the project’s existing animation/hitbox data when integrating this pattern;
do not maintain a second competing pose clock in combat.

## Guard and contact consumption

Guard checks the direction from attacker toward defender against the defender's
facing. A negative dot product means the attack came from in front. Exactly
perpendicular contact does not block in this example. An unblockable flag
bypasses guard. This is a chosen front half-plane, not a universal shield shape;
some games need a narrower cone, shield geometry or explicit attack heights.

The swing records an overlapping target before immunity/guard resolution. A
blocked or immune contact therefore consumes that swing's opportunity against
that occupant. It cannot repeat a clang every active tick, or wait inside an
immune target until protection ends and then deal damage. A new swing resets
its record. Multi-hit moves should have explicit pulse identities or a chosen
repeat interval instead of disabling deduplication globally.

The Lab’s enemy intent can attempt to guard an approaching bolt. It uses a
staggered check, seeded chance, six-tick reaction delay, brief hold and cooldown;
a missed attempt is not rerolled every tick. Sentinels also have intermittent
ordinary guard phases. This illustrates AI intent, not a rule the damage
resolver imposes. Guard direction remains locked during a reactive hold.

## Pooled identity and lifetime

A contact record is `(target slot, generation)` within the current attack.
Reusing the slot increments its generation, so the new occupant can receive a
hit without confusing it with the old target. A pending damage candidate also
captures the target generation and is ignored after reuse.

Bolts capture owner slot, generation and team. In the Lab they may finish after
their owner's defeat; owner reuse invalidates them. Stale bolts do not inherit
the replacement's team or credit. The finite 90-tick lifetime and bounded round/
respawn reuse rate prevent a 16-bit generation from cycling back while an old
bolt or swing remains alive. Longer-lived references or arbitrary churn need
an explicit wrap policy or a wider generation, not a claim that integers never
wrap. Target generation alone is not enough if attacks are also pooled and
their records outlive their attack instance.

## Bounded work and overload

Six actors give at most 30 directed melee pairs. Four single-hit bolts add
at most four contacts, for 34. The 40-entry batch leaves explicit headroom.
An oversized external batch returns false before mutation. Raising actor/shot
capacities requires revisiting this derivation and the target's stack/RAM cost.
Do not silently truncate candidates and favor early actors.

Shot allocation is separate: a full pool drops the new bolt. Its cast still
consumes cooldown in this example. Other games may reserve a shot before
starting the cast or refund it; choose the rule deliberately. Cosmetic effect
reuse is allowed and cannot alter damage. The Lab's slow, bounded bolts use
endpoint overlap, not a general swept solver. Use the collision skill's
queries for fast/narrow projectiles or exact moving-target contact requirements.

## Freeze and interruption

Pause stops gameplay. Hit-stop freezes movement, action time, immunity,
cooldowns and effect lifetimes, while input snapshots still update. A fresh
sword request can enter the bounded buffer during hit-stop. The buffer then
expires after eight ordinary world ticks, including time spent attacking or
recovering. C is not buffered. These distinctions avoid indefinite deferred
attacks and prevent the second PAL logical step from delivering a press twice.

Hurt replaces the action; its old hit window immediately stops contributing.
Knockback uses one-pixel axis subdivisions against the authored arena solids.
Hurt duration and immunity differ; do not use blinking as the authority for
whether an actor can take damage. The renderer reads state after simulation.

Regression tests exercise guard directions and bypass, same-tick trades, damage
aggregation/ties, repeated contacts, stale targets/owners, action interruption,
pool exhaustion, pause/hit-stop, buffer expiry and a seeded 20,000-frame soak.
Base-M68000 probes execute the same model, not a separate reference rewrite.
