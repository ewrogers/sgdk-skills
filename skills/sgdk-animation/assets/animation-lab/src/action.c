#include "action.h"

#define F(t,p,e) {t,p,e,{0,0,0,0}}
static const AlFrame idle[]={F(24,0,0),F(24,1,0)};
static const AlFrame walk[]={F(6,2,AL_FOOT),F(6,3,0),F(6,4,AL_FOOT),F(6,5,0)};
static const AlFrame attack[]={F(8,6,0),{3,7,AL_SWING,{10,-12,28,12}},
    {3,8,0,{10,-12,28,12}},F(4,9,0),F(8,10,0)};
static const AlFrame hurt[]={F(5,11,AL_PAIN),F(5,12,0)};
static const AlFrame dead[]={F(6,13,AL_FALL),F(6,14,0),F(6,15,0),F(18,16,0)};
const AlClip al_clips[AL_CLIPS]={{idle,2,true},{walk,4,true},{attack,5,false},{hurt,2,false},{dead,4,false}};

bool al_play(AlClock *clock,u16 clip)
{
    if(clip>=AL_CLIPS)return false;
    *clock=(AlClock){0,(u8)clip,0,0,true,false};return true;
}

u16 al_clock_tick(AlClock *clock,AlFrame *shown)
{
    const AlClip *clip=&al_clips[clock->clip];
    *shown=clip->frames[clock->frame];
    if(clock->done)return 0;
    u16 events=clock->entered ? shown->events : 0;clock->entered=false;
    clock->age++;clock->elapsed++;
    if(clock->elapsed>=shown->ticks) {
        clock->elapsed=0;
        if(clock->frame+1<clip->count){clock->frame++;clock->entered=true;}
        else if(clip->loop){clock->frame=0;clock->age=0;clock->entered=true;}
        else {clock->done=true;events|=AL_END;}
    }
    return events;
}

void al_init(AlGame *game)
{
    *game=(AlGame){0};game->actor.x=100;game->actor.hp=3;al_play(&game->actor.clock,AL_IDLE);
}

AlBox al_body(const AlActor *actor) {return (AlBox){actor->x-6,158,12,20};}
AlBox al_dummy(void) {return (AlBox){214,152,12,26};}
bool al_overlap(AlBox a,AlBox b)
{
    return a.w>0 && a.h>0 && b.w>0 && b.h>0 && a.x<b.x+b.w && a.x+a.w>b.x && a.y<b.y+b.h && a.y+a.h>b.y;
}

static void begin_attack(AlActor *actor)
{
    al_play(&actor->clock,AL_ATTACK);actor->buffer=0;actor->hit_latched=false;
    if(actor->attacks<9999)actor->attacks++;
}

u16 al_tick(AlActor *actor,u16 held,u16 pressed)
{
    if(actor->flash)actor->flash--;
    if(actor->clock.clip==AL_DEAD) {
        if(actor->clock.done && (pressed&AL_A)) {
            actor->hp=3;actor->buffer=0;al_play(&actor->clock,AL_IDLE);
        }
    } else if(pressed&(AL_B|AL_KO)) {
        if(pressed&AL_KO)actor->hp=0;else if(actor->hp)actor->hp--;
        actor->buffer=0;al_play(&actor->clock,actor->hp ? AL_HURT : AL_DEAD);
    } else {
        if((actor->clock.clip==AL_ATTACK || actor->clock.clip==AL_HURT) && actor->clock.done)
            al_play(&actor->clock,AL_IDLE);
        if(pressed&AL_A)actor->buffer=4;
        bool free=actor->clock.clip==AL_IDLE || actor->clock.clip==AL_WALK;
        bool cancel=actor->clock.clip==AL_ATTACK && actor->clock.age>=18;
        if(actor->buffer && (free || cancel))begin_attack(actor);
        else if(free) {
            s16 direction=((held&AL_RIGHT)!=0)-((held&AL_LEFT)!=0);
            u16 desired=direction ? AL_WALK : AL_IDLE;
            if(desired!=actor->clock.clip)al_play(&actor->clock,desired);
            if(direction)actor->left=direction<0;
            actor->x+=direction*2;
            if(al_overlap(al_body(actor),al_dummy()))actor->x=direction>0 ? 208 : 232;
            if(actor->x<16)actor->x=16;
            if(actor->x>304)actor->x=304;
        }
    }
    if(actor->buffer)actor->buffer--;
    AlFrame shown;u16 events=al_clock_tick(&actor->clock,&shown);actor->pose=shown.pose;
    actor->hit=shown.hit;
    if(actor->hit.w) {
        if(actor->left)actor->hit.x=-actor->hit.x-actor->hit.w;
        actor->hit.x+=actor->x;actor->hit.y+=178;
        if(!actor->hit_latched && al_overlap(actor->hit,al_dummy())) {
            actor->hit_latched=true;actor->flash=10;if(actor->hits<9999)actor->hits++;
            events|=AL_HIT;
        }
    }
    return events;
}

void al_frame(AlGame *game,u16 held,u16 hz,bool paused,bool slow,AlBatch *batch)
{
    *batch=(AlBatch){0};
    u16 pressed=held & (u16)~game->previous;game->previous=held;
    if(paused){game->pending=0;return;}
    if(hz!=50 && hz!=60)return;
    game->pending|=pressed&(AL_A|AL_B|AL_KO);
    game->phase+=60;
    while(game->phase>=hz) {
        game->phase-=hz;
        if(slow && (game->slow_phase++&3))continue;
        batch->events[batch->count++]=al_tick(&game->actor,held,game->pending);
        game->pending=0;
    }
}
