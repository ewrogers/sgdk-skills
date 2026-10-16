---
name: sgdk-effects
description: Build bounded SGDK particles, explosions, trails, weather, hit feedback and screen shake. Use for cosmetic effects, shared animation tiles, sprite/scanline admission and graceful degradation, rather than gameplay collision or general graphics setup alone.
---

# SGDK Effects

Make effects readable and affordable without changing the game's rules. Follow
the project's art direction and comfort settings; the Lab's lantern, colors and
weather are examples, not a required visual style.

## Establish ownership and cost

Find the existing renderer, camera, pools, palettes and frame loop first.
Decide which effects are world-space, screen-space or attached to an emitter.
Separate cosmetic particles from damage, collision and scoring. A missed spark
must not change whether an attack hits. Use a cosmetic RNG stream rather than
advancing the gameplay RNG when quality settings change.

Budget simulation work, RAM, resident tiles, queued transfers, whole-frame
hardware sprites and per-scanline sprite counts/pixels separately. One SGDK
Sprite can contain several hardware pieces. Transparent pixels still occupy
the hardware rectangle. Reusing tiles saves VRAM/uploads, not scanline capacity.

- [Simulation and emission](references/simulation.md): lifetime, fixed steps,
  trails, bounded admission, priorities and cleanup.
- [Rendering and budgets](references/rendering.md): shared frame caches,
  SGDK adapters, exact scanline admission, weather and target profiling.
- [Camera and feedback](references/feedback.md): shake, hit flashes, palette
  ownership, reduced effects and tests.
- [Effects Lab](assets/effects-lab/README.md): a standalone executable example
  with original native art, particles, smoke, trails, rain/snow and shake.

## Bound the expensive paths

Choose a pool size and emission rate that fit the measured workload. On
exhaustion, drop low-priority decoration or replace an eligible older particle;
keep the gameplay event independent. Cap bursts, catch-up steps and any sorting
or scanline work. Offscreen culling must have an explicit lifetime policy.

Prefer shared resident frames for repeated small effects. Verify actual linked
frame geometry before assuming one particle equals one hardware sprite. Use
SGDK's Sprite engine for general objects; a measured batch of identical small
pieces may justify a renderer using the lower-level VDP sprite-table API.
Assign one owner to the table: do not let `SPR_update()` overwrite another
renderer’s entries.

## Prove the busy case

Test full pools, overlapping bursts, long-lived weather, concentrated scanlines,
camera edges, pause, cleanup and PAL/NTSC elapsed time. Compare an optimization
with a simple reference and keep cosmetic quality changes out of gameplay
replays. Label active/rendered particle counts separately from FPS.

Pull `ewrogers/sgdk:latest` and verify the selected SDK's API/source before
writing adapters. These examples were checked with SGDK 2.11 and GCC 16.2.0.
Host tests and executed M68000 probes establish contracts and resource data;
emulator/hardware tests establish presentation and frame-time headroom.

Other skills are optional: `sgdk-graphics` covers general rendering,
`sgdk-gameplay` gameplay pools, `sgdk-performance` broader profiling and
`sgdk-raster` scanline display effects. This skill installs independently.
