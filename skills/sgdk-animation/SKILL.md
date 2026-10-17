---
name: sgdk-animation
description: Coordinate SGDK gameplay actions with animation transitions, timed events, hitboxes, interruption and completion. Use for attacks firing twice, stale hitboxes, missed cues, action restart/cancel rules and PAL/NTSC animation timing, rather than basic sprite-sheet importing or frame playback alone.
---

# SGDK Animation

Start with the game's action rules: which transitions are allowed, when effects
occur, which frames can hit, and what interruption should cancel. Art, combat
style and the Lab's timing values are illustrative. Do not introduce a mandatory
state-machine framework or make the example's combat rules universal.

## Choose the authority

Keep gameplay time, visual pose and sprite submission distinct. Simple cosmetic
loops can use SGDK automatic animation. An action with damage windows and cues
often benefits from one logical clock selecting both its pose and its hitbox.
Do not let automatic playback and a manual player both advance the same sprite.

- [Action clocks and transitions](references/action-clocks.md): holds, explicit
  restart, completion, input buffering, cancellation, pause and PAL/NTSC clocks.
- [Events and hitboxes](references/events-and-hitboxes.md): boundary events,
  mirrored local boxes, attack identity and interrupted-action cleanup.
- [SGDK presentation](references/sgdk-presentation.md): manual/automatic playback,
  callbacks, ResComp indices and bounded sprite uploads.
- [Animation Lab](assets/animation-lab/README.md): native pose sheets, a training
  dummy, slow motion, diagnostics, controls and validation limits.

## Make boundary behavior explicit

Define what happens on action entry, every active tick, the last held tick and
interruption. A frame-entry event should not repeat just because its frame lasts
several ticks. Completion needs one delivery after the final hold, not a repeated
test of the last frame. An explicit restart resets timing and per-action hit
tracking, even if the new action uses the same animation as the old one.

Process every logical step's events before rendering the final pose. If one
display frame contains two steps, a single overwritten event variable or a
final-frame-only comparison can lose cues. Bound the work and event storage.
Use a consistent policy for competing requests and invalidate future hit windows
when an attack is interrupted. Do not undo damage that already occurred unless
the game's rules explicitly call for rollback.

## Prove timing and contacts

Pull `ewrogers/sgdk:latest` and verify the selected headers, resource format and
implementation. The Lab is checked against SGDK 2.11 / GCC 16.2.0. SGDK's sprite
engine presents frames; it does not implement the Lab's attack controller.

Test exact holds, first/last active ticks, repeated requests, buffer expiry,
interruption, mirrored contacts, completion, pause and nominal 50/60 Hz schedules.
Preserve target generations when hit records can outlive pooled objects.
Compare host/target behavior and linked frame data, then play the ROM to inspect
pose readability, audio cues, hitbox alignment and frame-time headroom.

`sgdk-graphics` covers basic sprite rendering, `sgdk-gameplay` broader collision
and movement, and `sgdk-audio` sound-driver policy. They are optional complements;
this skill and its Lab install independently.

For team/immunity checks, guard, damage batches and reactions after a hit
window produces contacts, use sgdk-combat when available. Keep the action clock
and per-attack hit identity authoritative rather than duplicating them.
