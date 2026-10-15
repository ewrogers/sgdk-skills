#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "workload.h"

static void compare(const PerfWorld *world)
{
    u8 a[PERF_PAIR_BYTES + 2], b[PERF_PAIR_BYTES + 2];
    PerfContacts all, grid, unrecorded;
    memset(a, 0xA5, sizeof(a)); memset(b, 0x5A, sizeof(b));
    all = perf_all_pairs(world, a + 1);
    grid = perf_grid(world, b + 1);
    unrecorded = perf_grid(world, 0);
    assert(all.contacts == grid.contacts && all.signature == grid.signature);
    assert(all.candidates == world->count * (world->count - 1) / 2 || world->count == 0);
    assert(grid.candidates <= all.candidates);
    assert(unrecorded.contacts == grid.contacts && unrecorded.candidates == grid.candidates);
    assert(unrecorded.signature == grid.signature);
    assert(!memcmp(a + 1, b + 1, PERF_PAIR_BYTES));
    assert(a[0] == 0xA5 && a[PERF_PAIR_BYTES + 1] == 0xA5);
    assert(b[0] == 0x5A && b[PERF_PAIR_BYTES + 1] == 0x5A);
}

static void reset_case(void)
{
    PerfWorld a, b;
    u16 i;
    assert(perf_reset(&a, 32) && perf_reset(&b, 32));
    assert(a.tick == 0 && a.count == 32);
    for (i = 0; i < PERF_CAPACITY; i++) {
        assert(a.bodies[i].x == b.bodies[i].x && a.bodies[i].y == b.bodies[i].y);
        assert(a.bodies[i].dx == b.bodies[i].dx && a.bodies[i].dy == b.bodies[i].dy);
    }
    b = a;
    assert(!perf_reset(&a, PERF_CAPACITY + 1));
    assert(!memcmp(&a, &b, sizeof(a)));
    assert(perf_reset(&a, 0)); compare(&a);
    assert(perf_reset(&a, PERF_CAPACITY)); compare(&a);
}

static void motion_case(void)
{
    PerfWorld world;
    unsigned tick, i;
    perf_reset(&world, PERF_CAPACITY);
    world.bodies[0] = (PerfBody){0, 0, -2, -1};
    world.bodies[1] = (PerfBody){PERF_WIDTH - PERF_BODY_SIZE, PERF_HEIGHT - PERF_BODY_SIZE, 2, 1};
    perf_move(&world);
    assert(world.bodies[0].x == 2 && world.bodies[0].dx == 2);
    assert(world.bodies[0].y == 1 && world.bodies[0].dy == 1);
    assert(world.bodies[1].x == PERF_WIDTH - PERF_BODY_SIZE - 2 && world.bodies[1].dx == -2);
    for (tick = 0; tick < 2000; tick++) {
        perf_move(&world);
        for (i = 0; i < world.count; i++) {
            assert(world.bodies[i].x >= 0 && world.bodies[i].x <= PERF_WIDTH - PERF_BODY_SIZE);
            assert(world.bodies[i].y >= 0 && world.bodies[i].y <= PERF_HEIGHT - PERF_BODY_SIZE);
        }
    }
    assert(world.tick == 2001);
}

static void edges_case(void)
{
    PerfWorld world;
    PerfBody a = {0, 0, 0, 0}, b = {8, 0, 0, 0};
    assert(!perf_overlaps(&a, &b));
    b.x = 7; assert(perf_overlaps(&a, &b));
    b.y = 8; assert(!perf_overlaps(&a, &b));
    perf_reset(&world, 2);
    /* All cell borders, all relative offsets near the eight-pixel shape. */
    for (s16 y = 0; y <= PERF_HEIGHT - PERF_BODY_SIZE; y += 16)
        for (s16 x = 0; x <= PERF_WIDTH - PERF_BODY_SIZE; x += 16)
            for (s16 offset = -9; offset <= 9; offset++) {
                world.bodies[0].x = x; world.bodies[0].y = y;
                world.bodies[1].x = x + offset; world.bodies[1].y = y + offset;
                if (world.bodies[1].x < 0 || world.bodies[1].y < 0 ||
                    world.bodies[1].x > PERF_WIDTH - PERF_BODY_SIZE ||
                    world.bodies[1].y > PERF_HEIGHT - PERF_BODY_SIZE) continue;
                compare(&world);
            }
}

static void equivalence_case(void)
{
    PerfWorld world;
    for (u16 count = 0; count <= PERF_CAPACITY; count += 8) {
        perf_reset(&world, count);
        for (u16 tick = 0; tick < 500; tick++) { compare(&world); perf_move(&world); }
    }
}

static void dense_case(void)
{
    PerfWorld world;
    PerfContacts contacts;
    perf_reset(&world, PERF_CAPACITY);
    contacts = perf_grid(&world, 0);
    assert(contacts.candidates < PERF_CAPACITY * (PERF_CAPACITY - 1) / 4);
    for (u16 i = 0; i < world.count; i++) { world.bodies[i].x = 16; world.bodies[i].y = 16; }
    compare(&world);
    contacts = perf_grid(&world, 0);
    assert(contacts.contacts == PERF_CAPACITY * (PERF_CAPACITY - 1) / 2);
    assert(contacts.candidates == contacts.contacts);
}

static void animation_case(void)
{
    PerfWorld world;
    perf_reset(&world, 8);
    for (u32 tick = 0; tick < 30; tick++) {
        world.tick = tick;
        assert(perf_frame(&world, false) == 0);
        assert(perf_frame(&world, true) == ((tick / 6) & 1));
    }
}

static void window_case(void)
{
    PerfWindow window, frozen;
    perf_window_reset(&window);
    assert(perf_window_mean(&window) == 0);
    for (u16 i = 0; i < PERF_WARMUP; i++)
        assert(!perf_window_push(&window, 999, 65000, 5));
    assert(window.samples == 0 && window.missed_refreshes == 0 && window.peak_cpu == 0);
    for (u16 i = 0; i < PERF_WINDOW; i++)
        assert(perf_window_push(&window, (u16)(20 + i), (u16)(512 + i), i == 3 ? 4 : 1));
    assert(window.samples == PERF_WINDOW && window.peak_cpu == 139 && window.frozen);
    assert(window.peak_dma == 631 && window.missed_refreshes == 3);
    assert(perf_window_mean(&window) == 79);
    frozen = window;
    assert(!perf_window_push(&window, 65535, 65535, 0xFFFFFFFFUL));
    assert(!memcmp(&window, &frozen, sizeof(window)));
    perf_window_reset(&window);
    assert(window.samples == 0 && window.cpu_sum == 0 && window.warmup == PERF_WARMUP);
    window.frozen = true;
    assert(!perf_window_push(&window, 99, 99, 9));
    assert(window.samples == 0 && window.warmup == PERF_WARMUP);
}

static void repeat_case(void)
{
    PerfWorld a, b;
    perf_reset(&a, PERF_CAPACITY); perf_reset(&b, PERF_CAPACITY);
    for (unsigned tick = 0; tick < 500; tick++) {
        PerfContacts all = perf_all_pairs(&a, 0), grid = perf_grid(&b, 0);
        assert(all.contacts == grid.contacts && all.signature == grid.signature);
        perf_move(&a); perf_move(&b);
        for (unsigned i = 0; i < a.count; i++) {
            assert(a.bodies[i].x == b.bodies[i].x && a.bodies[i].y == b.bodies[i].y);
            assert(a.bodies[i].dx == b.bodies[i].dx && a.bodies[i].dy == b.bodies[i].dy);
        }
    }
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (!strcmp(argv[1], "reset")) reset_case();
    else if (!strcmp(argv[1], "motion")) motion_case();
    else if (!strcmp(argv[1], "edges")) edges_case();
    else if (!strcmp(argv[1], "equivalence")) equivalence_case();
    else if (!strcmp(argv[1], "dense")) dense_case();
    else if (!strcmp(argv[1], "animation")) animation_case();
    else if (!strcmp(argv[1], "window")) window_case();
    else if (!strcmp(argv[1], "repeat")) repeat_case();
    else abort();
    puts(argv[1]); return 0;
}
