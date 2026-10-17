# Combat Lab: Copper Court

A small practice arena for action-combat rules. Strike with your sword, guard
against attacks you face, and fire a bolt. Raiders approach with a readable
windup; Sentinels guard their front and use an unblockable red heavy swing;
Arcanists keep their distance and fire bolts. Clear three rounds.

All art and music are original illustrative fixtures. The theme, camera,
movement, attack timing and difficulty are examples, not rules for other games.

## Build

With Docker running, from this directory:

```sh
docker pull ewrogers/sgdk:latest
python3 tools/build.py --output "/path/to/new combat lab"
```

Open `out/rom.bin` as Mega Drive in ares. Outputs must be new and outside this
source folder. `--platform linux/arm64` / `linux/amd64` selects the container;
`--mode debug` builds debug. `--region pal` / `ntsc` changes the header. Test the
actual emulator region too; a header change alone does not prove regional timing.

## Controls

| Input | Action |
| --- | --- |
| Start | Enter, pause/resume, or advance the round banner |
| D-pad | Move and face a direction |
| A | Sword strike; a short input buffer accepts a press near recovery |
| Hold B | Raise your shield; only attacks coming from in front are blocked |
| C | Cast a bolt; its white bar recharges before another cast |
| Second direct pad Start | Join during play, including while paused |

The top health bars match the players' colors. Guarding reduces movement speed.
There is no body-contact damage: enemies must land an attack or bolt. Red heavy
swings bypass guard, so move away during their longer windup. Raiders and Sentinels can attempt a bolt guard, with a six-tick
reaction delay, a brief hold and a cooldown. Attempts can fail; they do not read
and block every shot instantly. The shield presentation follows facing. A successful
block has a bright spark and clang. Hits cause a brief pause, knockback and
protected recovery. The sword sweeps through four native poses, each held for two logical ticks.
Its damage box follows that pose and direction. A short forearm contact region
bridges the extended blade to the body, so close enemies are not missed. Repeated overlap cannot damage
the same enemy twice in one swing.

Friendly fire is off. The model supports it as an explicit developer policy;
there is no extra menu to configure it. If one player falls while an ally is
alive, they return after two seconds. If nobody remains standing, Start retries
the round. Both players share a fixed arena; this Lab does not reimplement
multitap routing or a scrolling shared camera. Use the input/multiplayer skills
for those features in a larger game.

## Implementation and budgets

The portable `combat.c` owns damage policy, hit records, guard checks, reactions
and a bounded contact batch. Candidates use one pre-damage snapshot so attacks
can trade. Damage is added per target, with strongest-hit knockback and a stable
source-slot tie-break. Damage already dealt is not rolled back when an attacker
is interrupted. Reusing a target slot changes its generation. Bolts capture
their owner's generation; owner-slot reuse discards stale bolts, while a bolt
may finish after its owner is knocked out but before that slot is reused.

The example uses a 60-tick simulation on 50/60 Hz displays. Pause stops gameplay;
hit-stop also freezes cooldowns, movement and effect lifetimes. A can be buffered
during hit-stop; C is an immediate action request and is not buffered. Attacks
follow authored startup/active/recovery timings, using manual SGDK frames. The
sprite engine does not implement attacks, guard or damage.

The bounds are six actors, four single-hit bolts, six cosmetic effects and
40 contact records. The mathematical maximum contact batch is 34: six
attackers can each contact five targets, plus four bolts. The overload API
rejects an oversized batch before mutating combat. A full bolt pool drops the
new shot, without allocating or damaging a different target.

All actors use one hardware piece (24x32), swords one (24x24), bolts one (8x8),
and effects one (16x16). Six actors, six swords and four bolts fit 16 pieces /
320 pixels even if all share a scanline.
Cosmetic effects are admitted afterward only when that line remains within
20 pieces / 320 pixels. Four palettes are shared by art role. The renderer
reserves 160 sprite tiles; the live maximum is 154. HUD meters are tiles, not
sprites. CPU, DMA, VRAM and scanline budgets remain separate measurements.

## Playtest checklist

- Check strikes only damage during the committed swing, once per target.
- Face a Raider while guarding, then turn away. Try blocking a red heavy swing.
- Try bolts, their recharge, hit sparks, knockback, protected recovery and walls.
- Pause during an attack and join with the second pad. Check that joining does
  not unpause, while either joined Start resumes.
- Down one player while the other survives; then try all-down and round retry.
- Clear all three rounds, hear the cue and return to the title.
- Check PAL and NTSC presentation and dense two-player combat on real hardware
  when available. Automated CPU probes do not emulate VDP/audio or controllers.
