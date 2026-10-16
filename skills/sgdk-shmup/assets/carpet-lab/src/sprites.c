#include "view.h"
#include "resources.h"
static u16 count,text_line;
static char cached_text[2][BANNER_LINE_TILES+1];
void sprites_invalidate_text(void) { memset(cached_text,0,sizeof cached_text); }
void sprites_begin(void) { count=0; text_line=0; }
static void piece(s16 x,s16 y,u8 size,u16 tile)
{
    if (count>=80) return;
    VDP_setSpriteFull(count,x,y,size,tile,count+1); count++;
}
void sprite_draw(Art art,s16 x,s16 y,u16 frame,bool flip)
{
    const Visual *visual=&visuals[art]; const SpriteDefinition *definition=visual->definition;
    if (!definition) return;
    const Animation *animation=definition->animations[0]; if (frame>=animation->numFrame) frame%=animation->numFrame;
    const AnimationFrame *image=animation->frames[frame];
    s16 width=(s16)definition->w,height=(s16)definition->h;
    x=x/UNIT-width/2; y=y/UNIT-height/2;
    if (x+width<=0 || x>=320 || y+height<=0 || y>=224) return;
    u16 tile=visual->tiles[frame],number=image->numSprite&0x7F;
    for (u16 i=0;i<number;i++)
    {
        const FrameVDPSprite *p=&image->frameVDPSprites[i];
        piece(x+(flip?p->offsetXFlip:p->offsetX),y+p->offsetY,p->size,
            TILE_ATTR_FULL(visual->palette,FALSE,FALSE,flip,tile));
        tile+=p->numTile;
    }
}
void sprite_shield(s16 x,s16 y,u16 charges)
{
    const Visual *visual=&visuals[ART_EFFECT];
    /* Each quarter uses the existing 16px effect tiles. A 32px outline needs
     * no new patterns; mirrored corner geometry closes the aura around rider. */
    for (u16 corner=0;corner<4;corner++)
    {
        s16 left=x/UNIT-16+(corner&1)*16,top=y/UNIT-16+(corner>>1)*16;
        piece(left,top,SPRITE_SIZE(2,2),TILE_ATTR_FULL(visual->palette,FALSE,corner>=2,corner&1,visual->tiles[11]));
    }
    s16 marker_y=y-18*UNIT;
    if (marker_y<(HUD_HEIGHT+8)*UNIT) marker_y=(HUD_HEIGHT+8)*UNIT;
    for (u16 charge=0;charge<charges;charge++)
        sprite_draw(ART_EFFECT,x+((s16)charge*12-(charges==2?6:0))*UNIT,marker_y,4,FALSE);
}
void sprite_text(const char *text,s16 x,s16 y)
{
    if (text_line>=2) return;
    u16 slot=text_line++,length=strlen(text);
    if (length>BANNER_LINE_TILES) length=BANNER_LINE_TILES;
    char truncated[BANNER_LINE_TILES+1]; memcpy(truncated,text,length); truncated[length]=0;
    u16 tile=banner_tile+slot*BANNER_LINE_TILES;
    if (strcmp(cached_text[slot],truncated))
    {
        u32 pixels[BANNER_LINE_TILES*8];
        for (u16 i=0;i<length;i++)
        {
            u16 glyph=(u8)text[i]; if (glyph<32 || glyph>=128) glyph='?';
            memcpy(pixels+i*8,gradient_font.tiles+(glyph-32)*8,32);
        }
        VDP_loadTileData(pixels,tile,length,DMA_QUEUE_COPY);
        memcpy(cached_text[slot],text,length); cached_text[slot][length]=0;
    }
    /* H40 allows 20 sprite entries per line. Four adjacent glyph tiles share
     * one 32px sprite, rather than losing letters after the twentieth glyph. */
    for (u16 i=0;i<length;i+=4)
    {
        u16 width=length-i; if (width>4) width=4;
        piece(x+i*8,y,SPRITE_SIZE(width,1),TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,tile+i));
    }
}
void sprites_end(void)
{
    if (count) VDP_setSpriteLink(count-1,0);
    VDP_updateSprites(count,DMA_QUEUE);
}
