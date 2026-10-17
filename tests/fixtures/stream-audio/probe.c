#include <genesis.h>
#define RT_PROBE
#include "checks.h"
#include "controls.h"
__attribute__((noipa,used,externally_visible))
int main(bool reset)
{
    (void)reset;
    if(rt_controls(BUTTON_A)!=R_WARP || rt_controls(BUTTON_B)!=R_SLOW ||
       rt_controls(BUTTON_C)!=R_JINGLE || rt_controls(BUTTON_START)!=R_PAUSE)return 1001;
    u32 pal=0,ntsc=0;int e=route_checks(50,&pal);if(!e)e=route_checks(60,&ntsc);
    if(e)return e;
    volatile u16 *out=(volatile u16*)0xC00000;
    *out=pal>>16;*out=pal;*out=ntsc>>16;*out=ntsc;*out=sizeof(Route);return 0;
}
