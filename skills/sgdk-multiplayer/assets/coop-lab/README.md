# Co-op Lab: Bellkeepers

Join a small lantern patrol, collect eight drifting motes and restore the four
garden lamps. Each keeper has three hearts, a short-range moth stun and a dash.
A downed keeper returns after two seconds while the others keep playing. The
camera follows the group; an amber message asks you to regroup at its limits.

The game, native pixel art, music and finite sound cues are original examples.
Their theme and tuning illustrate the mechanics; they do not prescribe a style.

## Build and play

Docker must be running. From this directory:

```sh
docker pull ewrogers/sgdk:latest
python3 tools/build.py --output "/path/to/new coop lab"
```

Open `out/rom.bin` as Mega Drive in ares. Output must be new and outside this
source folder. `--platform linux/arm64` or `linux/amd64` selects the architecture;
`--mode debug` selects a debug build. `--region pal` / `ntsc` changes the header;
also select the corresponding actual emulator region when testing timing.

| Input | Action |
| --- | --- |
| Start on an unused pad | Join, including during play or pause |
| D-pad | Move your keeper |
| A | Stun nearby moths; 1.5-second cooldown |
| B | Short dash; one-second cooldown |
| Start on a joined pad | Pause/resume once, even if two players press together |
| Hold A+C, then press Start | Leave; the same press cannot immediately rejoin |
| Start after the lamps are restored | New round with the same player assignments |
| B at the empty title | Rescan adapters; routes are never redetected mid-session |
| C at the empty title | CPU tour: four simulated pads join over time |
| Start during the CPU tour | Return to the title |

## Pads and multitaps

Direct pads work through either port. Each detected Sega Team Player is enabled
explicitly; set its switch to **MULTI**. EA 4-Way Play uses both ports: SGDK reads
it through port 1 and port 2 remains off for selection. The Lab scans all eight
SGDK logical slots and assigns the next free **player** slot to a fresh Start
press. At most four players join; the fifth request waits for a new press after
a place opens. Existing players are never silently reassigned.

The CPU tour checks four-character presentation and game logic. It is **not** a
multitap protocol test. Emulator adapter support varies, and real adapters,
controller models and disconnect behavior still need physical-hardware checks.
Cached SGDK pad types cannot reliably prove whether a physical pad is connected.
Use explicit Leave instead of inferring departure from quiet input.

## Camera and respawns

Every player proposes a move before the shared camera constraint is applied.
Outward moves stop at the group span limit; inward movement can regroup. Solid
garden beds still block motion. Players do not block one another. Downed players
are excluded from camera framing and respawn near the surviving group at a
walkable location, preferring space away from moths. They return with temporary
invulnerability. If everyone falls, the camera holds and the first return uses
the entry checkpoint. Pause freezes gameplay timers, including respawns.

Nearest players collect motes; equal-distance ties rotate. A mote is awarded
once. Team progress survives an individual Leave. Starting a fresh session after
everyone leaves resets the round. These are explicit Lab policies, not SGDK
engine behavior or required rules for another game.

The renderer reserves 128 sprite tiles. The maximum is 20 logical sprites,
each one hardware piece: four keepers, four moths, eight motes and four lamps.
Even a conservative same-line arrangement uses 20 pieces / 256 sprite pixels.
Four palettes are shared by roles, not allocated one per player. HUD transfers
use retained buffers, drained before rebuilding; sprite/scroll work is separate.
Automated target probes validate model contracts and compiled assets, not VDP
timing, sound quality or adapter electronics. Check both PAL and NTSC in an
emulator, then use real pads/adapters before claiming hardware compatibility.
