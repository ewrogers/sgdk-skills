#include <genesis.h>
#include "resources.h"

typedef struct {
    const char *name;
    const Image *normal, *lit;
    const SpriteDefinition *mask[2], *actor, *object;
} Page;

static const Page pages[]={
    {"MODERN",&modern_normal,&modern_lit,{&modern_mask0,&modern_mask1},&modern_actor,&modern_object},
    {"FANTASY",&fantasy_normal,&fantasy_lit,{&fantasy_mask0,&fantasy_mask1},&fantasy_actor,&fantasy_object},
    {"SCI-FI",&scifi_normal,&scifi_lit,{&scifi_mask0,&scifi_mask1},&scifi_actor,&scifi_object},
    {"COLONIAL",&colonial_normal,&colonial_lit,{&colonial_mask0,&colonial_mask1},&colonial_actor,&colonial_object}
};

static const AnimationFrame *first_frame(const SpriteDefinition *definition)
{
    return definition->animations[0]->frames[0];
}

static u16 draw_sprite(const SpriteDefinition *definition,s16 x,s16 y,u16 palette,u16 tile,u16 *count)
{
    const AnimationFrame *frame=first_frame(definition);
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

static void draw_number(u16 value,u16 x,u16 y)
{
    char digits[6];u16 at=5;
    digits[at]=0;
    do {digits[--at]='0'+value%10;value/=10;}while(value);
    VDP_drawText(digits+at,x,y);
}

static void show_page(u16 page_index,bool lighting)
{
    const Page *page=&pages[page_index];
    const Image *image=lighting?page->lit:page->normal;
    u16 mask_tiles=0,mask_pieces=0;
    for(u16 i=0;i<2;i++)
    {
        const AnimationFrame *frame=first_frame(page->mask[i]);
        mask_tiles+=frame->tileset->numTile;mask_pieces+=frame->numSprite & 0x7F;
    }
    const u16 end=TILE_USER_INDEX+image->tileset->numTile+
        first_frame(page->actor)->tileset->numTile+first_frame(page->object)->tileset->numTile+
        (lighting?mask_tiles:0);
    SYS_disableInts();VDP_setEnable(FALSE);
    VDP_setHilightShadow(lighting);
    VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);VDP_clearSprites();
    PAL_setColor(63,RGB24_TO_VDPCOLOR(0xFFFFFF));
    VDP_setTextPalette(PAL3);VDP_setTextPriority(TRUE);
    if(end<=TILE_FONT_INDEX && mask_pieces+2<=80)
    {
        PAL_setPalette(PAL0,image->palette->data,DMA);
        PAL_setPalette(PAL1,image->palette->data+16,DMA);
        PAL_setPalette(PAL2,page->actor->palette->data,DMA);
        VDP_setBackgroundColor(0);
        /* High-priority base keeps unmasked pixels normal. PAL3 sprite pixels
         * 14/15 raise/lower brightness; their CRAM RGB values are irrelevant.
         * PAL3 color 15 remains ordinary white for background text pixels. */
        VDP_drawImageEx(BG_B,image,TILE_ATTR_FULL(PAL0,TRUE,FALSE,FALSE,TILE_USER_INDEX),4,6,FALSE,TRUE);
        u16 count=0,tile=TILE_USER_INDEX+image->tileset->numTile;
        /* Earlier SAT entries win: actors stay in front of the lighting mask. */
        tile=draw_sprite(page->actor,88,140,PAL2,tile,&count);
        tile=draw_sprite(page->object,128,148,PAL2,tile,&count);
        if(lighting)
            for(u16 i=0;i<2;i++)tile=draw_sprite(page->mask[i],32+i*128,48,PAL3,tile,&count);
        VDP_setSpriteLink(count-1,0);VDP_updateSprites(count,DMA);
        VDP_drawText(page->name,2,1);
        VDP_drawText(lighting?"SHADOW + HIGHLIGHT":"NORMAL",2,3);
        VDP_drawText("SAME TWO BACKGROUND PALETTES",2,23);
        if(lighting)
        {
            VDP_drawText("MASK SPRITES",2,24);draw_number(mask_pieces,15,24);
            VDP_drawText("TILES",21,24);draw_number(mask_tiles,28,24);
        }
        VDP_drawText("A/START TOGGLE  B NEXT  C BACK",2,26);
    }
    else VDP_drawText("COMPARISON EXCEEDS VRAM / SAT",2,12);
    VDP_setEnable(TRUE);SYS_enableInts();
}

int main(bool hard_reset)
{
    (void)hard_reset;JOY_init();
    VDP_setScreenWidth320();VDP_setScreenHeight224();VDP_setPlaneSize(64,32,TRUE);
    VDP_setWindowHPos(FALSE,0);VDP_setWindowVPos(FALSE,0);
    u16 page=0,previous=0;bool lighting=FALSE;
    show_page(page,lighting);
    while(TRUE)
    {
        SYS_doVBlankProcess();
        const u16 held=JOY_readJoypad(JOY_1),pressed=held & ~previous;previous=held;
        if(pressed & (BUTTON_A|BUTTON_START)){lighting=!lighting;show_page(page,lighting);}
        else if(pressed & BUTTON_B){page=(page+1)%4;show_page(page,lighting);}
        else if(pressed & BUTTON_C){page=(page+3)%4;show_page(page,lighting);}
    }
    return 0;
}
