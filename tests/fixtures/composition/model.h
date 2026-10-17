#ifndef COURIER_MODEL_H
#define COURIER_MODEL_H
#ifdef COURIER_HOST
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
#else
#include <genesis.h>
#endif
enum { C_UP=1, C_DOWN=2, C_LEFT=4, C_RIGHT=8, C_A=16, C_START=128 };
enum { C_PICKUP=1, C_TRAVEL=2, C_DELIVER=4 };
typedef struct {
    s16 x, y;
    u16 previous, phase, cue;
    u8 room, parcel, won, paused, events;
    u32 ticks, deliveries;
} Courier;
void courier_init(Courier *g, u16 held);
u8 courier_solid(const Courier *g, s16 x, s16 y);
u8 courier_can_deliver(const Courier *g);
void courier_frame(Courier *g, u16 held, u16 hz);
u32 courier_digest(const Courier *g);
#endif
