#include <genesis.h>
#include "resources.h"
#include "checks.h"

__attribute__((noipa,used,externally_visible))
int main(bool hard_reset)
{
    (void)hard_reset;
    int error=check_pool();if(error)return error;
    error=check_bursts();if(error)return error;
    error=check_motion();if(error)return error;
    error=check_render();if(error)return error;
    volatile u16 *port=(volatile u16 *)0xC00000;
    *port=sizeof(FxLab);*port=sizeof(FxRender);*port=sizeof(FxParticle);*port=sizeof(VDPSprite);
    FxLab s;fx_init(&s,0xCAFE);
    for(u16 i=0;i<60;i++)fx_frame(&s,i<30 ? FX_RIGHT : 0,60);
    u32 digest=state_digest(&s);*port=(u16)(digest>>16);*port=(u16)digest;
    const SpriteDefinition *defs[]={&fx_image,&lantern_image};
    for(u16 d=0;d<2;d++) {
        const SpriteDefinition *def=defs[d];
        if(def->numAnimation!=1 || def->maxNumSprite!=1)return 1000+d;
        *port=def->w;*port=def->h;*port=def->animations[0]->numFrame;*port=def->maxNumTile;
        for(u16 c=0;c<16;c++)*port=def->palette->data[c];
        for(u16 f=0;f<def->animations[0]->numFrame;f++) {
            const AnimationFrame *frame=def->animations[0]->frames[f];
            const TileSet *tiles=frame->tileset;
            if(tiles->compression!=COMPRESSION_NONE || (frame->numSprite&127)!=1)return 1010+d;
            const FrameVDPSprite *piece=&frame->frameVDPSprites[0];
            if(piece->offsetX || piece->offsetY || piece->size!=SPRITE_SIZE(def->w/8,def->h/8))return 1015+d;
            *port=tiles->numTile;
            for(u16 i=0;i<tiles->numTile*8;i++) {
                u32 row=tiles->tiles[i];*port=(u16)(row>>16);*port=(u16)row;
            }
        }
    }
    fx_init(&s,1);fx_clear(&s);
    for(u16 i=0;i<FX_CAPACITY;i++)fx_spawn(&s,FX_TRAIL,12+(i%8)*36,48+(i/8)*16,0,0,180);
    FxRender optimized,reference;
    *port=0xEA01;fx_select(&s,&optimized,64);*port=0xEA02;
    *port=0xEA03;fx_reference_select(&s,&reference,64);*port=0xEA04;
    if(!render_equal(&optimized,&reference))return 1020;
    /* Real mixed pressure: weather at maximum density, moving emitter, repeated
     * bursts and smoke. Include spawn frames rather than profiling only selection. */
    fx_init(&s,0xCAFE);s.weather=2;s.density=3;
    for(u16 i=0;i<180;i++)fx_frame(&s,0,60);
    for(u16 i=0;i<60;i++) {
        u16 held=(i<30 ? FX_RIGHT : FX_LEFT) | (i%12==0 ? FX_A : 0);
        *port=0xEA05;fx_frame(&s,held,60);*port=0xEA06;
        *port=0xEA07;fx_select(&s,&optimized,64);*port=0xEA08;
        fx_reference_select(&s,&reference,64);
        if(!render_equal(&optimized,&reference))return 1021;
    }
    return 0;
}
