#include "model.h"
#include <string.h>
void courier_init(Courier *g, u16 held)
{
    memset(g, 0, sizeof(*g));
    g->x=32; g->y=104; g->previous=held;
}
u8 courier_solid(const Courier *g, s16 x, s16 y)
{
    if (y<32 || y>=192 || x<0 || x>=320) return 1;
    if (x>=152 && x<168) {
        s16 opening=g->room ? 136 : 88;
        if (y<opening || y>=opening+32) return 1;
        if (!g->room && !g->parcel) return 1;
    }
    return 0;
}
static u8 fits(const Courier *g, s16 x, s16 y)
{
    return !(courier_solid(g,x-5,y-5) || courier_solid(g,x+5,y-5) ||
             courier_solid(g,x-5,y+5) || courier_solid(g,x+5,y+5));
}
u8 courier_can_deliver(const Courier *g)
{
    return g->room && g->parcel && !g->won && g->x>=252 && g->x<=292 && g->y>=132 && g->y<=172;
}
static void tick(Courier *g, u16 held, u16 pressed)
{
    s16 dx=0,dy=0;
    g->ticks++;
    if (g->cue) g->cue--;
    /* Axis priority is an explicit test-game choice, not a genre requirement. */
    if (held&C_LEFT) dx=-2; else if (held&C_RIGHT) dx=2;
    else if (held&C_UP) dy=-2; else if (held&C_DOWN) dy=2;
    if (!g->won) {
        if (fits(g,g->x+dx,g->y)) g->x+=dx;
        if (fits(g,g->x,g->y+dy)) g->y+=dy;
        if (!g->room && !g->parcel && g->x>=70 && g->x<=90 && g->y>=54 && g->y<=74) {
            g->parcel=1; g->events|=C_PICKUP; g->cue=12;
        }
        if (!g->room && g->parcel && g->x>=312 && g->y>=94 && g->y<=114) {
            g->room=1; g->x=16; g->events|=C_TRAVEL;
        }
        if (courier_can_deliver(g) && (pressed&C_A)) {
            g->won=1; g->deliveries++; g->cue=24; g->events|=C_DELIVER;
        }
    }
}
void courier_frame(Courier *g, u16 held, u16 hz)
{
    u16 pressed=held & (u16)~g->previous;
    g->previous=held; g->events=0;
    if (hz!=50 && hz!=60) return;
    if (pressed&C_START) { g->paused^=1; g->phase=0; }
    if (g->paused) return;
    g->phase+=60;
    while (g->phase>=hz) {
        g->phase-=hz;
        tick(g,held,pressed);
        pressed=0;
    }
}
u32 courier_digest(const Courier *g)
{
    u32 h=2166136261u;
#define MIX(v) h=(h^(u32)(v))*16777619u
    MIX(g->x); MIX(g->y); MIX(g->room); MIX(g->parcel); MIX(g->won);
    MIX(g->paused); MIX(g->ticks); MIX(g->deliveries); MIX(g->cue);
#undef MIX
    return h;
}
