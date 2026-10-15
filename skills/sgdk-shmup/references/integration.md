# SGDK integration and hardware budgets

The references were checked against SGDK 2.11 in `ewrogers/sgdk:latest`.
The lab's game rules and pattern functions are ordinary C, not SGDK APIs.

## APIs used

| Job | Actual SGDK interface |
| --- | --- |
| Input snapshot | `JOY_readJoypad(JOY_1)` with `BUTTON_*` masks |
| Region/frame boundary | `SYS_isPAL()`, `SYS_doVBlankProcess()` |
| Background | `VDP_drawImageEx()`, `VDP_setVerticalScroll()` |
| Side HUD | `VDP_setWindowHPos(TRUE, 15)`, `VDP_setWindowVPos(FALSE, 0)` |
| Text | `VDP_drawTextEx(..., DMA_QUEUE)` |
| Shared sprite frames | `SPR_initEx()`, `SPR_loadAllFrames()`, `SPR_addSpriteEx()` |
| Presentation | `SPR_setPosition()`, `SPR_setFrame()`, `SPR_setVRAMTileIndex()`, `SPR_setVisibility()` |
| Small palette effect | `PAL_setColors(..., DMA_QUEUE_COPY)` |
| Audio | `XGM2_load()`, `XGM2_playTrack()`, `XGM2_playPCM()`, pause/resume |

Sources: [joy.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/joy.h),
[sys.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h),
[vdp.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h),
[vdp_bg.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_bg.h),
[sprite_eng.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h),
[pal.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/pal.h),
[snd/xgm2.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/xgm2.h), and
[ResComp manual](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt).

## HUD and planes

Horizontal Window position uses 16-pixel units, so position 15 starts the right
panel at x=240. Vertical position uses 8-pixel units. Do not confuse these units
with each other or with a plane's tile coordinates. The opaque high-priority
panel masks world sprites at its edge. Plane B scrolls a repeating starfield;
Plane A is a stationary message layer. This lab does not stream a terrain map.

H40 requires a 4 KiB-aligned Window table. Use the lab's 64×32 plane layout,
which places Window at 0xD000. SGDK 2.11's automatic 32×32 layout instead requests
0xC800, which its H40 setter rounds down to 0xC000, overlapping Plane B.
The lab validates the actual table addresses and rejects overlaps during setup. This is a VRAM layout issue, not a reason to change compiler optimization.
Validate the selected layout and table ranges when changing resolution/planes.

A horizontal game can instead use a narrow top/bottom Window strip. Set its
playfield bounds first and revisit spawn points, movement clamps, boss placement
and retirement margins. Moving the HUD alone is not enough.

## Native art and VRAM

All example sheets are indexed native-resolution pixel art. Ships and enemies
are 16×16, the boss is 32×32 and bullets are 8×8. Each fits one hardware sprite;
a larger design may be split into several pieces by ResComp. Hardware tiles
remain 8×8 regardless of the logical asset dimensions.

The lab uses PAL0 for stars/panel, PAL1 for the ship, PAL2 for enemies/boss and
PAL3 for bullets/pickups/effects/font. Every sheet stays within sixteen entries,
with index zero transparent for sprites. These colors and the space theme are
specific to Shooter Lab; the skill does not prescribe an aesthetic.

A 160-tile sprite reservation is 5,120 VRAM bytes. The lab preloads all frames
and shares their indices: 16 ship tiles, 24 enemy tiles, 48 boss tiles, three
bullet tiles, eight pickup tiles and sixteen effect tiles before any SDK
optimization. Background patterns must end below the sprite reservation;
fonts and VDP tables consume additional VRAM.

`SPR_addSpriteEx(..., flags=0)` leaves tile ownership with the manual cache.
Set frame geometry before selecting its resident tile index. Do not add
an automatic per-instance tile allocator into that same reserved region.
The temporary `SPR_loadAllFrames()` table is copied then freed during setup.

## Frame and scanline limits

The fixed records require at most 61 hardware sprites: player, boss, six
enemies, twelve player shots, 32 hostile shots, three pickups and six effects.
Some states do not show all those categories at once. Total H40 capacity is
80 sprites, but each scanline has its own limit of 20 sprites and 320 sprite
pixels. An 80-object budget is not a promise that a crowded row can draw them.

Stagger emitters and choose readable densities. The lab's authored attack
sequences have scanline-pressure tests, but those are bounded scenario checks,
not a general proof for arbitrary attacks or replacement art. Test the combined
peak with player fire, drops, explosions and a large boss. Sprite priority can
protect important art ordering; it cannot manufacture extra hardware capacity.

If a game's attacks can exceed those limits, reduce density or deliberately
control which hazards are admitted before they become damaging. Cosmetic
particles can be dropped first. Do not allow invisible damaging bullets simply
because a sprite slot or scanline was exhausted. See [Plutiedev](https://plutiedev.com/)
for the hardware's sprite evaluation behavior.

## DMA, audio and CPU

Repeated bullet art is resident, so movement updates the SAT rather than
uploading every bullet's pixels. HUD lines update periodically or on state
changes. The bomb pulse queues a copied palette word for VBlank, keeping the
stack source alive and avoiding active-display CRAM writes. Loading writes
happen while blanked. Queue sizes and CPU load still need measurement.

The example's original VGM is converted to XGM2, with short PCM effects on two
channels. Effect priorities are a game choice: warnings/damage/bombs take
precedence over lesser events. Fire sounds follow the simulation-driven weapon cadence. Pause stops
effect channels and pauses music; a clear fanfare plays once. Do not do driver
or bus work in an H interrupt, and recheck PCM contention when adding heavy DMA.

Build optimized ROMs for timing checks. `SYS_getFPS()` is called once per display
frame; `SYS_getCPULoad()` is an estimate, not a cycle-accurate profiler or a DMA
budget. Fixed pools bound memory and work but do not make arbitrary density
cheap. Profile the busy case before reaching for custom assembly.

## Boss presentation and measured work

Shooter Lab keeps the warning, fly-in and health-bar reveal on its nominal
60 Hz game clock. The player can move during the sequence; boss damage,
contact and firing stay disabled until the reveal finishes. The audio adapter
uses SGDK's `XGM2_setFMVolume()` and `XGM2_setPSGVolume()` (0–100) to hand off
between stage and boss tracks once. Pause stops that handoff too.

The overhead bar and life icons use resident plane tiles, so they consume no
extra hardware sprites. Unchanged HUD strings, sprite frames and tile indices
are left alone. Ring/spiral emitters do not calculate an unused aimed vector.

The container regression runs the old and optimized projectile workload on
base-M68000 emulation. A measured full-pool, mostly-no-hit case used 6,807
instructions before and 4,492 afterward, including identical setup. This is an
instruction-count proxy, not a cycle count or an FPS guarantee. Actual frame
load still needs a release-ROM check with rendering, audio and effects active.
