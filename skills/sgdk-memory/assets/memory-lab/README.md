# Memory Lab

See why a large allocation can fail despite enough free bytes, then compare
shared, room-owned and queued-transfer lifetimes. This is an interactive
allocator demonstration, not a game or a custom replacement allocator.
It uses the actual SGDK heap. The colors and allocation sizes are illustrative.
The Lab is silent.

## Build and run

Requires Python 3.10+, Docker with Linux containers, and a Mega Drive emulator
or console. From this Lab directory, choose a new output folder:

```sh
docker pull ewrogers/sgdk:latest
python tools/build.py --output "build/memory review"
```

The helper copies a standalone project, then builds `out/rom.bin` and matching
`out/rom.out`. It refuses to overwrite output. Select a platform with
`--platform linux/arm64` or `linux/amd64`, or add `--mode debug`.
`--region pal` / `--region ntsc` changes the header hint; configure the emulator
to actually use that region. Automatic steps use the detected 50/60 Hz rate.

On macOS:

```sh
/Applications/ares.app/Contents/MacOS/ares --system "Mega Drive" "/absolute/path/out/rom.bin"
```

Other hosts can use their emulator ROM picker. Host-helper portability does
not establish emulator or physical-console support on every OS.

## Controls

| Control | Fragmentation page | Lifetimes page |
| --- | --- | --- |
| Left / Right | Switch page and clean up current test | Same |
| A | Advance allocation / holes / pack / cleanup | Load or unload a room |
| B | Try a 1,536-byte allocation, then free it if it fits | Queue a 128-byte RAM-to-VRAM upload |
| C | Release test reservations and restore baseline | Unload room; shared storage stays |
| Start | Toggle one automatic step per second | Automatically load/upload/unload rooms |

With the usual ares mapping, A/B/C are J/K/L and Start is Enter.

## What to look for

The six rectangles are allocation indicators, not a byte-accurate heap map.
Blue means live and grey means released. Gold marks the 512-byte shared payload.
Counters show actual `MEM_getFree`, `MEM_getAllocated` and
`MEM_getLargestFreeBlock` values. They include allocator headers and exclude
static data and the stack reservation.

On the fragmentation page, press A four times:

1. Allocate six 1,024-byte payloads. Reserve the large free tail and any other
   free runs over 512 bytes so unrelated spare space cannot hide the experiment.
   This deliberately makes the heap look full; it is not normal game usage.
2. Release alternating blocks. The 1,536-byte request fails even though total
   free space exceeds it. Live allocations separate the holes.
3. Pack free blocks. The same request still fails; the live blocks do not move.
4. Release remaining blocks and reservations. Free bytes and largest run must
   return to the baseline captured after shared storage was allocated.

Press B again after cleanup: the request fits and is immediately released.
Start repeats the four-step cycle. Expected failed requests are part of the
demonstration; a `CHECK FAILED` banner indicates an actual invariant failure.

On the lifetimes page, A publishes two room allocations only after both succeed.
B allocates staging and queues four actual tiles to VRAM. The green preview
shows the uploaded tiles. The temporary storage is released after the queue
drains, so its retained state may last just one displayed frame. Shared storage
survives room unloads. Start repeatedly loads, uploads and unloads; counters
should return to the same baseline after each unload.

## Accounting

```sh
python tools/account.py --elf "build/memory review/out/rom.out"
python tools/account.py --elf "build/memory review/out/rom.out" --json
```

The helper reads sections and symbols through the selected container, using
its stock stack reservation. It prints a comma-formatted ASCII table separating
initialized data, zeroed globals, padding, heap capacity, end marker and reserved
stack. This is not measured peak usage. Use the matching SDK and stock linker
layout; customized heap/stack boundaries need a matching report.

| Lab-owned storage | Payload |
| --- | --- |
| Shared allocation | 512 bytes |
| Six fragmentation blocks | 6,144 bytes |
| Tail / spare-run reservations | Varies with initialized SDK heap; fragmentation page only |
| Room buffers | 1,536 bytes total |
| Transfer staging | 128 bytes until completion |
| `MemoryLab` owner record | 122 static bytes on the tested M68000 build |
| Retained text/indicator tilemap | 2,240 static bytes |
| Preview tiles | 9 tiles / 288 bytes of VRAM, separate from RAM |

The selected allocator charges an additional two-byte header and even rounding
per allocation. Room payload therefore costs 1,540 allocator bytes; staging
costs 130. These figures exclude SDK allocations, other globals and stack.
The owner record contains a bounded array for up to 16 spare-run reservations;
if a more fragmented starting heap needs more, setup rolls back safely.

The Lab owns tiles 96–100 and 112–115 and uses PAL0 plus the SDK font. It does
not initialize the Sprite engine. Normal padded ROM size is 128 KiB. No source
or asset needs downloading beyond the toolchain.

Text and indicators are composed in the retained tilemap and queued during
VBlank, avoiding active-display CPU redraws while testing the heap. A screen
update transfers 2,240 bytes, plus at most 128 staging bytes, within a configured
4,096-byte DMA allowance. The tilemap stays unchanged until the queue drains;
it is a fixed Lab UI cost, not an allocator requirement for other games.

## Validation boundaries

Host tests execute the actual ownership module at `-O0` and `-O3`, with Unix
undefined-behavior checks and an independent first-fit heap oracle. They reject
each construction failure, retain delayed-transfer sources and run 500 complete
heap/room cycles. This model does not claim SGDK's exact search order.

Container tests build release/debug and execute the linked SGDK allocator as
base M68000 code. They test headers, odd/zero requests, pre-existing free holes,
fragmentation, pointer/content preservation, delayed ownership release and 32
cleanup cycles. Both architectures compare ROM hashes and accounting reports.
The CPU probe bypasses boot, so it explicitly invokes internal SDK memory
initialization; the interactive ROM uses ordinary SGDK startup.

These checks do not emulate VDP timing, measure peak stack, or prove every
game's RAM safety. The interactive ROM and ares checks establish presentation;
PAL presentation and physical hardware remain separate checks.
