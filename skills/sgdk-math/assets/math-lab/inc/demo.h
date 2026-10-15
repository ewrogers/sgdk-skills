#ifndef MATH_DEMO_H
#define MATH_DEMO_H

#ifdef __m68k__
#include <types.h>
#else
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;
#endif

#define STAR_COUNT 48
#define ORBIT_COUNT 16
#define PATTERN_COUNT 3
enum { KEY_UP=1, KEY_DOWN=2, KEY_LEFT=4, KEY_RIGHT=8,
       KEY_A=64, KEY_B=16, KEY_START=128 };

typedef struct { s16 x, y; } DemoPoint;
typedef struct { s16 x, y; u16 depth; DemoPoint screen; u8 shade; } DemoStar;
typedef struct {
    DemoStar stars[STAR_COUNT];
    DemoPoint orbit[ORBIT_COUNT];
    /* Shared read-only tables owned by main, not copied per object. */
    const s16 *sine;
    const u16 *perspective;
    u32 random, ticks;
    u16 phase, held, clock;
    u8 speed, pattern;
    bool paused, metrics;
} Demo;

void demo_init(Demo *demo, const s16 sine[360], const u16 perspective[256]);
void demo_frame(Demo *demo, u16 held, u16 hz);
void demo_step(Demo *demo);
bool demo_project(s16 x, s16 y, u16 depth, const u16 perspective[256], DemoPoint *out);
s16 demo_lerp(s16 from, s16 to, u16 amount);
u32 demo_distance_sq(s16 dx, s16 dy);

#endif
