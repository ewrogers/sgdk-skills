#include <genesis.h>
#include "resources.h"
#include "checks.h"
__attribute__((noipa,used,externally_visible))
int main(bool hard_reset)
{
    (void)hard_reset;
    int e=check_sweep();if(e)return e;e=check_ray();if(e)return e;
    e=check_cast_slide();if(e)return e;e=check_invalid();if(e)return e;e=check_demo();if(e)return e;
    volatile u16 *port=(volatile u16 *)0xC00000;
    *port=sizeof(CcBox);*port=sizeof(CcHit);*port=sizeof(CcMove);*port=sizeof(CcDemo);
    u32 digest=trace_digest();*port=(u16)(digest>>16);*port=(u16)digest;
    const SpriteDefinition *defs[]={&actor,&shot,&wall,&dot,&cursor};
    for(u16 d=0;d<5;d++) {
        const SpriteDefinition *def=defs[d];
        if(def->numAnimation!=1 || def->animations[0]->numFrame!=1 || def->maxNumSprite!=1)return 1000+d;
        *port=def->w;*port=def->h;*port=def->maxNumTile;
        for(u16 i=0;i<16;i++)*port=def->palette->data[i];
        const TileSet *tiles=def->animations[0]->frames[0]->tileset;
        if(tiles->compression!=COMPRESSION_NONE)return 1010+d;
        *port=tiles->numTile;
        for(u16 i=0;i<tiles->numTile*8;i++){*port=(u16)(tiles->tiles[i]>>16);*port=(u16)tiles->tiles[i];}
    }
    return 0;
}
