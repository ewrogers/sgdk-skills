# Puzzle Labs — Clockwork Garden and Falling Lanterns

Three small rooms about moving lanterns, opening gates and building a bridge.
The garden, music and puzzles are original examples, not a required style or ruleset.

Build a new standalone copy (Docker must be running):

```sh
python tools/build.py --output /path/to/new-puzzle-lab
```

Open its `out/rom.bin` in a Mega Drive emulator. D-pad moves, A undoes, B restarts
the room, and Start pauses. In ares's usual mapping: arrows, J, K and Return.
At room clear, A or Start continues. Hold a direction to repeat moves.

- Room 1: cover both diamond seals, then walk onto the lit exit.
- Room 2: leave a lantern on the square plate to open the gate. A gate occupied
  by a person or lantern stays open until that cell is empty.
- Room 3: push a spare lantern into the dark gap to make a bridge; cover the seal.

Undo includes pushes, filled gaps and the move counter. The most recent 64 legal
moves are retained; blocked moves do not use history. Restart restores this room.
There is no deadlock detector; a cornered lantern is a reason to undo or restart.
No save data is written. The room-clear message waits for a fresh button press.

`inc/puzzle.h` / `src/puzzle.c` are the portable rules. `src/main.c` owns SGDK
input, presentation, interpolation, audio and the PAL/NTSC clock adapter.
The source adapter is `variants/garden_main.c`; export selects it as `src/main.c`.
`tools/` generates crisp indexed art and original FM/PCM audio without external
packages. Build options: `--platform linux/arm64|linux/amd64`, `--mode debug`,
`--region pal|ntsc`. Output must be new and outside this source directory.

The Lab reserves five hardware sprites and 48 sprite tiles. The logical board
and undo history are fixed storage, with no heap allocation during play. Board
tile uploads occur only on a change, not every frame. See the skill's references
for API sources, exact budgets and test limitations.


## Falling Lanterns — Block Lab

Build the separate falling-block ROM with the same standalone tools:

```sh
python tools/build.py --lab blocks --output /path/to/new-block-lab
```

Left/Right moves, Down soft-drops, A rotates clockwise, B hard-drops, and Start
pauses. A or Start begins/retries after game over. In ares: arrows, J, K, Return.
Fill a horizontal row to clear it. The outline is the landing preview, and the
right panel shows the next piece, score and cleared rows. Music and finite cues
continue through game over so the waiting screen does not seem frozen.

`inc/blocks.h` / `src/blocks.c` own the separate fixed 10×18 board, active piece,
shuffle bag, bounded kick search, gravity, lock and simultaneous row compaction.
`variants/blocks_main.c` replaces `src/main.c` in an exported Block Lab build.
All source variants/tools are included in each copy, so either Lab can be built
again from an installed skill or exported project without the repository tests.

The block field uses plane tiles, no hardware sprites. Its retained field and
Window buffers use 2600 bytes and upload at most that amount per changed frame.
The custom rotation/score/timing rules are examples, not a compatibility claim
with any commercial falling-block game. No undo, hold piece or save is included.
