# Spare Time Lab

Move the cyan drone with the D-pad, tag gold lights and avoid the red drone while
the SDK user task computes in the background. The presentation and simple tagging
game are illustrative; they are not a required game design or architecture.

- **A:** decompress and verify an 8 KiB atlas eight times in the user task. The
  repeated passes intentionally make work visible; an actual loader normally
  decodes an asset once. Progress counts completed passes, not bytes within a call.
- **B:** calculate a 128x128 weather map with a coastline, storm cells and a
  green-to-red rainfall legend. Progress counts complete rows. This shows a
  procedural field/map calculation use case.
- **C:** cancel. Running work acknowledges cancellation at the next pass/row
  boundary; a decode can be preempted but is not aborted halfway through. An
  outstanding DMA loan must finish before its buffer can be released.
- **Start:** withhold/resume spare CPU time while the drone keeps moving. Held
  workers cannot acknowledge cancellation until resumed. This deliberately
  demonstrates starvation without discarding the saved PC/stack.

A finished image uploads 512 bytes per frame into a separate VRAM slot. The map
switches only after all 16 transfers complete. Unfinished output is never displayed.
The worker performs no input, VDP, DMA, allocator or audio operations. Its only SDK
work is the verified pure-RAM APLIB decoder.

Prepare a new directory and build:

```sh
python skills/sgdk-multitasking/assets/task-lab/tools/prepare.py build/task-lab
docker pull ewrogers/sgdk:latest
docker run --rm --mount "type=bind,source=$PWD/build/task-lab,target=/workspace" ewrogers/sgdk:latest make release
```

Use an absolute mount source in PowerShell. Add `--platform linux/arm64` or
`--platform linux/amd64` when selecting an architecture. Open `out/rom.bin` in a
Mega Drive emulator. All generated assets and source dependencies are local to
the prepared project; no other installed skill is needed.

The Lab uses a dedicated 2,048-byte user stack, including a 64-byte guard, in
addition to the SDK's normal stack reservation. The displayed watermark estimates
bytes touched; an intact guard is not a proof of worst-case stack depth. Output
staging is 8,192 bytes and the two VRAM slots total 16,384 bytes. Main RAM, VRAM,
DMA and CPU time are separate budgets. Foreground FPS counts game loops against
VBlank ticks; it is not a cycle counter or a measure of worker completion time.

SGDK already yields to a configured user task in its frame waits. The Lab has one
persistent callback and one owned job at a time. It uses compiler barriers and
aligned word flags for this particular single-68000 exception schedule, not C
threads. SDK task defaults mask HBlank interrupts; do not combine the example
with a raster effect without auditing the interrupt policy.

Host contracts cover ownership, cancellation, retries, deterministic procedural
output and stack guards. Target checks execute the actual SDK APLIB decoder and worker steps on base
M68000. They do not emulate interrupt/task switching, VDP/audio or console timing. Real video behavior and frame headroom need
an emulator or console check, especially with a game's other interrupts and DMA.
