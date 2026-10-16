#ifndef TEST_LAB_H
#define TEST_LAB_H

#ifdef __m68k__
#include <types.h>
#else
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;
#endif

enum { LAB_UP=1, LAB_DOWN=2, LAB_LEFT=4, LAB_RIGHT=8,
       LAB_B=16, LAB_C=32, LAB_A=64, LAB_START=128 };
enum { LAB_TICKS=720, LAB_RUNS=128, LAB_COINS=3, LAB_BOLTS=4 };

typedef struct { u16 ticks, held; } LabRun;
typedef struct { LabRun runs[LAB_RUNS]; u16 count, ticks; } LabTape;
typedef struct { s16 x, y; u16 life; } LabBolt;
typedef struct {
    u16 tick, active_ticks, rng, held, score, fired;
    s16 x, y;
    u8 paused;
    s16 coin_x[LAB_COINS], coin_y[LAB_COINS];
    LabBolt bolts[LAB_BOLTS];
} LabState;
typedef struct { u16 phase; } LabClock;

void lab_reset(LabState *state, u16 seed);
void lab_step(LabState *state, u16 held, bool movement_fault);
u32 lab_digest(const LabState *state);
/* Invalid rates do not consume phase or produce work. Nominal rates only. */
u16 lab_frame_ticks(LabClock *clock, u16 hz);
bool lab_record(LabTape *tape, u16 held);
bool lab_input(const LabRun *runs, u16 count, u16 tick, u16 *held);

#endif
