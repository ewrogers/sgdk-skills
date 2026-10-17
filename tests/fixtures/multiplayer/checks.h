#include "coop.h"
#define CHECK(x) do{if(!(x))return __LINE__;}while(0)
static CpInput empty_input(void){return (CpInput){{0},255};}
static void join_four(CpGame *g)
{
    cp_init(g);CpInput in=empty_input();for(u16 i=0;i<4;i++)in.held[i]=CP_START;
    cp_frame(g,&in,60);in=empty_input();cp_frame(g,&in,60);
}
static int check_routes(void)
{
    CpPlan p=cp_plan(CP_PORT_PAD,CP_PORT_PAD);CHECK(p.mask==3 && p.reader[0]==CP_READER_PAD && p.reader[1]==CP_READER_PAD);
    p=cp_plan(CP_PORT_TEAM,CP_PORT_PAD);CHECK(p.mask==0x1f && p.reader[0]==CP_READER_TEAM);
    p=cp_plan(CP_PORT_PAD,CP_PORT_TEAM);CHECK(p.mask==0xe3 && p.reader[1]==CP_READER_TEAM);
    p=cp_plan(CP_PORT_TEAM,CP_PORT_TEAM);CHECK(p.mask==255);
    p=cp_plan(CP_PORT_EA,CP_PORT_EA);CHECK(p.mask==0x1d && p.reader[0]==CP_READER_EA && p.reader[1]==CP_READER_OFF);
    p=cp_plan(CP_PORT_OTHER,CP_PORT_OTHER);CHECK(!p.mask && !p.reader[0] && !p.reader[1]);
    CpGame g;cp_init(&g);CpInput in=empty_input();in.held[7]=CP_START;cp_seed_inputs(&g,&in);
    cp_frame(&g,&in,60);CHECK(cp_count(&g)==0);
    in.held[7]=0;cp_frame(&g,&in,60);in.held[7]=CP_START;cp_frame(&g,&in,60);
    CHECK(g.player[0].device==7 && !g.paused && cp_invariant(&g));
    in.held[2]=CP_START;cp_frame(&g,&in,60);CHECK(g.player[1].device==2 && !g.paused);
    in.valid=0;cp_frame(&g,&in,60);CHECK(cp_count(&g)==2); /* quiet/unknown is not Leave */
    return 0;
}
static int check_session(void)
{
    CpGame g;cp_init(&g);CpInput in=empty_input();for(u16 d=0;d<8;d++)in.held[d]=CP_START;
    cp_frame(&g,&in,60);CHECK(cp_count(&g)==4 && !g.paused);
    for(u16 i=0;i<4;i++)CHECK(g.player[i].device==i);
    in.held[1]=CP_A|CP_C;cp_frame(&g,&in,60);in.held[1]|=CP_START;cp_frame(&g,&in,60);
    CHECK(cp_count(&g)==3 && g.player[1].device==CP_NONE && !g.paused);
    cp_frame(&g,&in,60);CHECK(cp_count(&g)==3); /* held rejected starts do not fill a vacancy */
    in.held[6]=0;cp_frame(&g,&in,60);in.held[6]=CP_START;cp_frame(&g,&in,60);
    CHECK(g.player[1].device==6 && g.player[2].device==2 && !g.paused);
    in=empty_input();cp_frame(&g,&in,60);in.held[0]=in.held[2]=CP_START;cp_frame(&g,&in,60);
    CHECK(g.paused);u32 tick=g.ticks;cp_frame(&g,&in,60);CHECK(g.paused && g.ticks==tick);
    in=empty_input();cp_frame(&g,&in,60);in.held[0]=CP_START;cp_frame(&g,&in,60);CHECK(!g.paused);
    in=empty_input();cp_frame(&g,&in,60);
    for(u16 d=0;d<8;d++)in.held[d]=CP_A|CP_C|CP_START;
    /* Unassigned devices would join: explicitly exclude them from this leave snapshot. */
    in.valid=(1u<<0)|(1u<<6)|(1u<<2)|(1u<<3);cp_frame(&g,&in,60);
    CHECK(cp_count(&g)==0 && !g.running && !g.paused);
    in=empty_input();cp_frame(&g,&in,60);in.held[5]=CP_START;cp_frame(&g,&in,60);
    CHECK(g.player[0].device==5 && g.score==0 && !g.won && cp_invariant(&g));
    return 0;
}
static int check_respawn(void)
{
    CpGame g;join_four(&g);CpInput in=empty_input();
    g.player[0].invulnerable=0;CHECK(cp_damage(&g,0));CHECK(!cp_damage(&g,0));
    for(u16 hit=0;hit<2;hit++){g.player[0].invulnerable=0;CHECK(cp_damage(&g,0));}
    CHECK(g.player[0].respawn==120 && g.player[0].hp==0 && g.player[0].deaths==1);
    g.paused=true;for(u16 n=0;n<20;n++)cp_frame(&g,&in,60);CHECK(g.player[0].respawn==120);
    g.paused=false;u16 x=g.player[1].x;in.held[1]=CP_RIGHT;
    cp_frame(&g,&in,60);CHECK(g.player[1].x>x && g.player[0].respawn==119);
    in=empty_input();for(u16 n=0;n<118;n++)cp_frame(&g,&in,60);CHECK(g.player[0].respawn==1 && !g.player[0].hp);
    cp_frame(&g,&in,60);CHECK(!g.player[0].respawn && g.player[0].hp==3 && g.player[0].device==0 && g.player[0].invulnerable);
    CHECK(g.events&CP_RESPAWN);CHECK(cp_invariant(&g));
    /* Remote return uses the group, rather than teleporting the camera to the entry. */
    for(u16 i=0;i<4;i++){g.player[i].x=408+i*16;g.player[i].y=320;g.player[i].invulnerable=90;}
    g.player[0].hp=0;g.player[0].respawn=1;cp_camera(&g);cp_frame(&g,&in,60);
    CHECK(g.player[0].x>320 && cp_invariant(&g));
    for(u16 i=0;i<4;i++){g.player[i].hp=0;g.player[i].respawn=1;}
    cp_frame(&g,&in,60);CHECK(cp_invariant(&g));
    for(u16 i=0;i<4;i++)CHECK(g.player[i].hp==3 && g.player[i].y<112);
    return 0;
}
static int check_camera(void)
{
    CpGame g;cp_init(&g);CpInput in=empty_input();in.held[0]=in.held[1]=CP_START;cp_frame(&g,&in,60);
    for(u16 swap=0;swap<2;swap++) {
        g.player[swap].x=88;g.player[1-swap].x=344;g.player[0].y=g.player[1].y=56;cp_camera(&g);
        in=empty_input();in.held[swap]=CP_LEFT;in.held[1-swap]=CP_RIGHT;cp_frame(&g,&in,60);
        CHECK(g.player[swap].x==88 && g.player[1-swap].x==344 && g.tethered && cp_invariant(&g));
        in.held[swap]=CP_RIGHT;in.held[1-swap]=CP_LEFT;cp_frame(&g,&in,60);
        CHECK(g.player[swap].x==90 && g.player[1-swap].x==342 && cp_invariant(&g));
    }
    CHECK(!cp_walkable(0,0) && !cp_walkable(120,96) && cp_walkable(56,56));
    g.player[0].x=104;g.player[0].y=88;g.player[1].x=88;g.player[1].y=56;cp_camera(&g);
    in=empty_input();in.held[0]=CP_RIGHT|CP_B;cp_frame(&g,&in,60);
    for(u16 n=0;n<12;n++)cp_frame(&g,&in,60);
    CHECK(g.player[0].x<=106 && cp_walkable(g.player[0].x,g.player[0].y) && cp_invariant(&g));
    return 0;
}
static int check_pickups_and_pause(void)
{
    CpGame g;join_four(&g);CpInput in=empty_input();
    for(u16 c=0;c<4;c++) {
        for(u16 i=0;i<4;i++){g.player[i].x=cp_coins[c].x;g.player[i].y=cp_coins[c].y;}
        cp_camera(&g);cp_frame(&g,&in,60);CHECK(g.score==c+1 && g.player[c].gems==1);
        cp_frame(&g,&in,60);CHECK(g.score==c+1);
    }
    for(u16 i=0;i<4;i++)CHECK(g.player[i].gems==1);
    /* A late join consumes Start while the session remains paused. */
    cp_init(&g);in.held[0]=CP_START;cp_frame(&g,&in,60);in.held[0]=0;cp_frame(&g,&in,60);
    in.held[0]=CP_START;cp_frame(&g,&in,60);CHECK(g.paused);
    in.held[1]=CP_START;cp_frame(&g,&in,60);CHECK(cp_count(&g)==2 && g.paused);
    return 0;
}
static u32 digest(const CpGame *g)
{
    u32 h=2166136261u;
#define MIX(v) do{h=(h^(u32)(v))*16777619u;}while(0)
    MIX(g->ticks);MIX(g->score);MIX(g->coin_mask);MIX(g->camera_x);MIX(g->camera_y);MIX(g->paused);MIX(g->won);
    for(u16 i=0;i<4;i++) {
        const CpPlayer *p=&g->player[i];MIX(p->device);MIX(p->x);MIX(p->y);MIX(p->hp);MIX(p->respawn);MIX(p->invulnerable);MIX(p->gems);MIX(p->deaths);
    }
#undef MIX
    return h;
}
static int check_timing(void)
{
    CpGame a,b;join_four(&a);b=a;CpInput in=empty_input();
    for(u16 n=0;n<500;n++)cp_frame(&a,&in,50);
    for(u16 n=0;n<600;n++)cp_frame(&b,&in,60);
    CHECK(digest(&a)==digest(&b) && a.ticks==602 && !a.phase && !b.phase);
    u32 tick=a.ticks;CHECK(cp_frame(&a,&in,0)==0 && a.ticks==tick);
    return 0;
}
static int check_soak(u16 count,u32 *trace)
{
    CpGame g;join_four(&g);u32 seed=0x12ab34cd;
    for(u16 n=0;n<count;n++) {
        CpInput in=empty_input();
        for(u16 d=0;d<8;d++) {
            seed=seed*1664525u+1013904223u;
            in.held[d]=(u16)((seed>>24)&127);
            if(n%137==d)in.held[d]|=CP_START;
        }
        cp_frame(&g,&in,n&1 ? 50 : 60);CHECK(cp_invariant(&g));
    }
    *trace=digest(&g);return 0;
}
