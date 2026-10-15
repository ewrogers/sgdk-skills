#ifndef GAMEPLAY_LAB_GAME_H
#define GAMEPLAY_LAB_GAME_H
#ifdef __m68k__
#include <types.h>
#else
#include <stdbool.h>
#include <stdint.h>
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t fix32;
#define TRUE true
#define FALSE false
#endif

enum { WORLD_W = 1024, WORLD_H = 224, VIEW_W = 320, UNIT = 1024,
       ACTOR_SIZE = 12, SHOT_SIZE = 4, SHOT_CAPACITY = 12, TARGET_COUNT = 6 };
enum { MOVE_LEFT = 1, MOVE_RIGHT = 2, MOVE_UP = 4, MOVE_DOWN = 8, FIRE = 16 };
typedef struct { s16 x, y; bool alive; } Target;
typedef struct { s16 x, y; s8 dx, dy; u16 life; bool active; } Shot;
typedef struct
{
    fix32 x, y; /* SGDK fix32 uses ten fractional bits; bounds keep sums safe. */
    s8 facing_x, facing_y;
    u8 cooldown;
    s16 camera_x;
    u16 hits, refused;
    Shot shots[SHOT_CAPACITY];
    Target targets[TARGET_COUNT];
} Game;
typedef struct { u16 phase; } Clock;

void game_reset(Game *game);
void game_tick(Game *game, u16 held);
bool game_try_fire(Game *game);
bool game_solid(s16 column, s16 row);
bool game_blocked(s16 x, s16 y, s16 size);
bool game_overlap(s16 ax, s16 ay, s16 aw, s16 ah,
                  s16 bx, s16 by, s16 bw, s16 bh);
u16 game_live_shots(const Game *game);
u16 clock_steps(Clock *clock, u16 refresh_hz, bool paused);
#endif
