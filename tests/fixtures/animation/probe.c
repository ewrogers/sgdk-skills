#include <genesis.h>
#include "resources.h"
#include "checks.h"
__attribute__((noipa,used,externally_visible))
int main(bool hard_reset)
{
    (void)hard_reset;
    int e=check_clock();if(e)return e;e=check_actions();if(e)return e;
    e=check_interrupts();if(e)return e;e=check_timing();if(e)return e;
    e=check_soak(2000);if(e)return e;
    volatile u16 *port=(volatile u16 *)0xC00000;
    *port=sizeof(AlClock);*port=sizeof(AlActor);*port=sizeof(AlGame);*port=sizeof(AlBatch);
    u32 digest=trace_digest();*port=(u16)(digest>>16);*port=(u16)digest;
    const SpriteDefinition *defs[]={&hero,&dummy,&arc,&attack_box,&body_box,&target_box};
    for(u16 d=0;d<6;d++) {
        const SpriteDefinition *def=defs[d];
        if(def->numAnimation!=1 || def->maxNumSprite!=1)return 1000+d;
        *port=def->w;*port=def->h;*port=def->animations[0]->numFrame;*port=def->maxNumTile;
        for(u16 i=0;i<16;i++)*port=def->palette->data[i];
        for(u16 f=0;f<def->animations[0]->numFrame;f++) {
            const TileSet *tiles=def->animations[0]->frames[f]->tileset;
            if(tiles->compression!=COMPRESSION_NONE)return 1010+d;
            *port=tiles->numTile;
            for(u16 i=0;i<tiles->numTile*8;i++){*port=(u16)(tiles->tiles[i]>>16);*port=(u16)tiles->tiles[i];}
        }
    }
    return 0;
}
