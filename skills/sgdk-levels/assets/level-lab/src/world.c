#include "world.h"

bool world_solid(const WorldMap *map, s16 column, s16 row)
{
    u16 index;
    if (column < 0 || row < 0 || column >= map->columns || row >= map->rows) return TRUE;
    /* world_init bounds the cell count so this offset fits u16. */
    index = (u16)(row * map->columns + column);
    return (map->bits[index / 8] & (1 << (index % 8))) != 0;
}

bool world_blocked(const WorldMap *map, fix32 x, fix32 y)
{
    s32 cell = (s32)map->cell * WORLD_UNIT;
    if (x < 0 || y < 0 || x > ((s32)map->columns * map->cell - WORLD_BODY) * WORLD_UNIT ||
        y > ((s32)map->rows * map->cell - WORLD_BODY) * WORLD_UNIT) return TRUE;
    for (s16 row = (s16)(y / cell); row <= (y + WORLD_BODY * WORLD_UNIT - 1) / cell; row++)
        for (s16 col = (s16)(x / cell); col <= (x + WORLD_BODY * WORLD_UNIT - 1) / cell; col++)
            if (world_solid(map, col, row)) return TRUE;
    return FALSE;
}

void world_camera(World *world)
{
    s32 x = world->x / WORLD_UNIT + WORLD_BODY / 2 - WORLD_VIEW_W / 2;
    s32 y = world->y / WORLD_UNIT + WORLD_BODY / 2 - WORLD_VIEW_H / 2;
    s32 max_x = (s32)world->map->columns * world->map->cell - WORLD_VIEW_W;
    s32 max_y = (s32)world->map->rows * world->map->cell - WORLD_VIEW_H;
    if (max_x < 0) max_x = 0;
    if (max_y < 0) max_y = 0;
    world->camera_x = (s16)(x < 0 ? 0 : x > max_x ? max_x : x);
    world->camera_y = (s16)(y < 0 ? 0 : y > max_y ? max_y : y);
}

bool world_warp(World *world, s16 x, s16 y)
{
    fix32 next_x = (s32)x * WORLD_UNIT, next_y = (s32)y * WORLD_UNIT;
    if (world_blocked(world->map, next_x, next_y)) return FALSE;
    world->x = next_x;
    world->y = next_y;
    world->walking = FALSE;
    world->animation_ticks = 0;
    world_camera(world);
    return TRUE;
}

bool world_init(World *world, const WorldMap *map, s16 x, s16 y)
{
    /* These are this lab's bounded-coordinate choices, not SGDK limits. */
    if (!world || !map || !map->bits || !map->cell || !map->columns || !map->rows ||
        (u32)map->columns * map->rows > 65535 ||
        (u32)map->columns * map->cell > 32767 || (u32)map->rows * map->cell > 32767)
        return FALSE;
    *world = (World){0};
    world->map = map;
    return world_warp(world, x, y);
}

static void move_axis(World *world, fix32 dx, fix32 dy)
{
    fix32 remaining = dx ? dx : dy;
    bool horizontal = dx != 0;
    while (remaining) {
        fix32 step = remaining > WORLD_UNIT ? WORLD_UNIT :
                     remaining < -WORLD_UNIT ? -WORLD_UNIT : remaining;
        fix32 x = world->x + (horizontal ? step : 0);
        fix32 y = world->y + (horizontal ? 0 : step);
        if (world_blocked(world->map, x, y)) break;
        world->x = x;
        world->y = y;
        remaining -= step;
    }
}

void world_tick(World *world, u16 held)
{
    fix32 old_x = world->x, old_y = world->y;
    u8 old_facing = world->facing;
    bool was_walking = world->walking;
    s16 dx = ((held & WORLD_RIGHT) != 0) - ((held & WORLD_LEFT) != 0);
    s16 dy = ((held & WORLD_DOWN) != 0) - ((held & WORLD_UP) != 0);
    fix32 speed = dx && dy ? 1448 : 2 * WORLD_UNIT;
    move_axis(world, dx * speed, 0);
    move_axis(world, 0, dy * speed);
    if (dx) world->facing = dx > 0 ? WORLD_FACE_RIGHT : WORLD_FACE_LEFT;
    else if (dy) world->facing = dy > 0 ? WORLD_FACE_DOWN : WORLD_FACE_UP;
    world->walking = world->x != old_x || world->y != old_y;
    if (was_walking != world->walking || old_facing != world->facing)
        world->animation_ticks = 0;
    else world->animation_ticks = (world->animation_ticks + 1) % (world->walking ? 32 : 96);
    world_camera(world);
}

u16 world_steps(World *world, u16 hz, bool paused)
{
    u16 count;
    if (paused || (hz != 50 && hz != 60)) { world->clock_phase = 0; return 0; }
    world->clock_phase += 60;
    count = world->clock_phase / hz;
    world->clock_phase %= hz;
    return count;
}
