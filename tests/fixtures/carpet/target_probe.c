#include "view.h"
#include "resources.h"
#define CHECK(value) do { if (!(value)) return __LINE__; } while (0)
static Game game;
static u16 frame_tiles(const SpriteDefinition *definition)
{
    const Animation *animation=definition->animations[0]; u16 total=0;
    for (u16 i=0;i<animation->numFrame;i++)
    {
        bool unique=TRUE;
        for (u16 j=0;j<i;j++) if (animation->frames[i]->tileset==animation->frames[j]->tileset) unique=FALSE;
        if (unique) total+=animation->frames[i]->tileset->numTile;
    }
    return total;
}
/* Direct-entry execution checks model/resource contracts, not VDP timing. */
__attribute__((noipa)) int main(bool hard_reset)
{
    (void)hard_reset;
    CHECK(gradient_font.numTile==96 && hero_image.w==32 && hero_image.h==32);
    CHECK(hero_image.animations[0]->numFrame==5 && hero_image.maxNumSprite==1);
    CHECK(moon_image.w==16 && moon_image.h==16 && ribbon_image.w==32 && ribbon_image.h==16);
    CHECK(ribbon_image.maxNumSprite==1 && button_image.animations[0]->numFrame==3);
    const Image *far[]={&far_1,&far_2,&far_3},*near[]={&near_1,&near_2,&near_3};
    const SpriteDefinition *boss[]={&scarab_image,&serpent_image,&lantern_image};
    u16 scenery=16+1+hud_tiles.numTile+health_tiles.numTile+label_font.numTile+1;
    u16 limit=0xC000/32-96-FRAME_CACHE_TILES;
    u16 common=frame_tiles(&hero_image)+frame_tiles(&teapot_image)+frame_tiles(&kite_image)+
        frame_tiles(&beetle_image)+frame_tiles(&sun_image)+frame_tiles(&moon_image)+
        frame_tiles(&storm_image)+frame_tiles(&ribbon_image)+frame_tiles(&effects_image);
    for (u16 i=0;i<3;i++)
    {
        CHECK(scenery+far[i]->tileset->numTile+near[i]->tileset->numTile<=limit);
        CHECK(common+frame_tiles(boss[i])+BANNER_LINE_TILES*2<=FRAME_CACHE_TILES);
        CHECK(boss[i]->w==64 && boss[i]->h==64 && boss[i]->maxNumSprite<=4);
        CHECK(boss[i]->animations[0]->numFrame==5);
    }
    CHECK(scenery+title_image.tileset->numTile+title_font.numTile<=limit);
    CHECK(scenery+far_3.tileset->numTile+title_font.numTile<=limit);
    visuals[ART_HERO].definition=&hero_image;visuals[ART_HERO].palette=PAL2;
    visuals[ART_HERO].tiles[0]=123;sprites_begin();sprite_draw(ART_HERO,-7*UNIT,10*UNIT,0,TRUE);
    const FrameVDPSprite *piece=&hero_image.animations[0]->frames[0]->frameVDPSprites[0];
    CHECK(vdpSpriteCache[0].x==(u16)(128-23+piece->offsetXFlip));
    CHECK(vdpSpriteCache[0].y==(u16)(128-6+piece->offsetY));
    CHECK(vdpSpriteCache[0].link==1);
    game_begin(&game,MOONWEAVE);game.state=PLAY;game.invulnerable=10000;
    for (u16 i=0;i<50;i++) game_frame(&game,RIGHT,50);
    CHECK(game.ticks==60 && game.x==184*UNIT);
    game_frame(&game,START,50);u32 tick=game.ticks;
    for (u16 i=0;i<50;i++) game_frame(&game,0,50);
    CHECK(game.paused && game.ticks==tick);
    game_begin(&game,MOONWEAVE);game.state=PLAY;game.meter=2;game_upgrade(&game);
    game.meter=3;game_upgrade(&game);CHECK(game.weapon==3);
    game.speed=3;game.meter=1;game.events=0;game_upgrade(&game);
    CHECK(game.meter==1 && (game.events&EV_DENIED));
    game.gems[0]=(Gem){.x=game.x,.y=game.y,.active=TRUE};game.meter=3;game_step(&game,0,0);
    CHECK(game.meter==4);
    game.enemies[0]=(Enemy){.x=180*UNIT,.y=game.y,.hp=10,.active=TRUE};
    game_player(&game,A,TRUE);game_combat(&game);
    CHECK(game.ribbon_active && game.enemies[0].hp==8 && game.shots[0].active && game.shots[1].active);
    for (u16 i=0;i<5;i++){game_player(&game,A,TRUE);game_combat(&game);}
    CHECK(game.enemies[0].hp==8);game_player(&game,A,TRUE);game_combat(&game);CHECK(game.enemies[0].hp==6);
    game_player(&game,0,TRUE);CHECK(!game.ribbon_active);
    game_begin(&game,MOONWEAVE);game.state=PLAY;
    game_emit(&game,TRUE,76*UNIT,100*UNIT,8*UNIT,0,1,1);
    for(u16 i=0;i<28;i++)game_combat(&game);
    CHECK(game.shots[0].x==228*UNIT && game.shots[0].y==100*UNIT && game.shots[0].vx==-8*UNIT);
    game_begin(&game,STORMGLASS);game.state=PLAY;
    for(u16 i=0;i<4;i++)game.enemies[i]=(Enemy){.x=(100+20*i)*UNIT,.y=100*UNIT,.hp=5,.active=TRUE};
    game_emit(&game,TRUE,96*UNIT,100*UNIT,6*UNIT,0,5,2);game_combat(&game);
    CHECK(game.enemies[0].hp==3 && game.enemies[1].hp==4 && game.enemies[2].hp==4 && game.enemies[3].hp==5);
    vu16 *port=(vu16 *)VDP_DATA_PORT;
    for(u16 i=0;i<16;i++){*port=direction_x[i];*port=direction_y[i];}
    return 0;
}
