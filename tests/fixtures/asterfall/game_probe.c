#include "game.h"
#include "ui_tiles.h"
#ifdef SGDK_GCC
#include <genesis.h>
#include "resources.h"
#else
#include <stdio.h>
#endif
#define CHECK(x) do{if(!(x))return __LINE__;}while(0)
static void press(Game *g,uint16_t key){game_step(g,key,key,key&(UP|DOWN|LEFT|RIGHT));}
static int checks(void)
{
    UITiles tiles={0};
    ui_tiles_set(&tiles,4,12,123);CHECK(tiles.dirty[12]&&tiles.cells[484]==123);
    tiles.dirty[12]=false;ui_tiles_set(&tiles,4,12,123);CHECK(!tiles.dirty[12]);
    ui_tiles_set(&tiles,40,12,456);CHECK(tiles.cells[484]==123&&!tiles.dirty[12]);
    ui_tiles_set(&tiles,0,28,456);CHECK(!tiles.dirty[12]);
    ui_tiles_clear(&tiles);CHECK(tiles.dirty[12]&&tiles.cells[484]==0);
    Game g;game_init(&g);CHECK(g.screen==INTRO);
    press(&g,START);CHECK(g.screen==TITLE);
    press(&g,CONFIRM);CHECK(g.screen==NAMING);
    press(&g,START);CHECK(g.screen==NAMING&&(g.events&CUE_DENIED));
    for(uint16_t i=0;i<10;i++)press(&g,CONFIRM);
    CHECK(g.name_length==8&&g.name[8]==0);
    press(&g,BACK);CHECK(g.name_length==7&&g.name[7]==0);
    g.keyboard=0;press(&g,UP);CHECK(g.keyboard==64);
    press(&g,RIGHT);CHECK(g.keyboard==65);press(&g,RIGHT);CHECK(g.keyboard==66);
    press(&g,CONFIRM);CHECK(g.screen==WORLD);
    for(uint16_t zone=0;zone<4;zone++)
    {
        g.screen=WORLD;g.selection=zone;press(&g,CONFIRM);
        CHECK(g.screen==FIELD&&g.zone==zone&&field_walkable(g.zone,g.hero.x,g.hero.y));
        for(uint16_t i=0;i<100;i++)game_step(&g,RIGHT,0,0);
        CHECK(field_walkable(g.zone,g.hero.x,g.hero.y));
        for(uint16_t i=0;i<100;i++)game_step(&g,UP|LEFT,0,0);
        CHECK(field_walkable(g.zone,g.hero.x,g.hero.y));
        press(&g,BACK);CHECK(g.screen==MENU);
        g.selection=0;press(&g,CONFIRM);CHECK(g.screen==STATUS);
        press(&g,BACK);CHECK(g.screen==MENU);
        g.selection=2;press(&g,CONFIRM);CHECK(g.screen==INVENTORY);
        press(&g,DOWN);CHECK(g.selection==2);press(&g,RIGHT);CHECK(g.selection==3);
        press(&g,CONFIRM);CHECK(g.detail);press(&g,BACK);CHECK(!g.detail&&g.screen==INVENTORY);
        press(&g,BACK);CHECK(g.screen==MENU&&g.selection==2);
        g.selection=3;press(&g,CONFIRM);CHECK(g.screen==SPELLS);
        press(&g,BACK);CHECK(g.screen==MENU&&g.selection==3);
        g.selection=6;press(&g,CONFIRM);CHECK(g.screen==OPTIONS);
        g.selection=1;g.music_volume=0;press(&g,LEFT);CHECK(g.music_volume==100);
        press(&g,RIGHT);CHECK(g.music_volume==0);
        press(&g,BACK);CHECK(g.screen==MENU&&g.selection==6);press(&g,BACK);CHECK(g.screen==FIELD);
        g.hero.x=g.npc[0].x;g.hero.y=g.npc[0].y;
        press(&g,CONFIRM);CHECK(g.screen==DIALOGUE&&g.dialogue_page==0);
        press(&g,CONFIRM);CHECK(g.screen==DIALOGUE&&g.screen_age>=300);
        press(&g,CONFIRM);CHECK(g.dialogue_page==1);
        press(&g,CONFIRM);press(&g,CONFIRM);CHECK(g.screen==FIELD);
    }
    for(uint16_t slot=0;slot<4;slot++)
    {
        g.screen=SAVES;g.selection=slot;press(&g,CONFIRM);
        CHECK(g.screen==FIELD&&g.zone==slot&&g.name_length>0&&g.name_length<=8);
    }
    CHECK(!field_walkable(4,144,96));CHECK(!field_walkable(0,-1,96));
    CHECK(!field_walkable(0,208,96));CHECK(!field_walkable(3,280,100));
    game_init(&g);for(uint16_t i=0;i<960;i++)game_step(&g,0,0,0);
    CHECK(g.screen==TITLE);
    return 0;
}
#ifdef SGDK_GCC
static int resource_checks(void)
{
    const Image *zones[]={&art_town,&art_dungeon,&art_forest,&art_coast};
    const SpriteDefinition *actors[]={&art_hero_idle,&art_hero_walk,&art_guide_idle,&art_guide_walk,&art_sage_idle,&art_sage_walk};
    CHECK(art_font_ivory.numTile==96&&art_font_gold.numTile==96&&art_font_title.numTile==104);
    const AnimationFrame *m0=art_title_mask0.animations[0]->frames[0];
    const AnimationFrame *m1=art_title_mask1.animations[0]->frames[0];
    CHECK((m0->numSprite&0x7F)<=8&&(m1->numSprite&0x7F)<=8);
    CHECK(m0->tileset->numTile+m1->tileset->numTile<=256);
    CHECK(16+96+104+16+art_panel.tileset->numTile+art_title.tileset->numTile+m0->tileset->numTile+m1->tileset->numTile<=1440);
    CHECK(art_map_location.tilemap->w==2&&art_travel_pin.w==16);
    for(u16 i=0;i<4;i++)
    {
        CHECK(zones[i]->tilemap->w==40&&zones[i]->tilemap->h==24);
        CHECK(zones[i]->palette->length==64);
        for(u16 t=0;t<960;t++)CHECK(((zones[i]->tilemap->tilemap[t]>>13)&3)<=1);
        /* 1,440 is the font start with the demo's normal 64x32 plane layout.
         * Include the maximum portrait reservation used by field dialogue. */
        CHECK(16+96+16+art_panel.tileset->numTile+zones[i]->tileset->numTile+108+64<=1440);
    }
    for(u16 i=0;i<6;i++)
    {
        CHECK(actors[i]->w==48&&actors[i]->h==48);
        CHECK(actors[i]->palette->length==16);
        CHECK(actors[i]->numAnimation==(i%2?4:1));
        for(u16 color=0;color<16;color++)CHECK(actors[i]->palette->data[color]==art_portrait_hero.palette->data[color]);
        for(u16 a=0;a<actors[i]->numAnimation;a++)
        {
            CHECK(actors[i]->animations[a]->numFrame==4);
            for(u16 f=0;f<4;f++)
            {
                const AnimationFrame *frame=actors[i]->animations[a]->frames[f];
                CHECK(frame->tileset->numTile<=36&&(frame->numSprite&0x7F)<=4);
            }
        }
    }
    return 0;
}
__attribute__((noipa)) int main(bool hard_reset)
{
    (void)hard_reset;int error=checks();return error?error:resource_checks();
}
#else
int main(void){int error=checks();if(error)fprintf(stderr,"Asterfall check failed at line %d\n",error);return error?1:0;}
#endif
