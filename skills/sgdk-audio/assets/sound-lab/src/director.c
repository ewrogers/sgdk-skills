#include "director.h"
#include <string.h>
static const u8 priority[AD_FX_COUNT]={2,4,8,12};
/* Leases include the padded sample tail and a small command-service allowance.
 * They are admission estimates, not a sample-position/status API. */
static const u16 duration[AD_FX_COUNT]={22,28,32,48};
static const u16 cooldown[AD_FX_COUNT]={6,10,15,30};
void ad_init(AdDirector *d)
{
    memset(d,0,sizeof(*d));d->current=AD_NONE;d->desired=AD_NONE;
    for(u16 i=0;i<2;i++)d->voice[i].id=AD_NONE;
}
bool ad_scene(AdDirector *d,u16 track)
{if(track>=3)return false;d->desired=(u8)track;d->stop_requested=false;d->timed_out=false;return true;}
bool ad_jingle(AdDirector *d)
{if(d->jingle || d->pending_jingle || d->paused)return false;d->pending_jingle=true;return true;}
void ad_stop(AdDirector *d)
{d->desired=AD_NONE;d->pending_jingle=false;d->stop_requested=true;d->queued=0;}
void ad_pause(AdDirector *d,bool paused)
{
    bool resuming=d->paused && !paused;
    d->paused=paused;d->queued=0;
    /* Resume is asynchronous too. A temporarily false status is not a jingle end. */
    if(resuming && (d->state==AD_PLAYING || d->state==AD_STARTING)){d->state=AD_STARTING;d->wait=0;}
    if(paused)for(u16 i=0;i<2;i++){d->voice[i].remaining=0;d->voice[i].id=AD_NONE;}
}
AdCommand ad_step(AdDirector *d,bool playing,bool fading,u16 hz)
{return ad_service(d,playing,fading,hz,1);}
AdCommand ad_service(AdDirector *d,bool playing,bool fading,u16 hz,u16 elapsed_vblanks)
{
    AdCommand out={0,AD_NONE,0,false};if(!elapsed_vblanks || (hz!=50 && hz!=60))return out;
    if(d->paused)return out;
    /* The SDK fade has a deferred stop. It must finish before any replacement. */
    if(d->state==AD_FADING) {
        if(fading)return out;
        d->state=AD_IDLE;d->current=AD_NONE;d->jingle=false;
    }
    if(d->stop_requested) {
        d->stop_requested=false;d->state=AD_IDLE;d->current=AD_NONE;d->jingle=false;
        out.command=AD_STOP;return out;
    }
    if(d->state==AD_STARTING) {
        if(playing){d->state=AD_PLAYING;d->wait=0;}
        else if((u32)d->wait+elapsed_vblanks>hz*2) {
            d->timed_out=true;d->desired=AD_NONE;d->pending_jingle=false;d->jingle=false;
            d->state=AD_IDLE;d->current=AD_NONE;out.command=AD_STOP;
        }
        else d->wait+=elapsed_vblanks;
        return out;
    }
    if(d->state==AD_PLAYING) {
        if(!playing) {d->state=AD_IDLE;d->current=AD_NONE;d->jingle=false;}
        else if(!d->jingle && (d->pending_jingle || d->desired!=d->current)) {
            d->state=AD_FADING;out.command=AD_FADE;out.frames=hz/2;return out;
        } else return out;
    }
    if(d->state==AD_IDLE && (d->pending_jingle || d->desired!=AD_NONE)) {
        d->jingle=d->pending_jingle;d->pending_jingle=false;
        d->current=d->jingle ? 3 : d->desired;d->state=AD_STARTING;d->wait=0;
        out.command=AD_PLAY;out.track=d->current;out.loop=!d->jingle;
    }
    return out;
}
bool ad_post(AdDirector *d,u16 effect)
{
    if(effect>=AD_FX_COUNT || d->paused){d->dropped++;return false;}
    for(u16 i=0;i<d->queued;i++)if(d->queue[i]==effect){d->suppressed++;return false;}
    if(d->cooldown[effect]){d->suppressed++;return false;}
    if(d->queued==AD_QUEUE){d->dropped++;return false;}
    d->queue[d->queued++]=(u8)effect;return true;
}
void ad_clock(AdDirector *d,u16 hz)
{ad_clock_frames(d,hz,1);}
void ad_clock_frames(AdDirector *d,u16 hz,u16 elapsed_vblanks)
{
    if(d->paused || (hz!=50 && hz!=60))return;
    u32 ticks=0;
    if(elapsed_vblanks==1) {
        d->phase+=60;while(d->phase>=hz){d->phase-=hz;ticks++;}
    } else {
        u32 phase=d->phase+(u32)elapsed_vblanks*60;ticks=phase/hz;d->phase=(u16)(phase%hz);
    }
    for(u16 i=0;i<AD_FX_COUNT;i++)d->cooldown[i]=d->cooldown[i]>ticks ? (u16)(d->cooldown[i]-ticks) : 0;
    for(u16 i=0;i<2;i++) {
        d->voice[i].remaining=d->voice[i].remaining>ticks ? (u16)(d->voice[i].remaining-ticks) : 0;
        if(!d->voice[i].remaining)d->voice[i].id=AD_NONE;
    }
}
void ad_flush(AdDirector *d,u8 effects[2])
{
    effects[0]=AD_NONE;effects[1]=AD_NONE;
    if(d->paused){d->queued=0;return;}
    /* One winning request per owned channel. Critical alert owns CH3;
     * ordinary gameplay owns CH2. Music percussion stays on CH1. */
    for(u16 i=0;i<d->queued;i++) {
        u8 id=d->queue[i],channel=id==AD_ALERT ? 1 : 0,old=effects[channel];
        if(old==AD_NONE || priority[id]>priority[old]) {
            if(old!=AD_NONE)d->dropped++;
            effects[channel]=id;
        } else d->dropped++;
    }
    for(u16 c=0;c<2;c++)if(effects[c]!=AD_NONE) {
        u8 id=effects[c];AdVoice *v=&d->voice[c];
        if(v->remaining && v->priority>priority[id]){effects[c]=AD_NONE;d->dropped++;continue;}
        v->id=id;v->priority=priority[id];v->remaining=duration[id];d->cooldown[id]=cooldown[id];d->scheduled++;
    }
    d->queued=0;
}
void ad_rejected(AdDirector *d,u16 voice)
{
    if(voice>=2)return;
    d->driver_rejected++;d->voice[voice].id=AD_NONE;d->voice[voice].remaining=0;
}
bool ad_valid(const AdDirector *d)
{
    if(d->state>AD_FADING || d->queued>AD_QUEUE || (d->desired!=AD_NONE && d->desired>=3) || (d->current!=AD_NONE && d->current>=4))return false;
    for(u16 i=0;i<2;i++)if(d->voice[i].id!=AD_NONE && d->voice[i].id>=AD_FX_COUNT)return false;
    for(u16 i=0;i<d->queued;i++)if(d->queue[i]>=AD_FX_COUNT)return false;
    return true;
}
u32 ad_digest(const AdDirector *d)
{
    u32 h=2166136261u;
#define MIX(v) h=(h^(u32)(v))*16777619u
    MIX(d->state);MIX(d->current);MIX(d->desired);MIX(d->jingle);MIX(d->pending_jingle);MIX(d->paused);
    MIX(d->stop_requested);MIX(d->timed_out);MIX(d->wait);MIX(d->phase);MIX(d->queued);
    for(u16 i=0;i<AD_FX_COUNT;i++){MIX(d->cooldown[i]);}
    for(u16 i=0;i<2;i++){MIX(d->voice[i].id);MIX(d->voice[i].priority);MIX(d->voice[i].remaining);}
    MIX(d->scheduled);MIX(d->suppressed);MIX(d->dropped);MIX(d->driver_rejected);
#undef MIX
    return h;
}
