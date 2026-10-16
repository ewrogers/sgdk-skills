# Depth sorting and occlusion

## Choose the simplest order the content allows

Foot Y or `x + y` can work for small actors and props standing on one flat floor.
A large building can span several such depths, and an elevated deck can cover a
character below while remaining behind a character standing on it. Subtracting
height from one scalar key does not solve those relationships in general.

Lantern Aqueduct uses three-dimensional axis-aligned **ordering bounds**. These
are conservative render proxies, not necessarily the collision boxes. It tests
whether their projected hexagons overlap before inferring a far-to-near edge.
This avoids inventing contradictory relationships from screen rectangles that
overlap only in empty corners.

For the Lab projection, the separating intervals are `x-y`, `x-z` and `y-z`.
Each interval uses its extrema across a box. If any pair of intervals is disjoint
or only touches, no pixels with area need an ordering edge. Otherwise, a box
entirely below/behind another on X, Y or Z can draw first. Reversing that separation
reverses the edge. If the actual boxes interpenetrate, the routine reports
`IL_AMBIGUOUS`; it does not invent an invisible physical separation.

The test oracle independently projects all eight corners, builds a convex hull
and tests its polygon edge normals. This checks the interval shortcut against
a different geometric implementation over 6,000 deterministic pairs.

## An ordering graph is not an unlimited scene graph

`IlItem` contains a box and stable ID. `il_sort()` accepts up to 24 items and 24
explicit links and returns indices in **far-to-near** order. Those capacities are
example work budgets, not Mega Drive or SGDK object limits. Choose and measure
an appropriate budget for another game.

Kahn's topological sort selects the lowest stable ID among eligible items.
Thus unrelated objects have deterministic order even if the input array changes.
Links refer to IDs and explicitly replace inferred order for that pair. They
are useful for a known supporting stair surface, where an actor overlaps a
coarse stepped-volume proxy while standing on it.

| Result | Meaning |
| --- | --- |
| `IL_SORT_OK` | All constraints were ordered |
| `IL_SORT_AMBIGUOUS` | At least one overlapping pair needs better bounds or an explicit rule |
| `IL_SORT_CYCLE` | No complete order satisfies the constraints |
| `IL_SORT_INVALID` | Bad bounds, capacity, pointer, duplicate ID or invalid link |

Ambiguous and cyclic cases still return a bounded deterministic fallback order,
with a diagnostic. Invalid input leaves the output untouched. Do not discard
that status and claim the sort is correct. Reauthor intersecting geometry, split
an object, or supply a justified support rule. Cycles between large objects can
require splitting even when each pair looks reasonable in isolation.
A logical Sprite's hardware pieces remain a group; ResComp cutting is not this
kind of logical splitting.

## Cache what does not move

`il_cache_init()` copies immutable static items and precomputes their pairwise
edges. It accepts only an unambiguous, acyclic static set. Its owned copy avoids
a dangling pointer to a temporary authoring array. The 436-byte cache in the
base-M68000 build is an explicit RAM-for-CPU tradeoff.

`il_sort_cached()` appends dynamic items to that fixed prefix. It reuses static
edges and computes only pairs involving a dynamic item. Output indices still
refer to the combined fixed-then-dynamic sequence. Stable IDs must be unique
across both groups. Rebuild the cache when a building's bounds, height or identity
changes; editing the original input does not change the cached copy.

The Lab caches 15 scenery items and updates one hero, reducing inferred pair
tests from 120 to 15 per displayed movement state. The bounded topological pass
still visits the combined graph; this is not a constant-time sorter. Rendering
uses the final logical state once per displayed frame. A larger game should also
select/cull an appropriate visible ordering set before sorting and uploading.

## Separate flat art from occluders

The ground diamonds and island foundation are baked into BG_B. The decks, support
columns, stairs, towers, trees, fountain and hero remain independently sortable
sprites. A whole bridge baked into the ground image could not hide the hero
beneath it correctly. Conversely, making every flat ground tile a Sprite would
waste hardware pieces and scanline bandwidth.

A cutaway hides the deck/stair drawings while retaining their gameplay surfaces.
The optional foot marker intentionally draws on top as debug information; it is
not an example of world occlusion. Asset anchors and proxy bounds must cover the
visible shape, including roofs or canopies extending beyond a tiny collision
footprint. When one proxy becomes too conservative to order cleanly, use smaller
logical pieces rather than a magic Z offset.
