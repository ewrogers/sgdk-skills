#ifndef RASTER_LAB_WATER_FX_H
#define RASTER_LAB_WATER_FX_H
#ifdef __m68k__
#include <types.h>
#else
#include <stdbool.h>
#include <stdint.h>
typedef int16_t s16;
typedef uint16_t u16;
#endif

enum { WATER_BUBBLES = 4, WATER_SPLASH_TICKS = 24 };
typedef struct {
    s16 y;
    u16 age;
    bool active;
} WaterBubble;
typedef struct {
    WaterBubble bubbles[WATER_BUBBLES];
    u16 emit_clock, splash_left;
    bool underwater;
} WaterFx;

void water_fx_init(WaterFx *fx, s16 center_y, u16 waterline);
void water_fx_step(WaterFx *fx, s16 center_y, u16 waterline);
#endif
