# Resource import example

A small SGDK project showing one indexed background tile and a two-frame 16 by 16 sprite. The art is deliberately simple so palette indices and tile costs are easy to inspect. It was authored for this repository, with no third-party art.

Copy this directory somewhere writable, then build it using your project's
pinned image. This example was tested with the following SGDK 2.11 image:

```sh
docker run --rm --mount "type=bind,source=ABSOLUTE_PROJECT_DIRECTORY,target=/workspace" ewrogers/sgdk:2.11-gcc16.2.0-binutils2.47@sha256:fd7a891d37773f792aad528bb6640d63bc0bd8e86ed140b8ef43d296f035f51c
```

The ROM is `out/rom.bin`. Open it in an emulator to check the blue tile at the upper left and the small animated marble near the center. This example does not cover scrolling, input or a complete game's palette allocation.

`res/resources.res` uses `PALETTE`, `SPRITE` and `IMAGE`. The sprite declaration's `2 2` frame size is measured in 8 by 8 tiles. Its image is 32 by 16 pixels, so it contains two frames across one animation row. The frame duration is eight 1/60-second units.

`src/main.c` uses the generated resource types and checks sprite allocation. It loads the background and sprite palettes into separate slots for clarity. A real project can share palette data when its indices agree. Resource import and C compilation are tested; colors and animation still deserve visual review.
