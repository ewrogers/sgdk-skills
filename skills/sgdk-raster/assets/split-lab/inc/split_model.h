#ifndef SPLIT_LAB_MODEL_H
#define SPLIT_LAB_MODEL_H
#ifdef __m68k__
#include <types.h>
#else
#include <stdint.h>
#include <stdbool.h>
typedef uint16_t u16;
typedef int16_t s16;
#define TRUE true
#define FALSE false
#endif

enum { SPLIT_WIDTH = 320, SPLIT_HEIGHT = 224, SPLIT_MIDDLE = 112,
       SPLIT_WORLD = 512, SPLIT_BODY = 16, SPLIT_VIEW_H = 92,
       SPLIT_TOP_Y = 16, SPLIT_BOTTOM_Y = 116,
       SPLIT_LEFT = 1, SPLIT_RIGHT = 2, SPLIT_UP = 4, SPLIT_DOWN = 8,
       SPLIT_CPU = 16, SPLIT_RESET = 32, SPLIT_PAUSE = 64 };
typedef struct { s16 x, y, camera_x, camera_y; } SplitActor;
typedef struct {
    SplitActor actor[2];
    u16 phase, clock, previous;
    bool cpu, paused;
} SplitModel;

void split_model_init(SplitModel *s);
bool split_model_step(SplitModel *s, u16 p1, u16 p2, u16 hz);
bool split_scroll(const SplitModel *s, s16 *lines, u16 count);
u16 split_bottom_scroll(const SplitModel *s);
u16 split_top_scroll(const SplitModel *s);
bool split_actor_position(const SplitModel *s, u16 view, u16 actor, s16 *x, s16 *y);
#endif
