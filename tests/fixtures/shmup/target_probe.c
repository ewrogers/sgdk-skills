#include <genesis.h>
#include "game.h"
#include "resources.h"
#include "view.h"
#include "sprite_list.h"
#define CHECK(value) do { if (!(value)) return __LINE__; } while (0)
static Game game;
static u16 frame_tiles(const SpriteDefinition *definition)
{
    u16 total=0;
    for (u16 a=0;a<definition->numAnimation;a++)
        for (u16 f=0;f<definition->animations[a]->numFrame;f++)
            total+=definition->animations[a]->frames[f]->tileset->numTile;
    return total;
}
/* Console startup ignores main's result. Keep it observable to direct entry. */
__attribute__((noipa)) int main(bool hard_reset)
{
    (void)hard_reset;
    /* The visibility member is a piece bitmask, not SpriteVisibility. */
    Sprite probe={0};
    CHECK(SPR_getVisibility(&probe)==HIDDEN);
    probe.visibility=0xFFFF;
    CHECK(SPR_getVisibility(&probe)==VISIBLE);
    CHECK(sci_font.numTile==96);
    CHECK(frame_tiles(&ship_image)+frame_tiles(&enemy_image)+frame_tiles(&boss_image)+frame_tiles(&reaver_image)+
        frame_tiles(&bullet_image)+frame_tiles(&pickup_image)+frame_tiles(&effect_image)+
        frame_tiles(&utility_image)+frame_tiles(&twin_image)<=FRAME_CACHE_TILES);
    CHECK(1+1+ENEMY_COUNT+SHOT_COUNT+BULLET_COUNT+PICKUP_COUNT+EFFECT_COUNT+2+3+8<=80);
    CHECK(twin_image.animations[0]->numFrame==2 && twin_image.maxNumSprite==1);
    CHECK(utility_image.animations[0]->numFrame==4 && utility_image.maxNumSprite==1);
    CHECK(title_image.tilemap->w==28 && title_image.tilemap->h==14);
    CHECK(title_image.tileset->numTile>0 && title_image.tileset->numTile<=392);
    CHECK(ship_image.maxNumTile==4 && boss_image.maxNumTile==16 && bullet_image.maxNumTile==1);
    CHECK(ship_image.animations[0]->numFrame==4 && enemy_image.animations[0]->numFrame==6);
    CHECK(bullet_image.animations[0]->numFrame==8 && pickup_image.animations[0]->numFrame==4);
    CHECK(boss_image.animations[0]->numFrame==7 && effect_image.animations[0]->numFrame==4);
    CHECK(ship_image.maxNumSprite==1 && enemy_image.maxNumSprite==1 && boss_image.maxNumSprite==1);
    CHECK(bullet_image.maxNumSprite==1 && pickup_image.maxNumSprite==1 && effect_image.maxNumSprite==1);
    CHECK(reaver_image.maxNumSprite==1 && reaver_image.animations[0]->numFrame==7);
    Actor actor={&ship_image,PAL1}; u16 tiles[]={123};
    sprite_list_begin(240);
    sprite_list_place(&actor,-7*UNIT,10*UNIT,16,true,0,tiles);
    const FrameVDPSprite *piece=&ship_image.animations[0]->frames[0]->frameVDPSprites[0];
    CHECK(vdpSpriteCache[0].x==(u16)(128-15+piece->offsetX));
    CHECK(vdpSpriteCache[0].y==(u16)(128+2+piece->offsetY));
    CHECK(vdpSpriteCache[0].link==1);
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
    game_start(&game); game.state=BOSS; game.boss=(Boss){.hp=10,.age=BOSS_READY_TICKS+10,.active=true};
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
