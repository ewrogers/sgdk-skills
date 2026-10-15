#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "game.h"

static Game g;
static void fresh(void)
{
    game_init(&g); g.teleport=false; g.player.invulnerable=0;
    for (u16 i=0;i<ENEMY_COUNT;i++) g.enemies[i].active=false;
}
static void place(s16 x,s16 y,bool ground)
{
    g.player.body=(Body){.x=x*UNIT,.y=y*UNIT,.width=12,.height=20,.ground=ground,.support=-1};
    g.teleport=false; g.player.coyote=0; g.player.variable_jump=false;
}
static void ticks(u16 held,u16 count)
{
    while (count--) game_frame(&g,held,60);
}
static void geometry(void)
{
    fresh();
    Body body={.x=32*UNIT,.y=236*UNIT,.width=12,.height=20};
    assert(!game_blocked(&g,&body,body.x,body.y));
    assert(game_blocked(&g,&body,body.x,body.y+UNIT));
    game_move(&g,&body,0,8*UNIT,true,false,0);
    assert(body.ground && body.y==236*UNIT);
    body=(Body){.x=590*UNIT,.y=160*UNIT,.width=12,.height=20};
    game_move(&g,&body,8*UNIT,0,false,false,0);
    assert(body.wall==1 && body.x+12*UNIT<=608*UNIT);
    assert(!game_blocked(&g,&body,body.x,body.y));
    Body touching=body; touching.x+=body.width*UNIT;
    assert(!game_overlap(&body,&touching)); touching.x--;
    assert(game_overlap(&body,&touching));
    assert(game_original_tile(-1,0)==SOLID && game_original_tile(1,18)==EMPTY);
}
static void slopes(void)
{
    fresh();
    Body b={.x=360*UNIT,.y=236*UNIT,.width=12,.height=20,.ground=true};
    s32 previous=b.y;
    for (u16 i=0;i<70;i++)
    {
        game_move(&g,&b,UNIT,GRAVITY,true,false,0);
        assert(!game_blocked(&g,&b,b.x,b.y));
        assert(b.ground);
        assert(b.y<=previous+UNIT); previous=b.y;
    }
    assert(b.y==188*UNIT);
    b.x=474*UNIT; b.y=188*UNIT; b.ground=true;
    for (u16 i=0;i<60;i++)
    {
        game_move(&g,&b,UNIT,GRAVITY,true,false,0);
        assert(!game_blocked(&g,&b,b.x,b.y)); assert(b.ground);
    }
    assert(b.y<=236*UNIT && b.y>235*UNIT);
}
static void one_way(void)
{
    fresh(); place(116,210,false);
    g.player.body.vy=-5*UNIT;
    bool passed=false;
    for (u16 i=0;i<45;i++)
    {
        ticks(A,1);
        if (g.player.body.y+20*UNIT<208*UNIT) passed=true;
    }
    assert(passed && g.player.body.ground && g.player.body.y==188*UNIT);
    ticks(0,1); ticks(DOWN|A,1);
    assert(g.player.drop && !g.player.body.ground && g.player.body.y>188*UNIT);
    ticks(0,40); assert(g.player.body.y<=236*UNIT && g.player.body.y>235*UNIT && g.player.body.ground);
}
static void jumps(void)
{
    fresh(); place(40,236,true); ticks(A,1);
    s32 high=g.player.body.y;
    for (u16 i=0;i<25;i++) { ticks(A,1); if (g.player.body.y<high) high=g.player.body.y; }
    assert(high<180*UNIT && high>150*UNIT);
    fresh(); place(40,236,true); ticks(A,1); ticks(0,25);
    assert(g.player.body.y>high+20*UNIT);
    fresh(); place(40,200,false); g.player.coyote=3; ticks(A,1);
    assert(g.player.body.vy<0 && (g.events&EVENT_JUMP));
    fresh(); place(40,200,false); ticks(A,1);
    assert(g.player.body.vy>0 && g.player.buffer);
    fresh(); place(40,233,false); g.player.body.vy=3*UNIT; ticks(A,1);
    assert(g.player.body.vy<0 && !g.player.body.ground);
    fresh(); place(595,180,false); g.player.body.wall=1; ticks(A,1);
    assert(g.player.body.vx<0 && g.player.body.vy<0 && g.player.lock);
    fresh(); place(540,170,false); g.player.boots=true; g.player.air_jump=true;
    ticks(A,1); assert(!g.player.air_jump && g.player.body.vy<0);
    ticks(0,1); s32 velocity=g.player.body.vy; ticks(A,1);
    assert(g.player.body.vy>velocity); /* No third jump. */
}
static void platform(void)
{
    fresh(); place(770,188,true); g.player.body.support=0;
    ticks(0,1); assert(g.player.body.x==771*UNIT && g.player.body.ground && g.player.body.support==0);
    ticks(0,40); assert(g.player.body.x==811*UNIT && g.player.body.ground);
    ticks(A,1); assert(g.player.body.vy<0 && g.player.body.support==-1);
    fresh(); place(770,188,true); g.player.body.support=0;
    ticks(DOWN|A,1); assert(g.player.drop && !g.player.body.ground);
    /* A static platform at the same height must not claim moving support. */
    Body b={.x=116*UNIT,.y=188*UNIT,.width=12,.height=20};
    game_move(&g,&b,0,UNIT,true,false,0); assert(b.ground && b.support==-1);
}
static void charge(void)
{
    fresh(); place(898,236,true); g.player.battery=true;
    ticks(B,CHARGE_TICKS-1); ticks(0,1); assert(!g.player.dashing);
    ticks(B,CHARGE_TICKS); assert(g.player.charge==CHARGE_TICKS);
    ticks(0,1); assert(g.player.dashing && g.player.dash==DASH_TICKS-1);
    ticks(0,11); assert(g.player.dashing && g.player.dash==0);
    assert(game_tile(&g,57,15)==EMPTY && g.terrain_revision>0);
    ticks(0,1); assert(!g.player.dashing);
    fresh(); place(40,170,false); g.player.battery=true; g.player.air_dash=true;
    ticks(B,CHARGE_TICKS); ticks(0,1);
    ticks(B,CHARGE_TICKS); ticks(0,1); assert(!g.player.dashing);
}
static void enemies(void)
{
    fresh(); game_enemies_init(&g);
    bool fell=false,hopped=false;
    for (u16 i=0;i<240;i++)
    {
        game_enemies_step(&g);
        assert(g.enemies[PATROL].body.x+12*UNIT<=288*UNIT);
        assert(g.enemies[PATROL].body.y<260*UNIT);
        if (g.enemies[WALKER].retired) fell=true;
        if (g.enemies[HOPPER].body.vy<0) hopped=true;
        assert(g.enemies[FLYER].body.y>=160*UNIT && g.enemies[FLYER].body.y<=176*UNIT);
    }
    assert(fell && hopped);
    fresh();
    Enemy *e=&g.enemies[0]; e->active=true; e->body=(Body){.x=40*UNIT,.y=230*UNIT,.width=12,.height=14};
    place(40,212,false); g.player.body.vy=3*UNIT;
    game_contacts(&g,229*UNIT,A);
    assert(!e->active && g.player.body.vy==JUMP_SPEED && g.score==2);
    fresh(); e=&g.enemies[0]; e->active=true; e->guarded=true;
    e->body=(Body){.x=40*UNIT,.y=230*UNIT,.width=12,.height=14};
    place(40,212,false); g.player.body.vy=3*UNIT; game_contacts(&g,229*UNIT,0);
    assert(e->active && g.player.hearts==2);
    g.player.dashing=true; game_contacts(&g,229*UNIT,0); assert(!e->active);
    fresh(); e=&g.enemies[0]; e->active=true;
    e->body=(Body){.x=40*UNIT,.y=236*UNIT,.width=12,.height=14}; place(40,236,true);
    game_contacts(&g,256*UNIT,0); assert(g.player.hearts==2);
    game_contacts(&g,256*UNIT,0); assert(g.player.hearts==2);
}
static void progression(void)
{
    fresh(); place(544,224,false); ticks(0,1); assert(g.player.boots);
    place(704,224,false); ticks(0,1); assert(g.player.battery && g.checkpoint==2);
    place(880,224,false); ticks(0,1); assert(g.player.boost==360);
    place(1080,220,false); ticks(0,1); assert(g.secret && g.score==5);
    place(1208,224,false); ticks(0,1); assert(g.complete);
    u32 t=g.ticks; u16 celebration=g.celebration; ticks(0,100);
    assert(g.ticks==t && g.celebration!=celebration);
    ticks(START,1); assert(!g.complete && !g.player.boots && g.checkpoint==0);
    fresh(); g.checkpoint=2; g.player.boots=true; g.player.hearts=1; game_hurt(&g,1);
    assert(g.deaths==1 && g.player.body.x==704*UNIT && g.player.hearts==3 && g.player.boots);
    fresh(); place(300,330,false); ticks(0,1); assert(g.deaths==1 && g.teleport);
}
static void clock_test(void)
{
    fresh(); ticks(0,600); Game ntsc=g;
    fresh(); for (u16 i=0;i<500;i++) game_frame(&g,0,50);
    assert(g.ticks==ntsc.ticks && g.platform.x==ntsc.platform.x);
    assert(g.player.body.x==ntsc.player.body.x && g.player.body.y==ntsc.player.body.y);
    game_frame(&g,START,50); Game paused=g;
    for (u16 i=0;i<200;i++) game_frame(&g,0,50);
    assert(g.ticks==paused.ticks && g.player.invulnerable==paused.player.invulnerable && g.clock==paused.clock);
    game_frame(&g,START,50); assert(!g.paused && g.ticks>paused.ticks);
}
static void soak(void)
{
    fresh(); u32 seed=12345;
    for (u32 i=0;i<100000;i++)
    {
        seed=seed*1664525u+1013904223u;
        u16 held=((seed>>27)&3)==0?LEFT:RIGHT;
        if (seed&256) held|=A;
        if (seed&512) held|=B;
        if (i%400==0) held|=C;
        game_frame(&g,held,i&1?50:60);
        Body *b=&g.player.body;
        assert(b->x>=0 && b->x+12*UNIT<=WORLD_W*CELL*UNIT);
        assert(b->y>=0 && b->y<=(WORLD_H*CELL+33)*UNIT);
        assert(!game_blocked(&g,b,b->x,b->y));
        assert(g.camera_x>=0 && g.camera_x<=960 && g.camera_y>=0 && g.camera_y<=64);
        if (g.complete) game_init(&g);
    }
}
int main(int argc,char **argv)
{
    assert(argc==2);
    if (!strcmp(argv[1],"geometry")) geometry();
    else if (!strcmp(argv[1],"slopes")) slopes();
    else if (!strcmp(argv[1],"oneway")) one_way();
    else if (!strcmp(argv[1],"jumps")) jumps();
    else if (!strcmp(argv[1],"platform")) platform();
    else if (!strcmp(argv[1],"charge")) charge();
    else if (!strcmp(argv[1],"enemies")) enemies();
    else if (!strcmp(argv[1],"progression")) progression();
    else if (!strcmp(argv[1],"clock")) clock_test();
    else if (!strcmp(argv[1],"soak")) soak();
    else return 2;
    puts(argv[1]); return 0;
}
