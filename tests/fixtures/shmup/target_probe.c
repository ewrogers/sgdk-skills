#include <genesis.h>
#include "game.h"
#include "resources.h"
#define CHECK(value) do { if (!(value)) return __LINE__; } while (0)
static Game game;
/* Console startup ignores main's result. Keep it observable to direct entry. */
__attribute__((noipa)) int main(bool hard_reset)
{
    (void)hard_reset;
    CHECK(ship_image.maxNumTile==4 && boss_image.maxNumTile==16 && bullet_image.maxNumTile==1);
    CHECK(ship_image.animations[0]->numFrame==4 && enemy_image.animations[0]->numFrame==6);
    CHECK(bullet_image.animations[0]->numFrame==3 && pickup_image.animations[0]->numFrame==2);
    CHECK(boss_image.animations[0]->numFrame==3 && effect_image.animations[0]->numFrame==4);
    CHECK(ship_image.maxNumSprite==1 && enemy_image.maxNumSprite==1 && boss_image.maxNumSprite==1);
    CHECK(bullet_image.maxNumSprite==1 && pickup_image.maxNumSprite==1 && effect_image.maxNumSprite==1);
    CHECK(game_video_layout_valid(64,32,0xE000,0xC000,0xD000));
    CHECK(!game_video_layout_valid(32,32,0xE000,0xC000,0xC000));
    game_start(&game);
    CHECK(game_pattern(&game,RING,120*UNIT,50*UNIT,0,0,0)==8);
    for (u16 i=0;i<4;i++) CHECK(game.bullets[i].vx==-game.bullets[i+4].vx && game.bullets[i].vy==-game.bullets[i+4].vy);
    CHECK(game_aim(100,0)==4 && game_aim(0,100)==8);
    game_start(&game); game.invulnerable=10000;
    for (u16 i=0;i<50;i++) game_frame(&game,A,50);
    CHECK(game.ticks==60 && game.x==120*UNIT);
    game_frame(&game,START,50); u32 ticks=game.ticks;
    for (u16 i=0;i<50;i++) game_frame(&game,0,50);
    CHECK(game.paused && game.ticks==ticks);
    game_start(&game); game.invulnerable=0;
    game_bullet(&game,game.x,game.y+5*UNIT,0,4*UNIT); game_projectiles(&game);
    CHECK(game.lives==2 && game.invulnerable==120);
    game_start(&game); game.state=BOSS; game.boss=(Boss){.hp=10,.age=100,.active=true};
    game_boss_damage(&game,20); CHECK(game.state==CLEAR && game.score==5000);
    game_boss_damage(&game,20); CHECK(game.score==5000);
    game_start(&game);
    vu16 *port=(vu16 *)VDP_DATA_PORT;
    for (u16 i=0;i<16;i++)
    {
        CHECK(game_bullet(&game,0,0,i,2*UNIT));
        *port=game.bullets[i].vx; *port=game.bullets[i].vy;
    }
    return 0;
}
