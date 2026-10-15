---
name: sgdk-gameplay
description: Implement SGDK game logic with consistent timing, fixed-point movement, collision responses, camera following and reusable object pools. Use for runtime game rules and entity behavior rather than asset importing or low-level display effects.
---

# SGDK Gameplay

Help make a playable game loop on the Mega Drive. Preserve the project's
architecture; SGDK does not require a scene tree, physics engine or entity system.

Start with the existing update loop, target region, coordinate types and object
ownership. Identify one behavior to prove, such as a player stopping at a wall
or a bullet slot becoming reusable after a hit. Keep gameplay state testable
without the renderer where that helps.

## Choose the right reference

- [Timing and movement](references/timing-and-movement.md): simulation versus
  display frames, PAL/NTSC, input edges and SGDK fixed-point arithmetic.
- [Collision and pools](references/collision-and-pools.md): collision shapes,
  bounded motion checks, contact events, static slots and SGDK's pool API.
- [Cameras and budgets](references/cameras-and-budgets.md): world coordinates,
  map scrolling, offscreen behavior and performance tradeoffs.
- [Gameplay Lab](assets/gameplay-lab/README.md): a standalone scrolling target
  range with wall collision, a reusable bullet pool, pause and reset.

Preserve an existing toolchain pin. Use the tested image in the lab's build
instructions for a new project. Check unfamiliar calls against that image's
`inc/maths.h`, `inc/sys.h`, `inc/pool.h`, `inc/object.h`, `inc/map.h` and matching
`src` files before inventing a helper or assuming an API's behavior.

## Keep the hot path predictable

Choose a simulation cadence and keep it separate from rendering submission.
Snapshot input once per display frame, consume press/release edges once, and
make pause stop the relevant clocks. A second simulation step must not duplicate
one-shot input events or double-submit sprite DMA.

Use SGDK's numeric types deliberately. Convert to integer screen coordinates
at the rendering boundary; bound values before narrow casts, indexing or
arithmetic that could overflow. Avoid software floating point in hot paths.

Keep collision shapes separate from sprite canvas dimensions. Specify whether
edge contact counts, how fast movement avoids tunneling, and when objects are
removed. Use local tile queries or another useful broad phase before reaching
for all-pairs collision across a crowded scene.

Prefer loading-time allocation and reusable slots over repeated allocation in
the frame loop. Decide what full capacity means for that game: skip an optional
effect, refuse a shot, replace an old particle, or report a required allocation
failure. A game-object limit, SGDK sprite allocation and VDP scanline limits are
three different budgets.

## Prove the behavior

Test wall edges and corners, diagonal motion, fast projectiles, simultaneous
hits, pool exhaustion/reuse and pause/resume. Check the actual SDK adapter with
a release build as well as host-side tests. For regional timing, compare elapsed
simulation time, not just how many display frames were rendered.

Then play the ROM in an emulator or on hardware. Check camera boundaries, HUD
stability and crowded cases. Profile optimized builds with representative
workloads; a successful build or an empty scene does not establish frame-rate
headroom. Report what was compiled, executed in tests and actually playtested.
