# Sliding, candidates and contact policy

## Static wall sliding

A useful character response is to keep the tangential part of a requested move:

1. Find the earliest contact among nearby static walls.
2. Advance to it using the exact fraction of the current displacement.
3. Remove displacement components directed into the contacted axes.
4. Query again with the remaining displacement, within a fixed budget.

The Lab returns `CcMove`: the resulting box, status, contact count, remaining
unapplied displacement and query work counts. `CC_MISS` means it moved freely;
`CC_HIT` means a component was blocked. A diagonal equal-time corner blocks both
axes. Touching a vertical wall does not stop motion along it. Contact metadata
uses stable list order, but coalescing equal-time axes avoids depending on which
corner wall happened to be first.

```c
CcMove move = cc_slide(player, desired_delta, nearby_walls, wall_count);
if (move.kind == CC_INVALID || move.kind == CC_OVERLAP) {
    /* Keep or restore a previously validated position; log the bad spawn/data. */
} else {
    player = move.box;
    /* A caller must also handle CC_EXHAUSTED if adapting the solver. */
}
```

The budget is three list queries: at most 96 candidate visits and 96 exact slab
tests, with broad-phase rejection usually reducing the latter. Two independent
blocked axes consume the displacement in this simple axis-aligned solver;
`CC_EXHAUSTED` is a defensive partial-result status if the budget is consumed
with motion remaining. It is not permission to apply the rest unchecked.
The example validates the supplied list on every query as well; candidate counts
do not include those cheap validation passes. It performs no allocation.

Initial overlap is a separate problem. Do not run an unlimited sequence of
minimum translations until things look clear. Prefer a safe spawn/checkpoint,
restore the last valid position, or define a bounded depenetration policy with
a failure result. Fast moving walls, crushing, carrying, rotating shapes, slopes
and one-way platforms require additional rules; this solver does not implement
them. Relative-motion sweeps alone do not make static sliding support platforms.

## Select candidates without losing collisions

Collision does not depend on whether art is on screen. Use world or rebased local
coordinates, including the entire requested motion, and keep simulation bodies
alive independently of sprite visibility and VRAM availability.

For a small room, a fixed list is often enough. The Lab scans eight walls. Its
`cc_cast` first validates the list, then tests each wall against the swept union
of the body's old and new bounds before doing slab divisions. The returned
`visited` and `narrow` counters show list work and exact tests respectively;
these are not cycle estimates. A distant invalid wall still rejects the request.

For a tilemap, enumerate the **swept bounding rectangle**, not only the starting
or ending cells. Convert minimum and maximum raw coordinates with a floor
division that handles negative values; C signed division truncates toward zero.
Decide whether the maximum face includes the neighboring cell at an exact edge
so the chosen time-one contact policy is preserved. Clip to valid map bounds,
handle outside-map solidity explicitly and cap the number of cells visited.
A long segment may be better served by a bounded DDA grid walk, but the Lab does
not provide one. Corner ties and a maximum crossing count need their own tests.

Do not silently truncate a candidate list. If there are more than 32 solids,
reject or conservatively stop, split a sweep into carefully bounded chunks, or
implement a streaming minimum-hit query that visits a bounded map region without
collecting every box. Segmenting must preserve earliest contact and time remaining.
The example returns `CC_INVALID` for an oversized list; it does not resize it.

Category membership/masks can skip pairs that cannot interact. Keep triggers
separate from physical blockers: they may need all contacts, while a blocking
query needs the nearest one. Stable IDs/generations should outlive deferred hit
events; do not retain a pointer to a reused object-pool slot without validation.

## Preserve timing and correctness

A logical tick should snapshot old positions, build proposed motion, query,
resolve and then publish positions. Use the same order for actors and collision
candidates. Sprite updates and camera offsets happen afterward. If a game chooses
a different ordering for simultaneous dynamic collisions, document it rather
than pretending a pairwise sweep solves all interactions.

Keep continuous collision selective. A short slow move in a solid tile world
may be cheaper with the existing tile solver; rays and swept bullets are useful
where their guarantees matter. Measure release target code with representative
candidate density. Unrelated DMA, art uploads or debug overlays can hide collision
cost, and more broad-phase bookkeeping is not automatically faster for eight walls.
