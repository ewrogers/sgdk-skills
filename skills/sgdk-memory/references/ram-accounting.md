# Work RAM accounting

## Start with the linked program

Work RAM is 65,536 bytes. Count initialized `.data`, zero-initialized `.bss`,
alignment, heap storage and a stack allowance. Initialized writable data also
has a ROM initializer; do not charge that ROM copy to RAM a second time.
File size, ROM padding and debug ELF sections are not work-RAM usage.
Read-only tables can live in ROM when their declarations and generated code
allow it; verify section placement rather than assuming every `const` pointer
makes its pointee read-only.

SGDK's stock linker uses the `0xE0FF0000` work-RAM alias and places `_bend`
after static data/BSS. `MEM_init` starts the heap at that address, rounded to an
even boundary, and leaves a two-byte end marker below `MEMORY_HIGH`.
The selected `memory_base.h` reserves `STACK_SIZE = 0x0A00` (2,560 bytes) below
the top-of-RAM stack pointer. This is an SDK choice, not a hardware stack size
or proof that a game cannot overflow it. SDK initialization then allocates
runtime buffers from the heap; the linked static report does not include them.

Sources: [stock linker](https://github.com/Stephane-D/SGDK/blob/v2.11/md.ld),
[RAM/stack definitions](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/memory_base.h),
[allocator initialization](https://github.com/Stephane-D/SGDK/blob/v2.11/src/memory.c).

## Ask what a counter includes

| Evidence | What it establishes | What remains separate |
| --- | --- | --- |
| ELF `.data` / `.bss` | Linked writable static storage | Heap and peak stack |
| `MEM_getAllocated()` | Allocator-owned block bytes, including headers | Globals, stack, ROM, VRAM |
| `MEM_getFree()` | Sum of free heap block bytes, including their headers | Maximum single payload |
| `MEM_getLargestFreeBlock()` | Largest contiguous free run, including its header | A guaranteed allocation without rounding/overhead |
| Runtime baseline comparison | Growth or recovery at the same lifecycle point | Unvisited paths and corruption detection |
| Stack sample | Stack depth at that sample | Interrupt/call-path worst case |

These functions inspect heap metadata; do not call them for every entity in a
hot loop or while another context mutates the allocator. Compare like for like:
same SDK/build settings, initialized renderer/audio, loaded resources and queue
state. A rising count may be an intentionally retained cache; assign it an
owner and bound before calling it a leak.

Source: [counter implementations](https://github.com/Stephane-D/SGDK/blob/v2.11/src/memory.c).

## Use the report helper

From a copied Memory Lab, run:

```sh
python tools/account.py --elf out/rom.out
python tools/account.py --elf out/rom.out --json
```

It runs the selected container's `m68k-elf-size` and `m68k-elf-nm`, reads that
SDK's stack reservation, and checks that the static span, heap capacity,
alignment, end marker and reserved stack add up to 65,536 bytes. ARM64/AMD64
can be selected with `--platform linux/arm64` or `linux/amd64`.
It does not run or modify the ELF. Use it with the stock SGDK layout and the
same SDK configuration used to build the ELF. Custom linker layouts or custom
stack/heap boundaries need their own accounting; a well-formed stock report
cannot verify those choices. The report is a reservation breakdown, not a
peak-use measurement or a memory-safety certificate.

## Leave stack and transition headroom

Include large local arrays, nested calls, compiler-generated temporaries,
exception frames and interrupt handlers. Disassembly and compiler stack-use
output can help, but assembly helpers and indirect calls need separate review.
A debug stack watermark must be planted only in a deliberately reserved unused
region, before it becomes live. Do not fill all apparently free RAM while the
heap, SDK or stack is using it. A sample or watermark is observed use, not a
proof of the untested worst case.

During a transition the old room, the next room and scratch buffers may coexist.
Budget that overlap. VRAM, queued DMA source storage, ROM mapper windows and
sound-driver memory have different owners and limits. Saving main RAM by
re-reading or decoding data can cost CPU time and bus bandwidth; measure the
trade rather than optimizing a byte count in isolation.
