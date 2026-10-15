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
| Shared sprite frames | `SPR_initEx()`, `SPR_loadAllFrames()` |
| Presentation | `VDP_setSpriteFull()`, `VDP_setSpriteLink()`, `VDP_updateSprites()` |
| Small palette effect | `PAL_setColors(..., DMA_QUEUE_COPY)` |
| Audio | `XGM2_load()`, `XGM2_playTrack()`, `XGM2_playPCM()`, pause/resume |

Sources: [joy.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/joy.h),
[sys.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h),
[vdp.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h),
[vdp_bg.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_bg.h),
[sprite_eng.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h),
[vdp_spr.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_spr.h),
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

A 576-tile sprite reservation is 18,432 VRAM bytes. The lab preloads every
frame and shares the resulting indices. Its native frame canvases need at most
516 tiles before ResComp cropping and duplicate sharing. A target probe sums
actual compiled frame tile counts and checks them against the reservation.
Background/title/HUD patterns must end below that reservation; the normal font
and VDP tables consume additional VRAM.

The temporary `SPR_loadAllFrames()` table is copied then freed during setup.
The lab uses SGDK's low-level sprite list because every frame fits one hardware
piece. `sprite_list.c` applies ResComp's frame offsets, culls outside the view,
links visible entries and queues their SAT transfer for VBlank. An empty list
clears the previous frame. This list owns the SAT exclusively; do not mix it
with `SPR_update()`. Larger, multi-piece characters need a different renderer
or SGDK's general Sprite engine. Startup rejects unsupported frame geometry.

## Frame and scanline limits

There are 81 fixed visual records, but at most 76 coexist onscreen: the six
ordinary enemies never coexist with the boss and its four-piece guard. Player
fire reserves twenty slots and hostile fire 28. Three pickups, six effects,
two Twins, the player shield, a payload and eight blast-front effects share the
remaining budget. The guard is four 24×24 quadrants, each one hardware piece.
Target checks cover both ordinary-wave and boss maxima. Total H40 capacity is
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

The red overhead bar and life icons use resident plane tiles, so they consume no
extra hardware sprites. Unchanged HUD strings are left alone; the compact visible SAT is rebuilt each frame. Ring/spiral emitters do not calculate an unused aimed vector.

The container regression runs the old and optimized projectile workload on
base-M68000 emulation. The report records counts
for the current pool configuration, including identical setup. This is an
instruction-count proxy, not a cycle count or an FPS guarantee. Actual frame
load still needs a release-ROM check with rendering, audio and effects active.

## Bounded presentation effects

The title replaces isolated painted specks with 28 moving one-pixel sprites,
reusing hostile-bullet objects while the game is inactive. Gameplay adds two
utility sprites (shield and payload) and eight blast-front effects, plus two Twins and a four-piece boss guard. Their state-dependent maximum is
76 hardware pieces. A configured cache holds all frames;
there is still no per-frame image upload or allocation. Scanline tests include
active shields and payloads, not just ordinary shots.

`weapons.c` owns the delayed bomb's arming/radius and one-time boss damage. Pause
freezes it with the rest of the model. Do not use a cosmetic ring's sprite
positions as the collision rule. The destruction presentation starts immediately
at boss defeat; its star/banner/fanfare wait for the two-second sequence.

The example's seven short robot announcements share PCM channel 3. Ordinary
kill/hit cues do not interrupt a voice already playing; boss/game-over messages
and new pickup announcements can preempt it. Firing stays on channel 2. Music keeps FM/PSG, so these choices
avoid taking a music PCM channel. Choose different priorities or no speech for
a different game. WAV sources are committed and portable; optional regeneration
uses the open-source eSpeak NG synthesizer with a light offline robot effect. The original bitmap font occupies the
normal 96 ASCII slots and replaces the default font without growing that region.

The boss resource explicitly disables sprite cutting (`NONE` optimization) so
its sparse dissolve frames stay one 32×32 hardware piece. This spends more
resident tiles to keep the sprite/scanline budget predictable. The target
resource probe checks that compiled count rather than assuming it from PNG size.

The HUD's colored counters use two compact copies of only their required
letters/digits (48 tiles total), rather than three extra complete fonts or
changing palette entries used by the sprites. The enraged boss uses a wider,
faster deterministic sweep and a repeated dive. The bar changes color at the
same health threshold. Motion changes latch at the next four-second phrase
boundary, and position changes are capped to prevent a threshold teleport. The final star uses two native frames plus a small bob;
it remains animated after the fanfare.

Stage 2 carries score, lives and remaining upgrades from the first clear. Its
boss guard is a model-side timed rule, not just a drawn aura. Tests check that
protected hits consume shots without reducing health, and that exposed windows
accept damage. Twin offsets use a precomputed 64-position table derived from the direction
vectors, remain
relative to the ship and stay opposite for two satellites. Their firing is
at half the main cadence and shares a twenty-shot pool. At Power 3, each Twin
fires two parallel lasers. The total projectile reserve remains 48 pieces;
28 slots belong to hostiles. A life loss resets power
and removes Twins; shield absorption preserves both. The second backdrop and
music are lab presentation choices, not required shooter conventions.

The second boss has its own music, and the final credits loop is separate from
the short clear fanfare. Movement remains available during the two-second boss
destruction, then stops at the next-screen prompt. Credits use the title's
resident art over the scrolling starfield, with no new palette or sprite pool.

For phase profiling, the optional `SHOOTER_PROFILE` release flag measures the
model, sound dispatch and drawing with SGDK's `getSubTick()`. Its counter is
coarse in VBlank, so the HUD percentages are diagnostic estimates. Profiling
is omitted from the normal release and is not part of the gameplay timing rule.
