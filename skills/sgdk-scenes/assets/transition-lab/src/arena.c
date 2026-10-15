#include "arena.h"
#include "level.h"

void arena_restore(Lab *lab)
{
    lab->phase=lab->finish_ticks=lab->boss_defeat_ticks=0;
    lab->boss_attack_clock=lab->boss_windup=lab->boss_animation=lab->boss_path_wait=0;
    lab->boss_direction=2; lab->boss_walking=lab->boss_has_waypoint=false;
    for (u16 i=0;i<LASER_CAPACITY;++i) lab->lasers[i]=(Laser){0};
    lab->boss_x=144; lab->boss_y=88; lab->mega_x=144; lab->mega_y=88;
    for (u16 p=0;p<2;++p) {
        lab->facing[p]=0; lab->flip[p]=lab->moving[p]=false; lab->animation_ticks[p]=0;
        lab->invulnerable[p]=0;
    }
    for (u16 i=0;i<COIN_COUNT;++i)
        lab->loose[i]=(LooseCoin){.x=lab_coin_x(lab->run.room,i)*8,.y=lab_coin_y(lab->run.room,i)*8};
}
void arena_reset(Lab *lab)
{
    lab->run.x[0]=24; lab->run.x[1]=280;
    lab->run.y[0]=lab->run.y[1]=184;
    lab->run.coins=255; lab->run.carried[0]=lab->run.carried[1]=0; lab->run.boss_state=0;
    arena_restore(lab);
}
static bool overlap(s16 ax,s16 ay,s16 aw,s16 ah,s16 bx,s16 by,s16 bw,s16 bh)
{
    return ax<bx+bw && bx<ax+aw && ay<by+bh && by<ay+ah;
}
static void loose_tick(LooseCoin *coin,u16 room)
{
    if (coin->wait) --coin->wait;
    s16 old_x=coin->x,old_y=coin->y;
    coin->x+=coin->vx;
    if (level_solid(room,coin->x/8+4,coin->y/8+4,8,8)) { coin->x=old_x; coin->vx=-coin->vx; }
    coin->y+=coin->vy;
    if (level_solid(room,coin->x/8+4,coin->y/8+4,8,8)) { coin->y=old_y; coin->vy=-coin->vy; }
    coin->vx=coin->vx*15/16; coin->vy=coin->vy*15/16;
    if (coin->x<16*8) { coin->x=16*8; coin->vx=-coin->vx; }
    if (coin->x>288*8) { coin->x=288*8; coin->vx=-coin->vx; }
    if (coin->y<64*8) { coin->y=64*8; coin->vy=-coin->vy; }
    if (coin->y>192*8) { coin->y=192*8; coin->vy=-coin->vy; }
    if (coin->height || coin->lift) {
        coin->height+=coin->lift; coin->lift-=2;
        if (coin->height<=0) {
            coin->height=0; coin->lift=-coin->lift/2;
            if (coin->lift<4) coin->lift=0;
        }
    }
}
static void move_player(Lab *lab,u16 p,u16 keys)
{
    s16 dx=2*(((keys&KEY_RIGHT)!=0)-((keys&KEY_LEFT)!=0));
    s16 dy=2*(((keys&KEY_DOWN)!=0)-((keys&KEY_UP)!=0));
    s16 old_x=lab->run.x[p],old_y=lab->run.y[p],x=old_x+dx,y=old_y+dy;
    lab->run.x[p]=x<16 ? 16 : x>288 ? 288 : x;
    if (level_solid(lab->run.room,lab->run.x[p],old_y,16,16)) lab->run.x[p]=old_x;
    lab->run.y[p]=y<64 ? 64 : y>192 ? 192 : y;
    if (level_solid(lab->run.room,lab->run.x[p],lab->run.y[p],16,16)) lab->run.y[p]=old_y;
    bool moving=lab->run.x[p]!=old_x || lab->run.y[p]!=old_y;
    u8 facing=dx ? 1 : dy ? (dy<0 ? 2 : 0) : lab->facing[p];
    bool flip=dx ? dx<0 : dy ? false : lab->flip[p];
    if (moving!=lab->moving[p] || facing!=lab->facing[p] || flip!=lab->flip[p]) lab->animation_ticks[p]=0;
    else lab->animation_ticks[p]=(lab->animation_ticks[p]+1)%(moving ? 32 : 96);
    lab->moving[p]=moving; lab->facing[p]=facing; lab->flip[p]=flip;
}
static void scatter(Lab *lab,u16 p)
{
    static const s16 vx[]={32,23,0,-23,-32,-23,0,23};
    static const s16 vy[]={0,23,32,23,0,-23,-32,-23};
    u16 count=0,ordinal=0;
    for (u16 i=0;i<COIN_COUNT;++i) if (lab->run.carried[p]&(1u<<i)) ++count;
    for (u16 i=0;i<COIN_COUNT;++i) if (lab->run.carried[p]&(1u<<i)) {
        u16 direction=(ordinal++*8/count+lab->phase/30)%8;
        lab->loose[i]=(LooseCoin){.x=lab->run.x[p]*8,.y=lab->run.y[p]*8,
            .vx=vx[direction],.vy=vy[direction],.wait=45,.lift=24+(i%3)*2};
        --lab->run.score[p];
    }
    lab->run.coins|=lab->run.carried[p]; lab->run.carried[p]=0;
    lab->invulnerable[p]=90; lab->audio_events|=AUDIO_HIT;
}
static u8 aim_direction(s16 dx,s16 dy)
{
    u16 ax=dx<0 ? -dx : dx,ay=dy<0 ? -dy : dy;
    if (ay*2<ax) return dx<0 ? 4 : 0;
    if (ax*2<ay) return dy<0 ? 6 : 2;
    if (dy<0) return dx<0 ? 5 : 7;
    return dx<0 ? 3 : 1;
}
static void fire_laser(Lab *lab)
{
    for (u16 i=0;i<LASER_CAPACITY;++i) if (!lab->lasers[i].active) {
        lab->lasers[i]=(Laser){(lab->boss_x+8)*8,(lab->boss_y+8)*8,90,lab->boss_direction,true};
        lab->audio_events|=AUDIO_LASER; return;
    }
}
static void lasers_tick(Lab *lab)
{
    static const s16 dx[]={32,23,0,-23,-32,-23,0,23},dy[]={0,23,32,23,0,-23,-32,-23};
    for (u16 i=0;i<LASER_CAPACITY;++i) {
        Laser *shot=&lab->lasers[i];
        if (lab->run.boss_state) shot->active=false;
        if (!shot->active) continue;
        shot->x+=dx[shot->direction]; shot->y+=dy[shot->direction];
        if (!--shot->life || shot->x<16*8 || shot->x>288*8 || shot->y<64*8 || shot->y>192*8 ||
            level_solid(lab->run.room,shot->x/8+4,shot->y/8+4,8,8)) {
            shot->active=false; continue;
        }
        for (u16 p=0;p<2;++p) if ((lab->run.joined&(1u<<p)) &&
            overlap(lab->run.x[p]+2,lab->run.y[p]+2,12,12,shot->x/8+4,shot->y/8+4,8,8)) {
            if (!lab->invulnerable[p]) scatter(lab,p);
            shot->active=false; break;
        }
    }
}
static void boss_tick(Lab *lab)
{
    if (!lab->run.boss_state) {
        if (!lab->run.coins) {
            lab->run.boss_state=1; lab->boss_defeat_ticks=0;
            lab->mega_x=lab->boss_x; lab->mega_y=lab->boss_y;
            lab->audio_events|=AUDIO_POOF; return;
        }
        u16 target=(lab->run.joined&1) ? 0 : 1;
        if (lab->run.joined==3) {
            s16 dx0=lab->run.x[0]-lab->boss_x,dy0=lab->run.y[0]-lab->boss_y;
            s16 dx1=lab->run.x[1]-lab->boss_x,dy1=lab->run.y[1]-lab->boss_y;
            u16 d0=(dx0<0 ? -dx0 : dx0)+(dy0<0 ? -dy0 : dy0);
            u16 d1=(dx1<0 ? -dx1 : dx1)+(dy1<0 ? -dy1 : dy1);
            if (d1<d0) target=1;
        }
        s16 tx=lab->run.x[target]-8,ty=lab->run.y[target]-8;
        if (lab->boss_windup) {
            lab->boss_walking=false;
            if (!--lab->boss_windup) { fire_laser(lab); lab->boss_attack_clock=0; }
        } else if (++lab->boss_attack_clock>=150) {
            lab->boss_direction=aim_direction(tx-lab->boss_x,ty-lab->boss_y);
            lab->boss_windup=45; lab->boss_walking=false;
        } else {
            if (lab->boss_path_wait) --lab->boss_path_wait;
            if ((!lab->boss_has_waypoint && !lab->boss_path_wait) ||
                (lab->boss_has_waypoint && lab->boss_x==lab->boss_waypoint_x && lab->boss_y==lab->boss_waypoint_y)) {
                lab->boss_has_waypoint=level_next_step(lab->run.room,lab->boss_x,lab->boss_y,tx,ty,
                                                      &lab->boss_waypoint_x,&lab->boss_waypoint_y);
                if (!lab->boss_has_waypoint) lab->boss_path_wait=16;
            }
            lab->boss_walking=lab->boss_has_waypoint;
            if (lab->boss_has_waypoint) {
                s16 dx=lab->boss_waypoint_x-lab->boss_x,dy=lab->boss_waypoint_y-lab->boss_y;
                lab->boss_direction=aim_direction(dx,dy);
                lab->boss_animation=(lab->boss_animation+1)%32;
                s16 next_x=lab->boss_x+(dx>0)-(dx<0);
                s16 next_y=lab->boss_y+(!(lab->phase&1) ? (dy>0)-(dy<0) : 0);
                if (!level_solid(lab->run.room,next_x,lab->boss_y,32,32)) lab->boss_x=next_x;
                if (!level_solid(lab->run.room,lab->boss_x,next_y,32,32)) lab->boss_y=next_y;
            }
        }
        for (u16 p=0;p<2;++p) if ((lab->run.joined&(1u<<p)) && !lab->invulnerable[p] &&
            overlap(lab->run.x[p],lab->run.y[p],16,16,lab->boss_x,lab->boss_y,32,32)) scatter(lab,p);
    } else if (lab->run.boss_state==1) {
        if (lab->boss_defeat_ticks<48) { ++lab->boss_defeat_ticks; return; }
        for (u16 p=0;p<2;++p) if ((lab->run.joined&(1u<<p)) &&
            overlap(lab->run.x[p],lab->run.y[p],16,16,lab->mega_x,lab->mega_y,32,32)) {
            lab->run.boss_state=2; lab->finish_ticks=0; lab->audio_events|=AUDIO_WIN; break;
        }
    } else ++lab->finish_ticks;
}
void arena_tick(Lab *lab,const LabInput *input)
{
    bool boss=lab->run.room==ROOM_COUNT-1;
    for (u16 i=0;i<COIN_COUNT;++i) if (lab->run.coins&(1u<<i)) loose_tick(&lab->loose[i],lab->run.room);
    for (u16 p=0;p<2;++p) {
        if (lab->invulnerable[p]) --lab->invulnerable[p];
        if (!(lab->run.joined&(1u<<p)) || (!boss && lab->finish_ticks) || lab->run.boss_state==2) continue;
        move_player(lab,p,input->held[p]);
        for (u16 i=0;i<COIN_COUNT;++i) if ((lab->run.coins&(1u<<i)) && !lab->loose[i].wait &&
            overlap(lab->run.x[p],lab->run.y[p],16,16,lab->loose[i].x/8+4,lab->loose[i].y/8+4,8,8)) {
            lab->run.coins&=~(1u<<i); lab->run.carried[p]|=1u<<i;
            ++lab->run.score[p]; lab->audio_events|=AUDIO_COIN;
        }
    }
    lab->phase=(lab->phase+1)%240;
    if (boss) { boss_tick(lab); lasers_tick(lab); }
    else if (!lab->run.coins) {
        if (!lab->finish_ticks) lab->audio_events|=AUDIO_CLEAR;
        ++lab->finish_ticks;
    }
}
