# States and perception

## Make intentions visible

Separate a decision from its effect. An enemy can acquire a target, turn toward
it, show an alert, wind up and only then resolve an attack. That gives the
player something to react to and makes bugs easier to reproduce.

Enemy Lab uses `IDLE`, `PATROL`, `CHASE`, `RETURN`, `WINDUP`, `RECOVER` and
`STUNNED`. Guards patrol with a forward cone; hunters acquire in any direction;
sentries can fire an eight-way shot. These are game-design choices, not built-in
SGDK classes or a universal state-machine requirement.

`enemy_change()` clears the current route/request and advances a serial number.
A planner result with an older serial cannot overwrite a newer state. A level
load clears the planner and projectile pool. Do not let a coroutine, timer or
queued callback resurrect an enemy's old attack after stun or destruction.

## Decide what an enemy can know

The lab tests a squared distance, a guard's 90-degree acquisition cone and a
conservative ray through solid cells. Diagonal corner crossings check both
adjacent cells. It is cell-based sight, not a pixel-accurate visibility polygon.
The debug ring shows range/cone limits; it does not draw the occluded area.

Acquisition ranges are 112 pixels for guards, 96 for hunters and 144 for
sentries. Tracking has another 32 pixels of range, and guards may keep tracking
outside their initial cone. A visible target is retained; otherwise the nearest
eligible player wins, with P1 breaking a tie. This avoids rapid target swapping.

When sight is lost, the enemy follows its last seen position for up to two
seconds before returning home. It does not receive the hidden player's current
position. Each enemy's sight check runs every six model ticks, staggered by
enemy index. Detection therefore has a deliberate short delay.

## Attacks and stun

Melee winds up for 24 ticks and checks range/occlusion again at impact. Moving
away can dodge it. A sentry locks an eight-way aim direction, faces it and winds
up for 36 ticks; it does not steer a projectile after firing. Both recover for
45 ticks. These values use the lab's nominal 60-tick clock.

The eight fixed projectile slots use one-eighth-pixel coordinates. Cardinal
movement is 3 pixels/tick; diagonal components are rounded to 17/8 pixels.
Shots retire on walls, a player hit or their 90-tick limit. A full pool skips
the optional shot; no active projectile is overwritten.

The player's stun pulse has a 32-pixel range, respects walls and lasts 90 ticks
on the enemy. The `zZ` marker stays visible, with a body flash during the final
60 ticks. `!` and a short sound mark acquisition; firing has a separate sound.
Sounds triggered together are coalesced into the lab's two SFX channels rather
than creating an unbounded event queue.

## Timing, input and presentation

The model advances a nominal 60-tick clock. At 50 Hz, some frames advance two
small model steps; this does not make PAL render at 60 Hz. Pathfinding gets its
budget once per displayed frame, so its maximum searches/second differ by region.
Pause freezes the model and planner while normal VBlank and input keep running.

`JOY_readJoypad()` reads SGDK's cached snapshot. `SYS_doVBlankProcess()` performs
normal frame work including joypad updating; read once per player afterward
and derive held/pressed edges. Do not poll the physical ports again per enemy.
Keep edge history across pause and level changes to avoid repeating held Start.

Sources: [joypad implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/joy.c),
[system processing](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c).

The lab keeps sprite objects alive across retries and area changes. It disables
auto-animation with `SPR_setAutoAnimation()` and supplies model-driven frames
through `SPR_setAnimAndFrame()`. `SPR_update()` and normal VBlank processing still
run during pause. This prevents visual animation from silently continuing on a
separate clock.

Music uses `XGM2_pause()` / `XGM2_resume()` so it retains its place. PCM effects
may finish while the game is paused. Songs and samples remain resident in ROM;
this example does not establish safe bank-remapping behavior for audio.

Sources: [sprite API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h),
[XGM2 API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/xgm2.h).
