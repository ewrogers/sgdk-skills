#include "game.h"
#include "sprite_list.h"
/* One owner of the SAT. Resources are checked for one piece per frame at
 * startup. Order is explicit, and only visible pieces consume transfer space. */
static u16 sat_count;
static s16 clip_width;
void sprite_list_begin(u16 width) { sat_count=0; clip_width=(s16)width; }
void sprite_list_place(const Actor *sprite,s16 x,s16 y,u16 size,bool active,u16 frame,const u16 *tiles)
{
    if (!active || sat_count>=80) return;
    s16 extent=(s16)size;
    x=x/UNIT-extent/2; y=y/UNIT-extent/2;
    /* Keep these signed on the 68000, whose int is only 16 bits. */
    if (x+extent<=0 || x>=clip_width || y+extent<=0 || y>=FIELD_H) return;
    const FrameVDPSprite *piece=&sprite->definition->animations[0]->frames[frame]->frameVDPSprites[0];
    VDP_setSpriteFull(sat_count,x+piece->offsetX,y+piece->offsetY,piece->size,
        TILE_ATTR_FULL(sprite->palette,FALSE,FALSE,FALSE,tiles[frame]),sat_count+1);
    sat_count++;
}
void sprite_list_end(void)
{
    if (sat_count) VDP_setSpriteLink(sat_count-1,0);
    /* SGDK emits a hidden entry for zero sprites, clearing the previous list. */
    VDP_updateSprites(sat_count,DMA_QUEUE);
}
