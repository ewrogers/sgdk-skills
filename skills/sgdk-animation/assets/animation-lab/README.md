# Animation Lab

A clockwork trainee practices against a stationary dummy. See startup, active
and recovery phases, interrupt an attack, pause a held pose, and compare visible
boxes with the actual hits. The character, art, sounds and combat rules are
illustrative; this is not a prescribed style or a new SGDK engine framework.

## Build and run

Requires Python 3.10+, Docker with Linux containers, and a Mega Drive emulator
or console. From this Lab directory, choose a new output folder:

```sh
docker pull ewrogers/sgdk:latest
python tools/build.py --output "build/animation review"
```

The helper copies a standalone project, generates the native indexed PNGs and
builds `out/rom.bin` plus matching `out/rom.out`. It refuses to overwrite output.
Select `--platform linux/arm64` or `linux/amd64`, or add `--mode debug`.
`--region pal` / `--region ntsc` changes the header hint; actually select that
region in the emulator. The runtime detects its 50/60 Hz display rate.

On macOS:

```sh
/Applications/ares.app/Contents/MacOS/ares --system "Mega Drive" "/absolute/path/out/rom.bin"
```

Other hosts can use their emulator's ROM picker. Host-helper portability is
separate from emulator or physical-console testing on every OS.

## Controls and checks

| Control | Action |
| --- | --- |
| Left / Right | Walk and face that direction; body stops at the dummy |
| A | Request an attack; revive after death finishes |
| B | Take one hit, interrupting the current action |
| Down + B | Knock out immediately |
| C | Toggle diagnostic boxes |
| Up | Toggle quarter-speed logical motion |
| Start | Pause/resume logic |

With the usual ares mapping, A/B/C are J/K/L and Start is Enter.

Move close to the dummy and tap A. Its gold slash should appear only during
the active window and each attack should add at most one hit. Holding A does
not repeat. Face away and attacks should miss. Start an attack, then press B
during wind-up: no old strike or hitbox should appear afterward. Repeated B
hits eventually knock out; Down+B makes that check quicker. A cannot revive
until the death sequence finishes.

Up makes the phases easier to inspect. The controller accepts recovery cancels
from logical tick 18 of its 26-tick attack; a four-tick input buffer lets a press
just before that window survive. Earlier presses expire. This is one example
tuning, not a universal combo rule. Cyan outlines show body boxes, pink shows
the active attack box, and the gold arc is cosmetic. C must not change hits.

`SWINGS` counts entry cues, `HITS` successful dummy contacts, `STEPS` footstep
markers and `ENDS` completed non-looping actions, including hurt/death.
Counters saturate at 9,999. The HUD is sampled every six display frames and on
button presses; the actual pose and boxes update every display frame. Use slow
mode to inspect short phases rather than treating the sampled text as a trace.

Short PSG tones accompany steps, swings, impacts, hurt and knockout. There is
no background music or loaded music driver. Pause mutes the current cue without
replaying it on resume. Slow mode slows the logical tone envelope too. Audio
chooses one cue by priority per displayed frame; gameplay event accounting still
consumes every logical step's events.

## Implementation and budgets

`action.c` owns the original action clock and collision example. It samples the
current frame before advancing time, emits entry markers once, and completes a
one-shot only after its final hold. A new attack explicitly restarts the clock
and hit latch. The example's single-target latch is not a general multi-target
hit set. `main.c` adapts input and presents the resulting state through SGDK.

One second contains 60 logical opportunities over either 60 NTSC or 50 PAL
frames. A PAL frame may run two steps; each has its own event mask. Manual sprite
playback submits only the resulting final pose with one `SPR_update` per display
frame. Automatic sprite animation is disabled. Pause freezes the logical clock;
input edges pressed while paused are discarded. Slow mode retains short action
presses until its next logical step. This does not catch up after missed VBlanks.

| Resource | Lab use |
| --- | --- |
| Action state | `AlClock`: 8; `AlActor`: 28; `AlGame`: 36 target bytes |
| Event batch | 6 target bytes; at most two logical event masks/display frame |
| Retained HUD | 2,240 bytes of static RAM |
| Sprite instances | Six; at most one hardware piece each, including debug outlines |
| Sprite VRAM | 72 tiles reserved; definitions need at most 51 frame tiles together |
| Native frame data | 215 uncompressed tiles / 6,880 bytes, plus metadata and palettes |
| Floor | One resident tile at index 96 |
| Palettes | PAL0 UI/floor, PAL1 trainee, PAL2 dummy, PAL3 arc/outlines |
| DMA allowance | 4,096 bytes shared by HUD, sprite frames and sprite table |

The six frame allocations total at most 1,632 upload bytes; a full HUD update
adds 2,240. The selected sprite engine queues its used sprite-table entries
separately. These are example reservations, not console limits or a complete
SDK RAM total. The normal padded ROM is 128 KiB. Resource generation uses only
Python's standard library and original pixel data.

## Validation

Host checks execute the actual action model at `-O0` and `-O3`, with Unix
undefined-behavior checks. They verify exact holds and windows, completion once,
restart/cancel and buffer expiry, interrupted strikes, mirrored edge contacts,
pause/slow input handling, PAL/NTSC progress and a 100,000-frame bounded soak.

ARM64/AMD64 container tests build release/debug and execute the same rules as
base M68000 instructions. A 2,000-frame host/target trace must match. Probes check
every linked pose's dimensions, palette and pixel bytes against the generated
native PNGs. Both architectures compare ROM hashes and budget reports.

Those tests do not emulate VDP/audio timing, prove frame-time headroom or certify
hardware behavior. The user confirmed the controls, poses, sounds and timing in
NTSC ares. PAL presentation and physical-console checks remain separate.
