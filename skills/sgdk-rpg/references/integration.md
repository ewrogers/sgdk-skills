# Connecting RPG rules to SGDK

## Input and readable presentation

Read `JOY_readJoypad(JOY_1)` once per display frame after normal VBlank service.
Use press edges for confirm/back and an explicit repeat clock for navigation.
Consume the press that opens a menu; it must not also commit its first action.
The example's [clock](../assets/adventure-lab/src/clock.c) produces nominal
60-unit seconds from 50/60 Hz refresh without polling input twice.

Turns need not run at 60 Hz. Presentation clocks do: cursor bobbing, message
holds and effects should use elapsed time deliberately. The Lab stops battle
clocks while paused. Each attack result waits for A after a short input guard,
so fast enemy turns cannot erase information before the player reads it.
A final winning hit instead opens the spoils automatically after its hold.
This is an example pacing choice; preserve the requested game's design.

Anchor a target marker to the selected actor's screen rectangle. A bouncing
marker above its head remains readable when several enemies share the same
species name. Keep formations apart and clamp markers away from the HUD.
Dead/absent slots are not selectable. Do not infer targeting from whichever
sprite happens to cover the pointer.

## Graphics ownership

The Lab uses plane B for illustrations and high-priority plane A for UI.
Battlers are ordinary SGDK VDP sprite submissions. It deliberately uses manual
SAT handling in one renderer; a project using SGDK's `SPR_*` engine should keep
that owner rather than mixing two systems over the same sprite table.

The four palette lines have explicit jobs: background lines 0/1, a shared
character/enemy line 2, and UI/effect line 3. Related field directions and battle
art are quantized together. Other projects may split party and enemies into
separate lines or change palettes at a transition. This allocation and the
Lab's art style are examples, not RPG requirements.

Plan total pattern tiles, name tables, fonts, sprite pieces and scanline pixels
separately. A 32×48 logical battler needs multiple hardware sprites. Six party
and enemy slots do not mean six hardware sprites. Read `FrameVDPSprite` geometry
from actual ResComp output and account for target markers and effects too.
Use `TILE_FONT_INDEX` after choosing the plane layout to bound user reservations.
A sparse source image can use fewer unique background tiles, but only the
compiled resource count establishes that saving.

The [renderer](../assets/adventure-lab/src/view.c) loads scene resources and
palettes while the display is hidden. Cursor and text updates reuse glyph/art
patterns. It keeps a 40×28 tilemap plus its last submitted copy (4,480 bytes)
and queues only different rows with `DMA_QUEUE_COPY`. The copied transfer owns
its source until VBlank; transient strings never become queued pointers.
Frame tiles and the SAT are queued once for the next VBlank, with changed-frame
caching. Plane/sprite priority keeps actors behind result and pause panels.

The main loop services the previous queue before changing screens. Do not
clear unrelated queued work, free shared resources or stack a second sprite
engine over this renderer when adapting it. Compression, streaming and loading
policies can be changed independently of battle rules.

## Sound and storage

Music and finite effects use XGM2. One adapter owns the active track and pause
state. UI/action events trigger PCM once; holding a button or repainting a menu
does not continuously retrigger a sound. Track choice, orchestration and mood
belong to the example. Verify PCM/music channel ownership against the project's
actual driver rather than assuming an effects channel is always free.

The [checkpoint codec](../assets/adventure-lab/src/checkpoint.c) stores a version,
semantic IDs, quantities, sequence and CRC in 40 bytes. The
[SRAM adapter](../assets/adventure-lab/src/save.c) alternates two slots, invalidates
the destination first and commits its signature byte last. It reads back before
announcing success and preserves the older valid slot across interrupted writes.
Sequence comparison handles 16-bit wrap with a less-than-half-range age rule.
The ROM header declares odd-address SRAM; SGDK's functions receive logical byte
offsets, not manually doubled physical bus addresses.

The Lab saves before departure and when resting at the inn. Continue resumes
that checkpoint rather than a half-presented attack. Avoid serializing sprite
handles, resource pointers or transient animation state. Larger/banked carts
need their mapper/SRAM interaction checked separately.

## Validation boundaries

Host C and base M68000 execution test the rules and compiled resources. A fake
SRAM device interrupts every byte-write boundary, checks sequence wrap and
falls back after corruption. These are useful contracts, not an emulated VDP,
Z80 mixer or physical SRAM power-loss test. Play complete encounters in an
emulator and test physical hardware when available, especially readability,
scanline pressure, input, regional timing and saved progress after power-off.

## Sources

Verified against SGDK 2.11 in `ewrogers/sgdk:latest` (GCC 16.2.0):

- [Joypad declarations](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/joy.h).
- [VBlank service](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h).
- [Tilemap transfers](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_tile.h)
  and [implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp_tile.c).
- [VDP sprite submissions](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_spr.h).
- [XGM2 API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/xgm2.h).
- [SRAM API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sram.h)
  and [implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sram_a.s).
- [Mega Drive sprite limits](https://plutiedev.com/sprites).
