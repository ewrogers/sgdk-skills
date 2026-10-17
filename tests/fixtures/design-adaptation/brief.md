# Last Service

Build a small, playable SGDK ROM called **Last Service**. It is a tram dispatch
desk during a rainy night shift in a fictional, ordinary city in 1998. The player
is the dispatcher, not a character walking through the city.

## Presentation

Use a fixed overhead schematic: three incoming services, four numbered terminal
bays, shared track segments and a clear occupied/free indication. Trams are
small rectangular symbols, not characters or spaceships. Make the desk feel
quiet, workmanlike and slightly wistful: warm paper, charcoal ink, amber lamps
and a restrained red warning. Use a crisp, simple bitmap font. Avoid fantasy,
space, military and heroic imagery. No portraits or character creation.

Create the necessary small pixel graphics yourself. Use original assets for
this game; placeholders may be simple but intentional. Audio should be sparse:
a soft acknowledgement, a distinct rejected-route sound, and a quiet end-of-shift
cue. There is no continuous music requirement. Silence is part of the mood.

## Play

There is one 90-second shift. A deterministic timetable feeds at most three
waiting services at once. A dispatched tram occupies its destination bay and
one or more shared track segments for a short, visible journey, then releases
them. The point is to manage conflicting routes and waiting time, not reflexes.

The controller mapping is deliberately desk-like:

- Up/Down selects a waiting service. Keep its identity stable when possible.
- Left/Right selects one of four destination bays.
- **C** dispatches the selected service to that bay. Reject occupied bays or
  conflicting track segments without losing the service or changing its route.
- **B** clears the current bay selection without advancing the waiting list.
- **A** toggles the shift clock between running and held. No simulation time,
  arrivals or journeys advance while held, but selection and help still work.
- **Start** opens/closes a compact help overlay and holds the simulation while
  the overlay is visible. Preserve the prior running/held state on closing.

Show the selected service and bay, the running/held state, remaining time and
which routes are unavailable. Explain the controls on screen. Do not substitute
the usual A-confirm/B-back gameplay conventions for the assignments above.

Flow: title with Begin Shift and Desk Guide; Begin Shift opens the live desk
directly. After 90 seconds, show a shift report with delivered services, those
still waiting/in transit, and accumulated waiting time. Offer Retry Same Shift
or Title. Retry resets the same timetable. There are no stages, lives, upgrades,
currency, enemies, weapons or bosses. Deliveries are a service record, not a
collectible score or purchase economy.

## Engineering and handoff

Use SGDK and the available installed skills as appropriate. Keep capacity and
ownership explicit: waiting services, active journeys, input edges, display
updates and audio requests must have bounded behavior. Run the shift at the
same real-time duration in PAL and NTSC. Verify unfamiliar SDK calls against
the toolchain. Do not invent engine APIs.

Deliver a release ROM, source and concise controls/build notes. Include executable
tests for conflicts, release/reuse, full queues, clock hold/help, deterministic
retry and PAL/NTSC timing. Include a short engineering note listing the skill
references consulted, any code/assets reused with provenance, and the technical
contracts adopted. Separate what tests prove from what still needs playtesting.
