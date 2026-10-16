# Test Lab — Seeded Courier

A small SGDK scene for proving replay and regression checks. A courier moves
around a wall, collects gold motes and fires at them. The reference route
includes a pause. It starts automatically and finishes after 720 logic ticks.
The art and movement are illustrative; the testing techniques work with other
genres and styles.

## Build and play

Run from this directory with Python 3.10+ and Docker running:

```sh
docker pull ewrogers/sgdk:latest
python3 tools/build.py --output "build/ntsc" --region ntsc
python3 tools/build.py --output "build/pal" --region pal
```

Use `python` in place of `python3` if needed. The output directory must be new.
Add `--platform linux/arm64` or `--platform linux/amd64` to select the container
architecture, and `--mode debug` for symbols. Each output is a complete,
independent SGDK project; subsequent builds can use `docker run` with that
directory mounted at `/workspace`, then `make release` or `make debug`.

Open `build/ntsc/out/rom.bin` in ares. Its CLI can select the system explicitly:

```sh
ares --system "Mega Drive" "build/ntsc/out/rom.bin"
```

Use the actual executable path on your OS. The PAL build has an E header hint;
confirm the ROM displays **PAL 50HZ** after loading. Forced emulator region
settings can override that hint. No BIOS or SRAM is required.

| Control | Reference / replay | Recording |
| --- | --- | --- |
| A | Restart the reference route | Fire once per press |
| B | Begin a new recording | Finish and keep the take |
| C | Replay the latest take, or reference if none | Unused |
| D-pad | Up toggles the deliberate movement bug | Move |
| Start | Pause/resume the playback viewer | Pause/resume game logic; recorded |

Recordings live in RAM until a new recording or reset. They stop at 720 ticks
or 128 input runs; the last complete recorded tick is retained. Playback uses
the same seed and game functions. It compares your take's final digest with
the recorded result. Reference playback also checks every 60 ticks and reports
the **first differing checkpoint**. Viewer pause is outside the tape and hash.

The correct reference ends with score **2**, **7** shots fired, **660** active
ticks and digest **74891936** in hex. It takes 720 nominal NTSC frames or 600
nominal PAL frames. Turn on the bug with Up: movement changes from two to three
pixels/tick, and the report should say **DIVERGED AT TICK 60**. Up again fixes it.

## Automated QA without an emulator window

A C17 host compiler (`clang`, `cc` or `gcc`) runs the real portable model. The
helper passes paths as arguments, works in paths with spaces and refuses to
overwrite existing outputs. These commands are suitable as CI job steps in a
fresh workspace:

```sh
python3 tools/qa.py run --hz 60 --output actual-ntsc.json
python3 tools/qa.py run --hz 50 --output actual-pal.json
python3 tools/qa.py compare replays/courier.expected.json actual-ntsc.json
python3 tools/qa.py compare replays/courier.expected.json actual-pal.json
```

Use `--cc /path/to/compiler` if discovery does not find your compiler. The
comparator exits 0 on equality, 1 on divergence, or 2 for malformed reports,
missing tools or failed execution. Prove that failure works too:

```sh
python3 tools/qa.py run --fault --output actual-bug.json
python3 tools/qa.py compare replays/courier.expected.json actual-bug.json
```

The last command should exit **1**, reporting the first changed field at tick
60. Retain both reports and the tape as failure artifacts. Do not regenerate
the expected file to make the check green. A deliberate rule change needs a
reviewed baseline update and independent checks of the new behavior.

`replays/courier.json` uses explicit held masks and run lengths. The compiler
rejects unknown fields, unsupported versions, bad ranges and excess capacity:

```sh
python3 tools/qa.py compile replays/courier.json --output replay_data.h
```

The JSON format is a Lab example. Adapt the input/state seam for multiple
players, device changes, mouse deltas, saves or level transitions. Record the
game/asset revision alongside reports; the tape hash identifies the input,
not the build. The live RAM recorder has no host-file export in this slice.

## Optional target CPU check

For target arithmetic and optimization regressions, `tools/target_probe.c`
executes the same tape under both nominal schedules and emits state words.
Use a **separate prepared project**, replace its `src/main.c` with this probe,
then build. For example, on a POSIX host:

```sh
python3 tools/prepare.py "build/probe"
cp "build/probe/tools/target_probe.c" "build/probe/src/main.c"
docker run --rm --mount "type=bind,source=$PWD/build/probe,target=/workspace" ewrogers/sgdk:latest make release
docker run --rm --mount "type=bind,source=$PWD/build/probe,target=/workspace" ewrogers/sgdk:latest m68k-elf-nm out/rom.out > probe-symbols.txt
```

On PowerShell use `Copy-Item` instead of `cp` and an absolute mount source path;
save symbol output as UTF-8. Install optional `unicorn==2.1.4` in a Python
virtual environment, then run:

```sh
python3 tools/m68k.py build/probe/out/rom.bin --symbols probe-symbols.txt --output target-traces
python3 tools/qa.py compare replays/courier.expected.json target-traces/50.json
python3 tools/qa.py compare replays/courier.expected.json target-traces/60.json
```

This explicitly uses a base M68000 with an instruction limit. It bypasses
startup and does not emulate VDP, DMA, audio, joypad protocols or interrupts.
Do not load the probe ROM in ares: its output writes are a test protocol, not
valid screen rendering. Use the ordinary Test Lab ROM for presentation checks.

## Budgets and validation boundaries

The record buffer reserves **516 bytes** for 128 four-byte runs and counters.
Reference input and checkpoint hashes stay in ROM. State, recorder and sprite
handles are separate from SGDK's own work buffers. There is no per-tick heap
allocation. Startup caches sprite tiles and frees the temporary index tables.

The scene uses four palette lines, two background tiles and 14 cached sprite
tiles. It has at most eight hardware sprites, with a conservative same-line
bound of eight sprites / 72 sprite pixels. These bounds do not measure CPU
load or account for a different game's assets. Text stays on BG_A; the board
is BG_B. All palette setup happens with display disabled, and changing HUD
rows use SGDK's copied DMA queue storage.

Host tests cover pause/edges, wall bounds, pool exhaustion/reuse, recording
capacity and 100,000 repeatable ticks. Report tests reject missing, reordered
and foreign checkpoints. Docker checks compile both modes and execute the
probe on M68000. Native ARM64/AMD64 CI compares ROM hashes and trace results.
Neither equal traces nor matching ROMs prove real frame timing or presentation.

## Playtest checklist

1. Load the normal NTSC ROM. Check the courier, motes, wall and all text.
2. Let reference playback finish. It should show PASS and the expected hash.
3. Toggle the bug with Up, see divergence, then toggle it off and see PASS.
4. Press B, move/fire/pause, then B to finish. C should repeat your route and
   show PASS. Repeated C playback should match again. Starting a new recording
   intentionally replaces the old take.
5. Load the PAL ROM, confirm its detected mode, and repeat. The same reference
   should finish with the same state/hash over 600 nominal frames.
6. Check actual responsiveness and missed frames separately. The clock assumes
   completed nominal display frames; it does not recover lost real time.

There is intentionally no music or SFX in this focused Lab. Audio regressions
need their own listening/capture checks and are not inferred from a logic hash.
