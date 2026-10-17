#ifndef ROUTE_H
#define ROUTE_H
#include "stream_world.h"
#include "director.h"
#ifdef AD_HOST
typedef int16_t s16;
#endif
enum { R_LEFT=1,R_RIGHT=2,R_WARP=4,R_SLOW=8,R_JINGLE=16,R_PAUSE=32 };
typedef struct {
    StreamCache cache;
    AdDirector audio;
    SwFade fade,fade_before;
    AdCommand command;
    u16 player,camera,next_player,next_camera,destination,build,phase,previous;
    u16 requested_track,budget,waits;
    bool loading,covered,warp_reset,paused,pause_requested,inflight;
} Route;
void rt_init(Route *r,u16 held);
bool rt_begin(Route *r,u16 held,u16 hz,bool playing,bool audio_fading);
bool rt_begin_elapsed(Route *r,u16 held,u16 hz,bool playing,bool audio_fading,u16 elapsed_vblanks);
bool rt_view_ready(const Route *r,u16 camera);
bool rt_draw_ready(const Route *r);
void rt_fade_accepted(Route *r,bool accepted);
void rt_publish(Route *r,bool queued,u16 columns);
bool rt_end(Route *r,bool drained);
#endif
