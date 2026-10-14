# Build errors, addresses, and memory

## Reproduce the build

Use the project's pinned image or the fallback in `SKILL.md`. In the commands
below, replace `IMAGE` and the absolute project path. Keep the whole mount
argument quoted. Docker Desktop on Windows accepts an accessible path such as
`C:/Projects/game`; Linux and macOS use their normal absolute paths.

```sh
docker run --rm --mount "type=bind,source=/absolute/path/to/game,target=/workspace" IMAGE doctor
docker run --rm --mount "type=bind,source=/absolute/path/to/game,target=/workspace" IMAGE build clean
docker run --rm --mount "type=bind,source=/absolute/path/to/game,target=/workspace" IMAGE build debug
```

`build` invokes SGDK's makefile. Other commands run directly, with arguments
preserved. A failed command must remain a failure; do not hide it behind a
successful log-filtering command. Run as the host UID/GID on Linux when needed
to avoid root-owned output. Build tools and paths inside the image are Linux
tools even when the host is Windows or macOS.

SGDK 2.11 debug builds use `-O1`, `DEBUG=1`, debug information, and
`libmd_debug.a`. Release uses `-O3` and LTO. Clean before switching modes or
compiler flags because stale objects can make comparisons misleading. For an
optimized build with source information, clean and use
`build "EXTRA_FLAGS=-std=gnu17 -g"`. Keep the container's C dialect when replacing
`EXTRA_FLAGS`; the original SGDK sources predate GCC's newer default dialect.

## Resolve a fault to code

Capture the faulting program counter (PC), exception type, and any available
registers. On a 68000, odd instruction addresses or word/long accesses are
important address-error clues. Preserve the reported faulting address too.
An apparent return address in a stack dump is a clue, not a verified call stack.

Use the ELF from the **same build**, not a rebuilt version with shifted addresses:

```sh
docker run --rm --mount "type=bind,source=/absolute/path/to/game,target=/workspace" IMAGE m68k-elf-addr2line -e out/rom.out -f -C -i 0x00001234
docker run --rm --mount "type=bind,source=/absolute/path/to/game,target=/workspace" IMAGE m68k-elf-objdump -d -S out/rom.out
docker run --rm --mount "type=bind,source=/absolute/path/to/game,target=/workspace" IMAGE m68k-elf-nm -n -S out/rom.out
```

`rom.out` is an ELF, despite its extension. `symbol.txt` is SGDK's ordered symbol
listing. `addr2line` needs source information; `??:0` means it cannot resolve the
address. Optimization can inline functions or move instructions. Inspect nearby
disassembly and the source; do not promise an exact source statement for every PC.
ROM file offsets and CPU addresses are not interchangeable in every section.

The image includes `m68k-elf-gdb` for offline symbol/disassembly work. Remote
stepping requires an emulator or hardware debugger that actually implements a
compatible remote protocol. Do not assume ares exposes a GDB server, or invent
command-line switches. Include `<kdebug.h>` explicitly for SGDK's `KDebug_*`
functions, which target Gens KMod-style debug
support; they do not create a universal console, USB link, or Docker log stream.

## Account for bytes honestly

```sh
docker run --rm --mount "type=bind,source=/absolute/path/to/game,target=/workspace" IMAGE m68k-elf-size -A out/rom.out
docker run --rm --mount "type=bind,source=/absolute/path/to/game,target=/workspace" IMAGE m68k-elf-objdump -h out/rom.out
```

Use ELF section addresses and SGDK's linker script to distinguish ROM and RAM.
Initialized `.data` needs RAM at runtime and an initial image in ROM; `.bss`
needs RAM without the same payload in the ROM. ELF size includes metadata and
debug sections, so it is not cartridge size. `rom.bin` is the file to measure.

The Mega Drive has 64 KiB of main 68000 RAM. Static section totals leave out
peak stack use, heap allocations, and buffers created at runtime. Account for
SGDK's own reserved state and pools too. VRAM (64 KiB) and the Z80's RAM (8 KiB)
are separate memories; do not subtract either from the main-RAM total or present
a section subtotal as an exact free-memory figure.

The ROM helper reports header ranges without treating them as actual RAM
allocation or proof of corruption. Some upstream samples declare a fixed ROM
range larger than the padded file. It deliberately does not reject those ROMs
or apply a generic Sega checksum as if it were SGDK's build-time XOR checksum.

## Sources

Verified against the pinned SGDK 2.11 container. Recheck these when changing SDKs:

- [Build flags, outputs, and symbol injection](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/makefile.gen)
- [Linker memory layout](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/md.ld)
- [Default ROM header](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/src/boot/rom_head.c)
- [KDebug declarations](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/inc/kdebug.h)
- [GNU addr2line documentation](https://sourceware.org/binutils/docs/binutils/addr2line.html)
