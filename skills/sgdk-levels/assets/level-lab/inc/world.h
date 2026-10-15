#ifndef LEVEL_LAB_WORLD_H
#define LEVEL_LAB_WORLD_H
#ifdef __m68k__
#include <types.h>
#else
#include <stdbool.h>
#include <stdint.h>
typedef int32_t s32;
typedef uint32_t u32;
typedef int16_t s16;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t fix32;
#define TRUE true
#define FALSE false
#endif

enum { WORLD_UNIT = 1024, WORLD_VIEW_W = 320, WORLD_VIEW_H = 192,
       WORLD_BODY = 12, WORLD_LEFT = 1, WORLD_RIGHT = 2,
       WORLD_UP = 4, WORLD_DOWN = 8 };
enum { WORLD_FACE_DOWN, WORLD_FACE_RIGHT, WORLD_FACE_UP, WORLD_FACE_LEFT };

typedef struct { u16 columns, rows, cell; const u8 *bits; } WorldMap;
typedef struct {
    const WorldMap *map;
    fix32 x, y;
    s16 camera_x, camera_y;
    u16 clock_phase;
    u16 animation_ticks;
    u8 facing;
    bool walking;
} World;

bool world_solid(const WorldMap *map, s16 column, s16 row);
bool world_blocked(const WorldMap *map, fix32 x, fix32 y);
bool world_init(World *world, const WorldMap *map, s16 x, s16 y);
bool world_warp(World *world, s16 x, s16 y);
void world_camera(World *world);
void world_tick(World *world, u16 held);
u16 world_steps(World *world, u16 hz, bool paused);

#endif
