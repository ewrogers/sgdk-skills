---
name: sgdk-project
description: Create, inspect or build an SGDK C game project using the SGDK Docker toolchain. Use for project setup, release/debug builds, tool invocation and checking the installed SDK API.
---

# SGDK project

Help the developer get a normal SGDK project building. Use the container for
the toolchain and keep source files on the host. Do not introduce a game engine
or replace an existing build system as part of setup.

## Establish the project

1. Inspect existing source, resource files, Makefiles and toolchain pins. Preserve
   the project's SDK version and conventions unless an upgrade is requested.
2. If no toolchain is selected, use the tested image in
   [the Docker workflow](references/docker-workflow.md). Prefer its native
   architecture; do not force AMD64 on an ARM64 machine.
3. Run `doctor` and check the relevant headers, resource manual and upstream
   sample before writing unfamiliar SDK calls. Use actual declarations and
   comments, not a remembered API from another release or engine.

The portable helper is [scripts/sgdk.py](scripts/sgdk.py). Locate it relative to
this installed skill, not the current directory or a neighboring skill.
It needs Python 3.10+ and a running Docker engine with Linux containers.

```sh
python3 /path/to/sgdk-project/scripts/sgdk.py doctor
python3 /path/to/sgdk-project/scripts/sgdk.py init my-game
python3 /path/to/sgdk-project/scripts/sgdk.py --project my-game build release
```

`init` copies SGDK's Hello World example into a new folder and refuses existing
destinations. `build` uses SGDK's `makefile.gen`. For an existing project with
its own Makefile, use `--project my-game run make release` instead. The helper
passes arguments without a shell and returns the tool's exit status.

## Build a small proof

Keep the first milestone small: a valid ROM, then one visible change, then input
and gameplay. Rebuild changed C/resources and keep useful error output. Do not
keep trying renamed API calls until one compiles; resolve the declaration first.

Use optimized release builds as the normal playable build. A debug build is
useful for symbols and diagnostics, but its speed is not the release baseline.
SGDK owns the VBlank/DMA services; follow its frame-loop and initialization
patterns rather than inventing interrupt or video-register setup.

Budget CPU work, main RAM, VRAM, sprite scanlines and transfer bandwidth
separately. Reuse unchanged graphics rather than uploading or redrawing them
every frame. See [runtime decisions](references/runtime-decisions.md) when
choosing a frame loop, numeric types or memory strategy.

For large ROMs, mapper changes or low-level audio, read
[cartridge and sound boundaries](references/cartridge-and-sound.md). The normal
4 MiB ROM window, SGDK's padding, Z80 bus ownership and FM/PSG register ownership
are separate questions; do not assume linking or a helper name makes them safe.

## Verify and hand off

- Confirm the intended target built successfully and identify the new ROM.
  A stale `out/rom.bin` is not proof that a failed build succeeded.
- Keep generated `out/` files out of source control; keep source art and `.res`
  files. Do not clean unrelated directories or overwrite existing projects.
- Compile success verifies tool/API compatibility, not controls, visuals,
  audio, performance or hardware accuracy. State which emulator or hardware
  checks actually ran. Run emulators on the host, separately from Docker.
- If Docker or host file sharing is unavailable, explain that prerequisite.
  Do not silently install a native toolchain or change host security settings.
