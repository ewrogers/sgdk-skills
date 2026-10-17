# Shared-camera session patterns

## One input display, possibly two gameplay ticks

Bellkeepers uses a 60-tick simulation with a phase accumulator for 50/60 Hz
displays. Fresh edges are processed once per display, and are cleared for the
second PAL simulation tick. Pause stops simulation time; it does not prevent
snapshot processing for joining or resuming. This establishes logic cadence,
not proof that rendering meets its deadline on either video system.

A participant holds a logical device ID, position, health and independent
cooldowns. The session holds device history, shared progress and pause state.
The same Leave press cannot also Join. A rejected fifth join must be pressed
again when space opens. Last-player departure resets the next session; an
individual departure does not erase everyone else's work.

## Joint camera constraints

The Lab's 320 by 184 gameplay view allows live centers to span 256 horizontally
and 112 vertically. These are illustrative margins for its sprite size. First
calculate all proposed moves, then cancel outward moves that widen an excessive
span. Inward movement remains available. Validate wall clipping against the
final group span too; a correction at a corner must not push another character
outside the agreed framing. Players do not block one another here.

The camera centers the live bounding box and clamps to the world. A downed
player is excluded. With no live player, hold the current camera until a return.
Do not assign the camera permanently to the lowest numbered participant unless
that is an intentional game rule.

## Bounded independent returns

After two nominal seconds, a downed keeper searches 81 nearby cell centers
around the surviving group's midpoint. Score walkable, camera-compatible
candidates by distance with penalties for nearby enemies and crowding. A lack
of safe candidates leaves the keeper down and retries after 15 ticks. No
unbounded whole-map search runs in an update. If everyone is down, the first
return uses the entry checkpoint. Returns grant temporary invulnerability.

Make the distinction between preference and guarantee explicit: avoiding moths
is a penalty in this example, not a promise of an entirely empty spawn area.
Other games may require a hard exclusion radius or a protected checkpoint.

## Visible cooldown and stun feedback

Keep effect state distinct from the global clock: a stunned enemy’s patrol
clock stops while its recovery counter ticks. Resuming uses the same patrol
phase, not a jump to wherever the global clock has advanced. The Lab displays
a dazed pose and a short cast pulse instead of hiding stunned enemies. Its A
meter fills over 90 simulation ticks, pauses with the game, and needs a fresh
button edge to cast again. Health meters share each keeper’s identifying color; the recharge bar is white.
These are feedback examples, not required art or combat design.

## Verification checklist

- Join from either direct port and non-consecutive adapter IDs. Try a mixed
  pad/Team Player setup and two Team Players. Check EA separately.
- Hold Start through setup, join during pause, press two Starts together, leave
  and immediately join again, and attempt more joins than the chosen cap.
- Move players apart, regroup, press into corners, dash into walls and down one
  participant while others move. Test all-down recovery and blocked spawn space.
- Collect one pickup with two equally close players; verify one award and fair
  repeated ties. Preserve team progress across an individual departure.
- Check release/debug target behavior and equivalent PAL/NTSC simulation time.
  Use the CPU tour for four-player presentation, then actual controllers for
  reader behavior. Record emulator region/version and physical adapter models.

Host tests execute the actual model at O0/O3 with undefined-behavior checks and
20,000 seeded frames. Stun regressions cover frozen patrol position, damage
prevention, half/full recharge, paused recovery, held-button suppression and
movement resuming after the effect ends. A separate Python bounding-box oracle checks camera
results. Container tests compile actual sprite/font/map resources and execute
the model contracts on a base-M68000 CPU emulator. These do not emulate VDP,
audio, SDK startup or peripheral polling. They cannot certify physical adapters,
analog sound, PAL presentation or frame-rate headroom.
