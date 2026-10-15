#ifndef RASTER_LAB_MODEL_H
#define RASTER_LAB_MODEL_H
#ifdef __m68k__
#include <types.h>
#else
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
#define TRUE true
#define FALSE false
#endif

enum { RASTER_LINES = 224, RASTER_MIN_LINE = 64, RASTER_MAX_LINE = 192,
       RASTER_LEFT = 1, RASTER_RIGHT = 2, RASTER_UP = 4, RASTER_DOWN = 8,
       RASTER_TINT = 16, RASTER_WAVES = 32, RASTER_MUSIC = 64, RASTER_PAUSE = 128 };
typedef struct {
    u16 waterline, camera_x, phase, clock, previous;
    bool tint, waves, music, paused;
} RasterModel;

void raster_model_init(RasterModel *state);
bool raster_model_step(RasterModel *state, u16 held, u16 hz);
bool raster_scroll(const RasterModel *state, s16 *lines, u16 count);
bool raster_tint(const u16 *source, u16 *target, u16 count);
#endif
