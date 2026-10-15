#include <assert.h>
#include "world.h"

static void quiet(Game *game)
{
    game_init(game);
    for (u16 i=0;i<ENEMY_COUNT;++i) enemy_change(&game->enemies[i],STUNNED,60000);
    game->players[0].invulnerable=0;
}
static void contact_tests(void)
{
    Game game;
    for (u16 kind=BOMBARDIER;kind<=SENTRY;++kind) for (u16 state=IDLE;state<=ZAP;++state) {
        quiet(&game); game.players[0].pos=(Point){48,80};
        Enemy *enemy=&game.enemies[0]; enemy->kind=kind; enemy->pos=game.players[0].pos;
        enemy_change(enemy,state,24); enemy_contact(&game,0); enemy_contact(&game,0);
        assert(game.tries==(state==STUNNED ? 3 : 2));
        assert(game.players[0].hits==(state==STUNNED ? 0 : 1));
    }
    quiet(&game); Enemy *enemy=&game.enemies[0]; enemy->pos=(Point){48,80};
    enemy_change(enemy,RECOVER,90); game.players[0].pos=(Point){36,80};
    enemy_contact(&game,0); assert(game.tries==3);
    game.players[0].pos.x=37; enemy_contact(&game,0); assert(game.tries==2);
    quiet(&game); enemy=&game.enemies[0]; enemy->pos=game.players[0].pos;
    enemy_change(enemy,PATROL,0); game.players[0].invulnerable=120;
    enemy_contact(&game,0); assert(game.tries==3);
    game.players[1]=game.players[0]; game.players[1].invulnerable=0; game.players[1].active=false;
    enemy_contact(&game,0); assert(game.tries==3);
    game.players[1].active=true; enemy_contact(&game,0);
    assert(game.tries==2 && game.players[1].hits==1 && !game.players[0].hits);
    quiet(&game); enemy=&game.enemies[0]; enemy->pos=(Point){48,80};
    enemy_change(enemy,RECOVER,90); game.players[0].pos=(Point){36,80};
    Input input={.held={RIGHT,0}}; game_frame(&game,&input,1); assert(game.phase==CAUGHT);
    quiet(&game); enemy=&game.enemies[0]; enemy->pos=(Point){62,80};
    enemy_change(enemy,PATROL,0); enemy->route[0]=world_cell((Point){48,80}); enemy->route_len=1;
    game.players[0].pos=(Point){48,80}; game.tick=3; input=(Input){0};
    game_frame(&game,&input,1); assert(game.phase==PLAYING && enemy->pos.x==61);
    game_frame(&game,&input,1); assert(game.phase==PLAYING && enemy->pos.x==60);
    game_frame(&game,&input,1); assert(game.phase==CAUGHT && enemy->pos.x==59);
}
static void taser_tests(void)
{
    Game game; quiet(&game); Enemy *enemy=&game.enemies[2];
    enemy->pos=(Point){112,112}; enemy->facing=0; game.players[0].pos=(Point){112,128};
    assert(enemy_taser_hits(0,enemy,&game.players[0]));
    game.players[0].pos=(Point){128,112}; assert(!enemy_taser_hits(0,enemy,&game.players[0]));
    game.players[0].pos=(Point){112,96}; assert(!enemy_taser_hits(0,enemy,&game.players[0]));
    for (u16 facing=0;facing<4;++facing) {
        enemy->facing=facing; game.players[0].pos=enemy_taser_target(enemy);
        if (world_line(0,enemy->pos,game.players[0].pos)) assert(enemy_taser_hits(0,enemy,&game.players[0]));
    }
    enemy->facing=0; game.players[0].pos=(Point){112,128};
    enemy_change(enemy,WINDUP,1); enemy_tick(&game,2);
    assert(enemy->state==ZAP && enemy->timer==TASER_TICKS && !game.players[0].hits);
    enemy_tick(&game,2); assert(game.phase==CAUGHT && game.tries==2);
    game.phase=PLAYING; game.players[0].invulnerable=999;
    for (u16 tick=1;tick<TASER_TICKS;++tick) enemy_tick(&game,2);
    assert(enemy->state==RECOVER && enemy->timer==TASER_RECOVERY && TASER_RECOVERY==60);
    for (u16 tick=0;tick<TASER_RECOVERY;++tick) { enemy_sense(&game,2); enemy_tick(&game,2); }
    assert(enemy->state==RETURN); enemy_sense(&game,2); assert(enemy->state==WINDUP);
    enemy_change(enemy,STUNNED,90); game.players[0].invulnerable=0;
    enemy_tick(&game,2); assert(game.phase==PLAYING && enemy->state==STUNNED);
    enemy->pos=(Point){80,112}; enemy->facing=1; game.players[0].pos=(Point){112,112};
    assert(!enemy_taser_hits(0,enemy,&game.players[0]));
    enemy->pos=(Point){114,112}; game.players[0].pos=(Point){130,112};
    assert(enemy_taser_hits(0,enemy,&game.players[0])); /* Same inset clearance as its visual. */
    enemy->pos.x=116; assert(!enemy_taser_hits(0,enemy,&game.players[0]));
}
static void bomb_tests(void)
{
    Game game; quiet(&game); Enemy *enemy=&game.enemies[0]; enemy->pos=(Point){80,112};
    assert(!world_line(0,enemy->pos,(Point){112,112}));
    assert(world_lob_line(0,enemy->pos,(Point){112,112}));
    assert(!world_lob_line(0,(Point){128,112},(Point){176,112})); /* Two-cell cover. */
    assert(!bomb_throw(&game,BOMB_COUNT,(Point){112,112}));
    assert(bomb_throw(&game,0,(Point){112,112}));
    assert(bomb_throw(&game,1,(Point){272,176}));
    assert(!bomb_throw(&game,0,(Point){112,112})); /* Never overwrite a live bomb. */
    game.players[0].pos=(Point){112,112};
    for (u16 tick=0;tick<BOMB_FLIGHT;++tick) { bombs_tick(&game); assert(!game.players[0].hits); }
    assert(game.bombs[0].phase==BOMB_FUSED && game.bombs[0].timer==BOMB_FUSE);
    for (u16 tick=1;tick<BOMB_FUSE;++tick) { bombs_tick(&game); assert(!game.players[0].hits); }
    bombs_tick(&game); assert(game.bombs[0].phase==BOMB_EXPLODING && game.phase==CAUGHT);
    Point cells[1+4*BOMB_REACH]; u16 count=bomb_cells(0,&game.bombs[0],cells);
    bool left=false,right=false,up=false,down=false;
    for (u16 i=0;i<count;++i) {
        assert(cells[i].x==112 || cells[i].y==112);
        assert(!world_wall(0,world_cell(cells[i])%GRID_W,world_cell(cells[i])/GRID_W));
        if (cells[i].x<112) left=true; if (cells[i].x>112) right=true;
        if (cells[i].y<112) up=true; if (cells[i].y>112) down=true;
    }
    assert(!left && right && up && down); /* Left wall stops that arm. */
    for (u16 tick=0;tick<BOMB_BLAST;++tick) bombs_tick(&game);
    assert(game.bombs[0].phase==BOMB_FREE);
    quiet(&game); game.bombs[0]=(Bomb){.target={112,112},.phase=BOMB_EXPLODING,.timer=BOMB_BLAST};
    game.players[0].pos=(Point){128,128}; bombs_tick(&game); assert(game.phase==PLAYING); /* Diagonal is safe. */
    game.players[0].pos=(Point){128,112}; bombs_tick(&game); assert(game.phase==CAUGHT);
    quiet(&game); enemy=&game.enemies[0]; enemy->pos=(Point){112,64}; enemy->last_seen=(Point){112,112};
    enemy_change(enemy,WINDUP,1); enemy_tick(&game,0);
    assert(enemy->state==RECOVER && enemy->timer==240 && GRENADE_RECOVERY==240);
    assert(game.bombs[0].phase==BOMB_FLYING);
    enemy_change(enemy,STUNNED,90); bombs_tick(&game); assert(game.bombs[0].phase==BOMB_FLYING);
    game_load_level(&game,1); assert(game.bombs[0].phase==BOMB_FREE);
    quiet(&game); enemy=&game.enemies[4]; enemy_change(enemy,WINDUP,1); enemy_tick(&game,4);
    assert(enemy->state==RECOVER && enemy->timer==180 && SENTRY_RECOVERY==180);
    quiet(&game); game.bombs[0]=(Bomb){.target={112,112},.phase=BOMB_FUSED,.timer=20};
    game.paused=true; Input idle={0}; game_frame(&game,&idle,2); assert(game.bombs[0].timer==20);
}
void test_combat(void) { contact_tests(); taser_tests(); bomb_tests(); }

void test_introductions(void)
{
    for (u16 hz=50;hz<=60;hz+=10) {
        Game game; game_new_run(&game); Input input={0}; u16 fraction=0;
        for (u16 frame=0;frame<hz*30;++frame) game_frame(&game,&input,game_clock(&fraction,hz));
        assert(game.phase==INTRO_BOMBARDIER && !game.tick && game.tries==3 && !game.events);
    }
    Game game; game_new_run(&game); Input input; u16 previous[2]={0},held[2]={LEFT,0};
    game_input(&input,previous,held); game_frame(&game,&input,1); assert(game.phase==INTRO_SENTRY);
    game_input(&input,previous,held); game_frame(&game,&input,1); assert(game.phase==INTRO_SENTRY);
    input=(Input){.pressed={RIGHT,0}}; game_frame(&game,&input,1); assert(game.phase==INTRO_BOMBARDIER);
    game_frame(&game,&input,1); assert(game.phase==INTRO_SHOCKER);
    input=(Input){.pressed={A,0}}; game_frame(&game,&input,1);
    assert(game.phase==PLAYING && !game.players[0].pulse && game.players[0].invulnerable==120);
    game_load_level(&game,1); assert(game.phase==PLAYING);
    input=(Input){.pressed={B,0}}; game_frame(&game,&input,1); assert(game.phase==INTRO_BOMBARDIER && game.tries==3);
    input=(Input){.pressed={START,0}}; game_frame(&game,&input,1); assert(game.phase==PLAYING && !game.paused);
}
