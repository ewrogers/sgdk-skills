# Compression Lab

A small diagnostic gallery for SGDK's RAW, APLIB and LZ4W paths. The aqueduct is
original procedural pixel art; the entropy image is a deliberate poor-compression
control. **The random dots are intentional, not broken graphics.** The on-screen
byte check compares decoded data with the original outside the timed batch.
Neither sample is a required game style.

Prepare a **new** directory, then build through the container:

```sh
python skills/sgdk-compression/assets/compression-lab/tools/prepare.py build/compression-lab
docker pull ewrogers/sgdk:latest
docker run --rm --mount "type=bind,source=$PWD/build/compression-lab,target=/workspace" ewrogers/sgdk:latest make release
```

Use an absolute source path in PowerShell. Add `--platform linux/arm64` or
`--platform linux/amd64` when selecting an architecture explicitly. Open
`out/rom.bin` in a Mega Drive emulator.

Left/Right changes page:

- **Compare:** A measures again (controls wait for the synchronous batch); B changes sample. The table shows each stored
  BIN array's bytes, approximate microseconds per decode/copy and staging payload.
  RAW can upload directly from mapped ROM with zero decode staging; its measured
  operation here is a copy into the same 4,096-byte buffer as the codecs.
- **Load:** C chooses format; A loads the other picture behind a palette fade.
  Decode is synchronous, followed by eight 512-byte uploads while black.
- **Preload:** A decodes eight independent LZ4W blocks, one per frame, into the
  other VRAM slot while the current art stays visible. Once ready, A publishes
  the cached picture with no new decode. This is chunked loading, not an
  asynchronous SGDK decoder or the multitasking API.

The loader owns a 4,096-byte staging payload throughout its operation, including
queued transfers. Two VRAM slots occupy 8,192 bytes. Chunking caps decoded bytes per
step, not elapsed CPU time: different blocks decode at different speeds. The
model cannot publish early, overwrite borrowed data, or cancel a pending transfer.

Timing uses `getSubTick()` with interrupts enabled across 16 operations, including
interrupt overhead. The SDK timer is coarse/fixed during VBlank, so these are
estimates, not CPU cycles. Do not infer console timings from accelerated emulator
execution. The whole Lab ROM contains **all** formats and chunk variants: padded
ROM size is not the sum of visible table entries or a per-format shipping result.
Generated resource headers report array storage (excluding padding between symbols); LZ4W chunks
are separate resource files so they do not borrow another block's dictionary.

Host tests check ownership and failure behavior. Integration tests compile the
actual resources and execute SDK decoders on a base M68000, checking every output
byte and guard words in release/debug builds. Those checks do not emulate VDP
scanout, timer accuracy, fades or actual frame cost. PAL/NTSC emulator checks and
real-console timing remain separate.
