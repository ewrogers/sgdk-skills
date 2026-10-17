#ifndef ROUTE_CHECKS_H
#define ROUTE_CHECKS_H
#include "view.h"
#include <string.h>
#define CHECK(c) do { if(!(c))return __LINE__; }while(0)
typedef struct {
    u16 vram[SC_SLOTS*SC_PAGE_TILES*16],map[64][24],copy[128];
    const void *borrowed;
    u16 tile,tiles,peak,start,fade,song,track,plays,fades,columns,palette_owner[2];
    bool playing,fading,paused,offline,read_fail,queue_fail,map_fail,palette_fail,broken;
} TestBus;
static bool test_equal(const void *left,const void *right,u16 bytes)
{
    const u8 *a=left,*b=right;
    for(u16 i=0;i<bytes;i++)if(a[i]!=b[i])return false;
    return true;
}
static bool test_read(void *context,u16 page,u16 offset,void *dest,u16 bytes)
{
    TestBus *b=context;
    if(b->read_fail) { b->read_fail=false;return false; }
    for(u16 i=0;i<bytes/2;i++)((u16*)dest)[i]=(page+1)*512+offset+i*2;
    return true;
}
static bool test_queue(void *context,const void *source,u16 tile,u16 tiles)
{
    TestBus *b=context;
    if(b->queue_fail) { b->queue_fail=false;return false; }
    if(b->borrowed || tile<SC_CACHE_BASE || tiles>8) { b->broken=true;return false; }
    b->borrowed=source;b->tile=tile;b->tiles=tiles;
    memcpy(b->copy,source,tiles*32);
    if(tiles*32>b->peak)b->peak=tiles*32;
    return true;
}
static void test_command(TestBus *b,AdCommand c,u16 hz)
{
    if(c.command==AD_PLAY) {
        if(b->fading)b->broken=true;
        b->plays++;b->track=c.track;b->start=3;b->playing=false;b->song=c.loop ? 0 : hz;
    } else if(c.command==AD_FADE) { b->fades++;b->fading=true;b->fade=c.frames; }
    else if(c.command==AD_STOP) { b->playing=false;b->start=0; }
}
static int test_frame(Route *r,TestBus *b,u16 held,u16 hz,bool delay)
{
    CHECK(rt_begin(r,held,hz,b->playing && !b->paused,b->fading));
    b->paused=r->paused;test_command(b,r->command,hz);
    sc_step(&r->cache,r->budget,test_read,test_queue,b);
    if(b->borrowed)CHECK(b->tiles<=r->budget);
    bool ok=rt_draw_ready(r);u16 count=0;
    if(ok) {
        u16 start=r->loading ? r->camera/8+r->build : 0;
        u16 total=r->loading ? sw_columns(r->camera)-r->build : 0;
        if(total>4)total=4;
        if(!r->loading) { u16 col;if(sw_entering(r->camera,r->next_camera,&col)) { start=col;total=1; } }
        if(b->map_fail) { b->map_fail=false;ok=false; }
        for(u16 i=0;i<total && ok;i++) {
            u16 words[24];CHECK(rt_column(r,start+i,words));
            memcpy(b->map[(start+i)&63],words,sizeof(words));count++;
        }
    }
    if(b->palette_fail) { b->palette_fail=false;ok=false;rt_fade_accepted(r,false); }
    if(ok) {
        u16 candidate=r->loading ? r->camera : r->next_camera;
        for(u16 area=candidate/320;area<=(candidate+319)/320;area++)b->palette_owner[area&1]=area;
    }
    CHECK((b->borrowed ? b->tiles*32 : 0)+count*48+64+128+8+152<=1536);
    rt_publish(r,ok,count);b->columns+=count;
    if(delay) {
        u16 camera=r->camera;CHECK(!rt_end(r,false));
        CHECK(!rt_begin(r,held,hz,b->playing,b->fading));CHECK(r->camera==camera);
        if(b->borrowed)CHECK(test_equal(b->borrowed,b->copy,b->tiles*32));
    }
    if(b->borrowed) {
        CHECK(test_equal(b->borrowed,b->copy,b->tiles*32));
        memcpy(b->vram+(b->tile-SC_CACHE_BASE)*16,b->copy,b->tiles*32);b->borrowed=0;
    }
    CHECK(rt_end(r,true));CHECK(!b->broken);CHECK(ad_valid(&r->audio));
    if(!r->covered) {
#ifdef RT_PROBE
        const u16 xstep=13,ystep=7; /* Target probe samples presentation; host checks every visible cell. */
#else
        const u16 xstep=1,ystep=1;
#endif
        for(u16 col=r->camera/8;col<=(r->camera+319)/8;col+=xstep)for(u16 row=0;row<24;row+=ystep) {
            u16 page=sw_page(col,row),within=(row&3)*4+(col&3),tile;
            CHECK(sc_tile(&r->cache,page,within,&tile));
            CHECK((b->map[col&63][row]&2047)==tile);
            CHECK(b->palette_owner[(col/40)&1]==col/40);
            CHECK(b->vram[(tile-SC_CACHE_BASE)*16]==(page+1)*512+within*32);
        }
    }
    if(b->fading && !--b->fade) { b->fading=false;b->playing=false; }
    if(b->start && !b->offline && !--b->start)b->playing=true;
    else if(b->playing && !b->paused && b->song && !--b->song)b->playing=false;
    return 0;
}
static int route_checks(u16 hz,u32 *digest)
{
    Route r;TestBus bus={0};rt_init(&r,0);
    bus.read_fail=true;bus.queue_fail=true;
    for(u16 i=0;i<110;i++) { int e=test_frame(&r,&bus,0,hz,i==4);if(e)return e; }
    CHECK(!r.loading && !r.covered && r.audio.state==AD_PLAYING && r.audio.current==0);
    CHECK(r.cache.read_errors==1 && r.cache.queue_errors==1 && bus.peak<=256);
    /* Rejected columns cannot publish a new camera. */
    u16 camera=r.camera;bus.map_fail=true;
    { int e=test_frame(&r,&bus,R_RIGHT,hz,false);if(e)return e; }
    CHECK(r.camera==camera);
    bus.palette_fail=true;
    { int e=test_frame(&r,&bus,R_RIGHT,hz,false);if(e)return e; }
    CHECK(r.camera==camera);
    /* Slow bounded uploads, forward then backward across evictions. */
    { int e=test_frame(&r,&bus,R_SLOW,hz,false);if(e)return e; }CHECK(r.budget==2);
    for(u16 i=0;i<600;i++) { int e=test_frame(&r,&bus,R_RIGHT,hz,i==100);if(e)return e; }
    CHECK(r.player==1264 && r.waits==2 && r.audio.current==1);
    bool reversed=false;
    for(u16 i=0;i<700;i++) {
        int e=test_frame(&r,&bus,R_LEFT,hz,false);if(e)return e;
        if(!reversed && r.requested_track==0) {
            for(u16 j=0;j<12;j++) { e=test_frame(&r,&bus,R_RIGHT,hz,false);if(e)return e; }
            for(u16 j=0;j<hz*2;j++) { e=test_frame(&r,&bus,0,hz,false);if(e)return e; }
            CHECK(r.audio.current==1 && r.audio.desired==1);reversed=true;
        }
    }
    CHECK(reversed && r.player==16 && r.waits==2 && r.cache.evictions>0 && r.audio.current==0);
    /* Music wants the latest zone if a jingle or fade still owns the transport. */
    { int e=test_frame(&r,&bus,R_JINGLE,hz,false);if(e)return e; }
    for(u16 i=0;i<hz/2+12;i++) { int e=test_frame(&r,&bus,0,hz,false);if(e)return e; }
    CHECK(r.audio.current==3 && r.audio.jingle);
    /* Warp holds black until uploads drain, and a pause request waits for ownership. */
    { int e=test_frame(&r,&bus,R_WARP,hz,false);if(e)return e; }
    { int e=test_frame(&r,&bus,R_PAUSE,hz,false);if(e)return e; }CHECK(r.pause_requested && !r.paused);
    bool rejected_black=false;
    for(u16 i=0;i<220;i++) {
        bool reject=!rejected_black && r.fade.state==SW_FADE_OUT && r.fade.level==1 && r.fade.phase+32>=hz;
        if(reject) { bus.palette_fail=true;rejected_black=true; }
        int e=test_frame(&r,&bus,0,hz,i==20);if(e)return e;
        if(reject)CHECK(r.fade.state==SW_FADE_OUT && !r.warp_reset);
    }
    CHECK(rejected_black);
    CHECK(!r.loading && r.fade.state==SW_FADE_IDLE && r.paused && r.audio.current==1);
    u16 x=r.player,build=r.build;
    for(u16 i=0;i<40;i++) { int e=test_frame(&r,&bus,R_RIGHT,hz,false);if(e)return e; }
    CHECK(r.player==x && r.build==build);
    { int e=test_frame(&r,&bus,R_PAUSE,hz,false);if(e)return e; }
    for(u16 i=0;i<10;i++) { int e=test_frame(&r,&bus,0,hz,false);if(e)return e; }
    CHECK(!r.paused && r.audio.state==AD_PLAYING);
    *digest=ad_digest(&r.audio)^(r.cache.evictions*12345u)^r.player;
    /* A dead driver is bounded and does not cause an automatic play retry loop. */
    rt_init(&r,0);memset(&bus,0,sizeof(bus));bus.offline=true;
    for(u16 i=0;i<hz*5;i++) { int e=test_frame(&r,&bus,0,hz,false);if(e)return e; }
    CHECK(r.audio.timed_out && bus.plays==1 && !r.covered);
    { int e=test_frame(&r,&bus,R_WARP,hz,false);if(e)return e; }
    for(u16 i=0;i<hz*5;i++) { int e=test_frame(&r,&bus,0,hz,false);if(e)return e; }
    CHECK(r.audio.timed_out && bus.plays==2 && !r.covered && r.fade.state==SW_FADE_IDLE);
    rt_init(&r,0);
    CHECK(rt_begin_elapsed(&r,0,hz,false,false,1));CHECK(r.command.command==AD_PLAY);CHECK(rt_end(&r,true));
    CHECK(rt_begin_elapsed(&r,0,hz,false,false,hz*2+1));
    CHECK(r.audio.timed_out && r.command.command==AD_STOP);CHECK(rt_end(&r,true));
    return 0;
}
#endif
