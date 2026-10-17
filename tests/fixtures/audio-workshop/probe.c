#include <genesis.h>
#include "checks.h"
#include "resources.h"
#include "desk_font.h"
__attribute__((noipa,used,externally_visible))
int main(bool reset)
{
    (void)reset;u32 trace=0;int e=contracts();if(!e)e=soak(256,&trace);if(e)return e;
    volatile u16 *out=(volatile u16 *)0xc00000;*out=sizeof(AdDirector);*out=trace>>16;*out=trace;
    *out=desk.tileset->numTile;*out=desk.tilemap->w;*out=desk.tilemap->h;
    for(u16 i=0;i<16;i++)*out=desk.palette->data[i];
    for(u16 i=0;i<768;i++){*out=desk_font[i]>>16;*out=desk_font[i];}
    return 0;
}
