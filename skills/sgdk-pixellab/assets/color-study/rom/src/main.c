#include <genesis.h>
#include "resources.h"

typedef struct {
    const char *name;
    const Image *background;
    const SpriteDefinition *actor, *item, *panel;
} Page;
static const Page pages[]={
    {"FANTASY",&fantasy_bg,&fantasy_actor,&fantasy_item,&fantasy_ui},
    {"SCI-FI",&scifi_bg,&scifi_actor,&scifi_item,&scifi_ui},
    {"MODERN",&modern_bg,&modern_actor,&modern_item,&modern_ui},
    {"COLONIAL",&colonial_bg,&colonial_actor,&colonial_item,&colonial_ui}
};

static u16 frame_tiles(const SpriteDefinition *sprite)
{
    return sprite->animations[0]->frames[0]->tileset->numTile;
}

static u16 draw_sprite(const SpriteDefinition *definition,s16 x,s16 y,u16 palette,u16 tile,u16 *count)
{
    const AnimationFrame *frame=definition->animations[0]->frames[0];
    VDP_loadTileSet(frame->tileset,tile,DMA);
    u16 next=tile;
    for(u16 i=0;i<(frame->numSprite & 0x7F);i++)
    {
        const FrameVDPSprite *piece=&frame->frameVDPSprites[i];
        VDP_setSpriteFull(*count,x+piece->offsetX,y+piece->offsetY,piece->size,
            TILE_ATTR_FULL(palette,TRUE,FALSE,FALSE,next),*count+1);
        next+=piece->numTile;(*count)++;
    }
    return tile+frame->tileset->numTile;
}

static void show_page(u16 index)
{
    const Page *page=&pages[index];
    const u16 end=TILE_USER_INDEX+page->background->tileset->numTile+
        frame_tiles(page->actor)+frame_tiles(page->item)+frame_tiles(page->panel);
    SYS_disableInts();
    VDP_setEnable(FALSE);
    VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);
    VDP_clearSprites();
    if(end<=TILE_FONT_INDEX)
    {
        /* The Image has a 64-entry palette table. Load only its owned lines;
         * loading the whole table after the sprites/UI would erase their colors. */
        PAL_setPalette(PAL0,page->background->palette->data,DMA);
        PAL_setPalette(PAL1,page->background->palette->data+16,DMA);
        PAL_setPalette(PAL2,page->actor->palette->data,DMA);
        PAL_setPalette(PAL3,page->panel->palette->data,DMA);
        VDP_setBackgroundColor(0);
        VDP_drawImageEx(BG_B,page->background,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),4,6,FALSE,TRUE);
        u16 count=0,tile=TILE_USER_INDEX+page->background->tileset->numTile;
        tile=draw_sprite(page->actor,88,140,PAL2,tile,&count);
        tile=draw_sprite(page->item,128,148,PAL2,tile,&count);
        draw_sprite(page->panel,224,8,PAL3,tile,&count);
        VDP_setSpriteLink(count-1,0);VDP_updateSprites(count,DMA);
        VDP_setTextPalette(PAL3);
        VDP_drawText(page->name,2,1);
        VDP_drawText("BG 0+1   ACTORS 2   UI 3",6,24);
        VDP_drawText("A / START NEXT   B PREVIOUS",7,26);
    }
    else
    {
        PAL_setColor(15,RGB24_TO_VDPCOLOR(0xFFFFFF));
        VDP_setTextPalette(PAL0);VDP_drawText("ART RESERVATION DOES NOT FIT",4,12);
    }
    VDP_setEnable(TRUE);SYS_enableInts();
}

int main(bool hard_reset)
{
    (void)hard_reset;
    JOY_init();
    VDP_setScreenWidth320();VDP_setScreenHeight224();
    VDP_setPlaneSize(64,32,TRUE);
    VDP_setWindowHPos(FALSE,0);VDP_setWindowVPos(FALSE,0);
    u16 page=0,previous=0;
    show_page(page);
    while(TRUE)
    {
        SYS_doVBlankProcess();
        const u16 held=JOY_readJoypad(JOY_1),pressed=held & ~previous;
        previous=held;
        if(pressed & (BUTTON_A|BUTTON_START)) {page=(page+1)%4;show_page(page);}
        else if(pressed & BUTTON_B) {page=(page+3)%4;show_page(page);}
    }
    return 0;
}
