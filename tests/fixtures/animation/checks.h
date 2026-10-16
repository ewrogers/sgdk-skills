#ifndef ANIMATION_CHECKS_H
#define ANIMATION_CHECKS_H
#include "action.h"
#define CHECK(c) do {if(!(c))return __LINE__;} while(0)

static int check_clock(void)
{
    for(u16 clip=0;clip<AL_CLIPS;clip++)for(u16 f=0;f<al_clips[clip].count;f++) {
        const AlFrame *frame=&al_clips[clip].frames[f];
        CHECK(frame->ticks && frame->pose<17 && frame->hit.w>=0 && frame->hit.h>=0);
    }
    AlClock c;AlFrame shown;al_play(&c,AL_ATTACK);
    for(u16 t=0;t<26;t++) {
        u16 e=al_clock_tick(&c,&shown);
        CHECK((e&AL_SWING)==(t==8 ? AL_SWING : 0));
        CHECK((e&AL_END)==(t==25 ? AL_END : 0));
        CHECK((shown.hit.w!=0)==(t>=8 && t<14));
        CHECK(!c.done || t==25);
    }
    CHECK(c.done && c.age==26 && shown.pose==10);
    for(u16 i=0;i<60;i++)CHECK(!al_clock_tick(&c,&shown) && shown.pose==10);
    CHECK(al_play(&c,AL_ATTACK) && c.age==0 && c.frame==0 && !c.done);
    CHECK(!al_play(&c,AL_CLIPS) && c.clip==AL_ATTACK && c.age==0);
    al_play(&c,AL_WALK);u16 steps=0;
    for(u16 i=0;i<48;i++){u16 e=al_clock_tick(&c,&shown);CHECK(!(e&AL_END));steps+=(e&AL_FOOT)!=0;}
    CHECK(steps==4 && c.frame==0 && !c.age);
    return 0;
}

static int check_actions(void)
{
    AlGame g;al_init(&g);g.actor.x=190;u16 swings=0,ends=0;
    for(u16 t=0;t<60;t++) {
        u16 e=al_tick(&g.actor,AL_A,t ? 0 : AL_A);
        swings+=(e&AL_SWING)!=0;ends+=(e&AL_END)!=0;
    }
    CHECK(g.actor.attacks==1 && g.actor.hits==1 && swings==1 && ends==1);
    /* Exact edge contact is not overlap; mirror the whole local box. */
    CHECK(!al_overlap((AlBox){202,152,12,26},al_dummy()));
    al_init(&g);g.actor.x=250;g.actor.left=true;
    for(u16 t=0;t<9;t++)al_tick(&g.actor,0,t ? 0 : AL_A);
    CHECK(g.actor.hits==1 && g.actor.hit.x==212 && g.actor.hit.w==28);
    al_init(&g);g.actor.x=190;g.actor.left=true;
    for(u16 t=0;t<14;t++)al_tick(&g.actor,0,t ? 0 : AL_A);
    CHECK(!g.actor.hits);
    /* Cancel from recovery. A buffered request just before the window survives. */
    al_init(&g);
    for(u16 t=0;t<19;t++)al_tick(&g.actor,0,(t==0 || t==15) ? AL_A : 0);
    CHECK(g.actor.attacks==2 && g.actor.clock.age==1 && g.actor.pose==6);
    al_init(&g);
    for(u16 t=0;t<19;t++)al_tick(&g.actor,0,(t==0 || t==14) ? AL_A : 0);
    CHECK(g.actor.attacks==1); /* Four-tick input buffer expired before cancel. */
    al_init(&g);
    for(u16 t=0;t<200;t++)al_tick(&g.actor,AL_RIGHT,0);
    CHECK(g.actor.x==208 && !al_overlap(al_body(&g.actor),al_dummy()));
    g.actor.x=250;
    for(u16 t=0;t<200;t++)al_tick(&g.actor,AL_LEFT,0);
    CHECK(g.actor.x==232 && !al_overlap(al_body(&g.actor),al_dummy()));
    return 0;
}

static int check_interrupts(void)
{
    AlGame g;al_init(&g);
    for(u16 t=0;t<7;t++)al_tick(&g.actor,0,t ? 0 : AL_A);
    CHECK(al_tick(&g.actor,0,AL_B)&AL_PAIN);
    CHECK(g.actor.clock.clip==AL_HURT && g.actor.hp==2 && !g.actor.hit.w);
    u16 swings=0;
    for(u16 i=0;i<30;i++)swings|=al_tick(&g.actor,0,0)&AL_SWING;
    CHECK(!swings && !g.actor.hits && g.actor.clock.clip==AL_IDLE);
    CHECK(al_tick(&g.actor,0,AL_KO)&AL_FALL);
    CHECK(!g.actor.hp && g.actor.clock.clip==AL_DEAD);
    al_tick(&g.actor,0,AL_A);CHECK(!g.actor.hp);
    u16 completions=0;
    for(u16 i=0;i<100;i++)completions+=(al_tick(&g.actor,0,0)&AL_END)!=0;
    CHECK(completions==1 && g.actor.clock.done && g.actor.pose==16);
    al_tick(&g.actor,0,AL_A);CHECK(g.actor.hp==3 && g.actor.clock.clip==AL_IDLE);
    return 0;
}

static int check_timing(void)
{
    AlGame ntsc,pal;al_init(&ntsc);al_init(&pal);ntsc.actor.x=20;pal.actor.x=20;u16 a[6]={0},b[6]={0};
    for(u16 mode=0;mode<2;mode++) {
        AlGame *g=mode ? &pal : &ntsc;u16 *counts=mode ? b : a;u16 hz=mode ? 50 : 60;
        for(u16 frame=0;frame<hz;frame++) {
            AlBatch batch;al_frame(g,AL_RIGHT,hz,false,false,&batch);CHECK(batch.count<=2);
            for(u16 i=0;i<batch.count;i++)for(u16 bit=0;bit<6;bit++)counts[bit]+=(batch.events[i]&(1<<bit))!=0;
        }
    }
    CHECK(ntsc.actor.x==140 && pal.actor.x==140 && ntsc.actor.pose==pal.actor.pose);
    for(u16 i=0;i<6;i++)CHECK(a[i]==b[i]);
    AlGame g;al_init(&g);AlBatch batch;
    al_frame(&g,AL_A,60,false,false,&batch);u16 age=g.actor.clock.age,phase=g.phase;
    for(u16 i=0;i<100;i++){al_frame(&g,AL_B,50,true,false,&batch);CHECK(!batch.count);}
    CHECK(g.actor.clock.age==age && g.phase==phase && g.actor.hp==3);
    al_frame(&g,AL_B,60,false,false,&batch);CHECK(g.actor.hp==3); /* Paused edges discarded. */
    al_init(&g);
    al_frame(&g,0,60,false,true,&batch);al_frame(&g,AL_A,60,false,true,&batch);
    CHECK(!batch.count && !g.actor.attacks);
    for(u16 i=0;i<3;i++)al_frame(&g,0,60,false,true,&batch);
    CHECK(g.actor.attacks==1); /* Short press retained until the next slow tick. */
    return 0;
}

static int check_soak(u32 count)
{
    AlGame g;al_init(&g);u32 seed=57;
    for(u32 i=0;i<count;i++) {
        seed=seed*1664525UL+1013904223UL;u16 held=(u16)(seed>>24)&31;
        AlBatch batch;al_frame(&g,held,(i&1) ? 50 : 60,(seed&511)==0,(seed&1024)!=0,&batch);
        CHECK(batch.count<=2 && g.actor.x>=16 && g.actor.x<=304 && g.actor.pose<=16 && g.actor.hp<=3);
        CHECK(g.actor.clock.clip<AL_CLIPS && g.actor.clock.frame<al_clips[g.actor.clock.clip].count);
        CHECK(!g.actor.hit.w || (g.actor.clock.clip==AL_ATTACK && (g.actor.pose==7 || g.actor.pose==8)));
    }
    return 0;
}

static u32 trace_digest(void)
{
    AlGame g;al_init(&g);u32 seed=57,digest=2166136261UL;
    for(u16 i=0;i<2000;i++) {
        seed=seed*1664525UL+1013904223UL;AlBatch batch;
        al_frame(&g,(u16)(seed>>24)&31,(i&1) ? 50 : 60,(seed&511)==0,(seed&1024)!=0,&batch);
        u16 values[]={g.actor.x,g.actor.hp,g.actor.pose,g.actor.attacks,g.actor.hits,g.actor.clock.age,
            g.actor.clock.clip,g.actor.clock.frame,g.actor.clock.elapsed,g.actor.clock.done,g.pending,batch.count};
        for(u16 v=0;v<12;v++)digest=(digest^values[v])*16777619UL;
        for(u16 e=0;e<batch.count;e++)digest=(digest^batch.events[e])*16777619UL;
    }
    return digest;
}
#endif
