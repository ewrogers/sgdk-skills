#include "workload.h"

#define NO_BODY 255

_Static_assert(PERF_CAPACITY < NO_BODY, "Grid links need a free sentinel value");
_Static_assert(PERF_BODY_SIZE <= PERF_CELL_SIZE, "A 3x3 neighbor search assumes small bodies");

bool perf_reset(PerfWorld *world, u16 count)
{
    u16 i;
    if (count > PERF_CAPACITY) return false;
    world->count = count;
    world->tick = 0;
    for (i = 0; i < PERF_CAPACITY; i++) {
        world->bodies[i].x = (s16)((i * 37U + 11) % (PERF_WIDTH - PERF_BODY_SIZE + 1));
        world->bodies[i].y = (s16)((i * 23U + 7) % (PERF_HEIGHT - PERF_BODY_SIZE + 1));
        world->bodies[i].dx = (i & 1) ? 2 : -2;
        world->bodies[i].dy = (i & 2) ? 1 : -1;
    }
    return true;
}

static void move_axis(s16 *position, s16 *speed, s16 maximum)
{
    s16 next = (s16)(*position + *speed);
    /* This workload uses speeds 1..2, smaller than either arena dimension. */
    if (next < 0) { next = (s16)-next; *speed = (s16)-*speed; }
    if (next > maximum) { next = (s16)(2 * maximum - next); *speed = (s16)-*speed; }
    *position = next;
}

void perf_move(PerfWorld *world)
{
    u16 i;
    for (i = 0; i < world->count; i++) {
        move_axis(&world->bodies[i].x, &world->bodies[i].dx, PERF_WIDTH - PERF_BODY_SIZE);
        move_axis(&world->bodies[i].y, &world->bodies[i].dy, PERF_HEIGHT - PERF_BODY_SIZE);
    }
    world->tick++;
}

bool perf_overlaps(const PerfBody *a, const PerfBody *b)
{
    return a->x < b->x + PERF_BODY_SIZE && b->x < a->x + PERF_BODY_SIZE &&
           a->y < b->y + PERF_BODY_SIZE && b->y < a->y + PERF_BODY_SIZE;
}

static void clear_pairs(u8 *pairs)
{
    u16 i;
    if (pairs) for (i = 0; i < PERF_PAIR_BYTES; i++) pairs[i] = 0;
}

static void inspect(const PerfWorld *world, u16 i, u16 j, PerfContacts *result, u8 *pairs)
{
    result->candidates++;
    if (perf_overlaps(&world->bodies[i], &world->bodies[j])) {
        u16 key = (u16)(i * PERF_CAPACITY + j);
        result->contacts++;
        result->signature += (u32)(key + 1) * 2654435761UL;
        if (pairs) pairs[key / 8] |= (u8)(1U << (key % 8));
    }
}

PerfContacts perf_all_pairs(const PerfWorld *world, u8 *pairs)
{
    PerfContacts result = {0, 0, 0};
    u16 i, j;
    clear_pairs(pairs);
    for (i = 0; i < world->count; i++)
        for (j = i + 1; j < world->count; j++) inspect(world, i, j, &result, pairs);
    return result;
}

PerfContacts perf_grid(const PerfWorld *world, u8 *pairs)
{
    u8 heads[PERF_GRID_COLUMNS * PERF_GRID_ROWS];
    u8 next[PERF_CAPACITY];
    PerfContacts result = {0, 0, 0};
    u16 i;
    clear_pairs(pairs);
    for (i = 0; i < PERF_GRID_COLUMNS * PERF_GRID_ROWS; i++) heads[i] = NO_BODY;
    /* A body is in exactly one cell, using its top-left corner. */
    for (i = 0; i < world->count; i++) {
        u16 cell = (u16)(world->bodies[i].y / PERF_CELL_SIZE * PERF_GRID_COLUMNS +
                         world->bodies[i].x / PERF_CELL_SIZE);
        next[i] = heads[cell]; heads[cell] = (u8)i;
    }
    for (i = 0; i < world->count; i++) {
        s16 cx = (s16)(world->bodies[i].x / PERF_CELL_SIZE);
        s16 cy = (s16)(world->bodies[i].y / PERF_CELL_SIZE);
        s16 x, y;
        for (y = cy - 1; y <= cy + 1; y++) {
            if (y < 0 || y >= PERF_GRID_ROWS) continue;
            for (x = cx - 1; x <= cx + 1; x++) {
                u8 j;
                if (x < 0 || x >= PERF_GRID_COLUMNS) continue;
                j = heads[y * PERF_GRID_COLUMNS + x];
                while (j != NO_BODY) {
                    if (j > i) inspect(world, i, j, &result, pairs);
                    j = next[j];
                }
            }
        }
    }
    return result;
}

u16 perf_frame(const PerfWorld *world, bool animated)
{
    return animated ? (u16)((world->tick / 6) & 1) : 0;
}

void perf_window_reset(PerfWindow *window)
{
    window->warmup = PERF_WARMUP;
    window->samples = window->peak_cpu = window->peak_dma = 0;
    window->cpu_sum = window->missed_refreshes = 0;
    window->frozen = false;
}

bool perf_window_push(PerfWindow *window, u16 cpu, u16 dma_bytes, u32 refreshes)
{
    if (window->frozen) return false;
    if (window->warmup) { window->warmup--; return false; }
    window->samples++;
    window->cpu_sum += cpu;
    if (cpu > window->peak_cpu) window->peak_cpu = cpu;
    if (dma_bytes > window->peak_dma) window->peak_dma = dma_bytes;
    if (refreshes > 1) window->missed_refreshes += refreshes - 1;
    if (window->samples == PERF_WINDOW) window->frozen = true;
    return true;
}

u16 perf_window_mean(const PerfWindow *window)
{
    return window->samples ? (u16)(window->cpu_sum / window->samples) : 0;
}
