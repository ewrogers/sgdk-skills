# Carpet of the Crescent

A horizontal, three-stage magic-carpet shooter built directly with SGDK.
Ride through a sunlit bazaar, moonlit gardens and a storm observatory, each with
its own guardian and music. This is a companion to Shooter Lab's vertical game,
not a required visual style or framework for your own shooter.

## Build and play

From this folder, with Docker running:

```sh
docker pull ewrogers/sgdk:latest
docker run --rm --mount "type=bind,source=$PWD,target=/workspace" ewrogers/sgdk:latest make release
```

Run `python3 tools/check_vram.py` after building to check the actual linked
resources for all three stages, the title and credits. The game also checks
these layouts before leaving startup. Resource errors stop simulation and
clear old sprites and scrolling before showing the error screen.

The build updates the cartridge header after SGDK pads the ROM, so its declared
size matches the final image.

Open `out/rom.bin` in a Mega Drive emulator. In ares:

```sh
ares --system "Mega Drive" out/rom.bin
```

Use the D-pad to fly, A to fire, B to buy the highlighted upgrade and C for
slower, precise movement. Start pauses. Menus accept A or Start; Left/Right
chooses a path. After losing all lives, retry starts the current stage with
its original equipment.

## Pick a path

- **Sunfire:** fast fireballs, forked fire, then a three-flame spread.
- **Moonweave:** fast white/cyan crescents with a fixed hooked return, twin
  crescents, then Ribbon: hold A for a wide, continuous piercing beam with a shimmering
  sound. Its 192-pixel reach deals two damage every six simulation ticks to
  every overlapping target; releasing A stops it. Owning Twin adds a pair of
  crescents alongside the beam. Crescents continue through targets
  but hit each target only once.
- **Stormglass:** seeking sparks, paired sparks, then Tempest. A Tempest impact
  chains to up to two nearby enemies for one damage each, within 64 pixels of
  each hop. Ties use stable enemy order; chains never create more chains.

The selection screen previews each path. Gems advance the upgrade track at the
bottom of the screen. Only the purchase selected by B has a highlighted
background; owned upgrades do not reuse that selection color. Capped upgrades
are crossed out. Every gem advances one slot, including disabled slots; the
background always marks the selected slot. Duplicate purchases are rejected
without spending gems. The five slots offer speed, twin fire, advanced magic,
a following familiar or a two-hit barrier. Collect more gems to
reach a later slot, or wrap around to the first. Twin and the third upgrade can
coexist; buying either keeps the other.
Familiars and speed have their own limits. Speed cannot be bought
again at its maximum; a denied purchase keeps your gems. A hit consumes a barrier
charge first; otherwise it costs a life and resets equipment. A full aura
surrounds the rider while protected, with two energy markers for two hits and
one marker after taking the first hit.

The first two defeated enemies drop gems to help you get started; later drops
arrive every fourth defeat, with one additional gem every eight seconds. Drops
follow defeat count rather than which object-pool slot happened to be reused.

The top strip keeps lives, score and stage visible. Guardians get a jewel-ended
health bar that fills during their entrance. Their attack windows begin after
the entrance, with each guardian repeating three learnable attack phrases.

## How it is put together

The portable game model is in `game.c`, `weapons.c`, `stage.c` and `combat.c`.
Rendering, UI, resident sprite-frame caches and audio live in separate modules.
All gameplay objects use fixed-size pools; the model advances at a nominal
60 steps per second, with PAL rendering taking one or two steps per frame.
This is simulation scheduling, not a guarantee that an overloaded frame recovers.

Four palette slots are planned explicitly: distant scenery, foreground scenery,
actors, and effects/UI. Art remains at its native resolution. Sprite animation
frames stay in VRAM; scene changes upload the next scenery and guardian while
the display is blanked. The HUD uses plane tiles, leaving hardware sprites for
the game. Stage banners pack up to four letters into each hardware sprite
so long names stay below the per-scanline sprite-entry limit. Their glyph tiles
are uploaded only when a banner changes. The bottom rail has an opaque
background so scenery cannot show through transparent icon corners.

The current release and debug images are each 393,216 bytes after padding.
The linked-resource check reports these resident tile budgets:

| Scene | Scenery and UI / 880 | Sprite frames and banners / 560 |
| --- | ---: | ---: |
| Bazaar | 766 | 551 |
| Gardens | 862 | 551 |
| Observatory | 786 | 551 |
| Title | 716 | 195 |
| Credits | 612 | 195 |

These are VRAM reservations, not counts of sprites visible on a scanline.
Re-run the checker after editing art; the Gardens are close to their allowance.

`tools/` contains the reproducible font, UI and original music/SFX generators,
plus palette conversion and animation packing helpers. `res/art-provenance.json`
records PixelLab sources and edits. Moon Gardens' sky uses palette-limited
checkerboard transitions; detailed architecture stays crisp.

Host regressions cover game flow, upgrades, fixed pools, movement, PAL/NTSC
scheduling, collision equivalence, a long simulation soak, HUD layout, health-bar
fill and asset formats. Emulator checks are separate from those tests. No real
console timing or compatibility claim follows from a successful build.
