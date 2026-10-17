# SGDK adapters, budgets and verification

Validated against SGDK 2.11 / ResComp 3.95 in `ewrogers/sgdk:latest` (GCC 16.2.0).
The image changes: pull it, inspect `/opt/sgdk/inc`, source and ResComp manual
before extending an unfamiliar API. Puzzle rules themselves are ordinary C.

## Checked interfaces

| Interface | Use in the Labs | Primary source |
| --- | --- | --- |
| `JOY_readJoypad(JOY_1)` | One held snapshot per displayed frame; derive fresh edges | [joy.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/joy.h) |
| `SYS_isPAL`, `SYS_doVBlankProcess` | 50/60-Hz adapter, input/video service | [sys.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h) |
| `VDP_setPlaneSize`, Window position, plane address | Fixed 320×224 board and readable overlays | [vdp.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp.h) |
| `TileMap.tilemap`, `VDP_loadTileSet`, font/tile uploads | Uncompressed atlas entries include flip attributes; add the reserved tile base | [vdp_tile.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_tile.h) |
| `DMA_queueDma`, queue drain | Retained field/Window rows uploaded in VBlank | [dma.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h) |
| `SPR_initEx`, `SPR_addSprite`, manual frames/position/depth | Garden player and four crate slots; check allocation failure | [sprite_eng.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h) |
| `PAL_setPalette` | Resident world, actor and UI banks loaded while display is disabled | [pal.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/pal.h) |
| `XGM2_play`, pause/resume, `XGM2_playPCMEx` | Original FM loop and finite UI/game cues on PCM CH3 | [xgm2.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/snd/xgm2.h) |
| `Z80_setForceDelayDMA(TRUE)` | Short DMA delay mitigation for XGM2; not a proof of clean real-hardware PCM | [z80_ctrl.h](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/z80_ctrl.h) |
| `IMAGE ... NONE ALL`, `SPRITE`, `XGM2`, `WAV` | Actual indexed PNG/VGM/WAV compilation | [ResComp manual](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt) |

The Window plane replaces BG_A where enabled. Transparent Window pixels reveal
BG_B, not BG_A. Garden scenery/board therefore live on BG_B; opaque panel tiles
cover them only where text/modals require it. Title uses its own complete frame.
No palette writes occur in the input/redraw loop.

## Costs, not universal limits

| Item | Garden | Blocks |
| --- | --- | --- |
| Logical cells | 108 | 180 |
| Core board/state | `sizeof(PzBoard)` = 750 bytes on M68000, including history | Fixed `BlGame`; measure target `sizeof`, not host padding |
| Undo history | 64 × 8 = 512 bytes | None in this real-time example |
| Retained Window map | 2240 bytes | 2240 bytes |
| Retained board map | 864 bytes | 360 bytes |
| Maximum map payload per redraw | 3104 bytes, 46 queued rows | 2600 bytes, 46 queued rows |
| Hardware sprites | Five 16-pixel-wide pieces, at most 80 pixels on one scanline | None; blocks are plane tiles |
| Sprite reservation | 48 tiles (22 live frame tiles) | None |
| Other user tiles | 42 scenery + 45 atlas + 1 blank + 8 border | 9 block + 8 border |
| Font | 96 tiles | 96 tiles |
| Palette banks | Three (0, 1, 3) | One (3) |
| Padded ROM | 128 KiB | 128 KiB |

These are example allocations, not hardware maxima or whole-system RAM totals.
Add SGDK driver/DMA/sprite bookkeeping, stack, audio state and other game modules
to a full memory report. Parsing stages a temporary 750-byte board on the stack.
Use `sgdk-memory` for lifetime/link-map accounting and `sgdk-performance` for
measured CPU/DMA headroom. Do not infer FPS from the logical cell count.

The Mega Drive has four 16-entry palette banks and 8×8 hardware tiles. H40 sprite
limits are 80 pieces overall, 20 pieces/320 sprite pixels per scanline, separate
from VRAM and DMA budgets. Here dense boards use plane tiles, not one sprite per
cell. Fixed logical capacity is not a hardware restriction on puzzle size.

Buffers remain resident until `SYS_doVBlankProcess` consumes their queued rows.
The loop drains pending work before editing them again. Both payloads fit the
Lab's 4096-byte configured transfer allowance; sprite frame/SAT traffic must be
included too. This small example redraws a whole retained board only on changes;
a larger game should dirty only affected rows/rectangles. Never reuse a queued
stack array or clear a live text plane midway through active display.

## Evidence limits

Host tests use C17 at O0/O3 with undefined-behavior checks where available, an
independent grid oracle, solution search and bounded random traces. Container
checks compile both Labs in release/debug plus PAL/NTSC variants and execute
shared C contracts on a **base M68000** model. They verify real compiled resource
geometry/palettes/font and compare ROM hashes across ARM64/AMD64 hosts.

CPU probes do not emulate VDP, joypad protocol, DMA scheduling or sound hardware.
ares playtests check actual display, input, audio and flow; the user's reviews
confirmed both Labs. Real-console PAL/NTSC input feel, display edges, audio/DMA
contention and flash-cart compatibility remain separate hardware checks.
