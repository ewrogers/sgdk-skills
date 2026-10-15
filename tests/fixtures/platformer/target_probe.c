#include <genesis.h>
#include "game.h"
#include "resources.h"
#define CHECK(test) do { if (!(test)) return __LINE__; } while (0)
static Game game;
/* Keep the result observable when entered directly, outside console startup. */
__attribute__((noipa)) int main(bool hard_reset)
{
    (void)hard_reset;
    CHECK(hero_image.numAnimation==1 && hero_image.animations[0]->numFrame==10);
    CHECK(enemy_image.numAnimation==1 && enemy_image.animations[0]->numFrame==20);
    CHECK(item_image.numAnimation==1 && item_image.animations[0]->numFrame==13);
    CHECK(hero_image.maxNumTile==6 && enemy_image.maxNumTile==4 && item_image.maxNumTile==4);
    CHECK(course_map_tileset0.numTile<128);
    game_init(&game);
    for (u16 i=0;i<ENEMY_COUNT;i++) game.enemies[i].active=false;
    game.teleport=false;
    game.player.invulnerable=0;
    game_frame(&game,A,60);
    CHECK(game.player.body.vy<0 && !game.player.body.ground);
    for (u16 i=0;i<60;i++) game_frame(&game,0,60);
    CHECK(game.player.body.ground && game.player.body.y>235*UNIT && game.player.body.y<=236*UNIT);
    Body slope={.x=360*UNIT,.y=236*UNIT,.width=12,.height=20,.ground=true};
    for (u16 i=0;i<70;i++)
    {
        game_move(&game,&slope,UNIT,GRAVITY,true,false,0);
        CHECK(slope.ground && !game_blocked(&game,&slope,slope.x,slope.y));
    }
    CHECK(slope.y==188*UNIT);
    game.player.body=(Body){.x=898*UNIT,.y=236*UNIT,.width=12,.height=20,.ground=true};
    game.player.battery=true;
    for (u16 i=0;i<CHARGE_TICKS;i++) game_frame(&game,B,60);
    CHECK(game.player.charge==CHARGE_TICKS);
    game_frame(&game,0,60);
    CHECK(game.player.dashing && game_tile(&game,57,15)==EMPTY);
    game_init(&game);
    for (u16 i=0;i<50;i++) game_frame(&game,0,50);
    CHECK(game.ticks==60);
    game_frame(&game,START,50);
    for (u16 i=0;i<50;i++) game_frame(&game,0,50);
    CHECK(game.paused && game.ticks==60);
    return 0;
}
