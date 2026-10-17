#include <genesis.h>
#include "checks.h"
#include "blocks_checks.h"
#include "resources.h"
#include "font.h"
__attribute__((noipa,used,externally_visible))
int main(bool hard_reset)
{
    (void)hard_reset;int e=contracts();if(!e)e=block_contracts();u32 h=0,bh=0;if(!e)e=soak(256,&h);if(!e)e=block_soak(256,&bh);if(e)return e;h^=bh;
    volatile u16 *out=(volatile u16 *)0xC00000;
    *out=sizeof(PzBoard);*out=sizeof(PzState);*out=h>>16;*out=h;
    *out=garden.tileset->numTile;*out=atlas.tileset->numTile;*out=atlas.tilemap->w;*out=atlas.tilemap->h;
    const SpriteDefinition *defs[]={&hero,&crate};
    for(u16 i=0;i<2;i++) {
        const SpriteDefinition *d=defs[i];*out=d->w;*out=d->h;*out=d->maxNumSprite;*out=d->maxNumTile;*out=d->numAnimation;
        for(u16 a=0;a<d->numAnimation;a++)*out=d->animations[a]->numFrame;
        for(u16 c=0;c<16;c++)*out=d->palette->data[c];
    }
    for(u16 i=0;i<768;i++){*out=font_tiles[i]>>16;*out=font_tiles[i];}
    return 0;
}
