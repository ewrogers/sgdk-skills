#include "combat.h"
#define CHECK(x) do{if(!(x))return __LINE__;}while(0)
static CcInput none(void){return (CcInput){{0,0}};}
static void setup(CcGame *g)
{
    cc_init(g);g->mode=CC_PLAY;g->joined[0]=true;
    cc_spawn(g,0,CC_PLAYER,0,120,160);cc_spawn(g,2,CC_RAIDER,1,140,160);
}
static CcHit hit(const CcGame *g,u16 source,u16 target,u16 damage)
{
    const CcActor *a=&g->actor[source],*b=&g->actor[target];
    return (CcHit){source,target,b->generation,a->team,damage,b->x-a->x,b->y-a->y,true};
}
static int check_damage(void)
{
    CcGame g;setup(&g);CcHit h=hit(&g,0,2,2);
    CHECK(cc_resolve(&g,&h,1) && g.actor[2].hp==2 && g.actor[2].state==CC_HURT && g.actor[2].knock==8);
    CHECK(g.hit_count==1 && g.hitstop==4);CHECK(cc_resolve(&g,&h,1) && g.actor[2].hp==2);
    setup(&g);g.actor[2].invulnerable=1;h=hit(&g,0,2,2);
    CHECK(cc_resolve(&g,&h,1) && g.actor[2].hp==4);
    CcInput in=none();cc_frame(&g,&in,60);CHECK(!g.actor[2].invulnerable);
    CHECK(cc_resolve(&g,&h,1) && g.actor[2].hp==2);
    g.actor[2].invulnerable=0;h.generation++;CHECK(cc_resolve(&g,&h,1) && g.actor[2].hp==2);
    setup(&g);h=hit(&g,0,2,2);h.team=1;CHECK(cc_resolve(&g,&h,1) && g.actor[2].hp==4);
    g.friendly_fire=true;CHECK(cc_resolve(&g,&h,1) && g.actor[2].hp==2);
    setup(&g);h=hit(&g,0,2,8);CHECK(cc_resolve(&g,&h,1) && !g.actor[2].hp && !g.actor[2].active);
    CHECK(!cc_resolve(&g,&h,CC_HITS+1));
    return 0;
}
static int check_guard(void)
{
    CcGame g;setup(&g);CcHit h=hit(&g,0,2,2);g.actor[2].state=CC_GUARD;g.actor[2].facing=3;
    CHECK(cc_resolve(&g,&h,1) && g.actor[2].hp==4 && g.block_count==1 && !g.hit_count);
    g.actor[2].facing=1;CHECK(cc_resolve(&g,&h,1) && g.actor[2].hp==2); /* Back exposed. */
    setup(&g);g.actor[2].state=CC_GUARD;g.actor[2].facing=3;h=hit(&g,0,2,2);h.guardable=false;
    CHECK(cc_resolve(&g,&h,1) && g.actor[2].hp==2 && !g.block_count);
    /* Exact side contact is not a front block. */
    setup(&g);g.actor[2].state=CC_GUARD;g.actor[2].facing=0;h=hit(&g,0,2,2);
    CHECK(cc_resolve(&g,&h,1) && g.actor[2].hp==2);
    return 0;
}
static int check_trades(void)
{
    CcGame a,b;setup(&a);a.actor[0].hp=2;a.actor[2].hp=2;b=a;
    CcHit hits[2]={hit(&a,0,2,2),hit(&a,2,0,2)};
    CHECK(cc_resolve(&a,hits,2));CcHit swap=hits[0];hits[0]=hits[1];hits[1]=swap;CHECK(cc_resolve(&b,hits,2));
    CHECK(!a.actor[0].hp && !a.actor[2].hp && a.actor[0].state==CC_DOWN && a.hit_count==2);
    CHECK(a.actor[0].hp==b.actor[0].hp && a.actor[2].hp==b.actor[2].hp);
    setup(&a);cc_spawn(&a,3,CC_RAIDER,1,100,160);a.actor[0].hp=6;
    hits[0]=hit(&a,2,0,2);hits[1]=hit(&a,3,0,1);b=a;
    CHECK(cc_resolve(&a,hits,2) && a.actor[0].hp==3 && a.actor[0].kx==-3);
    swap=hits[0];hits[0]=hits[1];hits[1]=swap;CHECK(cc_resolve(&b,hits,2));
    CHECK(b.actor[0].kx==a.actor[0].kx && b.actor[0].hp==a.actor[0].hp);
    return 0;
}
static int check_identity(void)
{
    CcGame g;setup(&g);CcActor *a=&g.actor[0];u16 gen=g.actor[2].generation;
    CHECK(cc_start(a,CC_STRIKE) && !cc_active(a));
    for(u16 t=0;t<26;t++){a->time=t;CHECK(cc_active(a)==(t>=6 && t<14));}
    CHECK(cc_record(a,2,gen) && !cc_record(a,2,gen));
    cc_spawn(&g,2,CC_RAIDER,1,140,160);CHECK(g.actor[2].generation!=gen);
    CHECK(cc_record(a,2,g.actor[2].generation));
    a->state=CC_HURT;CHECK(!cc_start(a,CC_STRIKE) && !cc_active(a));
    a->state=CC_IDLE;CHECK(cc_start(a,CC_STRIKE) && !a->hit_mask && a->time==0);
    for(u16 f=0;f<4;f++){a->facing=f;CcBox b=cc_attack_box(a);CHECK(b.w>0 && b.h>0);}
    CHECK(!cc_overlap((CcBox){0,0,4,4},(CcBox){4,0,4,4}));
    return 0;
}
static int check_clock_and_join(void)
{
    CcGame a,b;cc_init(&a);CcInput in=none();in.held[0]=CC_START;
    cc_frame(&a,&in,60);CHECK(a.mode==CC_PLAY && a.joined[0] && !a.paused);
    in=none();cc_frame(&a,&in,60);in.held[0]=CC_START;cc_frame(&a,&in,60);CHECK(a.paused);
    in.held[1]=CC_START;cc_frame(&a,&in,60);CHECK(a.joined[1] && a.paused);
    in=none();cc_frame(&a,&in,60);in.held[0]=in.held[1]=CC_START;cc_frame(&a,&in,60);CHECK(!a.paused);
    a.hitstop=4;a.actor[0].bolt_cd=20;u16 cooldown=a.actor[0].bolt_cd;in=none();
    for(u16 i=0;i<4;i++)cc_frame(&a,&in,60);
    CHECK(!a.hitstop && a.actor[0].bolt_cd==cooldown);
    /* Pause also freezes hitstop; A can be buffered while hitstop runs. */
    a.hitstop=3;a.paused=true;cc_frame(&a,&in,60);CHECK(a.hitstop==3);
    a.paused=false;in.held[0]=CC_A;cc_frame(&a,&in,60);CHECK(a.actor[0].buffer==8);
    setup(&a);a.actor[2].x=260;a.actor[2].y=80;a.actor[0].invulnerable=1000;b=a;in=none();
    for(u16 n=0;n<500;n++)cc_frame(&a,&in,50);
    for(u16 n=0;n<600;n++)cc_frame(&b,&in,60);
    CHECK(a.ticks==600 && a.ticks==b.ticks && a.actor[0].hp==b.actor[0].hp && a.actor[2].x==b.actor[2].x && a.mode==b.mode);
    u32 ticks=a.ticks;CHECK(!cc_frame(&a,&in,0) && a.ticks==ticks);
    return 0;
}
static int check_arc_contacts(void)
{
    const s16 dx[4]={0,1,0,-1},dy[4]={1,0,-1,0};
    for(u16 facing=0;facing<4;facing++)for(u16 reach=20;reach<=32;reach+=12) {
        CcGame g;setup(&g);g.actor[0].x=160;g.actor[0].y=160;g.actor[0].facing=facing;
        g.actor[2].x=160+dx[facing]*reach;g.actor[2].y=160+dy[facing]*reach;
        CHECK(cc_start(&g.actor[0],CC_STRIKE));g.actor[0].time=6;CcInput in=none();
        for(u16 n=0;n<12;n++)cc_frame(&g,&in,60);
        CHECK(g.actor[2].hp==2 && g.hit_count==1 && g.actor[0].hp==6);
    }
    return 0;
}
static int check_enemy_bolt_guard(void)
{
    u16 reacted=0,missed=0;
    for(u16 seed=0;seed<32;seed++) {
        CcGame g;setup(&g);g.rng=(u32)seed*0x1020304u;g.ticks=5;
        g.actor[0].x=40;g.actor[2].x=160;
        g.shot[0]=(CcShot){80,150,3,0,0,g.actor[0].generation,0,90,true};
        CcInput in=none();cc_frame(&g,&in,60);
        CHECK(g.actor[2].guard_cd==75);
        if(g.actor[2].guard_wait) {
            reacted++;CHECK(g.actor[2].guard_wait==6 && g.actor[2].state==CC_IDLE);
            for(u16 n=0;n<5;n++)cc_frame(&g,&in,60);
            CHECK(g.actor[2].state!=CC_GUARD);
            cc_frame(&g,&in,60);CHECK(g.actor[2].state==CC_GUARD && g.actor[2].facing==3);
        } else {
            missed++;cc_frame(&g,&in,60);CHECK(!g.actor[2].guard_wait && g.actor[2].guard_cd==74);
        }
    }
    CHECK(reacted && missed);return 0;
}
static int check_buffers_and_pool(void)
{
    CcGame g;setup(&g);g.actor[2].x=260;g.actor[2].y=80;CcInput in=none();
    CHECK(cc_start(&g.actor[0],CC_STRIKE));in.held[0]=CC_A;cc_frame(&g,&in,60);in=none();
    for(u16 i=0;i<8;i++)cc_frame(&g,&in,60);
    CHECK(!g.actor[0].buffer);for(u16 i=0;i<20;i++)cc_frame(&g,&in,60);
    CHECK(g.actor[0].state==CC_IDLE); /* Old input did not become a later combo. */
    CHECK(cc_start(&g.actor[0],CC_CAST));
    for(u16 i=0;i<CC_SHOTS;i++)g.shot[i]=(CcShot){40,190,0,0,0,g.actor[0].generation,0,90,true};
    for(u16 i=0;i<8;i++)cc_frame(&g,&in,60);
    u16 count=0;for(u16 i=0;i<CC_SHOTS;i++)if(g.shot[i].active)count++;CHECK(count==CC_SHOTS);
    g.actor[0].generation++;cc_frame(&g,&in,60);
    for(u16 i=0;i<CC_SHOTS;i++)CHECK(!g.shot[i].active); /* Stale pooled owner. */
    return 0;
}
static u32 digest(const CcGame *g)
{
    u32 h=2166136261u;
#define MIX(v) do{h=(h^(u32)(v))*16777619u;}while(0)
    MIX(g->ticks);MIX(g->mode);MIX(g->wave);MIX(g->hit_count);MIX(g->block_count);MIX(g->hitstop);
    for(u16 i=0;i<CC_ACTORS;i++){const CcActor *a=&g->actor[i];MIX(a->x);MIX(a->y);MIX(a->hp);MIX(a->state);MIX(a->time);MIX(a->generation);MIX(a->invulnerable);MIX(a->buffer);}
    for(u16 i=0;i<CC_SHOTS;i++){MIX(g->shot[i].active);MIX(g->shot[i].x);MIX(g->shot[i].y);MIX(g->shot[i].life);}
#undef MIX
    return h;
}
static int check_soak(u16 frames,u32 *trace)
{
    CcGame g;cc_init(&g);CcInput in={{CC_START,CC_START}};cc_frame(&g,&in,60);u32 seed=0x14c0ffee;
    for(u16 n=0;n<frames;n++) {
        for(u16 p=0;p<2;p++){seed=seed*1664525u+1013904223u;in.held[p]=(seed>>24)&127;}
        if(g.mode!=CC_PLAY && n%8==0)in.held[0]|=CC_START;
        cc_frame(&g,&in,n&1 ? 50 : 60);CHECK(cc_invariant(&g));
    }
    *trace=digest(&g);return 0;
}
static int contracts(void)
{
    int e=check_damage();if(!e)e=check_guard();if(!e)e=check_trades();if(!e)e=check_identity();
    if(!e)e=check_clock_and_join();
    if(!e)e=check_buffers_and_pool();
    if(!e)e=check_arc_contacts();
    if(!e)e=check_enemy_bolt_guard();
    return e;
}
