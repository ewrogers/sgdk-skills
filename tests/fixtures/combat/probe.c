#include <genesis.h>
#include "checks.h"
#include "resources.h"
#include "font.h"
__attribute__((noipa,used,externally_visible))
int main(bool hard_reset)
{
    (void)hard_reset;int e=contracts();u32 trace=0;if(!e)e=check_soak(256,&trace);if(e)return e;
    volatile u16 *port=(volatile u16 *)0xC00000;
    *port=sizeof(CcGame);*port=sizeof(CcActor);*port=(u16)(trace>>16);*port=(u16)trace;
    *port=court.tileset->numTile;*port=title.tileset->numTile;*port=court.tilemap->w;*port=court.tilemap->h;
    const SpriteDefinition *defs[]={&fighters,&bolts,&sparks,&swords};
    for(u16 i=0;i<4;i++) {
        const SpriteDefinition *d=defs[i];*port=d->w;*port=d->h;*port=d->maxNumSprite;*port=d->maxNumTile;*port=d->numAnimation;
        for(u16 a=0;a<d->numAnimation;a++)*port=d->animations[a]->numFrame;
        for(u16 c=0;c<16;c++)*port=d->palette->data[c];
    }
    for(u16 i=0;i<768;i++){*port=(u16)(font_tiles[i]>>16);*port=(u16)font_tiles[i];}
    return 0;
}
