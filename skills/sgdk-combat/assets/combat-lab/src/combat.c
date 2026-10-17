#include "combat.h"
static const s16 dir_x[4]={0,1,0,-1},dir_y[4]={1,0,-1,0};
static s16 sign(s16 n){return n<0 ? -1 : n>0 ? 1 : 0;}
static s16 absolute(s16 n){return n<0 ? -n : n;}
static u16 windup(const CcActor *a){return a->kind==CC_PLAYER ? 6 : a->kind==CC_SENTINEL ? 24 : 18;}
static u16 recovery(const CcActor *a){return a->kind==CC_PLAYER ? 12 : 20;}
static CcBox body(const CcActor *a){return (CcBox){a->x-6,a->y-16,12,12};}
bool cc_alive(const CcActor *a){return a->active && a->hp && a->state!=CC_DOWN;}
bool cc_overlap(CcBox a,CcBox b){return a.x<b.x+b.w && b.x<a.x+a.w && a.y<b.y+b.h && b.y<a.y+a.h;}
bool cc_walkable(s16 x,s16 y)
{
    if(x<20 || x>300 || y<66 || y>207)return false;
    /* Small static plinths; movement is subdivided into one-pixel axis steps. */
    static const CcBox solids[]={{76,99,24,24},{220,99,24,24}};
    CcBox feet={x-5,y-4,10,8};
    for(u16 i=0;i<2;i++)if(cc_overlap(feet,solids[i]))return false;
    return true;
}
void cc_spawn(CcGame *g,u16 slot,u16 kind,u16 team,s16 x,s16 y)
{
    if(slot>=CC_ACTORS)return;
    u16 generation=g->actor[slot].generation+1;if(!generation)generation=1;
    g->actor[slot]=(CcActor){0};CcActor *a=&g->actor[slot];
    a->generation=generation;a->kind=kind;a->team=team;a->x=x;a->y=y;
    a->hp=kind==CC_PLAYER ? 6 : kind==CC_SENTINEL ? 8 : 4;a->active=true;a->facing=0;
}
void cc_init(CcGame *g){*g=(CcGame){0};g->mode=CC_TITLE;g->rng=0x1042cafe;}
void cc_round(CcGame *g,bool restart)
{
    g->mode=CC_PLAY;g->paused=false;g->hitstop=g->phase=0;
    if(restart)g->wave=0;
    for(u16 i=0;i<CC_ACTORS;i++)g->actor[i].active=false;
    for(u16 i=0;i<CC_SHOTS;i++)g->shot[i].active=false;
    for(u16 i=0;i<CC_FX;i++)g->fx[i].life=0;
    for(u16 p=0;p<2;p++)if(g->joined[p]){cc_spawn(g,p,CC_PLAYER,0,128+p*48,184);g->actor[p].invulnerable=60;}
    cc_spawn(g,2,CC_RAIDER,1,52,88);cc_spawn(g,3,CC_SENTINEL,1,160,82);
    if(g->wave>=1)cc_spawn(g,4,CC_ARCANIST,1,268,88);
    if(g->wave>=2)cc_spawn(g,5,CC_RAIDER,1,48,164);
}
u16 cc_enemies(const CcGame *g){u16 n=0;for(u16 i=2;i<CC_ACTORS;i++)if(cc_alive(&g->actor[i]))n++;return n;}
bool cc_start(CcActor *a,u16 action)
{
    if(!cc_alive(a) || (a->state!=CC_IDLE && a->state!=CC_GUARD))return false;
    if(action!=CC_STRIKE && action!=CC_CAST)return false;
    if(action==CC_CAST && a->bolt_cd)return false;
    a->state=action;a->time=0;a->hit_mask=0;
    if(action==CC_CAST)a->bolt_cd=a->kind==CC_PLAYER ? 75 : 120;
    return true;
}
bool cc_active(const CcActor *a){return cc_alive(a) && a->state==CC_STRIKE && a->time>=windup(a) && a->time<windup(a)+8;}
u16 cc_swing_frame(const CcActor *a){return cc_active(a) ? (a->time-windup(a))/2 : 0;}
CcBox cc_swing_origin(const CcActor *a)
{
    /* Pivot (1,12) in the east-facing 24x24 sheet, rotated around the body. */
    static const s16 px[4]={11,1,12,22},py[4]={1,12,22,11};
    return (CcBox){a->x-px[a->facing]+dir_x[a->facing]*10,a->y-10-py[a->facing]+dir_y[a->facing]*10,24,24};
}
CcBox cc_attack_box(const CcActor *a)
{
    /* The short forearm section bridges the body to the extended blade. */
    static const CcBox east[4]={{-10,0,22,15},{-10,3,34,12},{-10,9,34,12},{-10,9,22,15}};
    CcBox b=east[cc_swing_frame(a)];u16 turns=(5-a->facing)%4;
    for(u16 n=0;n<turns;n++) {
        CcBox next={24-b.y-b.h,b.x,b.h,b.w};b=next;
    }
    CcBox origin=cc_swing_origin(a);b.x+=origin.x;b.y+=origin.y;return b;
}
bool cc_record(CcActor *a,u16 target,u16 generation)
{
    if(target>=CC_ACTORS)return false;
    u16 bit=1u<<target;
    if((a->hit_mask&bit) && a->hit_generation[target]==generation)return false;
    a->hit_mask|=bit;a->hit_generation[target]=generation;return true;
}
static void effect(CcGame *g,const CcActor *a,u16 kind)
{
    u16 slot=0;
    for(u16 i=0;i<CC_FX;i++)if(!g->fx[i].life){slot=i;break;}else if(g->fx[i].life<g->fx[slot].life)slot=i;
    g->fx[slot]=(CcFx){a->x,a->y-10,18,kind};
}
bool cc_resolve(CcGame *g,const CcHit *hits,u16 count)
{
    if(count>CC_HITS)return false; /* Capacity failure is explicit, before mutation. */
    u16 damage[CC_ACTORS]={0},strongest[CC_ACTORS]={0},winner[CC_ACTORS];
    s16 kx[CC_ACTORS]={0},ky[CC_ACTORS]={0};
    for(u16 i=0;i<CC_ACTORS;i++)winner[i]=CC_NONE;
    /* Gather from the same pre-damage snapshot, so equal-tick attacks can trade. */
    for(u16 n=0;n<count;n++) {
        const CcHit *h=&hits[n];if(h->target>=CC_ACTORS || !h->damage || h->damage>8)continue;
        CcActor *t=&g->actor[h->target];
        if(!cc_alive(t) || t->generation!=h->generation || t->invulnerable)continue;
        if(h->source==h->target || (!g->friendly_fire && t->team==h->team))continue;
        if(h->source!=CC_NONE && (h->source>=CC_ACTORS || !cc_alive(&g->actor[h->source])))continue;
        /* dx/dy points from attacker toward defender: front guard faces against it. */
        s32 dot=(s32)dir_x[t->facing]*h->dx+(s32)dir_y[t->facing]*h->dy;
        if(t->state==CC_GUARD && h->guardable && dot<0) {
            g->block_count++;g->events|=CC_BLOCK;effect(g,t,1);if(g->hitstop<2)g->hitstop=2;continue;
        }
        damage[h->target]+=h->damage;g->hit_count++;
        if(h->damage>strongest[h->target] || (h->damage==strongest[h->target] && h->source<winner[h->target])) {
            strongest[h->target]=h->damage;winner[h->target]=h->source;kx[h->target]=sign(h->dx)*3;ky[h->target]=sign(h->dy)*3;
        }
    }
    for(u16 i=0;i<CC_ACTORS;i++)if(damage[i]) {
        CcActor *a=&g->actor[i];a->hp=damage[i]>=a->hp ? 0 : a->hp-damage[i];
        a->state=a->hp ? CC_HURT : CC_DOWN;a->time=0;a->buffer=0;a->knock=a->hp ? 8 : 0;
        a->guard_wait=a->guard_hold=0;
        a->kx=kx[i];a->ky=ky[i];a->invulnerable=a->kind==CC_PLAYER ? 45 : 18;
        if(!a->hp){if(a->kind==CC_PLAYER)a->down=120;else a->active=false;g->events|=CC_FALL;}
        g->events|=CC_IMPACT;effect(g,a,0);if(g->hitstop<4)g->hitstop=4;
    }
    return true;
}
static void walk(CcActor *a,s16 dx,s16 dy,u16 speed)
{
    for(u16 s=0;s<speed;s++) {
        if(cc_walkable(a->x+dx,a->y))a->x+=dx;
        if(cc_walkable(a->x,a->y+dy))a->y+=dy;
    }
}
static void aim(CcActor *a,s16 dx,s16 dy)
{
    if(absolute(dx)>absolute(dy))a->facing=dx<0 ? 3 : 1;
    else if(dy)a->facing=dy<0 ? 2 : 0;
}
static void fire(CcGame *g,u16 slot)
{
    CcActor *a=&g->actor[slot];
    for(u16 i=0;i<CC_SHOTS;i++)if(!g->shot[i].active) {
        s16 dx=dir_x[a->facing],dy=dir_y[a->facing];
        if(a->kind==CC_ARCANIST) {
            u16 target=CC_NONE;s16 nearest=32767;
            for(u16 p=0;p<2;p++)if(cc_alive(&g->actor[p])) {
                s16 d=absolute(a->x-g->actor[p].x)+absolute(a->y-g->actor[p].y);
                if(d<nearest){nearest=d;target=p;}
            }
            if(target!=CC_NONE) {
                s16 vx=g->actor[target].x-a->x,vy=g->actor[target].y-a->y;
                dx=absolute(vx)*2<absolute(vy) ? 0 : sign(vx);
                dy=absolute(vy)*2<absolute(vx) ? 0 : sign(vy);
            }
        }
        s16 speed=dx && dy ? 2 : 3;
        g->shot[i]=(CcShot){a->x+dx*12,a->y-10+dy*12,dx*speed,dy*speed,slot,a->generation,a->team,90,true};
        g->events|=CC_FIRE;break; /* Exhaustion loses this cosmetic/gameplay shot, no heap fallback. */
    }
}
static bool enemy_guard(CcGame *g,u16 slot)
{
    CcActor *a=&g->actor[slot];
    if(a->kind==CC_ARCANIST)return false;
    if(a->guard_wait) {
        if(!--a->guard_wait){a->guard_hold=18;a->state=CC_GUARD;}
        return true;
    }
    if(a->guard_hold){a->guard_hold--;a->state=CC_GUARD;return true;}
    if(a->guard_cd || (g->ticks+slot*3)%12)return false;
    for(u16 i=0;i<CC_SHOTS;i++) {
        const CcShot *s=&g->shot[i];if(!s->active || s->team==a->team)continue;
        s16 dx=a->x-s->x,dy=a->y-10-s->y;
        if(absolute(dx)+absolute(dy)>110 || (s32)dx*s->dx+(s32)dy*s->dy<=0)continue;
        bool lane=s->dx && s->dy ? absolute(absolute(dx)-absolute(dy))<22 : s->dx ? absolute(dy)<22 : absolute(dx)<22;
        if(!lane)continue;
        a->guard_cd=75;g->rng=g->rng*1664525u+1013904223u;
        u16 chance=a->kind==CC_SENTINEL ? 2 : 1;
        if((g->rng>>30)<chance){aim(a,-s->dx,-s->dy);a->guard_wait=6;a->state=CC_IDLE;return true;}
        break;
    }
    return false;
}
static void actors(CcGame *g,const CcInput *in)
{
    for(u16 i=0;i<CC_ACTORS;i++) {
        CcActor *a=&g->actor[i];if(!a->active)continue;
        if(a->state==CC_DOWN) {
            if(a->down && !--a->down){cc_spawn(g,i,CC_PLAYER,0,128+i*48,184);a->invulnerable=90;}
            continue;
        }
        if(a->invulnerable)a->invulnerable--;
        if(a->bolt_cd)a->bolt_cd--;
        if(a->guard_cd)a->guard_cd--;
        bool buffered=a->buffer!=0;if(buffered)a->buffer--;
        if(a->knock){walk(a,sign(a->kx),sign(a->ky),3);a->knock--;}
        if(a->state==CC_HURT){if(++a->time>=18){a->state=CC_IDLE;a->time=0;}continue;}
        if(a->state==CC_STRIKE || a->state==CC_CAST) {
            a->time++;
            if(a->state==CC_CAST && a->time==8)fire(g,i);
            u16 duration=a->state==CC_CAST ? 24 : windup(a)+8+recovery(a);
            if(a->time>=duration){a->state=CC_IDLE;a->time=0;}
            continue;
        }
        if(i<2) {
            u16 held=in->held[i];s16 dx=((held&CC_RIGHT)!=0)-((held&CC_LEFT)!=0),dy=((held&CC_DOWN_KEY)!=0)-((held&CC_UP)!=0);
            if(dx || dy)aim(a,dx,dy);
            a->state=(held&CC_B) ? CC_GUARD : CC_IDLE;
            walk(a,dx,dy,a->state==CC_GUARD ? 1 : 2);
            if(buffered && cc_start(a,CC_STRIKE)){a->buffer=0;g->events|=CC_SWING;}
        } else {
            if(enemy_guard(g,i))continue;
            u16 target=CC_NONE;s16 nearest=32767;
            for(u16 p=0;p<2;p++)if(cc_alive(&g->actor[p])) {
                s16 d=absolute(a->x-g->actor[p].x)+absolute(a->y-g->actor[p].y);
                if(d<nearest){nearest=d;target=p;}
            }
            if(target==CC_NONE)continue;
            s16 dx=g->actor[target].x-a->x,dy=g->actor[target].y-a->y;aim(a,dx,dy);
            if(a->kind==CC_ARCANIST) {
                if(nearest>120 && !(g->ticks&1))walk(a,sign(dx),sign(dy),1);
                else if(nearest<56 && !(g->ticks&1))walk(a,-sign(dx),-sign(dy),1);
                else if(!a->bolt_cd && nearest<160)cc_start(a,CC_CAST);
            } else if(nearest<34){cc_start(a,CC_STRIKE);g->events|=CC_SWING;}
            else if(a->kind==CC_SENTINEL && (g->ticks%80)<32)a->state=CC_GUARD;
            else {a->state=CC_IDLE;if(!(g->ticks&1))walk(a,sign(dx),sign(dy),1);}
        }
    }
}
static void contacts(CcGame *g)
{
    CcHit hits[CC_HITS];u16 count=0;
    for(u16 i=0;i<CC_ACTORS;i++)if(cc_active(&g->actor[i])) {
        CcActor *a=&g->actor[i];CcBox attack=cc_attack_box(a);
        for(u16 j=0;j<CC_ACTORS;j++)if(j!=i && cc_alive(&g->actor[j])) {
            CcActor *t=&g->actor[j];if(!g->friendly_fire && t->team==a->team)continue;
            if(cc_overlap(attack,body(t)) && cc_record(a,j,t->generation)) {
                hits[count++]=(CcHit){i,j,t->generation,a->team,2,t->x-a->x,t->y-a->y,a->kind!=CC_SENTINEL};
            }
        }
    }
    for(u16 i=0;i<CC_SHOTS;i++)if(g->shot[i].active) {
        CcShot *s=&g->shot[i];
        if(!s->life || s->owner>=CC_ACTORS || g->actor[s->owner].generation!=s->generation){s->active=false;continue;}
        s->life--;s->x+=s->dx;s->y+=s->dy;
        if(!cc_walkable(s->x,s->y+10)){s->active=false;continue;}
        CcBox shot={s->x-3,s->y-3,6,6};
        for(u16 j=0;j<CC_ACTORS;j++)if(j!=s->owner && cc_alive(&g->actor[j])) {
            CcActor *t=&g->actor[j];if(!g->friendly_fire && t->team==s->team)continue;
            if(cc_overlap(shot,body(t))) {
                u16 owner=cc_alive(&g->actor[s->owner]) ? s->owner : CC_NONE;
                hits[count++]=(CcHit){owner,j,t->generation,s->team,1,s->dx,s->dy,true};s->active=false;break;
            }
        }
    }
    /* Maximum: 6*5 melee contacts + 4 single-hit shots = 34, below 40. */
    cc_resolve(g,hits,count);
}
static void tick(CcGame *g,const CcInput *input)
{
    g->ticks++;
    if(g->hitstop){g->hitstop--;return;}
    for(u16 i=0;i<CC_FX;i++)if(g->fx[i].life)g->fx[i].life--;
    actors(g,input);contacts(g);
    bool living=false;for(u16 p=0;p<2;p++)if(cc_alive(&g->actor[p]))living=true;
    if(!living)g->mode=CC_LOST;
    else if(!cc_enemies(g)){g->mode=g->wave==2 ? CC_COMPLETE : CC_CLEAR;g->events|=CC_WIN;}
}
u16 cc_frame(CcGame *g,const CcInput *input,u16 hz)
{
    g->events=0;if(hz!=50 && hz!=60)return 0;
    u16 edge[2];bool toggle=false,advance=false;
    for(u16 p=0;p<2;p++){edge[p]=input->held[p]&~g->previous[p];g->previous[p]=input->held[p];}
    for(u16 p=0;p<2;p++) {
        if((edge[p]&CC_START) && !g->joined[p] && (g->mode==CC_PLAY || g->mode==CC_TITLE)) {
            g->joined[p]=true;g->events|=CC_JOIN;
            if(g->mode==CC_TITLE)cc_round(g,true);else {cc_spawn(g,p,CC_PLAYER,0,128+p*48,184);g->actor[p].invulnerable=90;}
            edge[p]&=~CC_START;
        }
        if(edge[p]&CC_START) {if(g->mode==CC_PLAY)toggle=true;else advance=true;}
    }
    if(g->mode==CC_TITLE && (edge[0]&CC_A)){g->joined[0]=true;cc_round(g,true);edge[0]=0;}
    if(advance) {
        if(g->mode==CC_CLEAR){g->wave++;cc_round(g,false);}
        else if(g->mode==CC_LOST)cc_round(g,false);
        else if(g->mode==CC_COMPLETE){cc_init(g);g->previous[0]=input->held[0];g->previous[1]=input->held[1];}
    }
    if(toggle)g->paused=!g->paused;
    if(g->mode!=CC_PLAY || g->paused)return 0;
    for(u16 p=0;p<2;p++)if(g->joined[p] && cc_alive(&g->actor[p])) {
        if(edge[p]&CC_A)g->actor[p].buffer=8;
        if((edge[p]&CC_C) && !g->hitstop)cc_start(&g->actor[p],CC_CAST);
    }
    g->phase+=60;u16 steps=g->phase/hz;g->phase%=hz;u16 ran=0;
    for(u16 n=0;n<steps && g->mode==CC_PLAY;n++){tick(g,input);ran++;}
    return ran;
}
bool cc_invariant(const CcGame *g)
{
    if(g->wave>2 || g->mode>CC_COMPLETE || g->phase>=60)return false;
    for(u16 i=0;i<CC_ACTORS;i++) {
        const CcActor *a=&g->actor[i];if(!a->active)continue;
        if(!a->generation || a->facing>3 || a->state>CC_DOWN || a->hp>8 || !cc_walkable(a->x,a->y))return false;
        if(a->state==CC_DOWN && a->hp)return false;
    }
    for(u16 i=0;i<CC_SHOTS;i++)if(g->shot[i].active && (g->shot[i].owner>=CC_ACTORS || g->shot[i].life>90))return false;
    return true;
}
