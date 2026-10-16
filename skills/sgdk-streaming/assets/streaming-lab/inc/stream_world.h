#ifndef STREAM_WORLD_H
#define STREAM_WORLD_H
#include "stream_cache.h"
u16 sw_camera(u16 player);
u16 sw_page(u16 world_column,u16 row);
u16 sw_columns(u16 camera);
u16 sw_wanted(u16 camera,u16 future,bool covered,u16 pages[SC_SLOTS]);
/* Ordinary movement is at most four pixels/frame. Teleports use a covered redraw. */
bool sw_entering(u16 before,u16 after,u16 *column);
#endif
