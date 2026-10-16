#include <genesis.h>
#include "checks.h"
#include "resources.h"
#include "font.h"
__attribute__((noipa,used,externally_visible))
int main(bool hard_reset)
{
    (void)hard_reset;
    int e=check_projection();if(e)return e;e=check_relations();if(e)return e;e=check_sort();if(e)return e;
    e=check_world(32);if(e)return e;e=check_cache_and_timing();if(e)return e;e=check_soak(128);if(e)return e;
    volatile u16 *port=(volatile u16 *)0xC00000;
    *port=sizeof(IlGame);*port=sizeof(IlCache);*port=sizeof(IlBox);*port=sizeof(IlItem);
    u32 digest=trace_digest(128);*port=(u16)(digest>>16);*port=(u16)digest;
    *port=ground.tileset->numTile;*port=ground.tilemap->w;*port=ground.tilemap->h;
    const SpriteDefinition *defs[]={&hero,&deck,&post,&ramp_l,&ramp_r,&tower,&tree,&fountain,&marker};
    for(u16 i=0;i<9;i++) {
        const SpriteDefinition *d=defs[i];*port=d->w;*port=d->h;*port=d->maxNumTile;*port=d->maxNumSprite;*port=d->animations[0]->numFrame;
        for(u16 j=0;j<16;j++)*port=d->palette->data[j];
    }
    for(u16 i=0;i<768;i++){*port=(u16)(font_tiles[i]>>16);*port=(u16)font_tiles[i];}
    for(u16 i=0;i<IL_STATIC;i++){*port=(u16)il_props[i].x;*port=(u16)il_props[i].y;*port=(u16)il_props[i].z;*port=il_props[i].kind;}
    return 0;
}
