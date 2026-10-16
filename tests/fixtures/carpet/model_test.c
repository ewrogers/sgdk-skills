#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "game.h"
void reference_game_combat(Game *game);
static Game game;
static void begin(Loadout kind) { game_init(&game); game_begin(&game,kind); game.state=PLAY; game.invulnerable=0; }
static void tick(u16 held,u16 count,u16 hz) { while (count--) game_frame(&game,held,hz); }
static void flow(void)
{
    game_init(&game); tick(A,1,60); assert(game.state==HOWTO);
    tick(A,3,60); assert(game.state==HOWTO); tick(0,1,60); tick(A,1,60); assert(game.state==LOADOUT);
    tick(0,1,60); tick(LEFT,1,60); assert(game.loadout==STORMGLASS);
    tick(0,1,60); tick(RIGHT,1,60); assert(game.loadout==SUNFIRE);
    tick(0,1,60); tick(A,1,60); assert(game.state==STAGECARD && game.stage==1);
    tick(0,150,60); assert(game.state==PLAY);
    for (u16 stage=1;stage<=3;stage++)
    {
        game.stage_time=STAGE_TICKS-1;game_step(&game,0,0);assert(game.state==BOSS && game.boss.active);
        game.boss.age=250; game_boss_damage(&game,BOSS_HP);assert(game.state==CLEAR);
        s16 x=game.x; game_step(&game,RIGHT,0); assert(game.x>x);
        game.age=150;x=game.x;game_step(&game,RIGHT,0);assert(game.x==x);
        game.age=329;game_step(&game,0,0);
        if (stage<3) { assert(game.stage==stage+1 && game.state==STAGECARD); tick(0,150,60); }
        else assert(game.state==CREDITS);
    }
    tick(0,1,60);tick(START,1,60);assert(game.state==TITLE);
}
static void meter(void)
{
    begin(SUNFIRE);
    for (u16 i=0;i<6;i++)
    {
        game.gems[0]=(Gem){.x=game.x,.y=game.y,.active=true};game_step(&game,0,0);
        assert(game.meter==i%5+1);
    }
    game.meter=1;for(u16 i=0;i<3;i++){game.meter=1;game_upgrade(&game);}assert(game.speed==3);
    game.meter=1;game.events=0;game_upgrade(&game);assert(game.speed==3 && game.meter==1 && (game.events&EV_DENIED) && !(game.events&EV_UPGRADE));
    game.gems[0]=(Gem){.x=game.x,.y=game.y,.active=true};game_step(&game,0,0);
    assert(game.meter==2); /* Every gem advances exactly one slot, even past capped upgrades. */
    game.meter=2;game_upgrade(&game);assert(game.weapon==1 && game.meter==0);
    game.meter=3;game_upgrade(&game);assert(game.weapon==3);
    for(u16 i=0;i<2;i++){game.meter=4;game_upgrade(&game);}assert(game.familiars==2);
    game.meter=4;game.events=0;game_upgrade(&game);
    assert(game.familiars==2 && game.meter==4 && (game.events&EV_DENIED));
    game.meter=5;game_upgrade(&game);assert(game.barrier==2);
    game.meter=5;game.events=0;game_upgrade(&game);assert(game.barrier==2 && game.meter==5 && (game.events&EV_DENIED));
    game.meter=3;game.events=0;game_upgrade(&game);assert(game.weapon==3 && game.meter==3 && (game.events&EV_DENIED));
    game.gems[0]=(Gem){.x=game.x,.y=game.y,.active=true};game_step(&game,0,0);
    assert(game.meter==4 && !game_upgrade_available(&game,4)); /* Disabled slots remain selected. */
    game_hurt(&game);assert(game.lives==3 && game.barrier==1 && game.weapon==3 && game_upgrade_available(&game,5));
    game.invulnerable=0;game_hurt(&game);assert(game.lives==3 && game.barrier==0);
    game.invulnerable=0;game_hurt(&game);assert(game.lives==2 && !game.weapon && !game.familiars && !game.speed);
    game.invulnerable=0;game_hurt(&game);game.invulnerable=0;game_hurt(&game);assert(game.state==OVER);
    tick(0,1,60);tick(START,1,60);assert(game.lives==3 && game.state==STAGECARD);
}
static void weapons(void)
{
    for(u16 kind=0;kind<3;kind++)for(u16 upgrade=0;upgrade<4;upgrade++)
    {
        begin((Loadout)kind);game.weapon=upgrade;game_player(&game,A,true);
        u16 count=0;for(u16 i=0;i<SHOT_COUNT;i++)if(game.shots[i].active)
        {
            count++;
            u16 expected=kind==MOONWEAVE && (upgrade&2)?1:kind+((upgrade&2)?3:0);
            assert(game.shots[i].kind==expected);
        }
        u16 expected=kind==MOONWEAVE && (upgrade&2)?((upgrade&1)?2:0):1+((upgrade&1)?1:0)+((upgrade&2) && kind==SUNFIRE?2:0);
        assert(count==expected);
        assert(game.ribbon_active==(kind==MOONWEAVE && (upgrade&2)));
    }
    begin(MOONWEAVE);game_emit(&game,true,100*UNIT,100*UNIT,4*UNIT,0,1,1);game.shots[0].age=27;
    game_combat(&game);assert(game.shots[0].vx==-8*UNIT);
    begin(STORMGLASS);game.enemies[0]=(Enemy){.x=160*UNIT,.y=160*UNIT,.hp=3,.active=true};
    game_emit(&game,true,100*UNIT,100*UNIT,4*UNIT,0,2,1);game.shots[0].age=7;game_combat(&game);assert(game.shots[0].vy>0);
    begin(MOONWEAVE);game_emit(&game,true,76*UNIT,100*UNIT,8*UNIT,0,1,1);
    for(u16 i=0;i<20;i++)game_combat(&game);
    assert(game.shots[0].x==236*UNIT && game.shots[0].y==100*UNIT);
    for(u16 i=0;i<8;i++)game_combat(&game);
    assert(game.shots[0].x==228*UNIT && game.shots[0].y==100*UNIT && game.shots[0].vx==-8*UNIT);
    game_combat(&game);assert(game.shots[0].x==220*UNIT && game.shots[0].y==100*UNIT);
    begin(MOONWEAVE);game.enemies[0]=(Enemy){.x=100*UNIT,.y=100*UNIT,.hp=3,.active=true};
    game_emit(&game,true,96*UNIT,100*UNIT,8*UNIT,0,1,1);game_combat(&game);
    assert(game.enemies[0].hp==2 && game.shots[0].active);game_combat(&game);assert(game.enemies[0].hp==2);
    begin(STORMGLASS);
    for(u16 i=0;i<4;i++)game.enemies[i]=(Enemy){.x=(100+20*i)*UNIT,.y=100*UNIT,.hp=5,.active=true};
    game_emit(&game,true,96*UNIT,100*UNIT,6*UNIT,0,5,2);game_combat(&game);
    assert(game.enemies[0].hp==3 && game.enemies[1].hp==4 && game.enemies[2].hp==4 && game.enemies[3].hp==5);
    assert(!game.shots[0].active && game.effects[0].kind==2 && game.effects[1].kind==2);
    begin(STORMGLASS);game.enemies[0]=(Enemy){.x=100*UNIT,.y=100*UNIT,.hp=5,.active=true};
    game.enemies[1]=(Enemy){.x=240*UNIT,.y=100*UNIT,.hp=5,.active=true};
    game_emit(&game,true,96*UNIT,100*UNIT,6*UNIT,0,5,2);game_combat(&game);assert(game.enemies[1].hp==5);
    begin(SUNFIRE);for(u16 i=0;i<SHOT_COUNT;i++)assert(game_emit(&game,true,0,100*UNIT,UNIT,0,0,1));
    Shot before=game.shots[0];assert(!game_emit(&game,true,1,1,UNIT,0,0,1));assert(!memcmp(&before,&game.shots[0],sizeof before));
}
static void ribbon(void)
{
    begin(MOONWEAVE);game.weapon=2;
    for(u16 i=0;i<3;i++)game.enemies[i]=(Enemy){.x=(120+i*70)*UNIT,.y=game.y,.hp=20,.active=true};
    game.enemies[3]=(Enemy){.x=20*UNIT,.y=game.y,.hp=20,.active=true};
    game.enemies[4]=(Enemy){.x=120*UNIT,.y=game.y+18*UNIT,.hp=20,.active=true};
    game.enemies[5]=(Enemy){.x=300*UNIT,.y=game.y,.hp=20,.active=true};
    game_player(&game,A,true);assert(game.ribbon_active && (game.events&EV_RIBBON));
    game_combat(&game);
    for(u16 i=0;i<3;i++)assert(game.enemies[i].hp==18);
    assert(game.enemies[3].hp==20 && game.enemies[4].hp==20 && game.enemies[5].hp==20 && game.ribbon_wait==6);
    for(u16 t=0;t<5;t++){game_player(&game,A,true);game_combat(&game);}
    assert(game.enemies[0].hp==18);game_player(&game,A,true);game_combat(&game);assert(game.enemies[0].hp==16);
    game_player(&game,0,true);game_combat(&game);assert(!game.ribbon_active && game.enemies[0].hp==16);
    game_player(&game,A,true);game_combat(&game);assert(game.ribbon_active && game.enemies[0].hp==16); /* Tapping cannot bypass cooldown. */
    tick(START,1,60);Game saved=game;tick(0,60,60);
    assert(game.paused && game.ticks==saved.ticks && game.ribbon_wait==saved.ribbon_wait && game.enemies[0].hp==saved.enemies[0].hp);
    begin(MOONWEAVE);game.weapon=3;game_player(&game,A,true);
    assert(game.ribbon_active && game.shots[0].active && game.shots[1].active && game.shots[0].kind==1 && game.shots[1].kind==1);
    begin(MOONWEAVE);game.weapon=2;game.state=BOSS;game.boss=(Boss){.x=270*UNIT,.y=game.y,.hp=10,.age=180,.active=true};
    game_player(&game,A,true);game_combat(&game);assert(game.boss.hp==10);
    game.boss.age=250;game.ribbon_wait=0;game_combat(&game);assert(game.boss.hp==8);
    game.boss.hp=2;game.ribbon_wait=0;game_combat(&game);assert(game.state==CLEAR && !game.ribbon_active);
    begin(MOONWEAVE);game.weapon=2;
    for(u16 i=0;i<SHOT_COUNT;i++)assert(game_emit(&game,true,0,0,0,0,1,1));
    game.enemies[0]=(Enemy){.x=200*UNIT,.y=game.y,.hp=10,.active=true};
    game_player(&game,A,true);game_combat(&game);assert(game.enemies[0].hp==8); /* Beam does not need a free bullet slot. */
}
static void drops(void)
{
    begin(SUNFIRE);u16 drops=0;
    for(u16 kill=0;kill<16;kill++)
    {
        game.enemies[0]=(Enemy){.x=200*UNIT,.y=100*UNIT,.hp=1,.active=true};
        game_enemy_kill(&game,0);
        for(u16 i=0;i<GEM_COUNT;i++)if(game.gems[i].active){drops++;game.gems[i].active=false;}
    }
    assert(drops==5 && game.enemy_kills==16);
    game_stage_begin(&game,2);assert(game.enemy_kills==0);
}
static void timing(void)
{
    begin(SUNFIRE);game.invulnerable=10000;tick(RIGHT,60,60);s16 x=game.x;u32 ticks=game.ticks;
    begin(SUNFIRE);game.invulnerable=10000;tick(RIGHT,50,50);assert(game.x==x && game.ticks==ticks);
    tick(START,1,50);Game saved=game;tick(0,100,50);assert(game.paused && game.ticks==saved.ticks && game.x==saved.x);
    tick(START,1,50);assert(!game.paused);
    begin(SUNFIRE);game.invulnerable=10000;tick(UP|LEFT,200,60);assert(game.x==16*UNIT && game.y==(HUD_HEIGHT+16)*UNIT);
    tick(DOWN|RIGHT,300,60);assert(game.x==304*UNIT && game.y==192*UNIT);
    begin(SUNFIRE);game_player(&game,RIGHT,false);s16 full=game.x;
    begin(SUNFIRE);game_player(&game,RIGHT|C,false);assert(game.x<full);
    begin(SUNFIRE);game.familiars=2;for(u16 i=0;i<32;i++)game_player(&game,RIGHT,false);
    s16 x0,y0,x1,y1;game_familiar(&game,0,&x0,&y0);game_familiar(&game,1,&x1,&y1);assert(x0>x1 && y0<y1);
}
static u32 seed=8123;
static u16 random_below(u16 limit) { seed=seed*1664525u+1013904223u;return (seed>>16)%limit; }
static void equivalence(void)
{
    for(u16 trial=0;trial<12000;trial++)
    {
        begin((Loadout)(trial%3));game.x=random_below(320*UNIT);game.y=(40+random_below(170))*UNIT;
        game.invulnerable=random_below(2)?0:60;game.barrier=random_below(3);
        for(u16 i=0;i<ENEMY_COUNT;i++)game.enemies[i]=(Enemy){.x=random_below(320*UNIT),.y=(40+random_below(180))*UNIT,.hp=random_below(5)+1,.active=random_below(2)};
        if(trial%3==0){game.state=BOSS;game.boss=(Boss){.x=260*UNIT,.y=120*UNIT,.hp=random_below(4)+1,.age=250,.active=true};}
        for(u16 i=0;i<SHOT_COUNT;i++)game.shots[i]=(Shot){.x=random_below(352*UNIT)-16*UNIT,.y=random_below(224*UNIT),.vx=(s16)random_below(257)-128,.vy=(s16)random_below(257)-128,.kind=random_below(6),.damage=random_below(3)+1,.age=random_below(40),.active=true};
        for(u16 i=0;i<BULLET_COUNT;i++)game.bullets[i]=(Shot){.x=random_below(352*UNIT)-16*UNIT,.y=random_below(224*UNIT),.vx=(s16)random_below(129)-64,.vy=(s16)random_below(129)-64,.active=true};
        Game expected=game;reference_game_combat(&expected);game_combat(&game);assert(!memcmp(&game,&expected,sizeof game));
    }
}
static void soak(void)
{
    begin(STORMGLASS);
    for(u32 tick=0;tick<100000;tick++)
    {
        if(game.state==OVER || game.state==CREDITS)game_begin(&game,(Loadout)random_below(3));
        game_frame(&game,(random_below(2)?LEFT:RIGHT)|(random_below(2)?UP:DOWN)|A|(random_below(20)==0?B:0),tick&1?50:60);
        assert(game.stage>=1 && game.stage<=3 && game.loadout<=STORMGLASS && game.familiars<=2 && game.speed<=3 && game.meter<=5);
        assert(game.x>=16*UNIT && game.x<=304*UNIT && game.y>=(HUD_HEIGHT+16)*UNIT && game.y<=192*UNIT);
    }
}
int main(int argc,char **argv)
{
    assert(argc==2);
    if(!strcmp(argv[1],"flow"))flow();else if(!strcmp(argv[1],"meter"))meter();
    else if(!strcmp(argv[1],"weapons"))weapons();else if(!strcmp(argv[1],"drops"))drops();else if(!strcmp(argv[1],"ribbon"))ribbon();else if(!strcmp(argv[1],"timing"))timing();
    else if(!strcmp(argv[1],"equivalence"))equivalence();else if(!strcmp(argv[1],"soak"))soak();else return 2;
    puts(argv[1]);return 0;
}
