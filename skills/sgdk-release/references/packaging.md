# Package a release candidate

## Build once, keep that build

Start from the intended source revision. Record any uncommitted changes rather
than pretending the commit alone describes the result. Use a clean output
folder and the project's actual build procedure. A stale ROM left after a
failed build is not a candidate.

For a normal SGDK project, from its directory:

```sh
docker run --rm --pull=always --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/sgdk:latest build clean
docker run --rm --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/sgdk:latest build release
```

In PowerShell use `${PWD}` instead of `$(pwd)` and Docker's Linux-container
mode. Pull/build on the intended native architecture when available. Both
ARM64 and AMD64 images are supported; host packaging needs only Python 3.10+.

Record the resolved image identity, SDK/compiler versions, source revision,
build command and candidate hash. For example, keep the output of
`docker image inspect ewrogers/sgdk:latest` and the image's `doctor` command.
Rebuilding with a changed `latest` tag need not reproduce an old release.

Archive the matching release ELF (`out/rom.out`), `symbol.txt` and any map file
before another build overwrites them. Keep a private source/build record too.
Do not describe a separate debug ELF as matching the release ROM. Optimization
can limit source-level debugging even when symbols are available.

## Use the helper

Inspect first; `--json` returns a machine-readable report. Invalid ROMs exit 1,
valid ones exit 0, and warnings remain visible in either case. Argument errors
use argparse's exit 2. A missing/unreadable/oversize file returns an error on
stderr. The helper does not repair or rewrite inputs.

```sh
python3 /path/to/sgdk-release/scripts/release.py inspect out/rom.bin --json
python3 /path/to/sgdk-release/scripts/release.py package --rom out/rom.bin --output releases/my-game-0.1.0.zip --name "My Game" --version 0.1.0 --readme release-notes.txt --license LICENSE.txt --credits CREDITS.txt
```

Create the output directory first. Choose a new archive path: the helper never
overwrites an existing release. It validates before packaging and publishes a
finished ZIP through an exclusive same-directory hard link; a filesystem that
does not support hard links reports a failure instead of replacing anything.

`--credits` is optional. `--build-info path/to/build-record.txt` adds provenance
you prepared. `--symbols out/rom.out` optionally includes the selected ELF;
use a separate developer archive if you do not want symbols in the player ZIP.
The helper checks the basic big-endian ELF32/M68000 header, not its correspondence to the ROM. Select both
from the same preserved build and keep their hashes together.

## What goes in the ZIP

The game name supplies a lowercase filename slug; the version must be a short
filename-safe label. A title needs an ASCII letter/digit for that slug. The
archive contains the exact ROM, supplied README/license, optional credits,
build information and symbols, plus `manifest.json` with byte counts and
SHA-256 hashes. It does not search the project or collect unrequested files.

ZIP entries have a fixed timestamp/order/permissions and are stored without
compression. Identical inputs produce identical archive bytes, independently
of host file times or compression-library versions. No absolute source paths
are added to the manifest. Supplied documents and ELF files can themselves
contain local paths or other information; inspect them before publishing.

The manifest records the validation profile and warnings. It cannot identify
the unpadded payload by guessing where zero bytes begin, prove the license
is sufficient, or infer what gameplay you tested.

## Write for someone who wants to play

Keep release notes short: game/version, controller buttons, supported regions,
how to start, whether it saves, and known issues. List the emulator/version
and hardware actually tested. Do not turn a `JUE` header into a claim of
PAL timing support; test each advertised region.

Retain the project's license, SGDK's required notice and the notices for used
code, fonts, art and music. Check the selected dependency licenses; do not
relicense them as MIT or copy the entire compiler toolchain into the game ZIP.
This skill does not perform a legal review.

Extract the ZIP into a fresh folder and inspect/play that ROM. Check startup,
controls, menus, audio, pause/resume, a busy scene and any save/reload path.
Keep physical-cart mapping, battery retention and hardware checks separate
from emulator results. If changing the candidate, regenerate the archive and
repeat the affected checks. Publishing remains a separate authorized action.

Sources: SGDK's [build recipe](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/makefile.gen),
[license notice](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/license.txt)
and [GCC runtime exception](https://github.com/Stephane-D/SGDK/blob/ef9292c03fe33a2f8af3a2589ab856a53dcef35c/COPYING.RUNTIME).
Archive naming, documents and provenance are this skill's release conventions,
not SGDK runtime APIs.
