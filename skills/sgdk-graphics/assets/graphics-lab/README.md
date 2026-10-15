# Graphics Lab

A small SGDK project with a moving animated actor, two wrapping background
planes, a fixed Window HUD, palette cycling and an asynchronous fade. The art
is original test art, kept deliberately simple so changes are easy to see.

Copy this folder into a **new** project folder. From that copy, build using
Docker with Linux containers:

```sh
docker run --rm --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/sgdk:2.11-gcc16.2.0-binutils2.47@sha256:7c43a41e5bfa177b59b938d56bf496419df6b55778a6b5c68c629591ccc21b34 build
```

In PowerShell, replace `$(pwd)` with `${PWD}`. The output is `out/rom.bin`.
Use `build debug` for symbols or `build clean` to remove generated outputs.
The image supports Linux ARM64 and AMD64, including through Docker Desktop.
The lab has no dependency on other installed skills or host libraries.

## Try it

| Control | Action |
| --- | --- |
| Left / Right | Move the actor |
| A | Stop or resume its animation on the displayed frame |
| B | Play a one-shot burst; the counter increments once, then the last frame stays |
| C | Fade the actor palette out or in; a request during a fade is ignored |

After a burst completes, B plays it again. Resume restarts the current frame's
full hold duration, matching SGDK's automatic animation behavior. The scrolling
and color cycle keep running when only the actor animation is stopped.

This example advances once per VBlank. Its movement and hold lengths are tuned
for NTSC; they take longer on PAL. It is not a region-normalized game loop.

## How it fits together

- `src/main.c` owns display setup, the frame loop and a HUD updated only when
  the count changes.
- `src/demo.c` owns the actor, animation state, scroll offsets and effects.
  `demo_end()` is a blocking teardown for this standalone owner, including
  flushing pending work. Do not use it to remove just one object in a larger game.
- `res/resources.res` sets the frame grid and per-frame holds. Each sheet row
  is an animation, each column a frame.
- PAL0 belongs to the far plane, PAL1 to the cycling near plane, PAL2 to the
  actor and PAL3 to the HUD. The two effects never write the same entries.

The planes are 64x32 tiles and wrap at 512x256 pixels. This shows parallax, not
large-level streaming. Each background is loaded once. The sprite engine's
reservation is established before checking the remaining static tile range.

To recreate the three PNGs, run `python3 scripts/make_lab_art.py NEW_DIRECTORY`
from the installed `sgdk-graphics` skill folder. This optional generator uses
only Python's standard library and refuses to replace existing files. It is
part of the skill, not a build dependency of this copied project.

## Visual checks

Compilation checks resources and API compatibility, not the picture. In an
emulator or on a console, check that:

1. The HUD stays fixed while both backgrounds scroll at different speeds.
2. Scrolling wraps without jumping, and Left/Right clamp the actor on screen.
3. A holds the frame, B finishes exactly once, and B can replay after completion.
4. C fades only the actor while foreground highlights continue cycling.
5. Frame changes and palette effects produce no missing art or color dots.

Use a release ROM when measuring performance. Record the region, emulator or
hardware and workload alongside the result. The lab's tests do not certify
60 Hz rendering or real-hardware scanline behavior.
