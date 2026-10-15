#include "demo.h"
#ifdef __m68k__
#include <maths.h>
#endif

static s32 product(s16 left, s16 right)
{
#ifdef __m68k__
    /* Explicit 16x16 -> 32 MULS, rather than a general 32-bit helper. */
    return muls(left, right);
#else
    return (s32)left * right;
#endif
}

/* Every caller supplies [0, 718]. One subtraction replaces a general modulo. */
static u16 wrap_angle(u16 angle)
{
    return angle >= 360 ? angle - 360 : angle;
}

static u32 next_random(Demo *demo)
{
    u32 value = demo->random;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    demo->random = value;
    return value;
}

static void reset_star(Demo *demo, DemoStar *star, bool scatter_depth)
{
    u32 value = next_random(demo);
    star->x = (s16)(value & 255) - 128;
    star->y = (s16)((value >> 8) % 192) - 96;
    if (!star->x && !star->y) star->x = 1;
    star->depth = scatter_depth ? 32 + (u16)((value >> 16) % 224) : 255;
}

bool demo_project(s16 x, s16 y, u16 depth, const u16 perspective[256], DemoPoint *out)
{
    if (depth < 16 || depth > 255) return false;
    u16 scale = perspective[depth];
    if (scale > 1024) return false;
    /* Widen before multiplying. Divide signed values rather than shifting
     * them: C17 division explicitly truncates toward zero on every host. */
    s32 px = 160 + product(x, (s16)scale) / 256;
    s32 py = 120 + product(y, (s16)scale) / 256;
    if (px < 4 || px > 315 || py < 40 || py > 211) return false;
    out->x = (s16)px;
    out->y = (s16)py;
    return true;
}

s16 demo_lerp(s16 from, s16 to, u16 amount)
{
    if (amount > 256) amount = 256;
    return (s16)((s32)from + ((s32)to - from) * amount / 256);
}

u32 demo_distance_sq(s16 dx, s16 dy)
{
    /* Each signed product fits s32; add as u32 to cover (-32768,-32768). */
    return (u32)product(dx, dx) + (u32)product(dy, dy);
}

static s16 sine_at(const Demo *demo, u16 angle)
{
    return demo->sine[wrap_angle(angle)];
}

static void formation(Demo *demo)
{
    for (u16 i = 0; i < ORBIT_COUNT; i++)
    {
        u16 angle = wrap_angle(demo->phase + i * 360 / ORBIT_COUNT);
        s16 x = sine_at(demo, angle + 90);
        s16 y = sine_at(demo, demo->pattern == 1 ? angle * 2 : angle);
        s16 radius = demo->pattern == 2 ? 18 + i * 4 : 72;
        demo->orbit[i].x = 160 + product(x, radius) / 64;
        demo->orbit[i].y = 122 + product(y, demo->pattern == 2 ? radius * 3 / 4 : 58) / 64;
    }
}

void demo_init(Demo *demo, const s16 sine[360], const u16 perspective[256])
{
    *demo = (Demo){0};
    demo->sine = sine;
    demo->perspective = perspective;
    demo->random = 0x5347444B;
    demo->speed = 2;
    demo->metrics = true;
    for (u16 i = 0; i < STAR_COUNT; i++)
    {
        reset_star(demo, &demo->stars[i], true);
        if (!demo_project(demo->stars[i].x, demo->stars[i].y, demo->stars[i].depth,
                          perspective, &demo->stars[i].screen))
            demo->stars[i].screen = (DemoPoint){160, 120};
    }
    formation(demo);
}

void demo_step(Demo *demo)
{
    if (demo->paused) return;
    demo->ticks++;
    demo->phase = wrap_angle(demo->phase + demo->speed);
    for (u16 i = 0; i < STAR_COUNT; i++)
    {
        DemoStar *star = &demo->stars[i];
        if (star->depth <= 16 + demo->speed)
            reset_star(demo, star, false);
        else
            star->depth -= demo->speed;
        if (!demo_project(star->x, star->y, star->depth, demo->perspective, &star->screen))
        {
            reset_star(demo, star, false);
            demo_project(star->x, star->y, star->depth, demo->perspective, &star->screen);
        }
        star->shade = star->depth < 80 ? 2 : star->depth < 160 ? 1 : 0;
    }
    formation(demo);
}

void demo_frame(Demo *demo, u16 held, u16 hz)
{
    u16 pressed = held & ~demo->held;
    if (pressed & KEY_B)
    {
        const s16 *sine = demo->sine;
        const u16 *perspective = demo->perspective;
        demo_init(demo, sine, perspective);
    }
    demo->held = held;
    if (pressed & KEY_LEFT) demo->pattern = (demo->pattern + PATTERN_COUNT - 1) % PATTERN_COUNT;
    if (pressed & KEY_RIGHT) demo->pattern = (demo->pattern + 1) % PATTERN_COUNT;
    if ((pressed & KEY_UP) && demo->speed < 8) demo->speed++;
    if ((pressed & KEY_DOWN) && demo->speed > 1) demo->speed--;
    if (pressed & KEY_A) demo->metrics = !demo->metrics;
    if (pressed & KEY_START) demo->paused = !demo->paused;
    if (demo->paused) return;
    /* Nominal 60 updates/s on both displays. PAL occasionally performs two
     * steps. The clock does not try to hide missed hardware frames. */
    if (hz != 50 && hz != 60) return;
    demo->clock += 60;
    while (demo->clock >= hz)
    {
        demo->clock -= hz;
        demo_step(demo);
    }
}
