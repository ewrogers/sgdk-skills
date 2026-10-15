# States and perception

## Make intentions visible

Separate a decision from its effect. An enemy can acquire a target, turn toward
it, show an alert, wind up and only then resolve an attack. That gives the
player something to react to and makes bugs easier to reproduce.

Enemy Lab uses `IDLE`, `PATROL`, `CHASE`, `RETURN`, `WINDUP`, `RECOVER` and
`STUNNED`, plus the Shocker's active `ZAP`. Bombardiers patrol with a forward cone;
Shockers acquire in any direction; sentries fire an eight-way shot.
These are game-design choices, not built-in
SGDK classes or a universal state-machine requirement.

`enemy_change()` clears the current route/request and advances a serial number.
A planner result with an older serial cannot overwrite a newer state. A level
load clears the planner and projectile pool. Do not let a coroutine, timer or
queued callback resurrect an enemy's old attack after stun or destruction.

## Decide what an enemy can know

The lab tests a squared distance, a Bombardier's 90-degree acquisition cone and a
conservative ray through solid cells. Diagonal corner crossings check both
adjacent cells. It is cell-based sight, not a pixel-accurate visibility polygon.
The debug ring shows range/cone limits; it does not draw the occluded area.

Acquisition ranges are 112 pixels for Bombardiers, 96 for Shockers and 144 for
sentries. Tracking has another 32 pixels of range, and Bombardiers may keep tracking
outside their initial cone. A visible target is retained; otherwise the nearest
eligible player wins, with P1 breaking a tie. This avoids rapid target swapping.

When sight is lost, the enemy follows its last seen position for up to two
seconds before returning home. It does not receive the hidden player's current
position. Each enemy's sight check runs every six model ticks, staggered by
enemy index. Detection therefore has a deliberate short delay.

## Attacks and stun

Bombardiers warn for 24 ticks before throwing toward a locked, grid-aligned
landing spot within 96 pixels. Sight and lobbing may cross one solid grid cell;
thicker cover blocks them. Ordinary sight for other enemies remains blocked by
any solid cell. This is a deliberate low-cover rule for this enemy, not general
vision through walls or a hardware restriction.

Each Bombardier has one bomb slot. Flight lasts 30 ticks, with a visual arc,
then a marked 60-tick fuse. The 18-tick explosion covers its center cell plus
up to two cells along each cardinal direction. Each arm stops at the first
wall or arena edge. Rendering and damage use the same cell list; diagonals
outside the cross are safe. A live bomb is never overwritten. Scene changes
clear the slots, pause freezes them, and stun does not recall a bomb in flight.

Shockers lock their facing during a 24-tick wind-up, then activate a one-tile
rectangle straight ahead for 12 ticks. They test the player's inset body and
wall occlusion; dodging sideways avoids the taser. Sentries lock an eight-way
aim direction and warn for 36 ticks before firing a straight projectile.

Recovery after throwing, zapping or firing is 240, 60 or 180 ticks respectively:
four seconds for Bombardier, one for Shocker and three for Sentry. No new attack
begins during recovery. These values use the lab's nominal 60-tick clock.

Body contact is separate from attack timing. Overlapping 12-by-12 bodies cause
a hit before or after enemy movement, including during wind-up and recovery.
Stunned enemies are harmless. The player's visible entry invulnerability and
the normal phase/active checks still apply. A hit ends play for that attempt,
so multiple overlapping enemies cannot consume several tries in the same tick.

The eight fixed projectile slots use one-eighth-pixel coordinates. Cardinal
movement is 3 pixels/tick; diagonal components are rounded to 17/8 pixels.
Shots retire on walls, a player hit or their 90-tick limit. A full pool skips
the optional shot; no active projectile is overwritten.

The player's stun pulse has a 32-pixel range, respects walls and lasts 90 ticks
on the enemy. The `zZ` marker stays visible, with a body flash during the final
60 ticks. A pulse recharges for 180 ticks (three seconds); its HUD bar fills blue
and turns green when ready. Pause freezes the cooldown. `!` and a short sound
mark acquisition; firing has a separate sound.
Sounds triggered together are coalesced into the lab's two SFX channels rather
than creating an unbounded event queue.

Each area has a marked entry pad, with later entries placed at the preceding
exit. Nearby walls shelter the entry using the same collision and sight rules
as the rest of the map. The marker grants no immunity or targeting exemption.

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

New runs open on help for Bombardier, Shocker and Sentry, with animated portraits
and short advice. Left/Right browses one card per press; A or Start enters area
one. There is no timeout. The simulation stays frozen while input, portrait
animation and VBlank continue. Retries and area changes do not repeat help.

Music uses `XGM2_pause()` / `XGM2_resume()` so it retains its place. PCM effects
may finish while the game is paused. Songs and samples remain resident in ROM;
this example does not establish safe bank-remapping behavior for audio.
Area changes load a distinct FM loop. Game Over and completion pause the music
while normal input and VBlank continue; Start begins another run without a timeout.

The result panels live on plane B, behind high-priority text on plane A but
above low-priority actors. Putting both text and its background on one plane
would replace background tiles with transparent font cells, making the panel
look cut out. Redraw the playfield when leaving a panel, including a same-area retry.

Sources: [sprite API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h),
[XGM2 API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/xgm2.h).
