#include "raster_model.h"

static const s16 wave[32] = {0, 1, 2, 2, 3, 3, 4, 4, 4, 4, 4, 3, 3, 2, 2, 1,
                            0, -1, -2, -2, -3, -3, -4, -4, -4, -4, -4, -3, -3, -2, -2, -1};

void raster_model_init(RasterModel *s)
{
    *s = (RasterModel){.waterline = 128, .tint = TRUE, .waves = TRUE, .music = TRUE};
    water_fx_init(&s->fx, 60, s->waterline);
}

bool raster_model_step(RasterModel *s, u16 held, u16 hz)
{
    if (!s || (hz != 50 && hz != 60)) return FALSE;
    u16 pressed = held & (u16)~s->previous;
    s->previous = held;
    if (pressed & RASTER_TINT) s->tint = !s->tint;
    if (pressed & RASTER_WAVES) s->waves = !s->waves;
    if (pressed & RASTER_MUSIC) s->music = !s->music;
    if (pressed & RASTER_PAUSE) s->paused = !s->paused;
    if (s->paused) return TRUE;
    s->clock += 60;
    while (s->clock >= hz) {
        s->clock -= hz;
        s->phase = (s->phase + 1) & 255;
        s16 line = s->waterline + ((held & RASTER_DOWN) != 0) - ((held & RASTER_UP) != 0);
        s16 x = s->camera_x + 2 * (((held & RASTER_RIGHT) != 0) - ((held & RASTER_LEFT) != 0));
        if (line < RASTER_MIN_LINE) line = RASTER_MIN_LINE;
        if (line > RASTER_MAX_LINE) line = RASTER_MAX_LINE;
        if (x < 0) x = 0;
        if (x > 704) x = 704; /* Original 1024-pixel map minus 320-pixel view. */
        s->waterline = line;
        s->camera_x = x;
        u16 travel = s->phase < 128 ? s->phase : 255 - s->phase;
        water_fx_step(&s->fx, 60 + travel, s->waterline);
    }
    return TRUE;
}

bool raster_scroll(const RasterModel *s, s16 *lines, u16 count)
{
    if (!s || !lines || count != RASTER_LINES || s->waterline < RASTER_MIN_LINE ||
        s->waterline > RASTER_MAX_LINE || s->camera_x > 704) return FALSE;
    for (u16 y = 0; y < count; ++y) {
        s16 offset = -(s16)s->camera_x;
        if (s->waves && y >= s->waterline)
            offset += wave[((y - s->waterline) / 2 + s->phase / 2) & 31];
        lines[y] = offset;
    }
    return TRUE;
}

bool raster_tint(const u16 *source, u16 *target, u16 count)
{
    if (!source || !target || !count || count > 32) return FALSE;
    for (u16 i = 0; i < count; ++i) {
        u16 c = source[i];
        u16 r = ((c >> 1) & 7) / 2;
        u16 g = (((c >> 5) & 7) + 1) / 2;
        u16 b = ((c >> 9) & 7) / 2 + 3;
        /* A stylized underwater palette, not a physical light simulation.
           Keep black outlines/unused entries black, reducing CRAM writes. */
        target[i] = i % 16 && c ? (b << 9) | (g << 5) | (r << 1) : 0;
    }
    return TRUE;
}
