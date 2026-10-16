#ifndef COLLISION_CHECKS_H
#define COLLISION_CHECKS_H
#include "demo.h"
#define CHECK(c) do {if(!(c))return __LINE__;} while(0)
static int check_sweep(void)
{
    CcBox a={0,0,16,16},b={80,0,16,16};CcVec zero={0,0};
    CcHit h=cc_sweep(a,(CcVec){128,0},b,zero);
    CHECK(h.kind==CC_HIT && h.time==2048 && h.nx==-1 && !h.ny);
    CHECK(h.num==64 && h.den==128);
    h=cc_sweep(b,(CcVec){-128,0},a,zero);CHECK(h.kind==CC_HIT && h.nx==1 && h.time==2048);
    h=cc_sweep(a,(CcVec){32,0},b,(CcVec){-96,0});CHECK(h.kind==CC_HIT && h.time==2048);
    CHECK(cc_sweep(a,zero,b,zero).kind==CC_MISS);
    CHECK(cc_sweep(a,(CcVec){128,0},(CcBox){80,16,16,16},zero).kind==CC_MISS);
    CHECK(cc_sweep(a,(CcVec){64,0},b,zero).kind==CC_HIT); /* End-face contact. */
    CHECK(!cc_overlap(a,(CcBox){16,0,16,16}));
    CHECK(cc_sweep(a,(CcVec){1,0},(CcBox){16,0,16,16},zero).time==0);
    CHECK(cc_sweep(a,(CcVec){-1,0},(CcBox){16,0,16,16},zero).kind==CC_MISS);
    CHECK(cc_sweep(a,(CcVec){0,1},(CcBox){16,0,16,16},zero).kind==CC_MISS);
    CHECK(cc_sweep(a,zero,(CcBox){8,8,16,16},zero).kind==CC_OVERLAP);
    h=cc_sweep(a,(CcVec){128,128},(CcBox){80,80,16,16},zero);
    CHECK(h.kind==CC_HIT && h.nx==-1 && h.ny==-1 && h.time==2048);
    CHECK(cc_sweep(a,(CcVec){128,128},(CcBox){80,48,16,16},zero).kind==CC_MISS); /* Grazes one instant. */
    h=cc_sweep((CcBox){-16000,-16000,4096,4096},(CcVec){4096,4096},(CcBox){-7808,-7808,4096,4096},zero);
    CHECK(h.kind==CC_HIT && h.time==4096 && h.nx==-1 && h.ny==-1);
    return 0;
}
static int check_ray(void)
{
    CcBox b={80,32,32,32};CcHit h=cc_ray((CcVec){0,48},(CcVec){160,0},b);
    CHECK(h.kind==CC_HIT && h.time==2048 && h.nx==-1);
    CHECK(cc_ray((CcVec){0,32},(CcVec){160,0},b).kind==CC_MISS); /* Along top edge. */
    CHECK(cc_ray((CcVec){96,48},(CcVec){0,0},b).kind==CC_OVERLAP);
    CHECK(cc_ray((CcVec){80,48},(CcVec){1,0},b).kind==CC_HIT);
    CHECK(cc_ray((CcVec){80,48},(CcVec){-1,0},b).kind==CC_MISS);
    CHECK(cc_ray((CcVec){112,48},(CcVec){-32,0},b).nx==1);
    CHECK(cc_ray((CcVec){0,48},(CcVec){0,0},b).kind==CC_MISS);
    return 0;
}
static int check_cast_slide(void)
{
    CcBox walls[]={{80,-64,16,256},{-64,80,256,16},{1000,1000,16,16}};
    CcBox a={0,0,16,16};CcStats stats;
    CcHit h=cc_cast(a,(CcVec){128,96},walls,3,&stats);
    CHECK(h.kind==CC_HIT && h.index==0 && h.nx==-1 && !h.ny && stats.visited==3 && stats.narrow==2);
    CcMove m=cc_slide(a,(CcVec){128,96},walls,3);
    CHECK(m.kind==CC_HIT && m.contacts==2 && m.box.x==64 && m.box.y==64);
    CHECK(!m.remaining.x && !m.remaining.y && m.stats.visited<=CC_MAX_BOXES*CC_SLIDES);
    for(u16 i=0;i<3;i++)CHECK(!cc_overlap(m.box,walls[i]));
    m=cc_slide(a,(CcVec){128,32},walls,3);
    CHECK(m.kind==CC_HIT && m.box.x==64 && m.box.y==32 && m.contacts==1);
    m=cc_slide(a,(CcVec){128,128},walls,3);
    CHECK(m.kind==CC_HIT && m.box.x==64 && m.box.y==64 && m.contacts==1);
    CcBox swapped[]={walls[1],walls[0],walls[2]};
    CcMove reverse=cc_slide(a,(CcVec){128,128},swapped,3);
    CHECK(reverse.box.x==m.box.x && reverse.box.y==m.box.y);
    m=cc_slide((CcBox){64,0,16,16},(CcVec){0,48},walls,3);
    CHECK(m.box.x==64 && m.box.y==48 && m.kind==CC_MISS);
    m=cc_slide((CcBox){70,0,16,16},(CcVec){-32,0},walls,3);
    CHECK(m.kind==CC_OVERLAP && m.box.x==70 && m.remaining.x==-32);
    m=cc_slide(a,(CcVec){100,40},NULL,0);CHECK(m.box.x==100 && m.box.y==40);
    /* Exact rational choice: both display times round down to the same value. */
    CcBox thin[]={{2001,-16,1,64},{2000,-16,1,64}};
    h=cc_cast((CcBox){0,0,1,1},(CcVec){4096,0},thin,2,&stats);
    CHECK(h.index==1 && h.num==1999);
    CcBox full[CC_MAX_BOXES];
    for(u16 i=0;i<CC_MAX_BOXES;i++)full[i]=(CcBox){(s16)(1000+i*16),1000,16,16};
    h=cc_cast(a,(CcVec){128,0},full,CC_MAX_BOXES,&stats);
    CHECK(h.kind==CC_MISS && stats.visited==CC_MAX_BOXES && !stats.narrow);
    full[31]=walls[0];h=cc_cast(a,(CcVec){128,0},full,CC_MAX_BOXES,&stats);
    CHECK(h.kind==CC_HIT && h.index==31 && stats.visited==CC_MAX_BOXES && stats.narrow==1);
    return 0;
}
static int check_invalid(void)
{
    CcBox good={0,0,16,16};CcVec zero={0,0};CcStats stats;
    CHECK(cc_sweep(good,(CcVec){4097,0},good,zero).kind==CC_INVALID);
    CHECK(cc_sweep((CcBox){16380,0,16,16},(CcVec){16,0},good,zero).kind==CC_INVALID);
    CHECK(cc_sweep((CcBox){0,0,0,16},zero,good,zero).kind==CC_INVALID);
    CHECK(cc_sweep((CcBox){0,0,4097,16},zero,good,zero).kind==CC_INVALID);
    CHECK(cc_ray((CcVec){-16385,0},zero,good).kind==CC_INVALID);
    CHECK(cc_cast(good,zero,NULL,1,&stats).kind==CC_INVALID && !stats.visited);
    CHECK(cc_cast(good,zero,&good,33,&stats).kind==CC_INVALID);
    CcBox bad[]={{1000,1000,16,16},{16000,16000,-1,16}};
    CHECK(cc_cast(good,zero,bad,2,&stats).kind==CC_INVALID && !stats.visited);
    CHECK(cc_slide(good,zero,bad,2).kind==CC_INVALID);
    return 0;
}
static int check_demo(void)
{
    CcDemo a,b;cd_init(&a,0);cd_init(&b,0);
    for(u16 i=0;i<600;i++)cd_frame(&a,0,60,false);
    for(u16 i=0;i<500;i++)cd_frame(&b,0,50,false);
    CHECK(a.tick==b.tick && a.shots==b.shots && a.hits==b.hits && a.misses==b.misses && a.hits>0 && a.misses>0);
    u16 phase=b.phase,tick=b.tick;
    for(u16 i=0;i<100;i++)CHECK(!cd_frame(&b,CD_RIGHT,50,true));
    CHECK(b.phase==phase && b.tick==tick && !cd_frame(&b,0,0,false));
    cd_init(&a,2);
    for(u16 i=0;i<1000;i++) {
        cd_tick(&a,(i/50)%2 ? CD_RIGHT|CD_DOWN : CD_LEFT|CD_UP);
        CHECK(a.move.kind!=CC_OVERLAP && a.move.kind!=CC_INVALID && a.move.kind!=CC_EXHAUSTED);
        for(u16 j=0;j<8;j++)CHECK(!cc_overlap(a.player,cd_walls[j]));
    }
    return 0;
}
static u32 trace_digest(void)
{
    u32 state=19,digest=2166136261UL;
    for(u16 i=0;i<1000;i++) {
        state=state*1664525UL+1013904223UL;
        s16 x=(s16)((state>>16)%8000)-4000;
        state=state*1664525UL+1013904223UL;
        s16 y=(s16)((state>>16)%8000)-4000;
        CcBox a={x,y,128,192},b={-256,-128,512,384};
        CcVec delta={(s16)(-x),(s16)(-y)};
        CcHit h=cc_sweep(a,delta,b,(CcVec){0,0});
        digest=(digest^(u16)h.kind)*16777619UL;digest=(digest^h.time)*16777619UL;
        digest=(digest^(u16)h.nx)*16777619UL;digest=(digest^(u16)h.ny)*16777619UL;
        CcMove m=cc_slide(a,delta,&b,1);
        digest=(digest^(u16)m.box.x)*16777619UL;digest=(digest^(u16)m.box.y)*16777619UL;
    }
    return digest;
}
#endif
