# Collision and reusable objects

## Define the collision rules

SGDK draws sprites and supplies math/resource data; it does not choose your
physics responses. The lab uses simple boxes and a separate 32-pixel logical
collision grid. Its background still consists of ordinary 8x8 VDP tiles.
A collision cell need not be the same size as a graphics tile.

Use half-open boxes, `[x, x + width)`, when touching edges should not overlap.
Widen sums before adding widths to narrow coordinates, and reject empty shapes.
The visual canvas can contain transparent padding; collider bounds should be a
gameplay decision rather than whatever size ResComp exported.

Resolve player movement one axis at a time for simple wall sliding. The lab
checks at most one pixel of travel per candidate and keeps the last safe
position. Fractional positions are checked before conversion to render pixels;
a blocked step can leave a gap smaller than one pixel. This is not a slope,
one-way-platform or moving-platform solver.

## Prevent tunneling and repeated effects

Testing only the final position can skip a wall or target. Use a swept test or
bounded substeps justified by maximum speed and the smallest collidable feature.
The lab's four-pixel/tick bullets test four one-pixel steps. That is sufficient
for its stationary boxes and walls; it is not a universal guarantee for two
fast moving objects, teleports or thinner subpixel obstacles.

Pick a stable update order. The lab moves the player, advances existing bullets,
retires hits/expired bullets, then admits a new shot. A newly admitted shot first
moves on the next simulation tick. A hit disables its bullet and target before
later bullets inspect the same target, so a target scores once.

For enter/stay/exit behavior, track previous versus current contacts, usually
with compact IDs or bitsets for a small bounded group. Define what happens when
an object is removed while touching another. Keep event delivery and destruction
order explicit rather than depending on pointer reuse or renderer visibility.
Add layers/masks when they simplify real interaction categories; the small lab
only needs player-versus-walls and bullets-versus-targets/walls.

## Choose a pool that fits the project

A fixed array with an active flag is often enough for bullets or particles.
Its memory is easy to account for and a scan over a small capacity is cheap.
The lab reserves twelve `Shot` records and reports failed shot admission.
This number is an example capacity, not an SGDK or hardware limit.

SGDK also provides `Pool` and an optional callback-based `Object` layer. A pool
allocates its backing storage at creation, then reuses those slots:

```c
/* During loading, after validating count, record size and available RAM. */
Pool *bullets = POOL_create(32, sizeof(Bullet));
if (bullets == NULL) { /* Handle loading failure. */ }
else {
    Bullet *bullet = POOL_allocate(bullets);
    if (bullet != NULL) {
        /* Initialize every field before using a reused slot. */
        POOL_release(bullets, bullet, TRUE);
    }
    POOL_destroy(bullets);  /* After all users have stopped using the pool. */
}
```

`Bullet` is your own C struct. `POOL_allocate()` returns `NULL` when full.
Only release an object that is currently allocated from that exact pool, once.
The debug logs are not a general validation barrier against a double free.
With `maintainCoherency` enabled, release can reorder the allocation stack;
do not assume insertion order or remove entries during a forward iteration
without handling that change. `POOL_reset()` invalidates the previous allocation
state; do not retain those references as if they still identify live objects.

The generic pool needs its descriptor, word-aligned record storage, a two-byte
index per slot and an allocation pointer per slot, plus heap bookkeeping.
Check products in widened arithmetic before handing sizes to 16-bit allocation
APIs. A static array avoids that pool metadata but has its own capacity cost.
Neither approach is a requirement to build an entire entity framework.

## Separate game slots from rendering slots

The lab allocates its SGDK sprites during setup and hides unused ones. Resetting
the game releases logical bullet slots; it does not free SGDK sprite instances
or their reserved VRAM. That trades a small fixed cost for predictable reuse.

A bigger game may reserve fewer visible sprite instances than game objects,
share static frame tiles, or release rendering resources when changing levels.
Always handle `SPR_addSprite()` failure separately from a full gameplay pool.
After a slot is reused, a retained pointer or index can refer to a different
object. Use generation-tagged handles if references need to outlive a tick.

## Sources

- [SGDK 2.11 pool contract](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/pool.h)
- [Pool allocation, release order and storage](https://github.com/Stephane-D/SGDK/blob/v2.11/src/pool.c)
- [Optional object callbacks](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/object.h)
- [Memory allocator sizes](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/memory.h)
- [Sprite allocation API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h)
