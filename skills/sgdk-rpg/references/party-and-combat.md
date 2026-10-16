# Party and combat rules

## Definitions and mutable state

Keep names, base stats, item effects, costs and enemy definitions in `static
const` tables when they do not change. Give stored content stable IDs. A save
record's weapon ID must still identify the same weapon after a content update,
or its version needs a deliberate migration. A pointer into a compiled table
is not a portable saved ID.

The example keeps three different kinds of state:

- **Definitions:** base stats, weapon names and ability descriptions in ROM.
- **Campaign:** current HP/MP, equipped IDs, item counts, XP, gold and route.
- **Battle:** participants, temporary effects, order, RNG and the last event.

Those boundaries are useful, not a required struct layout. Avoid copying every
string or whole definition into every actor. RPGs can have much more content
than fits in work RAM because only the active state needs to live there.
Check banked-resource lifetime separately when a project exceeds its normal
ROM window; a large item table does not itself require a mapper.

In [party.c](../assets/adventure-lab/src/party.c), `member_stats()` starts from
base stats and applies level/equipment once. Switching a weapon repeatedly
cannot accumulate bonuses. Define what happens when a stat change lowers a
maximum below the current value: clamp, preserve an explicit temporary excess,
or reject the change. The Lab's weapons change attack/defense/speed, not HP/MP
caps. Its level-up increases living members' HP by the gained maximum; it does
not silently revive a fallen member.

## An action is a transaction

Separate selecting an action from committing it. During selection a player can
cancel, change targets or discover an unavailable ability without spending MP.
At commitment, validate the current actor, phase, targeting rule, living/fallen
state, cost and stock before mutating anything. Revalidate if your game permits
other actors to move or act while the menu is open.

[The battle module](../assets/adventure-lab/src/battle.c) accepts an `Action`.
These are the Lab's functions, not SGDK calls:

```c
Action action = {ART, 0, target};
if (battle_target(&battle, action, target)) {
    bool committed = battle_commit(&battle, &campaign, action);
    /* Animate the recorded event only after commitment succeeds. */
    (void)committed;
}
```

The second check inside `battle_commit()` remains necessary. An invalid action
leaves MP, item stock, turn order and RNG untouched. Regular healing requires
a living target; revival requires a fallen ally. A full-health Tonic is rejected
without consuming it. The Lab allows a healing spell to be cast on a full-health
living ally; that is an explicit example rule, not an inventory requirement.

For multi-hit, area or delayed abilities, decide whether targets are locked at
commit time, checked again at each hit, skipped if defeated, or retargeted.
Specify that policy before writing the animation. A visual projectile should
not accidentally choose damage or consume an item on every rendered frame.

## Time, order and effects

A round snapshot sorted by speed is easy to reason about. Use a deterministic
tie-breaker, skip fallen participants, and decide whether speed changes affect
the current order or the next snapshot. The Lab samples order each round and
uses actor ID to break equal-speed ties. It does not require active-time bars.
For active-time or action combat, establish whose clocks advance during target
selection, messages, pause and stun; display refresh is not a universal turn.

Give effects units and expiry points. In the example:

- Guard reduces incoming damage until the actor's next turn begins.
- Soaked increases lightning damage and lasts two of the affected actor's turns.
- Weakened reduces attack power for two of the affected actor's actions.
- Falling clears temporary combat effects; revival returns an active actor.
- The boss's charged strike is announced on one turn and executed on its next.

Use widened intermediates before multiplying damage, clamp against current HP,
and bound costs and quantities before narrowing. The Lab uses integer values
and a private unsigned 32-bit PRNG. Its fixed per-encounter seeds make regression
runs repeatable; a real game can choose/save seeds differently. Drawing, sound
and menu navigation do not consume combat randomness.

## Outcomes and checkpoints

Detect victory or defeat after the committed effects. Record the outcome and
claim its rewards once. Repainting a result screen, skipping an animation or
pressing confirm repeatedly must not grant another reward. Define simultaneous
wipe behavior if the game supports recoil or effects on death. The example
uses defeat precedence when neither side survives.

Decide which state persists after battle. The Lab copies surviving HP/MP back,
awards capped XP/gold and replenishes a small amount of stock on victory. Defeat
restores the departure checkpoint, including inventory. This avoids losing
consumables while retrying the same lesson. Other games may choose entirely
different costs or persistence policies.

The checked checkpoint codec stores explicit bytes and validates into a local
candidate. It rejects invalid IDs, caps and impossible all-fallen checkpoints.
The SRAM journal adapter is separate; CRC validation is not proof that a
physical cartridge retained the bytes after power loss.

## Sources

- SGDK 2.11 [numeric types](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/types.h)
  and [memory API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/memory.h).
- [GCC integer overflow behavior](https://gcc.gnu.org/onlinedocs/gcc-16.2.0/gcc/Integers-implementation.html).
- Party, turn order, targeting and checkpoint policies above are original Lab
  game code; they are not services provided by the SGDK library.
