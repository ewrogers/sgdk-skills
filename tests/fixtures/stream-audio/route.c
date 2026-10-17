#include "route.h"
static u16 rt_camera_bound(s16 x) { return x<0 ? 0 : x>960 ? 960 : (u16)x; }
static void music(Route *r,u16 track)
{
    /* Only a new game request retries a failed start, not every polling frame. */
    if(track!=r->requested_track) { r->requested_track=track;ad_scene(&r->audio,track); }
}
void rt_init(Route *r,u16 held)
{
    *r=(Route){0};sc_init(&r->cache);ad_init(&r->audio);
    r->player=160;r->loading=true;r->covered=true;r->budget=8;
    r->previous=held;r->requested_track=AD_NONE;music(r,0);
}
bool rt_view_ready(const Route *r,u16 camera)
{
    if(camera>960)return false;
    u16 tile;
    for(u16 area=camera/320;area<=(camera+319)/320;area++)
        for(u16 p=0;p<6;p++)if(!sc_tile(&r->cache,area*6+p,0,&tile))return false;
    return true;
}
bool rt_draw_ready(const Route *r)
{
    return r->loading ? rt_view_ready(r,r->camera) && rt_view_ready(r,rt_camera_bound((s16)r->camera-160)) &&
        rt_view_ready(r,rt_camera_bound((s16)r->camera+160)) : rt_view_ready(r,r->next_camera);
}
bool rt_begin(Route *r,u16 held,u16 hz,bool playing,bool fading)
{
    return rt_begin_elapsed(r,held,hz,playing,fading,1);
}
bool rt_begin_elapsed(Route *r,u16 held,u16 hz,bool playing,bool fading,u16 elapsed_vblanks)
{
    if(r->inflight || (hz!=50 && hz!=60))return false;
    r->inflight=true;
    u16 pressed=held & (u16)~r->previous;r->previous=held;
    if(pressed&R_PAUSE)r->pause_requested=!r->pause_requested;
    /* Let owned fades and pending starts finish before applying a pause. */
    bool stable=!r->loading && r->fade.state==SW_FADE_IDLE && !fading &&
                (r->audio.state==AD_PLAYING || r->audio.state==AD_IDLE);
    if(stable && r->paused!=r->pause_requested) {
        r->paused=r->pause_requested;r->phase=0;ad_pause(&r->audio,r->paused);
    }
    if(pressed&R_SLOW)r->budget=r->budget==8 ? 2 : 8;
    if(!r->paused && (pressed&R_JINGLE))ad_jingle(&r->audio);
    if(!r->paused && !r->loading && r->fade.state==SW_FADE_IDLE && (pressed&R_WARP)) {
        r->destination=r->player<640 ? 960 : 0;
        sw_fade_begin(&r->fade);r->warp_reset=false;
        music(r,r->destination>=640 ? 1 : 0);
    }
    if(r->fade.state==SW_FADE_LOAD && !r->warp_reset && sc_reset(&r->cache)) {
        r->loading=true;r->covered=true;r->camera=r->destination;
        r->player=r->destination+160;r->build=0;r->warp_reset=true;
    }
    bool audio_ready=(r->audio.state==AD_PLAYING && r->audio.current==r->requested_track) || r->audio.timed_out;
    r->fade_before=r->fade;
    if(!r->paused)sw_fade_tick(&r->fade,hz,!r->loading && audio_ready);
    r->next_player=r->player;r->next_camera=r->camera;
    if(!r->paused && !r->loading && r->fade.state==SW_FADE_IDLE) {
        r->phase+=60;u16 steps=0;
        while(r->phase>=hz) { r->phase-=hz;steps++; }
        s16 x=(s16)r->player;
        if((held&(R_LEFT|R_RIGHT))==R_LEFT)x-=2*steps;
        if((held&(R_LEFT|R_RIGHT))==R_RIGHT)x+=2*steps;
        r->next_player=x<16 ? 16 : x>1264 ? 1264 : (u16)x;
        r->next_camera=sw_camera(r->next_player);
    }
    u16 pages[SC_SLOTS];
    u16 n=sw_wanted(rt_camera_bound((s16)r->next_camera-160),rt_camera_bound((s16)r->next_camera+160),false,pages);
    if(sc_pin(&r->cache,pages,n))
        for(u16 i=0;i<n;i++)if(sc_require(&r->cache,pages[i])!=SC_AVAILABLE)break;
    ad_clock_frames(&r->audio,hz,elapsed_vblanks);
    r->command=ad_service(&r->audio,playing,fading,hz,elapsed_vblanks);
    return true;
}
void rt_fade_accepted(Route *r,bool accepted)
{
    if(r->inflight && !accepted)r->fade=r->fade_before;
}
void rt_publish(Route *r,bool queued,u16 columns)
{
    if(!r->inflight)return;
    if(r->loading) {
        if(queued && columns<=4 && columns<=sw_columns(r->camera)-r->build && rt_draw_ready(r)) {
            r->build+=columns;
            if(r->build>=sw_columns(r->camera))r->loading=false;
        }
    } else if(queued && rt_view_ready(r,r->next_camera)) {
        r->camera=r->next_camera;r->player=r->next_player;
        if(r->fade.state==SW_FADE_IDLE)music(r,r->player>=640 ? 1 : 0);
    } else if(r->next_camera!=r->camera && r->waits<65535)r->waits++;
}
bool rt_end(Route *r,bool drained)
{
    if(!r->inflight || !drained)return false;
    sc_complete(&r->cache,true);r->inflight=false;
    if(!r->loading)r->covered=false;
    return true;
}
