#ifndef ADAPTATION_REVIEW_CHECKS_H
#define ADAPTATION_REVIEW_CHECKS_H
#include "desk.h"
#define REQUIRE(c) do { if (!(c)) return __LINE__; } while (0)
static unsigned long review_trace;

/* Reviewer-owned oracle, derived from the drawn X/Y interlocks, not desk_mask. */
static int review_checks(void) {
    static const unsigned char routes[3][4] = {
        {1, 1, 3, 3}, {3, 3, 3, 3}, {3, 3, 2, 2}
    };
    Desk d, pal, ntsc;
    unsigned int origin, bay, other, destination, i, second, frame;
    unsigned short tick, phase, remaining;
    unsigned long waited;
    review_trace = 0;
    for (origin = 0; origin < 3; ++origin) {
        for (bay = 0; bay < 4; ++bay) {
            REQUIRE(desk_mask(origin, bay) == routes[origin][bay]);
            for (other = 0; other < 3; ++other) {
                for (destination = 0; destination < 4; ++destination) {
                    desk_reset(&d);
                    d.service[0].state = TRANSIT;
                    d.service[0].origin = origin;
                    d.service[0].bay = bay;
                    d.service[0].remaining = 1;
                    d.service[1].state = WAITING;
                    d.service[1].origin = other;
                    d.selected = 1;
                    d.bay = destination;
                    desk_input(&d, K_C);
                    if (routes[origin][bay] & routes[other][destination]) {
                        REQUIRE(d.service[1].state == WAITING && d.sound == 2);
                        REQUIRE(d.selected == 1 && d.bay == destination);
                    } else {
                        REQUIRE(d.service[1].state == TRANSIT && d.sound == 1);
                    }
                    desk_tick(&d);
                    REQUIRE(d.service[0].state == DELIVERED);
                }
            }
        }
    }
    desk_reset(&d);
    desk_input(&d, K_B);
    REQUIRE(d.bay == NONE && d.selected == 0);
    desk_input(&d, K_C);
    REQUIRE(d.sound == 2 && d.service[0].state == WAITING);
    desk_input(&d, K_RIGHT);
    desk_input(&d, K_C);
    REQUIRE(d.service[0].state == TRANSIT);
    desk_frame(&d, 50); /* Keep a fractional phase through the hold. */
    desk_input(&d, K_A);
    tick = d.tick; phase = d.phase; remaining = d.service[0].remaining;
    waited = d.wait_ticks;
    for (i = 0; i < 600; ++i) desk_frame(&d, 50);
    REQUIRE(d.tick == tick && d.phase == phase && d.wait_ticks == waited);
    REQUIRE(d.service[0].remaining == remaining);
    desk_input(&d, K_START);
    desk_input(&d, K_A | K_C); /* Help must swallow desk commands. */
    REQUIRE(d.help && d.held && d.tick == tick);
    desk_input(&d, K_START);
    REQUIRE(!d.help && d.held);
    desk_input(&d, K_A);
    desk_input(&d, K_START);
    for (i = 0; i < 600; ++i) desk_frame(&d, 60);
    REQUIRE(d.tick == tick && !d.held && d.help);
    desk_input(&d, K_START);
    REQUIRE(!d.help && !d.held);

    desk_reset(&pal); desk_reset(&ntsc);
    for (second = 0; second < 90; ++second) {
        unsigned short action = second % 3 == 0 ? K_C : K_RIGHT;
        desk_input(&pal, action); desk_input(&ntsc, action);
        for (frame = 0; frame < 50; ++frame) desk_frame(&pal, 50);
        for (frame = 0; frame < 60; ++frame) desk_frame(&ntsc, 60);
        REQUIRE(pal.tick == ntsc.tick && pal.phase == ntsc.phase);
        REQUIRE(pal.wait_ticks == ntsc.wait_ticks);
        REQUIRE(pal.selected == ntsc.selected && pal.bay == ntsc.bay);
        REQUIRE(pal.finished == (second == 89));
        REQUIRE(ntsc.finished == (second == 89));
        for (i = 0; i < SERVICES; ++i) {
            REQUIRE(pal.service[i].state == ntsc.service[i].state);
            REQUIRE(pal.service[i].remaining == ntsc.service[i].remaining);
            review_trace = (review_trace * 33 + pal.service[i].state * 7 +
                            pal.service[i].remaining) & 0xFFFFFFFFUL;
        }
        REQUIRE(desk_count(&pal, WAITING) <= 3);
    }
    desk_reset(&pal);
    REQUIRE(pal.tick == 0 && pal.phase == 0 && pal.wait_ticks == 0);
    REQUIRE(!pal.finished && !pal.help && !pal.held);
    REQUIRE(desk_count(&pal, WAITING) == 1 && pal.selected == 0);
    return 0;
}
#undef REQUIRE
#endif
