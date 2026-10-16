#include "lab.h"

static u16 next_random(LabState *s)
{
    /* Unsigned wrap is defined. This stream belongs only to game logic. */
    s->rng = (u16)((u32)s->rng * 25173u + 13849u);
    return s->rng;
}

static void place_coin(LabState *s, u16 i)
{
    s->coin_x[i] = (s16)(32 + next_random(s) % 112);
    s->coin_y[i] = (s16)(48 + next_random(s) % 120);
}

void lab_reset(LabState *s, u16 seed)
{
    s->tick=0; s->active_ticks=0; s->rng=seed; s->held=0;
    s->score=0; s->fired=0; s->x=24; s->y=96; s->paused=0;
    for (u16 i=0; i<LAB_COINS; i++) place_coin(s, i);
    s->coin_x[0]=80; s->coin_y[0]=96;
    for (u16 i=0; i<LAB_BOLTS; i++) {
        s->bolts[i].x=0; s->bolts[i].y=0; s->bolts[i].life=0;
    }
}

static s16 distance(s16 a, s16 b) { return a>b ? a-b : b-a; }

static bool walkable(s16 x, s16 y)
{
    return x>=8 && x<=312 && y>=40 && y<=180 &&
           !(x>139 && x<181 && y>75 && y<149);
}

void lab_step(LabState *s, u16 held, bool movement_fault)
{
    u16 pressed=held & (u16)~s->held;
    s->held=held;
    s->tick++;
    if (pressed & LAB_START) s->paused=!s->paused;
    if (s->paused) return;
    s->active_ticks++;
    s16 speed=movement_fault ? 3 : 2;
    s16 dx=((held & LAB_RIGHT)!=0)-((held & LAB_LEFT)!=0);
    s16 dy=((held & LAB_DOWN)!=0)-((held & LAB_UP)!=0);
    s16 x=s->x+dx*speed, y=s->y+dy*speed;
    if (walkable(x,s->y)) s->x=x;
    if (walkable(s->x,y)) s->y=y;
    if (pressed & LAB_A) {
        for (u16 i=0; i<LAB_BOLTS; i++) {
            LabBolt *b=&s->bolts[i];
            if (b->life) continue;
            b->x=s->x; b->y=s->y; b->life=80;
            s->fired++;
            break;
        }
    }
    for (u16 i=0; i<LAB_BOLTS; i++) {
        LabBolt *b=&s->bolts[i];
        if (!b->life) continue;
        b->x+=4;
        b->life--;
        if (b->x>316) b->life=0;
    }
    for (u16 i=0; i<LAB_COINS; i++) {
        bool hit=distance(s->x,s->coin_x[i])<10 && distance(s->y,s->coin_y[i])<10;
        for (u16 j=0; j<LAB_BOLTS; j++) {
            LabBolt *b=&s->bolts[j];
            if (b->life && distance(b->x,s->coin_x[i])<8 && distance(b->y,s->coin_y[i])<8) {
                b->life=0; hit=true;
            }
        }
        if (hit) { s->score++; place_coin(s,i); }
    }
}

static u32 word(u32 hash, u16 value)
{
    /* Explicit big-endian bytes, never struct padding or host byte order. */
    hash=(hash ^ (value>>8)) * 16777619u;
    return (hash ^ (value & 255)) * 16777619u;
}

u32 lab_digest(const LabState *s)
{
    u32 h=2166136261u;
    h=word(h,s->tick); h=word(h,s->active_ticks); h=word(h,s->rng);
    h=word(h,s->held); h=word(h,s->score); h=word(h,s->fired);
    h=word(h,(u16)s->x); h=word(h,(u16)s->y); h=word(h,s->paused);
    for (u16 i=0; i<LAB_COINS; i++) {
        h=word(h,(u16)s->coin_x[i]); h=word(h,(u16)s->coin_y[i]);
    }
    for (u16 i=0; i<LAB_BOLTS; i++) {
        h=word(h,(u16)s->bolts[i].x); h=word(h,(u16)s->bolts[i].y);
        h=word(h,s->bolts[i].life);
    }
    return h;
}

u16 lab_frame_ticks(LabClock *clock, u16 hz)
{
    if (hz!=50 && hz!=60) return 0;
    clock->phase+=60;
    u16 count=clock->phase/hz;
    clock->phase%=hz;
    return count;
}

bool lab_record(LabTape *tape, u16 held)
{
    if (tape->ticks>=LAB_TICKS || (held & (u16)~255u)) return false;
    if (tape->count && tape->runs[tape->count-1].held==held) {
        tape->runs[tape->count-1].ticks++;
    } else {
        if (tape->count>=LAB_RUNS) return false;
        tape->runs[tape->count].held=held;
        tape->runs[tape->count].ticks=1;
        tape->count++;
    }
    tape->ticks++;
    return true;
}

bool lab_input(const LabRun *runs, u16 count, u16 tick, u16 *held)
{
    if (!held || count>LAB_RUNS) return false;
    for (u16 i=0; i<count; i++) {
        if (!runs[i].ticks) return false;
        if (tick<runs[i].ticks) { *held=runs[i].held; return true; }
        tick-=runs[i].ticks;
    }
    return false; /* Do not reuse the last frame at end of tape. */
}
