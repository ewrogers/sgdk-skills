#ifndef STREAM_WORLD_H
#define STREAM_WORLD_H
#include "stream_cache.h"
u16 sw_camera(u16 player);
u16 sw_page(u16 world_column,u16 row);
u16 sw_columns(u16 camera);
u16 sw_wanted(u16 camera,u16 future,bool covered,u16 pages[SC_SLOTS]);
/* Ordinary movement is at most four pixels/frame. Teleports use a covered redraw. */
bool sw_entering(u16 before,u16 after,u16 *column);

enum { SW_FADE_IDLE, SW_FADE_OUT, SW_FADE_LOAD, SW_FADE_IN };
typedef struct { u16 phase; u8 state,level; } SwFade;
void sw_fade_begin(SwFade *fade);
void sw_fade_tick(SwFade *fade,u16 hz,bool loaded);
u16 sw_fade_color(u16 color,u16 level);
#endif
