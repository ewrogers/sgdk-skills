#include "demo.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static s16 sine[360];
static u16 perspective[256];

static void tables(void)
{
    for (unsigned i = 0; i < 360; i++)
        sine[i] = (s16)(sin(i * 3.14159265358979323846 / 180.0) * 64);
    for (unsigned i = 1; i < 256; i++) perspective[i] = 16384 / i;
}

static void projection(void)
{
    DemoPoint point = {123, 45};
    assert(!demo_project(0, 0, 0, perspective, &point));
    assert(!demo_project(0, 0, 256, perspective, &point));
    assert(point.x == 123 && point.y == 45);
    for (u16 z = 16; z <= 255; z++)
        for (s16 x = -128; x <= 127; x += 7)
            for (s16 y = -96; y <= 95; y += 9)
            {
                bool valid = demo_project(x, y, z, perspective, &point);
                double expected_x = 160.0 + (double)x * 64 / z;
                double expected_y = 120.0 + (double)y * 64 / z;
                if (valid)
                {
                    assert(fabs(point.x - expected_x) < 1.5);
                    assert(fabs(point.y - expected_y) < 1.5);
                    assert(point.x >= 4 && point.x <= 315 && point.y >= 40 && point.y <= 211);
                }
                else
                    assert(expected_x < 5.5 || expected_x > 313.5 || expected_y < 41.5 || expected_y > 209.5);
            }
}

static void helpers(void)
{
    assert(demo_distance_sq(-32768, -32768) == 2147483648UL);
    assert(demo_distance_sq(3, -4) == 25);
    assert(demo_distance_sq(0, 0) == 0);
    assert(demo_lerp(-32768, 32767, 0) == -32768);
    assert(demo_lerp(-32768, 32767, 256) == 32767);
    assert(demo_lerp(-32768, 32767, 65535) == 32767);
    for (u16 amount = 0; amount <= 256; amount++)
    {
        s16 value = demo_lerp(-100, 300, amount);
        assert(value >= -100 && value <= 300);
        assert(fabs(value - (-100.0 + 400.0 * amount / 256)) < 1);
        assert(demo_lerp(300, -100, amount) >= -100);
    }
}

static void patterns(void)
{
    Demo demo;
    demo_init(&demo, sine, perspective);
    demo.speed = 1;
    for (unsigned pattern = 0; pattern < PATTERN_COUNT; pattern++)
    {
        demo.pattern = pattern;
        for (unsigned tick = 0; tick < 360; tick++)
        {
            demo_step(&demo);
            for (unsigned i = 0; i < ORBIT_COUNT; i++)
            {
                unsigned degrees = (demo.phase + i * 360 / ORBIT_COUNT) % 360;
                double angle = degrees * 3.14159265358979323846 / 180;
                int radius = pattern == 2 ? 18 + i * 4 : 72;
                double x = 160 + cos(angle) * radius;
                double y = 122 + sin(angle * (pattern == 1 ? 2 : 1)) * (pattern == 2 ? radius * 3 / 4 : 58);
                assert(fabs(demo.orbit[i].x - x) < 2.5);
                assert(fabs(demo.orbit[i].y - y) < 2.5);
            }
        }
        assert(demo.phase == 0);
    }
}

static void clock_test(void)
{
    Demo ntsc, pal;
    demo_init(&ntsc, sine, perspective);
    demo_init(&pal, sine, perspective);
    for (int i = 0; i < 3600; i++) demo_frame(&ntsc, 0, 60);
    for (int i = 0; i < 3000; i++) demo_frame(&pal, 0, 50);
    assert(ntsc.ticks == 3600 && pal.ticks == 3600);
    assert(ntsc.random == pal.random && ntsc.phase == pal.phase);
    assert(!memcmp(ntsc.stars, pal.stars, sizeof(ntsc.stars)));
    assert(!memcmp(ntsc.orbit, pal.orbit, sizeof(ntsc.orbit)));
    demo_frame(&ntsc, KEY_START, 60);
    Demo frozen = ntsc;
    for (int i = 0; i < 600; i++) demo_frame(&ntsc, 0, 60);
    assert(ntsc.ticks == frozen.ticks && ntsc.clock == frozen.clock);
    assert(!memcmp(ntsc.stars, frozen.stars, sizeof(ntsc.stars)));
    demo_frame(&ntsc, KEY_START, 60);
    assert(!ntsc.paused && ntsc.ticks == frozen.ticks + 1);
}

static void input(void)
{
    Demo demo;
    demo_init(&demo, sine, perspective);
    demo_frame(&demo, KEY_LEFT, 60);
    assert(demo.pattern == 2);
    demo_frame(&demo, KEY_LEFT, 60);
    assert(demo.pattern == 2);
    demo_frame(&demo, 0, 60);
    demo_frame(&demo, KEY_RIGHT | KEY_A, 60);
    assert(demo.pattern == 0 && !demo.metrics);
    for (int i = 0; i < 30; i++)
    {
        demo_frame(&demo, 0, 60);
        demo_frame(&demo, KEY_UP, 60);
    }
    assert(demo.speed == 8);
    for (int i = 0; i < 30; i++)
    {
        demo_frame(&demo, 0, 60);
        demo_frame(&demo, KEY_DOWN, 60);
    }
    assert(demo.speed == 1);
    demo_frame(&demo, KEY_B, 60);
    assert(demo.speed == 2 && demo.pattern == 0 && demo.metrics && demo.ticks == 1);
    demo_frame(&demo, KEY_B, 60);
    assert(demo.ticks == 2);
}

static void soak(void)
{
    Demo demo;
    demo_init(&demo, sine, perspective);
    demo.speed = 8;
    for (int frame = 0; frame < 100000; frame++)
    {
        demo_step(&demo);
        assert(demo.random && demo.phase < 360);
        for (int i = 0; i < STAR_COUNT; i++)
        {
            DemoStar *star = &demo.stars[i];
            assert(star->depth >= 16 && star->depth <= 255);
            assert(star->screen.x >= 4 && star->screen.x <= 315);
            assert(star->screen.y >= 40 && star->screen.y <= 211);
            assert(star->shade <= 2);
        }
    }
    Demo reset;
    demo_init(&reset, sine, perspective);
    demo_init(&demo, sine, perspective);
    assert(!memcmp(&reset, &demo, sizeof(demo)));
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    tables();
    if (!strcmp(argv[1], "projection")) projection();
    else if (!strcmp(argv[1], "helpers")) helpers();
    else if (!strcmp(argv[1], "patterns")) patterns();
    else if (!strcmp(argv[1], "clock")) clock_test();
    else if (!strcmp(argv[1], "input")) input();
    else if (!strcmp(argv[1], "soak")) soak();
    else return 1;
    puts(argv[1]);
    return 0;
}
