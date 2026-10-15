#ifndef PERFORMANCE_LAB_WORKLOAD_H
#define PERFORMANCE_LAB_WORKLOAD_H

#ifdef __m68k__
#include <types.h>
#else
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef uint32_t u32;
#endif

#define PERF_CAPACITY 64
#define PERF_BODY_SIZE 8
#define PERF_WIDTH 320
#define PERF_HEIGHT 120
#define PERF_CELL_SIZE 16
#define PERF_GRID_COLUMNS (PERF_WIDTH / PERF_CELL_SIZE)
#define PERF_GRID_ROWS ((PERF_HEIGHT + PERF_CELL_SIZE - 1) / PERF_CELL_SIZE)
#define PERF_PAIR_BYTES (PERF_CAPACITY * PERF_CAPACITY / 8)
#define PERF_WARMUP 16
#define PERF_WINDOW 120

typedef struct {
    s16 x, y, dx, dy;
} PerfBody;

typedef struct {
    PerfBody bodies[PERF_CAPACITY];
    u16 count;
    u32 tick;
} PerfWorld;

typedef struct {
    u16 candidates;
    u16 contacts;
    u32 signature;
} PerfContacts;

typedef struct {
    u16 warmup, samples, peak_cpu, peak_dma;
    u32 cpu_sum, missed_refreshes;
    bool frozen;
} PerfWindow;

/* Invalid capacities leave the world alone. Zero is a valid idle baseline. */
bool perf_reset(PerfWorld *world, u16 count);
void perf_move(PerfWorld *world);
bool perf_overlaps(const PerfBody *a, const PerfBody *b);
/* Optional pair bitmap is for exact host comparisons, not allocated by the ROM. */
PerfContacts perf_all_pairs(const PerfWorld *world, u8 *pairs);
PerfContacts perf_grid(const PerfWorld *world, u8 *pairs);
u16 perf_frame(const PerfWorld *world, bool animated);
void perf_window_reset(PerfWindow *window);
bool perf_window_push(PerfWindow *window, u16 cpu, u16 dma_bytes, u32 refreshes);
u16 perf_window_mean(const PerfWindow *window);

#endif
