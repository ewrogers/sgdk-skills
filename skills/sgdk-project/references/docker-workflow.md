# Docker workflow

The tested default is SGDK 2.11 in this immutable multi-platform image:

```text
ewrogers/sgdk:2.11-gcc16.2.0-binutils2.47@sha256:7c43a41e5bfa177b59b938d56bf496419df6b55778a6b5c68c629591ccc21b34
```

It contains GCC 16.2.0, binutils 2.47, GDB 18.1, Java and SGDK's resource tools.
It sets `GDK=/opt/sgdk`, puts the tools on `PATH` and defaults to
`EXTRA_FLAGS=-std=gnu17`. Libraries are built with the same compiler, preserving
SGDK's optimizer workarounds. Prefer a digest for reproducibility; the shorter
`ewrogers/sgdk:2.11` tag can move when the toolchain is updated.

## Host differences

- macOS: Docker Desktop supports both Apple Silicon and Intel Macs.
- Linux: Docker Engine or Docker Desktop. The helper supplies the current UID
  and GID when mounting a project so generated files belong to the caller.
- Windows: Docker Desktop must use Linux containers. Use WSL commands or
  native Python with the helper; it handles paths with spaces without Bash.
  Replace `python3` with `py -3` if that is the installed Python launcher.
- Docker normally selects the host architecture. `--platform linux/arm64` or
  `--platform linux/amd64` is available for deliberate checks; an opposite
  architecture may use emulation and be slower.

For an explicit platform with the default image, the helper selects the verified
architecture-specific digest from that index. This avoids a Docker Desktop
cache conflict when testing both architectures locally. Custom image references
are passed through unchanged.

Host helper behavior is tested on Windows, macOS and Linux in CI. Container
builds run on native Linux ARM64 and AMD64; Windows Docker runtime verification
is a separate check, not implied by Python tests.

## Common commands

Global helper options go before the subcommand:

```sh
python3 /path/to/sgdk.py --project my-game build debug
python3 /path/to/sgdk.py --project my-game build clean
python3 /path/to/sgdk.py --project my-game run m68k-elf-size out/rom.out
python3 /path/to/sgdk.py run sh -c 'grep -n "SYS_doVBlankProcess" "$GDK/inc/sys.h"'
```

`run` accepts a tool and its arguments. Host shell operators are not interpreted
by the helper. Use `sh -c` explicitly for a container-side shell, and quote
`$GDK` so the host does not expand it. For an interactive terminal use the
Docker CLI directly with `-it`.

Without Python, the equivalent release build on macOS/Linux/WSL is:

```sh
docker run --rm -v "$PWD:/workspace" ewrogers/sgdk:2.11 build release
```

In native PowerShell use:

```powershell
docker run --rm --mount "type=bind,source=$($PWD.Path),target=/workspace" ewrogers/sgdk:2.11 build release
```

These shorter examples use the moving tag for readability. Use the pinned
reference above, or the project's existing pin, for reproducible work. A bind
mount's host folder must exist and be shared with Docker Desktop. A permissions
error is not a reason to run a privileged container or mount the Docker socket.

## Find the real API

For the selected version, inspect `$GDK/inc`, `$GDK/src`, `$GDK/sample` and
`$GDK/bin/rescomp.txt` inside the container. SDK defaults and driver interactions
are documented in header comments as well as declarations. The installed
container is the immediate authority when website documentation differs.

Keep errors from the compiler/ResComp and fix their cause. If an API is absent,
say so and choose an available alternative or discuss a version change. Never
claim a guessed signature is supported merely because a similarly named API
exists elsewhere.

Sources: [container usage and build recipes](https://github.com/ewrogers/sgdk-docker),
[SGDK 2.11 headers](https://github.com/Stephane-D/SGDK/tree/v2.11/inc),
[SGDK project makefile](https://github.com/Stephane-D/SGDK/blob/v2.11/makefile.gen).
