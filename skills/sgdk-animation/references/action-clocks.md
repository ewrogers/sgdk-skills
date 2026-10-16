# Action clocks and transitions

## Separate an action from a sprite row

A gameplay action may contain startup, active and recovery phases. Its visible
poses can come from one sprite row or several resources. A sprite frame number
alone does not describe input permissions, damage, movement or completion.
Keep a small explicit action record rather than hiding these rules in scattered
checks against renderer state.

Animation Lab uses `AlClock` with clip, frame, elapsed time, action age, an
entry flag and a completion latch. `AlFrame` stores a hold, visual pose, entry
events and a local attack box. These are original example types, not SGDK API.
Other games can use their own data layout, callbacks or simpler state records.

## Define a tick precisely

At each logical tick, the Lab samples the current frame's pose and box, delivers
its entry events once, then consumes one hold tick. Only after that does it
advance to the next frame. Completion is emitted at the end of the final held
tick. The last pose stays selected until the action controller chooses its next
state. Loops re-enter frame zero and can repeat footstep markers; they do not
emit one-shot completion on every loop.

The Lab's example attack is 26 ticks at a nominal 60 logical ticks/second:

| Tick range, starting at zero | Phase | Behavior |
| --- | --- | --- |
| 0–7 | Startup | Brace; no attack box |
| 8–13 | Active | One swing cue at 8; two visible slash poses; active box |
| 14–17 | Recovery | Follow-through; cannot cancel yet |
| 18–25 | Recovery | A fresh or still-buffered request may start another attack |

Durations and permissions are game design choices. Do not prescribe them for
another game. The point is that the code, event markers and collision windows
agree about boundaries and preserve the final pose's full hold.

## Restart and interruption

An explicit `al_play` resets the example clock, including when replaying the
same clip. Attack entry also clears its single-target hit latch. Returning the
same animation index to SGDK is not a substitute for resetting gameplay state.

The Lab prioritizes knockout/hurt over attack and movement. Hurt replaces the
clock immediately, discards buffered attacks and selects an empty attack box.
Future swing markers from the interrupted attack cannot fire. Damage already
committed stays committed. Death holds its final pose and allows revival only
after its completion latch is set. This is one policy, not an engine requirement.

Input buffering belongs to action logic. The example keeps a new attack request
for four logical ticks and consumes it when an allowed transition occurs.
Holding A does not repeatedly generate requests. Slow motion retains an input
edge until the next simulation step; pausing deliberately discards new action
edges. If a game buffers inputs while paused, define and test that separately.

## Use one time base deliberately

The Lab's nominal scheduler performs 60 logical steps over 60 NTSC or 50 PAL
display frames. A PAL frame can contain two steps, whose events are handled
individually. Rendering submits only the final pose once per displayed frame.
Slow mode consumes one of every four logical opportunities. Pause freezes action
ages, movement and hit windows without letting queued action presses build up.

This does not recover time lost to missed VBlanks. A real elapsed-time catch-up
policy needs a work cap and a decision about input/history and contact sampling.
If advancing a clip by an arbitrary time delta, traverse crossed boundaries in
order; jumping to the resulting frame can lose events or an entire active window.
The Lab avoids an unbounded delta walk by keeping logical steps fixed and bounded.
