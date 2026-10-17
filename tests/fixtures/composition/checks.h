#ifndef COURIER_CHECKS_H
#define COURIER_CHECKS_H
#include "model.h"
#define REQUIRE(c) do { if (!(c)) return __LINE__; } while (0)
static int courier_checks(void)
{
    Courier g, pal, ntsc;
    courier_init(&g,0);
    for (u16 i=0;i<100;i++) courier_frame(&g,C_RIGHT,60);
    REQUIRE(g.x==146 && !g.parcel && !g.room); /* Closed gate stops a fast held run. */
    courier_init(&g,0);
    for (u16 i=0;i<20;i++) courier_frame(&g,C_UP,60);
    for (u16 i=0;i<24;i++) courier_frame(&g,C_RIGHT,60);
    REQUIRE(g.parcel && g.x==80 && g.y==64);
    for (u16 i=0;i<20;i++) courier_frame(&g,C_DOWN,60);
    for (u16 i=0;i<116;i++) courier_frame(&g,C_RIGHT|C_A,60);
    REQUIRE(g.room==1 && !g.won && g.parcel && g.x==16);
    for (u16 i=0;i<24;i++) courier_frame(&g,C_DOWN|C_A,60);
    for (u16 i=0;i<128;i++) courier_frame(&g,C_RIGHT|C_A,60);
    REQUIRE(!g.won && g.x==272 && g.y==152); /* Held confirm cannot deliver. */
    courier_frame(&g,0,60); courier_frame(&g,C_A,60);
    REQUIRE(g.won && g.deliveries==1);
    for (u16 i=0;i<60;i++) courier_frame(&g,C_A,60);
    REQUIRE(g.deliveries==1);
    courier_init(&g,0);g.room=1;g.parcel=1;g.x=250;g.y=144;
    REQUIRE(!courier_can_deliver(&g));
    courier_frame(&g,C_A,60);REQUIRE(!g.won);
    courier_frame(&g,C_RIGHT,60);REQUIRE(courier_can_deliver(&g));
    courier_frame(&g,C_A,60);REQUIRE(g.won);
    courier_init(&g,0);g.phase=40;
    courier_frame(&g,C_START|C_RIGHT,50);
    u32 frozen=courier_digest(&g);
    for (u16 i=0;i<100;i++) courier_frame(&g,C_START|C_RIGHT,50);
    REQUIRE(g.paused && courier_digest(&g)==frozen && !g.phase);
    courier_frame(&g,0,50); courier_frame(&g,C_START,50);
    REQUIRE(!g.paused && g.ticks==1);
    /* Equal simulated time under 50/60 Hz schedules, including the PAL double step. */
    courier_init(&pal,0); courier_init(&ntsc,0);
    for (u16 second=0;second<8;second++) {
        u16 held=(second&1) ? C_RIGHT : C_UP;
        for (u16 i=0;i<50;i++) courier_frame(&pal,held,50);
        for (u16 i=0;i<60;i++) courier_frame(&ntsc,held,60);
        REQUIRE(courier_digest(&pal)==courier_digest(&ntsc));
    }
    REQUIRE(pal.ticks==480 && !pal.phase && !ntsc.phase);
    courier_init(&g,C_START);courier_frame(&g,C_START,50);
    REQUIRE(!g.paused); /* Entry seeds held input instead of inventing a press. */
    return 0;
}
static u32 courier_trace(void)
{
    Courier g;u32 seed=12345;
    courier_init(&g,0);
    for (u16 i=0;i<12000;i++) {
        seed=seed*1664525u+1013904223u;
        courier_frame(&g,(u16)((seed>>24)&159),50);
    }
    return courier_digest(&g);
}
#endif
