# Timing, input and checkpoints

## A screen is a game-code decision

A simple enum and a switch are enough for a small game. Separate persistent
session data from the resources of the visible screen. A pause menu can be an
overlay; it does not need to destroy and reload the level.

Transition Lab has one pending transition. It rejects a second request until
the current fade and load finish. Failed entry releases partial resources
before attempting Title; a failed Title load shows an error instead of looping.

## Keep input alive during fades and pause

After normal VBlank work, `JOY_readJoypad()` reads SGDK's cached snapshot.
Read it once per player and derive pressed edges from the previous snapshot.
Keep that history current while gameplay is frozen. Clearing it on every
screen change makes a held Start or A look like a fresh press.

Keep hardware polling separate from simulation. Transition Lab advances a
nominal 60-tick clock at both 50 and 60 Hz display rates. Some PAL frames run
two small model steps; rendering remains 50 Hz. This is the example's timing
policy, not a promise of identical motion smoothness or measured performance.

The first character-select join starts ten seconds. The second join does not
restart it. A joined player's A press removes one displayed second and snaps
the remaining time to a whole second. Holding A does not repeat.

Sources: [joypad implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/joy.c),
[normal system processing](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c).

## Freeze the systems you mean to freeze

Pause stops the lab's model ticks, projectile motion, coin hops, countdowns and
manual sprite animation. VBlank, input and music still run.

SGDK sprite auto-animation can continue during `SPR_update()`. For a game
that needs it frozen, disable auto-animation with
`SPR_setAutoAnimation(sprite, FALSE)` and drive frames from the chosen game
clock, or explicitly pause the relevant animation system.

`XGM2_pause()` / `XGM2_resume()` preserve the current music stream's position.
Loading a different pause song replaces that stream. The lab plays jazzy pause
music and restarts the room song on resume; it does not claim to restore the
old song position.

Sources: [sprite API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h),
[XGM2 API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/xgm2.h).

## Save at level entry

The lab writes its checkpoint only after successfully entering a fresh level,
including the first room and the boss room. Returning to Title, clearing a
room or finishing the game does not write a new checkpoint. Continue starts
that level again with its initial coin layout and the score brought into it.

A failed checkpoint write keeps the previous valid slot. The game can continue,
but the displayed warning means the newest level entry was not saved.

The byte codec is versioned and uses two 48-byte slots with CRC16, sequence
numbers and commit bytes. It validates ranges and coin ownership before
applying a record. It writes the older slot and checks readback; interrupted
writes leave the other valid slot available. Sequence wrap and conflicting
equal-sequence records are rejected.

This is a checkpoint, not an emulator savestate. Runtime sprite handles, DMA
queues, audio position, boss projectiles and transient animation are not saved.
The format can encode a room state for codec tests, but normal gameplay saves
only level starts. Continue also resets a valid older development record to the room's start,
removing that room's collected coins from its score.

SRAM access uses `SRAM_enableRO()` or `SRAM_enable()`, then
`SRAM_disable()` on every path. The lab is below the SRAM overlap window and
uses normal resident ROM resources. Test battery retention and interruption
behavior on the actual target separately.

Source: [SRAM API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sram.h).
