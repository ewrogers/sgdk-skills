#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "world.h"
#include "level_data.h"

static u8 bits[512];
static const WorldMap empty = {64, 64, 16, bits};

static void collision(void)
{
    World w;
    memset(bits, 0, sizeof bits);
    bits[(4 * 64 + 6) / 8] |= 1 << ((4 * 64 + 6) % 8);
    assert(world_solid(&empty, 6, 4) && !world_solid(&empty, 5, 4));
    assert(world_solid(&empty, -1, 4) && world_solid(&empty, 64, 0));
    assert(!world_blocked(&empty, 84 * WORLD_UNIT, 64 * WORLD_UNIT));
    assert(world_blocked(&empty, 84 * WORLD_UNIT + 1, 64 * WORLD_UNIT));
    assert(!world_blocked(&empty, 96 * WORLD_UNIT, 52 * WORLD_UNIT));
    assert(world_blocked(&empty, 96 * WORLD_UNIT, 52 * WORLD_UNIT + 1));
    assert(world_blocked(&empty, -1, 0) && world_blocked(&empty, 0, -1));
    assert(world_init(&w, &empty, 82, 64));
    for (int i = 0; i < 20; i++) world_tick(&w, WORLD_RIGHT);
    assert(w.x == 84 * WORLD_UNIT && w.y == 64 * WORLD_UNIT);
    for (int i = 0; i < 20; i++) world_tick(&w, WORLD_RIGHT | WORLD_DOWN);
    assert(w.y > 76 * WORLD_UNIT && w.x > 84 * WORLD_UNIT);
    assert(!world_blocked(&empty, w.x, w.y));
}

static void camera(void)
{
    World w;
    WorldMap tiny = {2, 2, 16, bits};
    memset(bits, 0, sizeof bits);
    assert(world_init(&w, &empty, 0, 0));
    assert(w.camera_x == 0 && w.camera_y == 0);
    assert(world_warp(&w, 1012, 1012));
    assert(w.camera_x == 704 && w.camera_y == 832);
    assert(!world_warp(&w, 1013, 1012));
    assert(w.x == 1012 * WORLD_UNIT && w.camera_x == 704);
    assert(!world_warp(&w, -1, 0));
    for (int i = 0; i < 10; i++) world_tick(&w, WORLD_RIGHT | WORLD_DOWN);
    assert(w.x == 1012 * WORLD_UNIT && w.y == 1012 * WORLD_UNIT);
    assert(world_init(&w, &tiny, 1, 1) && w.camera_x == 0 && w.camera_y == 0);
    assert(!world_init(NULL, &empty, 0, 0));
    assert(!world_init(&w, NULL, 0, 0));
    tiny.bits = NULL; assert(!world_init(&w, &tiny, 0, 0));
    tiny.bits = bits; tiny.cell = 0; assert(!world_init(&w, &tiny, 0, 0));
    tiny.cell = 16; tiny.columns = 3000; assert(!world_init(&w, &tiny, 0, 0));
    tiny.columns = 65535; tiny.rows = 65535; tiny.cell = 65535;
    assert(!world_init(&w, &tiny, 0, 0));
}

static void timing(void)
{
    World a, b;
    memset(bits, 0, sizeof bits);
    assert(world_init(&a, &empty, 64, 64) && world_init(&b, &empty, 64, 64));
    for (int i = 0; i < 120; i++)
        for (u16 n = world_steps(&a, 60, FALSE); n; n--) world_tick(&a, WORLD_RIGHT);
    for (int i = 0; i < 100; i++)
        for (u16 n = world_steps(&b, 50, FALSE); n; n--) world_tick(&b, WORLD_RIGHT);
    assert(a.x == b.x && a.x == 304 * WORLD_UNIT);
    for (int i = 0; i < 100; i++) assert(world_steps(&a, 60, TRUE) == 0);
    assert(a.x == 304 * WORLD_UNIT && a.clock_phase == 0);
    assert(world_steps(&a, 0, FALSE) == 0 && world_steps(&a, 55, FALSE) == 0);
    assert(world_warp(&a, 64, 64) && world_warp(&b, 64, 64));
    world_tick(&a, WORLD_RIGHT | WORLD_DOWN); world_tick(&b, WORLD_RIGHT);
    assert(a.x - 64 * WORLD_UNIT == 1448 && a.y - 64 * WORLD_UNIT == 1448);
    assert(b.x - 64 * WORLD_UNIT == 2048);
    fix32 x = a.x, y = a.y;
    world_tick(&a, WORLD_LEFT | WORLD_RIGHT | WORLD_UP | WORLD_DOWN);
    assert(a.x == x && a.y == y);
}

static void authored(void)
{
    World w;
    const s16 beacon[4][2] = {{64, 64}, {944, 64}, {944, 944}, {64, 944}};
    assert(level_collision.columns == 64 && level_collision.rows == 64 && level_collision.cell == 16);
    for (int i = 0; i < 4; i++) assert(world_init(&w, &level_collision, beacon[i][0], beacon[i][1]));
    for (s16 y = 0; y < 64; y++)
        for (s16 x = 0; x < 64; x++)
            if (x == 0 || x == 63 || y == 0 || y == 63) assert(world_solid(&level_collision, x, y));
    assert(world_init(&w, &level_collision, 64, 64));
    /* The first wall has a traversable gap; visual tile IDs are irrelevant. */
    assert(world_warp(&w, 16 * 16, 20 * 16));
    for (int i = 0; i < 80; i++) world_tick(&w, WORLD_RIGHT);
    assert(w.x > 23 * 16 * WORLD_UNIT);
    assert(world_warp(&w, 16 * 16, 10 * 16));
    for (int i = 0; i < 80; i++) world_tick(&w, WORLD_RIGHT);
    assert(w.x < 20 * 16 * WORLD_UNIT);
    for (unsigned i = 0; i < 20000; i++) {
        static const u16 moves[] = {WORLD_RIGHT, WORLD_DOWN, WORLD_LEFT, WORLD_UP,
                                   WORLD_UP | WORLD_RIGHT, WORLD_LEFT | WORLD_DOWN};
        world_tick(&w, moves[(i / 137) % 6]);
        assert(!world_blocked(w.map, w.x, w.y));
        assert(w.camera_x >= 0 && w.camera_x <= 704 && w.camera_y >= 0 && w.camera_y <= 832);
    }
}

static void animation(void)
{
    World w;
    memset(bits, 0, sizeof bits);
    assert(world_init(&w, &empty, 64, 64));
    for (int i = 0; i < 48; i++) world_tick(&w, 0);
    assert(!w.walking && w.facing == WORLD_FACE_DOWN && w.animation_ticks == 48);
    world_tick(&w, WORLD_RIGHT);
    assert(w.walking && w.facing == WORLD_FACE_RIGHT && w.animation_ticks == 0);
    for (int i = 0; i < 8; i++) world_tick(&w, WORLD_RIGHT);
    assert(w.animation_ticks == 8);
    for (int i = 0; i < 100; i++) assert(world_steps(&w, 60, TRUE) == 0);
    assert(w.animation_ticks == 8 && w.walking);
    world_tick(&w, 0);
    assert(!w.walking && w.facing == WORLD_FACE_RIGHT && w.animation_ticks == 0);
    world_tick(&w, WORLD_LEFT); assert(w.facing == WORLD_FACE_LEFT);
    world_tick(&w, WORLD_UP); assert(w.facing == WORLD_FACE_UP);
    world_tick(&w, WORLD_DOWN); assert(w.facing == WORLD_FACE_DOWN);
    for (int i = 0; i < 1000; i++) world_tick(&w, 0);
    assert(w.animation_ticks < 96);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (!strcmp(argv[1], "collision")) collision();
    else if (!strcmp(argv[1], "camera")) camera();
    else if (!strcmp(argv[1], "timing")) timing();
    else if (!strcmp(argv[1], "authored")) authored();
    else if (!strcmp(argv[1], "animation")) animation();
    else assert(0);
    puts(argv[1]);
    return 0;
}
