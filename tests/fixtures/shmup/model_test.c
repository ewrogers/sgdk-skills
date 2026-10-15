#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "game.h"
static Game g;
static void fresh(void) { game_start(&g); g.invulnerable=0; }
static void frames(u16 input,u16 count,u16 hz) { while (count--) game_frame(&g,input,hz); }
static void layout_test(void)
{
    assert(game_video_layout_valid(64,32,0xE000,0xC000,0xD000));
    assert(!game_video_layout_valid(32,32,0xE000,0xC000,0xC000));
    assert(!game_video_layout_valid(32,32,0xE000,0xC000,0xC800));
    assert(game_video_layout_valid(32,32,0xE000,0xC000,0xD000));
    assert(!game_video_layout_valid(64,64,0xE000,0xC000,0xD000));
    assert(!game_video_layout_valid(64,32,0xFC00,0xC000,0xD000));
    assert(!game_video_layout_valid(128,128,0,0x8000,0xC000));
}
static void pools(void)
{
    fresh();
    for (u16 i=0;i<BULLET_COUNT;i++) assert(game_bullet(&g,40*UNIT,40*UNIT,8,UNIT));
    Bullet before=g.bullets[0]; assert(!game_bullet(&g,0,0,8,UNIT));
    assert(g.denied==1 && !memcmp(&before,&g.bullets[0],sizeof before));
    g.bullets[7].active=false; assert(game_bullet(&g,41*UNIT,42*UNIT,4,2*UNIT));
    assert(g.bullets[7].x==41*UNIT && g.bullets[7].vx==2*UNIT);
    assert(!game_bullet(&g,0,0,16,UNIT) && !game_bullet(&g,0,0,0,-1));
    fresh(); for (u16 i=0;i<SHOT_COUNT;i++) assert(game_shot(&g,100*UNIT,100*UNIT,0,-6*UNIT));
    assert(!game_shot(&g,0,0,0,-UNIT));
    frames(0,45,60);
    for (u16 i=0;i<SHOT_COUNT;i++) assert(!g.shots[i].active);
}
static void patterns(void)
{
    assert(game_aim(0,100)==8 && game_aim(100,0)==4 && game_aim(0,-100)==0 && game_aim(-100,0)==12);
    fresh(); assert(game_pattern(&g,RING,100*UNIT,60*UNIT,0,0,0)==8);
    for (u16 i=0;i<4;i++)
    { assert(g.bullets[i].vx==-g.bullets[i+4].vx); assert(g.bullets[i].vy==-g.bullets[i+4].vy); }
    fresh(); assert(game_pattern(&g,FAN,100*UNIT,40*UNIT,100*UNIT,180*UNIT,0)==3);
    assert(g.bullets[0].vx==-g.bullets[2].vx && g.bullets[0].vx!=0 && g.bullets[1].vx==0);
    assert(g.bullets[0].vy==g.bullets[2].vy);
    fresh(); for (u16 i=0;i<BULLET_COUNT-2;i++) assert(game_bullet(&g,0,0,8,UNIT));
    assert(game_pattern(&g,FAN,0,0,0,100,0)==0);
    assert(!g.bullets[BULLET_COUNT-2].active && !g.bullets[BULLET_COUNT-1].active); /* Atomic volley. */
    fresh();
    for (u16 i=0;i<ENEMY_COUNT;i++) g.enemies[i]=(Enemy){.active=true};
    g.ticks=100; game_stage(&g); assert(g.spawn_index==0);
    g.enemies[3].active=false; game_stage(&g);
    assert(g.spawn_index==1 && g.enemies[3].kind==stage_spawns[0].kind);
    game_stage(&g); assert(g.spawn_index==1);
}
static void movement(void)
{
    fresh(); s16 x=g.x,y=g.y; frames(RIGHT,10,60); assert(g.x-x==30*UNIT);
    fresh(); frames(RIGHT|DOWN,10,60);
    assert(g.x-x<30*UNIT && g.y-y<30*UNIT);
    s32 dx=g.x-x,dy=g.y-y; assert(dx*dx+dy*dy<=(30*UNIT)*(30*UNIT));
    fresh(); frames(RIGHT|C,10,60); assert(g.x-x==15*UNIT);
    frames(RIGHT|DOWN,500,60); assert(g.x==(FIELD_W-8)*UNIT && g.y==(FIELD_H-8)*UNIT);
    frames(LEFT|UP,500,60); assert(g.x==8*UNIT && g.y==8*UNIT);
}
static void combat(void)
{
    fresh(); g.enemies[0]=(Enemy){.x=100*UNIT,.y=100*UNIT,.hp=1,.active=true};
    assert(game_shot(&g,100*UNIT,113*UNIT,0,-6*UNIT)); game_projectiles(&g);
    assert(!g.enemies[0].active && !g.shots[0].active && g.score==110);
    fresh(); assert(game_bullet(&g,g.x,g.y+5*UNIT,0,4*UNIT)); game_projectiles(&g);
    assert(g.lives==2 && g.invulnerable==120);
    game_hurt(&g); assert(g.lives==2);
    g.invulnerable=0; game_hurt(&g); g.invulnerable=0; game_hurt(&g); assert(g.state==OVER);
    fresh(); g.score=99999990UL; game_add_score(&g,100); assert(g.score==99999999UL);
    fresh(); g.combo=9; g.enemies[0]=(Enemy){.hp=1,.active=true}; game_enemy_kill(&g,0);
    assert(g.combo==10 && g.score==200);
}
static void pickups_test(void)
{
    fresh(); g.pickups[0]=(Pickup){.x=g.x,.y=g.y,.active=true}; frames(0,1,60); assert(g.power==2);
    g.pickups[0]=(Pickup){.x=g.x,.y=g.y,.active=true}; frames(0,1,60); assert(g.power==3);
    g.pickups[0]=(Pickup){.x=g.x,.y=g.y,.active=true}; frames(0,1,60); assert(g.power==3 && g.score==250);
    g.pickups[0]=(Pickup){.x=g.x,.y=g.y,.kind=1,.active=true}; frames(0,1,60); assert(g.bombs==3);
    frames(B,1,60); assert(g.bombs==2 && g.payload.active && !g.bomb_flash);
    frames(B,30,60); assert(g.bombs==2); /* One bomb per press. */
    frames(0,60,60); frames(B,1,60); assert(g.bombs==1);
    g.power=3; g.invulnerable=0; game_hurt(&g); assert(g.power==1 && g.combo==0);
}
static void waves(void)
{
    fresh();
    for (u16 frame=0;frame<2100;frame++)
    {
        g.invulnerable=200; frames(A,1,60);
        for (u16 i=0;i<ENEMY_COUNT;i++) g.enemies[i].active=false;
    }
    assert(g.spawn_index==24 && g.wave==WAVE_COUNT && g.state==BOSS && g.boss.active);
    g.boss.age=BOSS_READY_TICKS+10; g.boss.hp=1;
    g.shots[0]=(Bullet){.x=g.boss.x,.y=g.boss.y,.active=true}; game_projectiles(&g);
    assert(g.state==CLEAR && !g.boss.active && g.score>=5000);
    for (u16 i=0;i<BULLET_COUNT;i++) assert(!g.bullets[i].active);
    u32 time=g.ticks; frames(0,120,60); assert(g.ticks==time && g.phase_time>0);
    frames(START,1,60); assert(g.state==PLAY && g.stage==2 && g.score>=5000 && g.lives==3);
}
static void boss_test(void)
{
    fresh(); g.state=BOSS; g.boss=(Boss){.x=120*UNIT,.y=40*UNIT,.age=10,.hp=90,.active=true};
    game_boss_damage(&g,100); assert(g.boss.hp==90); /* Entry is telegraphed, not vulnerable. */
    fresh(); g.state=BOSS; g.boss=(Boss){.x=120*UNIT,.y=-24*UNIT,.hp=90,.active=true};
    for (u16 i=0;i<BOSS_READY_TICKS-1;i++)
    {
        game_boss_damage(&g,100); game_enemies(&g);
        assert(g.boss.hp==90);
        for (u16 j=0;j<BULLET_COUNT;j++) assert(!g.bullets[j].active);
    }
    Game saved=g; g.paused=true; frames(0,100,60); assert(g.boss.age==saved.boss.age); g.paused=false;
    g.boss.hp=45; g.boss.age=BOSS_READY_TICKS+119;
    s16 previous_x=g.boss.x,previous_y=g.boss.y;
    game_enemies(&g); assert(!g.boss.enraged);
    assert(g.boss.x-previous_x<=3*UNIT && previous_x-g.boss.x<=3*UNIT);
    assert(g.boss.y-previous_y<=3*UNIT && previous_y-g.boss.y<=3*UNIT);
    g.boss.age=BOSS_READY_TICKS+239; game_enemies(&g); assert(g.boss.enraged);
    g.boss.hp=90;
    for (u16 i=0;i<BULLET_COUNT;i++) g.bullets[i].active=false;
    g.boss.age=BOSS_READY_TICKS+10; game_boss_damage(&g,44); assert(g.boss.hp==46);
    game_boss_damage(&g,1); assert(g.boss.hp==45);
    g.boss.age=BOSS_READY_TICKS+299; game_enemies(&g);
    u16 count=0; for (u16 i=0;i<BULLET_COUNT;i++) if (g.bullets[i].active) count++;
    assert(count==8); /* Second phrase: a complete ring. */
    for (u16 i=0;i<BULLET_COUNT;i++) g.bullets[i].active=false;
    g.boss.age=BOSS_READY_TICKS+59; game_enemies(&g);
    count=0; for (u16 i=0;i<BULLET_COUNT;i++) count+=g.bullets[i].active;
    assert(count==3);
    for (u16 i=0;i<BULLET_COUNT;i++) g.bullets[i].active=false;
    g.boss.age=BOSS_READY_TICKS+539; game_enemies(&g);
    count=0; for (u16 i=0;i<BULLET_COUNT;i++) count+=g.bullets[i].active;
    assert(count==1);
    game_boss_damage(&g,100); assert(g.state==CLEAR && g.score==5000 && g.boss.hp==0);
    game_boss_damage(&g,100); assert(g.score==5000); /* No double award. */
    fresh(); g.state=BOSS; g.boss=(Boss){.age=BOSS_READY_TICKS+10,.hp=10,.active=true};
    game_bomb_launch(&g);
    for (u16 i=0;i<90;i++) game_bomb_step(&g);
    assert(g.state==BOSS && g.boss.hp==1 && g.score==0); /* Fire must finish. */
}
static void weapons_test(void)
{
    fresh(); g.power=3; g.shield=true; g.shield_time=SHIELD_TICKS;
    game_hurt(&g); assert(g.lives==3 && g.power==3 && !g.shield && g.invulnerable==60);
    game_hurt(&g); assert(g.lives==3);
    g.invulnerable=0; game_hurt(&g); assert(g.lives==2 && g.power==1);
    fresh(); g.pickups[0]=(Pickup){.x=g.x,.y=g.y,.kind=2,.active=true};
    frames(0,1,60); assert(g.shield && g.shield_time==SHIELD_TICKS && g.pickup_kind==2);
    g.invulnerable=30000; frames(0,SHIELD_TICKS-1,60); assert(g.shield);
    frames(0,1,60); assert(!g.shield && !g.shield_time);

    fresh(); g.bombs=0; frames(B,1,60); assert((g.events&EVENT_BOMB_EMPTY) && !g.payload.active);
    frames(B,1,60); assert(!(g.events&EVENT_BOMB_EMPTY));
    fresh(); game_bomb_launch(&g); assert(g.payload.active && g.bombs==1);
    game_bomb_launch(&g); assert(g.bombs==1); /* No overlapping payloads. */
    g.bullets[0]=(Bullet){.x=g.x,.y=g.y-12*UNIT,.active=true};
    g.bullets[1]=(Bullet){.x=g.x,.y=0,.active=true};
    for (u16 i=0;i<BOMB_ARM_TICKS-1;i++) game_bomb_step(&g);
    assert(g.bullets[0].active && g.bullets[1].active && !g.payload.radius);
    game_bomb_step(&g); assert(g.payload.radius==10 && g.bullets[0].active && (g.events&EVENT_BLAST));
    game_bomb_step(&g); assert(!g.bullets[0].active && g.bullets[1].active);
    Game saved=g; g.paused=true; frames(0,100,60);
    assert(!memcmp(&saved.payload,&g.payload,sizeof g.payload)); g.paused=false;
    while (g.payload.active) game_bomb_step(&g);
    assert(!g.bullets[1].active);
    fresh(); g.state=BOSS; g.boss=(Boss){.x=g.x,.y=g.y,.age=BOSS_READY_TICKS,.hp=90,.active=true};
    game_bomb_launch(&g);
    for (u16 i=0;i<100;i++) game_bomb_step(&g);
    assert(g.boss.hp==70 && g.payload.boss_hit); /* One bounded hit per blast. */
}

static void stage2_test(void)
{
    fresh(); g.score=5000; g.lives=2; g.power=3; g.twins=2;
    g.state=CLEAR; g.phase_time=BOSS_DEATH_TICKS;
    frames(START,1,60);
    assert(g.stage==2 && g.state==PLAY && g.score==5000 && g.lives==2 && g.power==3 && g.twins==2);
    fresh(); g.state=CLEAR; g.phase_time=359; frames(0,1,60);
    assert(g.stage==2 && g.state==PLAY);
    g.state=CLEAR; g.phase_time=BOSS_DEATH_TICKS; frames(A,1,60);
    assert(g.stage==1 && g.score==0 && g.twins==0);

    fresh(); g.stage=2; g.state=BOSS;
    g.boss=(Boss){.x=120*UNIT,.y=40*UNIT,.age=BOSS_READY_TICKS,.hp=90,.active=true};
    assert(game_boss_guarded(&g)); game_boss_damage(&g,10); assert(g.boss.hp==90);
    game_shot(&g,g.boss.x,g.boss.y,0,0); game_projectiles(&g);
    assert(!g.shots[0].active && g.boss.hp==90);
    g.boss.age=BOSS_READY_TICKS+45; assert(!game_boss_guarded(&g));
    game_boss_damage(&g,10); assert(g.boss.hp==80);
    g.boss.hp=45; g.boss.age=BOSS_READY_TICKS+60; assert(game_boss_guarded(&g));
    g.stage=1; assert(!game_boss_guarded(&g));

    fresh(); g.stage=2; g.twins=2;
    for (u16 tick=0;tick<256;tick++)
    {
        g.ticks=tick; s16 x0,y0,x1,y1; game_twin_point(&g,0,&x0,&y0); game_twin_point(&g,1,&x1,&y1);
        assert(x0+x1==2*g.x && y0+y1==2*g.y);
        s32 dx=x0-g.x,dy=y0-g.y;
        assert(dx*dx+dy*dy>=18*18*UNIT*UNIT && dx*dx+dy*dy<=21*21*UNIT*UNIT);
        g.x+=UNIT; game_twin_point(&g,0,&x1,&y1); assert(x1==x0+UNIT && y1==y0); g.x-=UNIT;
    }
    g.ticks=0; frames(A,1,60);
    u16 count=0; for (u16 i=0;i<SHOT_COUNT;i++) if (g.shots[i].active) { count++; assert(g.shots[i].vy==-6*UNIT); }
    assert(count==3);
    fresh(); g.stage=2; g.ticks=239; g.invulnerable=30000; frames(0,1,60);
    assert(g.pickups[0].active && g.pickups[0].kind==3);
    for (u16 i=0;i<3;i++)
    { g.pickups[0]=(Pickup){.x=g.x,.y=g.y,.kind=3,.active=true}; frames(0,1,60); }
    assert(g.twins==2 && g.score==250);
    g.invulnerable=0; g.power=3; game_hurt(&g); assert(g.twins==0 && g.power==1);
}

static void clock_test(void)
{
    fresh(); g.invulnerable=10000; frames(A|C,600,60); Game ntsc=g;
    fresh(); g.invulnerable=10000; frames(A|C,500,50);
    assert(g.ticks==ntsc.ticks && g.score==ntsc.score && g.lives==ntsc.lives);
    assert(!memcmp(g.enemies,ntsc.enemies,sizeof g.enemies));
    frames(START,1,50); Game paused=g; frames(0,300,50);
    assert(g.ticks==paused.ticks && g.clock==paused.clock && g.invulnerable==paused.invulnerable);
    assert(!memcmp(g.bullets,paused.bullets,sizeof g.bullets));
    frames(START,1,50); assert(!g.paused);
}
static void count_sprite(u16 counts[224],u16 pixels[224],s16 x,s16 y,u16 size,bool active)
{
    x=x/UNIT-size/2; y=y/UNIT-size/2;
    if (!active || x+size<=0 || x>=FIELD_W || y+size<=0 || y>=FIELD_H) return;
    for (s16 row=y;row<y+size;row++) if (row>=0 && row<224)
    { counts[row]++; pixels[row]+=size; }
}
static void pressure(void)
{
    u16 peak=0,pixel_peak=0;
    for (u16 route=0;route<8;route++)
    {
        fresh(); g.invulnerable=30000;
        for (u16 tick=0;tick<5000;tick++)
        {
            if (route%4==1) g.x=40*UNIT;
            else if (route%4==2) g.x=200*UNIT;
            else if (route%4==3) g.x=(20+tick%200)*UNIT;
            g.shield=true; g.shield_time=SHIELD_TICKS; g.twins=2;
            if (tick%360==0) g.bombs=3;
            frames((route>=4?A:0)|(tick%360==0?B:0),1,60);
            if (g.state==BOSS && g.boss.age>300) g.boss.hp=45;
            u16 counts[224]={0},pixels[224]={0};
            count_sprite(counts,pixels,g.x,g.y,16,true);
            count_sprite(counts,pixels,g.x,g.y,32,g.shield);
            count_sprite(counts,pixels,g.boss.x,g.boss.y,32,game_boss_guarded(&g));
            for (u16 i=0;i<g.twins;i++)
            { s16 x,y; game_twin_point(&g,i,&x,&y); count_sprite(counts,pixels,x,y,16,true); }
            count_sprite(counts,pixels,g.payload.x,g.payload.y,32,g.payload.active && g.payload.age<BOMB_ARM_TICKS);
            if (g.payload.active && g.payload.radius>=20)
                for (u16 i=0;i<8;i++)
                { s16 x,y; game_blast_point(&g,i,&x,&y); count_sprite(counts,pixels,x,y,16,true); }
            count_sprite(counts,pixels,g.boss.x,g.boss.y,32,g.boss.active);
            for (u16 i=0;i<BULLET_COUNT;i++) count_sprite(counts,pixels,g.bullets[i].x,g.bullets[i].y,8,g.bullets[i].active);
            for (u16 i=0;i<ENEMY_COUNT;i++) count_sprite(counts,pixels,g.enemies[i].x,g.enemies[i].y,16,g.enemies[i].active);
            for (u16 i=0;i<SHOT_COUNT;i++) count_sprite(counts,pixels,g.shots[i].x,g.shots[i].y,8,g.shots[i].active);
            for (u16 i=0;i<PICKUP_COUNT;i++) count_sprite(counts,pixels,g.pickups[i].x,g.pickups[i].y,16,g.pickups[i].active);
            for (u16 i=0;i<EFFECT_COUNT;i++) count_sprite(counts,pixels,g.effects[i].x,g.effects[i].y,16,g.effects[i].life>0);
            for (u16 row=0;row<224;row++)
            { if (counts[row]>peak) peak=counts[row]; if (pixels[row]>pixel_peak) pixel_peak=pixels[row]; }
        }
    }
    fprintf(stderr,"Authored scanline peak: %u sprites / %u pixels\n",peak,pixel_peak);
    assert(peak<=18 && pixel_peak<=288); /* Keep space for player shots/effects. */
}
static void soak(void)
{
    fresh(); u32 seed=82;
    for (u32 frame=0;frame<100000;frame++)
    {
        seed=seed*1664525u+1013904223u;
        u16 held=A;
        if (seed&1) held|=RIGHT; else held|=LEFT;
        if (seed&2) held|=UP; else held|=DOWN;
        if (seed&256) held|=C;
        if (frame%401==0) held|=B;
        if (g.state==OVER || g.state==CLEAR) game_start(&g);
        frames(held,1,frame&1?50:60);
        assert(g.x>=8*UNIT && g.x<=(FIELD_W-8)*UNIT && g.y>=8*UNIT && g.y<=(FIELD_H-8)*UNIT);
        assert(g.lives<=3 && g.power>=1 && g.power<=3 && g.bombs<=3 && g.score<=99999999UL);
        for (u16 i=0;i<BULLET_COUNT;i++) if (g.bullets[i].active)
        { assert(g.bullets[i].x>=-16*UNIT && g.bullets[i].x<=(FIELD_W+16)*UNIT); }
    }
}
void reference_game_projectiles(Game *game);
static u32 random_state=517;
static u16 next_random(u16 max)
{ random_state=random_state*1664525u+1013904223u; return (random_state>>16)%max; }
static void equivalence(void)
{
    /* Dense, sparse, overlapping targets, negative fractional velocities,
     * screen edges, invulnerability and boss defeat all share the same oracle. */
    for (u16 trial=0;trial<12000;trial++)
    {
        fresh(); g.x=next_random(240*UNIT); g.y=next_random(224*UNIT);
        g.shield=next_random(2); g.shield_time=g.shield?SHIELD_TICKS:0;
        g.invulnerable=next_random(2)?0:120; g.kills=next_random(12); g.power=next_random(3)+1;
        for (u16 i=0;i<ENEMY_COUNT;i++)
            g.enemies[i]=(Enemy){.x=next_random(240*UNIT),.y=next_random(224*UNIT),
                .hp=next_random(3)+1,.active=next_random(2)};
        if (trial%3==0) g.boss=(Boss){.x=120*UNIT,.y=40*UNIT,.hp=next_random(4)+1,
            .age=trial%2?BOSS_READY_TICKS:10,.active=true};
        for (u16 i=0;i<SHOT_COUNT;i++)
            g.shots[i]=(Bullet){.x=next_random(256*UNIT)-8*UNIT,.y=next_random(240*UNIT)-8*UNIT,
                .vx=(s16)next_random(193)-96,.vy=(s16)next_random(193)-96,.active=true};
        for (u16 i=0;i<BULLET_COUNT;i++)
            g.bullets[i]=(Bullet){.x=next_random(256*UNIT)-8*UNIT,.y=next_random(240*UNIT)-8*UNIT,
                .vx=(s16)next_random(129)-64,.vy=(s16)next_random(129)-64,.active=true};
        Game expected=g; reference_game_projectiles(&expected); game_projectiles(&g);
        assert(!memcmp(&g,&expected,sizeof g));
    }
}
static void upgrades_test(void)
{
    fresh(); g.power=2; game_step(&g,A,0);
    u16 n=0;
    for (u16 i=0;i<SHOT_COUNT;i++) if (g.shots[i].active)
    { assert(g.shots[i].vx==0 && g.shots[i].vy==-6*UNIT); n++; }
    assert(n==2);
    fresh(); g.power=3; g.twins=2; g.ticks=19; game_step(&g,A,0);
    n=0;
    for (u16 i=0;i<SHOT_COUNT;i++) if (g.shots[i].active)
    { assert(g.shots[i].vx==0); n++; }
    assert(n==7); /* Three main lasers, two from each satellite. */
    fresh(); g.shield=true; g.shield_time=1;
    g.pickups[0]=(Pickup){.x=g.x,.y=g.y,.kind=2,.active=true};
    game_step(&g,0,0); assert(g.shield && g.shield_time==SHIELD_TICKS);
    for (u16 kind=2;kind<=3;kind++)
    {
        fresh(); g.stage=2; g.shield=true; g.shield_time=SHIELD_TICKS; g.twins=2;
        g.kills=(kind+1)*3-1;
        g.enemies[0]=(Enemy){.x=120*UNIT,.y=60*UNIT,.active=true};
        game_enemy_kill(&g,0);
        for (u16 i=0;i<PICKUP_COUNT;i++) assert(!g.pickups[i].active);
    }
    fresh(); g.stage=2; g.twins=2; g.ticks=239; game_step(&g,0,0);
    for (u16 i=0;i<PICKUP_COUNT;i++) assert(!g.pickups[i].active);
    fresh(); g.shield=true; g.shield_time=SHIELD_TICKS; g.ticks=719; game_step(&g,0,0);
    for (u16 i=0;i<PICKUP_COUNT;i++) assert(!g.pickups[i].active);
    for (u16 pattern=FAN;pattern<=SPIRAL;pattern++)
    {
        fresh(); g.state=BOSS;
        u16 total=game_pattern(&g,(Pattern)pattern,120*UNIT,40*UNIT,g.x,g.y,0);
        assert(total>0);
        for (u16 i=0;i<total;i++) assert(g.bullets[i].kind==pattern);
        game_projectiles(&g);
        for (u16 i=0;i<total;i++) assert(g.bullets[i].kind==pattern);
    }
    /* Damage changes art now, but trajectory only at the next phrase boundary. */
    fresh(); g.state=BOSS; g.boss=(Boss){.x=120*UNIT,.y=44*UNIT,.age=BOSS_READY_TICKS+60,.hp=90,.active=true};
    Game healthy=g; g.boss.hp=45;
    for (u16 i=0;i<100;i++)
    {
        game_enemies(&g); game_enemies(&healthy);
        assert(g.boss.x==healthy.boss.x && g.boss.y==healthy.boss.y && !g.boss.enraged);
    }
}
int main(int argc,char **argv)
{
    assert(argc==2);
    if (!strcmp(argv[1],"upgrades")) upgrades_test();
    else if (!strcmp(argv[1],"equivalence")) equivalence();
    else if (!strcmp(argv[1],"layout")) layout_test();
    else if (!strcmp(argv[1],"pools")) pools();
    else if (!strcmp(argv[1],"patterns")) patterns();
    else if (!strcmp(argv[1],"movement")) movement();
    else if (!strcmp(argv[1],"combat")) combat();
    else if (!strcmp(argv[1],"pickups")) pickups_test();
    else if (!strcmp(argv[1],"waves")) waves();
    else if (!strcmp(argv[1],"boss")) boss_test();
    else if (!strcmp(argv[1],"weapons")) weapons_test();
    else if (!strcmp(argv[1],"stage2")) stage2_test();
    else if (!strcmp(argv[1],"clock")) clock_test();
    else if (!strcmp(argv[1],"pressure")) pressure();
    else if (!strcmp(argv[1],"soak")) soak();
    else return 2;
    puts(argv[1]); return 0;
}
