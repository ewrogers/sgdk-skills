#include <genesis.h>
#include "checks.h"
#include "controls.h"
__attribute__((noipa,used,externally_visible))
int main(bool hardReset)
{
    (void)hardReset;
    if(courier_controls(BUTTON_A)!=C_A || courier_controls(BUTTON_B|BUTTON_C))return 1001;
    Courier g;courier_init(&g,0);g.room=1;g.parcel=1;g.x=260;g.y=144;
    courier_frame(&g,courier_controls(BUTTON_B|BUTTON_C),50);
    if(g.won)return 1002;
    courier_frame(&g,courier_controls(BUTTON_A),50);
    if(!g.won || g.deliveries!=1)return 1003;
    int code=courier_checks();
    u32 trace=courier_trace();
    *(volatile u16*)0xC00000=(u16)(trace>>16);
    *(volatile u16*)0xC00000=(u16)trace;
    return code;
}
