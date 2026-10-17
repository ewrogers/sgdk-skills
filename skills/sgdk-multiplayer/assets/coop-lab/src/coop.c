#include "coop.h"
const char cp_map[24][33]={
#include "world_rows.inc"
};
const CpPoint cp_coins[CP_COINS]={{56,112},{184,64},{328,64},{456,112},{456,280},{328,336},{184,336},{56,280}};
const CpPoint cp_beacons[4]={{40,40},{472,40},{472,344},{40,344}};
static s16 cp_clamp(s16 x,s16 lo,s16 hi){return x<lo ? lo : x>hi ? hi : x;}
static s16 abs16(s16 x){return x<0 ? -x : x;}
static s32 distance(s16 x,s16 y,s16 xx,s16 yy){s16 dx=x-xx,dy=y-yy;return (s32)dx*dx+(s32)dy*dy;}
static bool live(const CpPlayer *p){return p->device!=CP_NONE && !p->respawn && p->hp;}
CpPlan cp_plan(u16 a,u16 b)
{
    CpPlan p={{0,0},0};
    if(a==CP_PORT_EA || b==CP_PORT_EA)return (CpPlan){{CP_READER_EA,CP_READER_OFF},0x1d};
    const u16 types[2]={a,b};
    for(u16 i=0;i<2;i++) {
        p.reader[i]=types[i]==CP_PORT_TEAM ? CP_READER_TEAM : types[i]==CP_PORT_PAD ? CP_READER_PAD : CP_READER_OFF;
        if(p.reader[i]==CP_READER_TEAM)p.mask|=i ? 0xe2 : 0x1d;
        else if(p.reader[i]==CP_READER_PAD)p.mask|=1u<<i;
    }
    return p;
}
bool cp_walkable(s16 x,s16 y)
{
    if(x<21 || y<24 || x>CP_WORLD_W-22 || y>CP_WORLD_H-21)return false;
    for(s16 yy=y-4;yy<=y+4;yy+=8)for(s16 xx=x-5;xx<=x+5;xx+=10)
        if(cp_map[yy/16][xx/16]=='#')return false;
    return true;
}
u16 cp_count(const CpGame *g){u16 n=0;for(u16 i=0;i<CP_PLAYERS;i++)if(g->player[i].device!=CP_NONE)n++;return n;}
static bool bounds(const CpGame *g,s16 *l,s16 *r,s16 *t,s16 *b)
{
    bool found=false;
    for(u16 i=0;i<CP_PLAYERS;i++)if(live(&g->player[i])) {
        const CpPlayer *p=&g->player[i];
        if(!found){*l=*r=p->x;*t=*b=p->y;found=true;}
        else {if(p->x<*l)*l=p->x;if(p->x>*r)*r=p->x;if(p->y<*t)*t=p->y;if(p->y>*b)*b=p->y;}
    }
    return found;
}
void cp_camera(CpGame *g)
{
    s16 l=0,r=0,t=0,b=0;
    if(bounds(g,&l,&r,&t,&b)) {
        g->camera_x=cp_clamp((l+r-CP_VIEW_W)/2,0,CP_WORLD_W-CP_VIEW_W);
        g->camera_y=cp_clamp((t+b-CP_VIEW_H)/2,0,CP_WORLD_H-CP_VIEW_H);
    }
}
static bool spawn(CpGame *g,u16 index)
{
    s16 l=0,r=0,t=0,b=0;bool group=bounds(g,&l,&r,&t,&b);
    s16 cx=group ? (l+r)/2 : 56,cy=group ? (t+b)/2 : 56;
    s32 best=0x7fffffff;CpPoint point={56,56};bool found=false;
    /* At most 81 local cells; failure retries later instead of scanning the world. */
    s16 ax=(cx/16)*16+8,ay=(cy/16)*16+8;
    for(s16 oy=-4;oy<=4;oy++)for(s16 ox=-4;ox<=4;ox++) {
        s16 x=ax+ox*16,y=ay+oy*16;
        if(!cp_walkable(x,y))continue;
        if(group && (x<r-CP_SPAN_X || x>l+CP_SPAN_X || y<b-CP_SPAN_Y || y>t+CP_SPAN_Y))continue;
        s32 d=abs16(cx-x)+abs16(cy-y);
        for(u16 i=0;i<CP_PLAYERS;i++)if(live(&g->player[i]) && abs16(x-g->player[i].x)<14 && abs16(y-g->player[i].y)<14)d+=1000;
        for(u16 e=0;e<CP_ENEMIES;e++){CpPoint q=cp_enemy(g,e);if(!g->enemy_stun[e] && abs16(x-q.x)<32 && abs16(y-q.y)<32)d+=10000;}
        if(d<best){best=d;point=(CpPoint){x,y};found=true;}
    }
    if(!found){g->player[index].respawn=15;g->player[index].hp=0;return false;}
    CpPlayer *p=&g->player[index];p->x=point.x;p->y=point.y;p->hp=3;p->respawn=0;p->invulnerable=90;
    p->dash=p->dash_cd=p->stun_cd=p->walk=0;p->facing=0;p->moving=false;return true;
}
void cp_init(CpGame *g)
{
    *g=(CpGame){0};g->pause_owner=CP_NONE;
    for(u16 i=0;i<CP_PLAYERS;i++)g->player[i].device=CP_NONE;
}
void cp_seed_inputs(CpGame *g,const CpInput *input)
{
    for(u16 d=0;d<CP_DEVICES;d++)g->previous[d]=(input->valid&(1u<<d)) ? input->held[d] : 0;
}
CpPoint cp_enemy(const CpGame *g,u16 index)
{
    static const CpPoint base[4]={{88,144},{408,144},{144,288},{368,288}};
    u16 p=(u16)((g->ticks+index*53)%128);s16 offset=(p<64 ? p : 127-p)-32;
    return (CpPoint){base[index%4].x+offset,base[index%4].y};
}
bool cp_damage(CpGame *g,u16 i)
{
    if(i>=CP_PLAYERS || !live(&g->player[i]) || g->player[i].invulnerable || g->paused || g->won)return false;
    CpPlayer *p=&g->player[i];p->hp--;g->events|=CP_HIT;
    if(!p->hp){p->respawn=120;p->deaths++;p->moving=false;p->dash=0;}
    else p->invulnerable=60;
    return true;
}
static void controls(CpGame *g,const CpInput *in,u16 *held,u16 *pressed)
{
    u16 edge[CP_DEVICES],used=0,consumed=0;
    for(u16 d=0;d<CP_DEVICES;d++) {
        u16 h=(in->valid&(1u<<d)) ? in->held[d] : 0;
        edge[d]=h&~g->previous[d];g->previous[d]=h;
    }
    for(u16 i=0;i<CP_PLAYERS;i++) {
        CpPlayer *p=&g->player[i];if(p->device==CP_NONE)continue;
        u16 d=p->device,h=g->previous[d];
        if((edge[d]&CP_START) && (h&(CP_A|CP_C))==(CP_A|CP_C)) {
            consumed|=1u<<d;*p=(CpPlayer){0};p->device=CP_NONE;g->events|=CP_LEAVE;
        } else used|=1u<<d;
    }
    for(u16 d=0;d<CP_DEVICES;d++)if((edge[d]&CP_START) && !(used&(1u<<d)) && !(consumed&(1u<<d))) {
        for(u16 i=0;i<CP_PLAYERS;i++)if(g->player[i].device==CP_NONE) {
            if(!g->running) {
                g->score=g->coin_mask=g->phase=g->tie=0;g->ticks=0;g->won=false;
                for(u16 e=0;e<CP_ENEMIES;e++)g->enemy_stun[e]=0;
            }
            g->player[i]=(CpPlayer){0};g->player[i].device=(u8)d;spawn(g,i);used|=1u<<d;consumed|=1u<<d;
            g->running=true;g->events|=CP_JOIN;break;
        }
    }
    bool toggle=false;
    for(u16 i=0;i<CP_PLAYERS;i++) {
        u16 d=g->player[i].device;held[i]=pressed[i]=0;
        if(d==CP_NONE)continue;
        held[i]=g->previous[d];pressed[i]=(consumed&(1u<<d)) ? 0 : edge[d];
        if(pressed[i]&CP_START){toggle=true;g->pause_owner=i;}
    }
    if(!cp_count(g)){g->running=false;g->paused=false;g->pause_owner=CP_NONE;}
    else if(toggle) {
        if(g->won) {
            /* Keep assignments and input history, replace only round state. */
            g->ticks=0;g->score=g->coin_mask=g->phase=0;g->won=g->paused=false;
            for(u16 e=0;e<CP_ENEMIES;e++)g->enemy_stun[e]=0;
            for(u16 i=0;i<CP_PLAYERS;i++)if(g->player[i].device!=CP_NONE) {
                g->player[i].hp=0;g->player[i].respawn=1;
            }
            for(u16 i=0;i<CP_PLAYERS;i++)if(g->player[i].device!=CP_NONE) {
                g->player[i].gems=g->player[i].deaths=0;spawn(g,i);
            }
        } else {g->paused=!g->paused;g->events|=CP_PAUSE;}
    }
    cp_camera(g);
}
static void move(CpGame *g,const u16 *held,const u16 *pressed)
{
    s16 nx[CP_PLAYERS],ny[CP_PLAYERS],ox[CP_PLAYERS],oy[CP_PLAYERS];
    s16 l=0,r=0,t=0,b=0;bounds(g,&l,&r,&t,&b);
    for(u16 i=0;i<CP_PLAYERS;i++) {
        CpPlayer *p=&g->player[i];ox[i]=nx[i]=p->x;oy[i]=ny[i]=p->y;if(!live(p))continue;
        if((pressed[i]&CP_B) && !p->dash_cd){p->dash=10;p->dash_cd=60;}
        s16 speed=p->dash ? 5 : 2;
        s16 dx=((held[i]&CP_RIGHT)!=0)-((held[i]&CP_LEFT)!=0);
        s16 dy=((held[i]&CP_DOWN)!=0)-((held[i]&CP_UP)!=0);
        /* Axis subdivision avoids tunnelling at dash speed and keeps corners usable. */
        for(s16 s=0;s<speed;s++) {
            if(cp_walkable(nx[i]+dx,ny[i]))nx[i]+=dx;
            if(cp_walkable(nx[i],ny[i]+dy))ny[i]+=dy;
        }
        if(dx || dy)p->facing=dx<0 ? 3 : dx>0 ? 1 : dy<0 ? 2 : 0;
    }
    for(u16 axis=0;axis<2;axis++) {
        s16 low=32767,high=-32767;for(u16 i=0;i<CP_PLAYERS;i++)if(live(&g->player[i])) {
            s16 v=axis ? ny[i] : nx[i];if(v<low)low=v;if(v>high)high=v;
        }
        s16 limit=axis ? CP_SPAN_Y : CP_SPAN_X,center=axis ? t+b : l+r;
        if(high-low>limit)for(u16 i=0;i<CP_PLAYERS;i++)if(live(&g->player[i])) {
            s16 old=axis ? g->player[i].y : g->player[i].x,v=axis ? ny[i] : nx[i];
            if((2*old<=center && v<old) || (2*old>=center && v>old)) {
                if(axis)ny[i]=old;else nx[i]=old;g->tethered=true;
            }
        }
    }
    for(u16 i=0;i<CP_PLAYERS;i++)if(live(&g->player[i])) {
        CpPlayer *p=&g->player[i];
        /* Reject a diagonal corner that became invalid after one-axis tethering. */
        if(!cp_walkable(nx[i],ny[i])){nx[i]=p->x;ny[i]=p->y;}
        p->moving=nx[i]!=p->x || ny[i]!=p->y;if(p->moving)p->walk++;
        p->x=nx[i];p->y=ny[i];
    }
    if(bounds(g,&l,&r,&t,&b) && (r-l>CP_SPAN_X || b-t>CP_SPAN_Y)) {
        /* Corner clipping can undo an inward proposal: restore this joint move. */
        for(u16 i=0;i<CP_PLAYERS;i++)if(live(&g->player[i])){g->player[i].x=ox[i];g->player[i].y=oy[i];g->player[i].moving=false;}
        g->tethered=true;
    }
}
static void tick(CpGame *g,const u16 *held,const u16 *pressed)
{
    g->ticks++;g->tethered=false;
    for(u16 e=0;e<CP_ENEMIES;e++)if(g->enemy_stun[e])g->enemy_stun[e]--;
    for(u16 i=0;i<CP_PLAYERS;i++) {
        CpPlayer *p=&g->player[i];if(p->device==CP_NONE)continue;
        if(p->respawn){if(!--p->respawn && spawn(g,i))g->events|=CP_RESPAWN;continue;}
        if(p->invulnerable)p->invulnerable--;
        if(p->dash)p->dash--;
        if(p->dash_cd)p->dash_cd--;
        if(p->stun_cd)p->stun_cd--;
        if((pressed[i]&CP_A) && !p->stun_cd) {
            p->stun_cd=90;g->events|=CP_STUN;
            for(u16 e=0;e<CP_ENEMIES;e++){CpPoint q=cp_enemy(g,e);if(distance(p->x,p->y,q.x,q.y)<=48*48)g->enemy_stun[e]=120;}
        }
    }
    move(g,held,pressed);
    for(u16 c=0;c<CP_COINS;c++)if(!(g->coin_mask&(1u<<c))) {
        s32 best=1000000;u16 chosen=CP_NONE;
        for(u16 j=0;j<CP_PLAYERS;j++) {
            u16 i=(g->tie+j)%CP_PLAYERS;CpPlayer *p=&g->player[i];if(!live(p))continue;
            s32 d=distance(p->x,p->y,cp_coins[c].x,cp_coins[c].y);
            if(d<=14*14 && d<best){best=d;chosen=i;}
        }
        if(chosen!=CP_NONE){g->coin_mask|=1u<<c;g->score++;g->player[chosen].gems++;g->tie=(chosen+1)%CP_PLAYERS;g->events|=CP_PICKUP;}
    }
    for(u16 i=0;i<CP_PLAYERS;i++)if(live(&g->player[i]))for(u16 e=0;e<CP_ENEMIES;e++)if(!g->enemy_stun[e]) {
        CpPoint q=cp_enemy(g,e);if(distance(g->player[i].x,g->player[i].y,q.x,q.y)<13*13)cp_damage(g,i);
    }
    if(g->score==CP_COINS){g->won=true;g->events|=CP_CLEAR;}
    cp_camera(g);
}
u16 cp_frame(CpGame *g,const CpInput *in,u16 hz)
{
    g->events=0;if(hz!=50 && hz!=60)return 0;
    u16 held[CP_PLAYERS],pressed[CP_PLAYERS];controls(g,in,held,pressed);
    if(!g->running || g->paused || g->won)return 0;
    g->phase+=60;u16 count=g->phase/hz;g->phase%=hz;
    for(u16 n=0;n<count;n++) {
        tick(g,held,pressed);for(u16 i=0;i<CP_PLAYERS;i++)pressed[i]=0;
        if(g->won)break;
    }
    return count;
}
bool cp_invariant(const CpGame *g)
{
    u16 used=0,lives=0;s16 l=0,r=0,t=0,b=0;
    if(g->camera_x<0 || g->camera_x>CP_WORLD_W-CP_VIEW_W || g->camera_y<0 || g->camera_y>CP_WORLD_H-CP_VIEW_H)return false;
    for(u16 i=0;i<CP_PLAYERS;i++) {
        const CpPlayer *p=&g->player[i];if(p->device==CP_NONE)continue;
        if(p->device>=CP_DEVICES || (used&(1u<<p->device)))return false;
        used|=1u<<p->device;
        if(live(p)) {
            lives++;if(!cp_walkable(p->x,p->y) || p->x<g->camera_x+8 || p->x>g->camera_x+CP_VIEW_W-8 ||
                p->y<g->camera_y+22 || p->y>g->camera_y+CP_VIEW_H-3)return false;
        }
    }
    if(lives && bounds(g,&l,&r,&t,&b) && (r-l>CP_SPAN_X || b-t>CP_SPAN_Y))return false;
    return true;
}
void cp_demo_input(const CpGame *g,u32 frame,CpInput *in)
{
    *in=(CpInput){0};in->valid=15;
    const CpPoint route[8]={{56,112},{56,280},{184,336},{328,336},{456,280},{456,112},{328,64},{184,64}};
    CpPoint goal=route[(frame/150)%8];
    for(u16 i=0;i<4;i++) {
        if(frame==i*90){in->held[i]=CP_START;continue;}
        if(g->player[i].device==CP_NONE)continue;
        s16 x=goal.x+(i&1 ? 12 : -12),y=goal.y+(i&2 ? 10 : -10);
        const CpPlayer *p=&g->player[i];
        if(p->x<x-2)in->held[i]|=CP_RIGHT;else if(p->x>x+2)in->held[i]|=CP_LEFT;
        if(p->y<y-2)in->held[i]|=CP_DOWN;else if(p->y>y+2)in->held[i]|=CP_UP;
        if(frame%120==i*7)in->held[i]|=CP_A;
    }
}
