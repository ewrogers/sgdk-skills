# Console checklist

Keep one record per ROM hash and environment. Copy this into a test report;
do not replace unknowns with a successful build result.

| Field | Record |
| --- | --- |
| ROM | Filename, SHA-256, build mode, build commit |
| Toolchain | Image used, SGDK/compiler/ResComp versions |
| Environment | Emulator + version **or** console model/revision |
| Video | Actual PAL/NTSC setting, cable/output, display/capture path |
| Cartridge | Model, firmware, mapper/save configuration; or not applicable |
| Input | Controller models, adapters, ports |
| Observation | Observed / failed / not tested, exact steps and elapsed time |

## Emulator checks

1. Start fresh, open Boot, record entry values and video flag. Use software reset
   and forced init separately. A restored emulator state is not a cold boot.
   On the readiness page, record the build label and ROM hash; confirm HARD START,
   DATA `1234`, BSS `0000`, and changing meter/counter values across fresh frames.
   Retained globals on a warm entry should show REVIEW rather than a false PASS.
2. Run in actual PAL and NTSC modes, not just two ROM filenames. Compare roughly
   ten seconds of VInt/logic counts. At steady service, 500 PAL / 600 NTSC loops
   each advance 600 nominal logic units. No independent frequency claim follows.
3. Inspect the pattern edges, stripes and ramps. Toggle the sprite test: at the
   limit both rows fit; above it trailing content should drop. Record any
   emulator sprite-limit override; use normal limits for the comparison.
4. On DMA/Audio, play music with A. Compare B's 0/1024/2048-byte modes without
   enabling the tone. Then press Start for the continuous PCM reference; press
   Start again to stop it.
   Listen separately and record audible changes, loop gaps and queue rejections.
5. Try both pads. On Controllers, A locks the page so Left/Right can be tested
   without navigating; A unlocks it again. Check held-bit release as well as
   presses. Use a matching
   peripheral Lab for mouse/multitap setup rather than assuming pad tests cover it.
6. On SRAM, A reads. **Holding B and pressing A writes this ROM's record.**
   Read again, reset, read again. Restart the emulator without a save state and
   check its save-file behavior. This does not test a battery.

## Still needs a console / flashcart

All items below remain **not tested** until someone actually runs them:

- Cold boot, physical reset, repeated boot and the intended cartridge's startup.
  Confirm startup with the intended peripheral combinations and expansions.
- Actual PAL and NTSC machines, edge cropping and readability on the intended
  display path. A PAL emulator is not a PAL console result.
- Sprite dropout and DMA/audio behavior on the intended hardware revisions.
  Record the exact workload; avoid a blanket “works on hardware” claim.
- Real 3/6-button pads, both ports and any supported mouse/tap/adapter models.
- SRAM write/read/reset and persistence after a documented safe power cycle.
  Some flashcarts flush SRAM only through their menu/reset procedure. Follow
  that procedure; do not cut power during a write or firmware save operation.
- Long sessions, scene transitions and the actual game's peak workloads. A small
  diagnostic ROM cannot certify a different game's RAM, stack or mapper behavior.

A failure report should include the smallest reproduction, its exact hash and
what differs from the emulator. Exclude BIOS dumps, ROMs from commercial games,
credentials and unrelated save files from shared reports.
