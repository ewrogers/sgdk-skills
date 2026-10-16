#include <genesis.h>
#include "resources.h"
#include "expected.h"
#define CHECK(x) do {if(!(x))return __LINE__;}while(0)

static u8 mask_pixel(const AnimationFrame *frame,u16 x,u16 y)
{
    u16 tile=0;
    for(u16 i=0;i<(frame->numSprite & 0x7F);i++)
    {
        const FrameVDPSprite *piece=&frame->frameVDPSprites[i];
        const u16 w=(((piece->size>>2)&3)+1)*8,h=((piece->size&3)+1)*8;
        if(x>=piece->offsetX && x<piece->offsetX+w && y>=piece->offsetY && y<piece->offsetY+h)
        {
            const u16 px=x-piece->offsetX,py=y-piece->offsetY;
            const u16 pattern=tile+(px/8)*(h/8)+py/8;
            return(frame->tileset->tiles[pattern*8+py%8]>>(28-(px%8)*4))&15;
        }
        tile+=piece->numTile;
    }
    return 0;
}

__attribute__((noipa)) int main(bool hard_reset)
{
    (void)hard_reset;
    const SpriteDefinition *masks[]={&modern_mask0,&modern_mask1,&fantasy_mask0,&fantasy_mask1,
        &scifi_mask0,&scifi_mask1,&colonial_mask0,&colonial_mask1};
    const Image *normal[]={&modern_normal,&fantasy_normal,&scifi_normal,&colonial_normal};
    const Image *lit[]={&modern_lit,&fantasy_lit,&scifi_lit,&colonial_lit};
    for(u16 p=0;p<4;p++)
    {
        CHECK(normal[p]->palette->length==64 && lit[p]->palette->length==64);
        for(u16 c=0;c<64;c++)CHECK(normal[p]->palette->data[c]==lit[p]->palette->data[c]);
        CHECK(lit[p]->tilemap->w==32 && lit[p]->tilemap->h==16);
        for(u16 t=0;t<512;t++)CHECK(((lit[p]->tilemap->tilemap[t]>>13)&3)<=1);
    }
    for(u16 m=0;m<8;m++)
    {
        const SpriteDefinition *definition=masks[m];
        CHECK(definition->w==128 && definition->h==128 && definition->numAnimation==1);
        CHECK(definition->animations[0]->numFrame==1);
        const AnimationFrame *frame=definition->animations[0]->frames[0];
        CHECK((frame->numSprite & 0x7F)<=16 && frame->tileset->numTile<=256);
        CHECK(frame->tileset->compression==COMPRESSION_NONE);
        for(u16 i=0;i<64;i++)CHECK(mask_pixel(frame,(i*37)%128,(i*43+i*i*7)%128)==expected[m][i]);
    }
    return 0;
}
