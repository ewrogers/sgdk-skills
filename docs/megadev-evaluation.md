# Sega CD toolchain evaluation

Research and local build snapshot: 15 October 2026. The prototype builds passed
on native ARM64 and emulated AMD64. Emulator playback and native AMD64 CI remain
separate checks.

## Local results

A separate Debian 13 prototype image was built for each architecture. It uses
native host packages: GCC 14.2.0, binutils 2.44 and genisoimage 1.1.11, with
Megadev at the source revision below. Debian supplies a working `mkisofs` alias.
No SGDK runtime, external BIOS or downloaded firmware was added.

| Check | ARM64 on Apple Silicon | AMD64 on Apple Silicon |
|---|---|---|
| Host tools | Native `aarch64` | Emulated `x86_64` |
| C/module template | Passed | Passed |
| Hello World | Passed | Passed |
| Backup RAM example | Passed | Passed |
| PCM playback example | Passed | Passed |
| Transform and graphics examples | Both passed | Both passed |
| Cartridge example | Passed | Passed |
| C template, US/NTSC, JP/NTSC, EU/PAL | All three built | All three built |

Across the seven projects, all 32 generated module/boot/cartridge payloads
matched between architectures. The six disc projects also had identical boot
areas and all 12 ISO filesystem file payloads matched. Disc signatures and
ISO9660 volume descriptors were checked. Whole ISO hashes differed because of
filesystem metadata; this is not a normalized reproducible-image test.

The examples were copied from upstream without source changes, with empty
`build` and `disc` directories prepared before `make all`. Success here proves
buildability and matching payloads, not successful BIOS boot, data loading,
PCM playback or backup-memory behavior on a running console.

Local reproduction files and logs are under ignored
`build/megadev-evaluation/`: `Dockerfile`, `check_examples.py`, `check_regions.py`,
the architecture reports and fresh example copies. This is an evaluation
artifact, not a published or production-ready container repository.

## Recommendation

Evaluate Megadev in a **separate Sega CD image** before creating a skill. It
already supplies main/sub CPU support, boot-sector construction, module loading
and disc-file access. Its API and project structure differ from SGDK, so this
should not become an SGDK build mode. Upstream explicitly expects familiarity
with C, 68000 assembly and embedded development. [Project overview][overview]

Pin the evaluation to Megadev commit
`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`. All Megadev links below refer to that
snapshot. A friendly public image tag can move independently of the recorded
source revision used to build it.

## What upstream actually provides

| Area | Source-backed capability | Practical boundary |
|---|---|---|
| Languages | C with GCC extensions and GNU 68000 assembly | No integrated C standard library; C++ and Z80 assembly builds are not supported workflows. |
| Build | GNU Make, GCC, binutils, `mkisofs` | `-m68000` is supplied; linking uses `-nostdlib`. |
| Cartridge/disc targets | `MEGADRIVE` and `MEGACD` | CD builds need their own boot and memory layout. |
| Main/sub CPUs | Separate initial programs and optional resident C modules | Work RAM, Word RAM and PRG RAM ownership need explicit planning. |
| Data loading | Sub CPU CD-ROM framework and module loaders | ISO9660 level-1 names, such as `TITLE.MMD;1`; errors and buffer ownership remain the game's responsibility. |
| Disc image | A bootable data-track `.iso` | Mixed-mode audio requires additional tracks and a CUE sheet. |

Sources: [manual][manual], [build rules][make], [C development notes][c],
[module architecture][template], [disc-file access][cdrom].

The `new_project` example is the stronger integration check: it builds C code
for both CPUs and loads separate modules from disc. `hello_world` is a smaller
assembly-only boot/font check; successfully building it would not prove C
modules or file loading. [Template][template], [template build][template-make],
[Hello World][hello]

## Container dependencies and architecture checks

Upstream's Debian 13 Dockerfile installs `gcc-m68k-linux-gnu`,
`binutils-m68k-linux-gnu`, `genisoimage`, `make`, `xxd`, Git, newlib headers and
formatting tools. Its alternate Alpine Dockerfile builds an `m68k-elf` compiler
from GCC 15.2.0 and binutils 2.45 sources, with the executable prefix set to
`m68k-linux-gnu-`. The prefix therefore does not, by itself, prove whether a
compiler targets Linux or bare metal. [Debian Dockerfile][debian],
[Alpine Dockerfile][alpine]

No bundled ELF or Mach-O host executable was found in this source snapshot.
The main build invokes ordinary host utilities and cross-compilers, which makes
native ARM64 and AMD64 containers a reasonable approach. The local builds above
test both host-tool variants; native AMD64 CI is still required before release.

Check both platforms for:

- Package availability and the actual compiler target/version.
- An executable `mkisofs`: the Makefile uses that name even when the installed
  package is called `genisoimage`.
- The GNU assembler extensions and register conventions used by the wrappers.
- Startup initialization, module relocations and unresolved compiler helper
  routines, especially when changing compiler versions.
- Base-68000 output, including linked code; do not assume a CPU flag proves
  every library or handwritten instruction is compatible.
- Matching boot headers, module bytes and filesystem contents. Raw ISO hashes
  can differ because of timestamps: the default copyright string also uses the
  build month. Normalize timestamps before requiring whole-image equality.

The upstream Dockerfiles clone an unpinned branch. Our evaluation should record
the exact source/compiler inputs instead. The C notes also explain that module
startup must initialize `.data` and `.bss`; merely declaring an initialized
global does not replace that startup work. [Build rules][make], [C notes][c]

## CD audio, PCM and the first lab

CD-DA playback wrappers exist on the **sub CPU**: `bios_msc_play1()` plays one
track and pauses, while `bios_msc_playr()` repeats it. Both take a pointer to a
16-bit track number. They are not SGDK sound-driver calls. The Hello World SP
also initializes the drive and waits for TOC reading even though its main-side
demo is tiny. [Sub CPU wrappers][bios], [Hello World SP][hello-sp]

Upstream documents a data ISO plus separate stereo, 16-bit, 44.1 kHz WAV files
referenced by a CUE sheet. Its example uses an actual two-second silent pregap
in each WAV; blindly copying its CUE against an unpadded file would skip real
audio. Audio tracks should not be copied into the ISO's file staging directory.
[Disc mastering][disc], [example CUE][cue]

The supplied `pcm_playback` example proves a different path: loading a file and
playing through the Sega CD PCM chip. It recommends the external SCDTools
converter; that dependency is not required just to build a data ISO or generate
an original CD-DA test tone. [PCM example][pcm]

A useful first **CD Lab** would display a known file's contents/checksum after
loading it from disc, then let the player play/stop an original generated CD-DA
track. Test the upstream examples first, and keep the original lab separate
from upstream code and attribution.

## Firmware, region and distribution

Megadev's repository license is MIT; retain its notice when redistributing
substantial parts. This is not evidence of permission to redistribute Sega
firmware. Build tools do not require a BIOS file, but ares' Mega CD frontend
loads separately configured BIOS firmware for US, Japan or Europe and returns
`noFirmware` if it is absent. Keep firmware user-supplied and outside images,
repositories and build artifacts. [Megadev license][license],
[ares firmware-loading source][ares]

Megadev selects a regional security block in `lib/security.c`, and its manual
says that block must match the BIOS region. These included byte arrays are
distinct from an entire BIOS ROM; the top-level MIT notice alone does not
establish their independent provenance. Record that provenance as a distribution
question rather than claiming every embedded byte is newly authored MIT code.
Use a matching-region BIOS for initial tests. [Security source][security],
[boot notes][boot]

## ares boot check

The ARM64-container build of upstream `new_project` boots in ares v148 with a
user-supplied US BIOS. The user confirmed animated particles and Start cycling
through Module 1, Module 2, Module 3 and back to Module 1. This exercises boot
and repeated loading of the example modules from disc. It does not yet test
our own CD-DA or PCM lab.

Open a CUE sheet pointing to the generated ISO as `MODE1/2048`; a bare ISO is
not accepted by ares' CD-ROM reader. Mega CD also has separate controller
mappings from Mega Drive, so check that Start is assigned.

An initially supplied BIOS stopped at a static intro background, including
with a known commercial disc. Replacing it with the user's US Model 2 v2.00w
BIOS (MD5 `854b9150240a198070150e4566ae1290`) restored the animated intro.
The user confirmed the comparison game booted with audio before testing the
Megadev example. Firmware stayed outside the repository and containers.
The checksum is also listed in the [Genesis Plus GX BIOS reference][bios-checksums].

## Remaining release checks

1. Repeat the passing example matrix on a native AMD64 runner.
2. Turn the local header, payload and regional-build probes into maintained CI.
3. An original mixed-mode CD Lab with deterministic data and audio assets.
4. An original lab verifying disc data and audible CD-DA playback with a
   user-supplied matching BIOS. The upstream module boot check passed; it does
   not prove these separate paths.
5. Clean checkout/build behavior, mounts containing spaces, output ownership,
   license notices and an image containing no user firmware.

[overview]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/README.md
[manual]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/manual.md
[make]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/megadev.make
[c]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/dev_in_c.md
[template]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/new_project/README.md
[template-make]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/new_project/makefile
[hello]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/examples/hello_world/README.md
[cdrom]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/cdrom.md
[debian]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/new_project/Dockerfile
[alpine]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/etc/Dockerfile_alpine
[bios]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/bios.h
[hello-sp]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/examples/hello_world/src/sp.s
[disc]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/disc.md
[cue]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/etc/example.cue
[pcm]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/examples/pcm_playback/README.md
[license]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/LICENSE
[ares]: https://github.com/ares-emulator/ares/blob/master/desktop-ui/emulator/mega-cd.cpp
[security]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/security.c
[boot]: https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/boot.md

[bios-checksums]: https://docs.libretro.com/library/genesis_plus_gx/#bios
