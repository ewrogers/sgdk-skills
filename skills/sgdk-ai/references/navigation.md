# Navigation and budgets

## Use the smallest useful search

A platformer may need authored jump links; a flying enemy may only need steering.
A uniform four-neighbor grid is a useful starting point for a small top-down
arena, but it is not a complete navigation solution for every game.

Enemy Lab uses 18 by 9 cells, each 16 pixels square. Robots have 16-pixel artwork
and a centered 12-by-12 collision box. The two-pixel margin makes doorways less
fussy without changing the visible robot. Projectiles test the same body box.

A solid cell blocks movement and sight. Diagonal search edges are absent, so
routes cannot cut across a wall corner. Movement checks the body bounds and
slides along a blocked axis, while keeping the artwork inside the arena.
Entry doorways are two cells wide. A visible outer wall marks the arena edge;
alternating interior walls prevent reaching an exit by hugging the perimeter.

If adapting this to larger actors, build walkability for their footprint. A
large boss needs wider corridors than the player. For changing maps, include a
map revision in requests/results and invalidate routes when geometry changes.
The lab's three maps are immutable; a level load resets all navigation state.

## Keep work and storage bounded

Breadth-first search is appropriate here because every edge has the same cost.
One shared planner retains a 162-cell queue and 162 parent entries. A cell is
marked when enqueued, so it can enter the queue only once. The frontier survives
between frames; no recursion or heap allocation is needed for searching.

The scheduler expands at most 16 cells and starts at most one request per
displayed frame. Up to six enemies can have one pending request each. Dispatch
is round-robin rather than always starting at enemy zero. A worst-case search
can take 11 frames; six such requests can take about 66 frames before all finish.
Queue delay is a gameplay concern even when CPU work per frame is bounded.

Each enemy stores at most 16 forward waypoints. A longer route keeps its first
segment and requests another search as it progresses. Chase refreshes at cell
boundaries rather than discarding its frontier whenever the player moves.
Requests have a short retry interval to avoid a stationary enemy searching on
every tick. Tune these policies to the game's actual response needs.

The planner reuses its queue to reconstruct a completed route. If an enemy was
stunned between grid points, the route first anchors it to a nearby walkable
cell. This avoids a replan that assumes the enemy has already teleported to a
cell center.

## Treat results as potentially stale

A request records the owner, state serial, origin and level. Before continuing
it, verify those still match. A stun, external move, reset or new level must not
apply an old path. The lab's active planner is replaced on level load; its
serial/origin checks cover ordinary state changes and movement.

No path is a valid result. Clear the pending request, wait before retrying and
let game code choose whether to return home, wait or choose another destination.
Do not spin in a loop until a path appears. Dynamic obstacles, doors and moving
platforms require more policy than this static-grid lab provides.

## Account for the complete cost

The two search arrays use 648 bytes, and the six waypoint arrays use 192 bytes.
Those are model storage costs, not total game RAM: enemy/player state, SGDK's
sprite pool, DMA buffers, audio driver and stack need space too.

The renderer reserves 224 sprite tiles (7,168 VRAM bytes) and ten background/
HUD tiles. It allocates 44 logical sprite objects. The two 64-pixel pulse sprites
split into multiple hardware sprites; the worst-case reservation is 50 hardware
pieces. Actual visibility is lower, but per-scanline sprite/pixel limits still
apply, especially with debug markers enabled. A pool capacity is not a guarantee
that every object can be drawn on one scanline.

`NAV n/16` is the number of expanded cells in that displayed frame. It is not
elapsed time, total instructions or a guarantee of 60 FPS. Profile optimized
release builds with representative enemies, attacks, sound and DMA. Measure
total frame time and search wait time separately.

Sources: [SGDK sprite engine](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sprite_eng.c),
[VDP sprite API and limits](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/vdp_spr.h),
[DMA API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h).

## Preserve the SGDK build settings

The lab's makefile adds `-fno-strict-aliasing` to the existing flags. SGDK 2.11's
CPU transfer path reads buffers as both 16-bit and 32-bit words. With optimized
LTO and strict aliasing enabled, writes to a 16-bit palette/text buffer can be
removed before a wider read, losing custom colors or text.

This is a compatibility setting for those buffer-access assumptions. Keep it
when adapting the lab; do not mask the problem by making gameplay unoptimized.
Use `make release` through the container so the local makefile is read. The
container's `build` shortcut directly selects SGDK's makefile and bypasses this
local addition.

Sources: [CPU transfer implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/dma.c),
[text transfer buffers](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp_bg.c),
[GCC strict-aliasing option](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html#index-fstrict-aliasing).
