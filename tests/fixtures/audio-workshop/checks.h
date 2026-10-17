#include "director.h"
#define CHECK(v) do{if(!(v))return __LINE__;}while(0)
static int contracts(void)
{
    AdDirector d;AdCommand c;u8 fx[2];ad_init(&d);CHECK(ad_valid(&d));
    CHECK(ad_step(&d,false,false,60).command==AD_NO_COMMAND);CHECK(!ad_scene(&d,9));
    ad_scene(&d,0);c=ad_step(&d,false,false,60);CHECK(c.command==AD_PLAY && c.track==0 && c.loop);
    for(u16 i=0;i<5;i++)CHECK(ad_step(&d,false,false,60).command==AD_NO_COMMAND);
    CHECK(d.state==AD_STARTING);ad_step(&d,true,false,60);CHECK(d.state==AD_PLAYING);
    ad_scene(&d,1);c=ad_step(&d,true,false,60);CHECK(c.command==AD_FADE && c.frames==30);
    ad_scene(&d,2);ad_scene(&d,0);ad_scene(&d,2);
    for(u16 i=0;i<5;i++)CHECK(ad_step(&d,true,true,60).command==AD_NO_COMMAND);
    c=ad_step(&d,false,false,60);CHECK(c.command==AD_PLAY && c.track==2);ad_step(&d,true,false,60);
    CHECK(ad_jingle(&d));CHECK(!ad_jingle(&d));CHECK(ad_step(&d,true,false,60).command==AD_FADE);
    c=ad_step(&d,false,false,60);CHECK(c.command==AD_PLAY && c.track==3 && !c.loop);
    CHECK(ad_step(&d,false,false,60).command==AD_NO_COMMAND);CHECK(d.jingle);
    ad_step(&d,true,false,60);ad_pause(&d,true);CHECK(ad_step(&d,false,false,60).command==AD_NO_COMMAND);
    ad_pause(&d,false);CHECK(d.state==AD_STARTING && d.jingle);
    for(u16 i=0;i<3;i++)CHECK(ad_step(&d,false,false,60).command==AD_NO_COMMAND);
    ad_step(&d,true,false,60);CHECK(d.state==AD_PLAYING && d.jingle);ad_scene(&d,1);CHECK(ad_step(&d,true,false,60).command==AD_NO_COMMAND);
    c=ad_step(&d,false,false,60);CHECK(c.command==AD_PLAY && c.track==1 && !d.jingle);
    ad_step(&d,true,false,60);ad_scene(&d,2);CHECK(ad_step(&d,true,false,50).frames==25);
    ad_stop(&d);CHECK(ad_step(&d,true,true,50).command==AD_NO_COMMAND);
    CHECK(ad_step(&d,false,false,50).command==AD_STOP);CHECK(ad_step(&d,false,false,50).command==AD_NO_COMMAND);
    ad_scene(&d,0);ad_step(&d,false,false,50);
    for(u16 i=0;i<100;i++)CHECK(ad_step(&d,false,false,50).command==AD_NO_COMMAND);
    CHECK(ad_step(&d,false,false,50).command==AD_STOP && d.timed_out);
    for(u16 i=0;i<200;i++)CHECK(ad_step(&d,false,false,50).command==AD_NO_COMMAND);
    ad_scene(&d,1);CHECK(!d.timed_out);CHECK(ad_step(&d,false,false,50).command==AD_PLAY);
    ad_stop(&d);CHECK(ad_step(&d,false,false,50).command==AD_STOP);
    ad_init(&d);CHECK(ad_post(&d,AD_COIN));CHECK(!ad_post(&d,AD_COIN));CHECK(ad_post(&d,AD_JUMP));CHECK(ad_post(&d,AD_IMPACT));CHECK(ad_post(&d,AD_ALERT));
    ad_flush(&d,fx);CHECK(fx[0]==AD_IMPACT && fx[1]==AD_ALERT);CHECK(d.scheduled==2 && d.suppressed==1 && d.dropped==2);
    CHECK(!ad_post(&d,AD_IMPACT));CHECK(ad_post(&d,AD_COIN));ad_flush(&d,fx);CHECK(fx[0]==AD_NONE && fx[1]==AD_NONE);
    ad_rejected(&d,0);CHECK(d.driver_rejected==1 && !d.voice[0].remaining);
    CHECK(ad_post(&d,AD_COIN));ad_flush(&d,fx);CHECK(fx[0]==AD_COIN);
    ad_pause(&d,true);u32 h=ad_digest(&d);ad_clock(&d,60);CHECK(ad_digest(&d)==h);
    CHECK(!ad_post(&d,AD_ALERT));ad_flush(&d,fx);CHECK(fx[0]==AD_NONE && !d.voice[0].remaining);
    CHECK(ad_step(&d,false,false,60).command==AD_NO_COMMAND);ad_pause(&d,false);
    for(u16 i=0;i<6;i++)ad_clock(&d,60);
    CHECK(ad_post(&d,AD_COIN));
    ad_stop(&d);CHECK(!d.queued);
    for(u16 hz=50;hz<=60;hz+=10) {
        ad_init(&d);ad_post(&d,AD_ALERT);ad_flush(&d,fx);
        for(u16 i=0;i<hz;i++)ad_clock(&d,hz);
        CHECK(!d.voice[1].remaining && !d.cooldown[AD_ALERT] && d.phase==0);
    }
    ad_init(&d);ad_post(&d,AD_ALERT);ad_flush(&d,fx);ad_clock_frames(&d,50,50);
    CHECK(!d.voice[1].remaining && !d.cooldown[AD_ALERT] && !d.phase);
    ad_scene(&d,0);ad_service(&d,false,false,50,1);CHECK(ad_service(&d,false,false,50,101).command==AD_STOP);
    return 0;
}
static int soak(u16 frames,u32 *trace)
{
    AdDirector d;ad_init(&d);u32 rng=241,h=2166136261u;
    for(u16 i=0;i<frames;i++) {
        rng=rng*1664525u+1013904223u;
        switch((rng>>24)&7) {
            case 0:ad_scene(&d,(u16)((rng>>16)%3));break;
            case 1:ad_jingle(&d);break;case 2:ad_stop(&d);break;
            case 3:ad_pause(&d,!d.paused);break;default:ad_post(&d,(u16)((rng>>16)%4));break;
        }
        u16 hz=i&1 ? 50 : 60;AdCommand c=ad_step(&d,(rng&8)!=0,(rng&16)!=0,hz);CHECK(c.command<=AD_STOP);
        if(c.command==AD_PLAY)CHECK(c.track<4);
        ad_clock(&d,hz);u8 fx[2];ad_flush(&d,fx);CHECK(ad_valid(&d));h=(h^ad_digest(&d))*16777619u;
    }
    *trace=h;return 0;
}
