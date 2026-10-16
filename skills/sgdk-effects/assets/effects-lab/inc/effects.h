#ifndef EFFECTS_H
#define EFFECTS_H

#ifdef __m68k__
#include <types.h>
#else
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int8_t s8;
#endif

enum { FX_CAPACITY=64, FX_WEATHER_MAX=32, FX_DRAW_MAX=64, FX_LINE_MAX=8, FX_WORLD_WIDTH=640,
       FX_TOP=40, FX_BOTTOM=192, FX_SCALE=16 };
enum { FX_SPARK, FX_SMOKE, FX_RAIN, FX_SNOW, FX_TRAIL, FX_KIND_COUNT };
enum { FX_UP=1, FX_DOWN=2, FX_LEFT=4, FX_RIGHT=8, FX_B=16, FX_C=32,
       FX_A=64, FX_START=128 };

/* Q4 positions/velocities; bounded well inside the signed 16-bit range. */
typedef struct {
    s16 x,y,vx,vy;
    u16 age,life;
    u8 kind,priority;
    u8 frame,burst;
} FxParticle;

typedef struct {
    FxParticle particles[FX_CAPACITY];
    u8 free_slots[FX_CAPACITY];
    u16 free_count;
    u32 ticks, rejected, replaced;
    u16 random, held, phase, occupied, priorities[3];
    s16 player_x,player_y;
    u8 weather,density,shake_left;
    bool paused,shake_enabled;
} FxLab;

typedef struct { s16 x,y; u16 slot; u8 frame; } FxDraw;
typedef struct {
    FxDraw items[FX_DRAW_MAX];
    s16 camera_x,camera_y;
    u16 count,visible,skipped;
    u8 bands[28];
} FxRender;

/* Diagnostic only: derive exact coverage from the selected rectangles. The
 * real-time admission path uses conservative eight-scanline bands. */
static inline u8 fx_line_count(const FxRender *r,u16 y)
{
    u8 count=0;
    for (u16 i=0;i<r->count;i++)
        if (y>=(u16)r->items[i].y && y<(u16)r->items[i].y+8) count++;
    return count;
}

void fx_init(FxLab *s,u16 seed);
bool fx_spawn(FxLab *s,u8 kind,s16 x,s16 y,s16 vx,s16 vy,u16 life);
void fx_burst(FxLab *s,s16 x,s16 y);
void fx_frame(FxLab *s,u16 held,u16 hz);
/* s and out must be separate, non-overlapping objects. */
void fx_select(const FxLab *restrict s,FxRender *restrict out,u16 budget);
u16 fx_active(const FxLab *s);
void fx_clear(FxLab *s);

#endif
