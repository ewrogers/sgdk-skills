#include <genesis.h>
#include "checks.h"
#include "resources.h"
#include "font.h"
__attribute__((noipa,used,externally_visible))
int main(bool hard_reset)
{
    (void)hard_reset;
    int e=check_routes();if(e)return e;e=check_session();if(e)return e;e=check_respawn();if(e)return e;e=check_camera();if(e)return e;
    e=check_pickups_and_pause();if(e)return e;e=check_stun();if(e)return e;e=check_timing();if(e)return e;u32 trace=0;e=check_soak(256,&trace);if(e)return e;
    volatile u16 *port=(volatile u16 *)0xC00000;
    *port=sizeof(CpGame);*port=sizeof(CpPlayer);*port=(u16)(trace>>16);*port=(u16)trace;
    *port=world.tileset->numTile;*port=title.tileset->numTile;*port=world.tilemap->w;*port=world.tilemap->h;
    const SpriteDefinition *defs[]={&hero,&moth,&mote,&beacon,&pulse};
    for(u16 i=0;i<5;i++) {
        const SpriteDefinition *d=defs[i];*port=d->w;*port=d->h;*port=d->maxNumSprite;*port=d->maxNumTile;*port=d->numAnimation;
        for(u16 a=0;a<d->numAnimation;a++)*port=d->animations[a]->numFrame;
        for(u16 c=0;c<16;c++)*port=d->palette->data[c];
    }
    for(u16 i=0;i<768;i++){*port=(u16)(font_tiles[i]>>16);*port=(u16)font_tiles[i];}
    *port=JOY_1;*port=JOY_2;*port=JOY_3;*port=JOY_4;*port=JOY_5;*port=JOY_6;*port=JOY_7;*port=JOY_8;
    return 0;
}
