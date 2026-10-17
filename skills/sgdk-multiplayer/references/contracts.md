# Controller routes and SGDK contracts

Verified against SGDK 2.11 in `ewrogers/sgdk:latest` (GCC 16.2.0,
ResComp 3.95). Pull the current image and check its sources when changing readers.

## Routes are not player numbers

SGDK exposes eight logical joystick slots. `JOY_1` through `JOY_8` are numeric
IDs 0 through 7. The Lab's route table follows the SDK implementation:

| Connection | Logical slots |
| --- | --- |
| Direct pad on port 1 | JOY_1 |
| Direct pad on port 2 | JOY_2 |
| Team Player on port 1 | JOY_1, JOY_3, JOY_4, JOY_5 |
| Team Player on port 2 | JOY_2, JOY_6, JOY_7, JOY_8 |
| EA 4-Way Play | JOY_1, JOY_3, JOY_4, JOY_5; both ports occupied |

Two Team Players can expose eight logical pads. Four participants is this Lab's
game policy, not a hardware maximum. Raising it also requires reviewing camera,
HUD, CPU, sprite and scanline budgets.

Use `JOY_getPortType(port)` to identify the detected topology, then
`JOY_setSupport(port, reader)` to select its reader. The SDK's default detection
does not enable every attached Team Player alongside ordinary pads. Enable each
detected adapter explicitly with `JOY_SUPPORT_TEAMPLAYER`, with its physical
switch at MULTI. For EA use `JOY_SUPPORT_EA4WAYPLAY` on port 1 and
`JOY_SUPPORT_OFF` on port 2. It uses both connectors, not an independent second
pad. `JOY_setSupport` does not redetect hardware.

`JOY_reset()` redetects and performs three VBlank-processing passes. Call it only
at a controlled setup point. After reader setup, allow snapshots to settle and
seed previous held states. Polling readers and button edge detection are
different jobs: let normal `SYS_doVBlankProcess()` service input, then read
`JOY_readJoypad(id)` and `JOY_getJoypadType(id)` into one frame snapshot. Do not
add a second manual `JOY_update()` for each participant.

The Lab accepts `JOY_TYPE_PAD3` and `JOY_TYPE_PAD6`. A cached type or zero held
buttons cannot prove that a physical controller is connected. Some readers can
continue to report a pad type after removal. Explicit Leave preserves ownership
without inventing disconnect events. Physical unplug/replug behavior must be
tested with the actual reader and adapters.

Primary sources:

- [SGDK 2.11 joy.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/joy.h)
- [SGDK 2.11 joy.c](https://github.com/Stephane-D/SGDK/blob/v2.11/src/joy.c)
- [Plutiedev: Sega multitap](https://plutiedev.com/sega-multitap)
- [Plutiedev: EA multitap](https://plutiedev.com/ea-multitap)

## Rendering and transfers

`SPR_initEx(tileCount)` reserves sprite patterns. `SPR_addSprite` can return
NULL; handle allocation failure. Manual playback uses
`SPR_setAutoAnimation(sprite, FALSE)`, `SPR_setAnim` and `SPR_setFrame`.
Lower `SPR_setDepth` values render in front, so `-footY` gives top-down ordering.

The Lab fixes a 40-pixel Window HUD above the world. A full-screen Window
replaces plane A, including its transparent cells: those reveal plane B.
Title art therefore lives on B, with opaque Window cells behind instructions.
Returning to gameplay restores B's world map and its camera scroll. Never use
a busy tile pattern as the only background for essential text.

HUD DMA uses retained buffers. Drain pending owners before changing their data,
check queue capacity and account for sprites/scroll transfers separately.
`DMA_canQueue` lengths are words; `DMA_setMaxTransferSize` is bytes.
Changing plane size can relocate VRAM tables: validate art against the actual
post-initialization `TILE_SPRITE_INDEX`, not a guessed permanent boundary.

- [SGDK sprite engine](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h)
- [VDP maps](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_tile.h)
- [VDP setup](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h)
- [DMA](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h)
