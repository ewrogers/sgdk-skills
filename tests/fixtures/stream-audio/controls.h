#ifndef ROUTE_CONTROLS_H
#define ROUTE_CONTROLS_H
#include <genesis.h>
#include "route.h"
static inline u16 rt_controls(u16 raw)
{
    u16 held=0;
    if(raw&BUTTON_LEFT)held|=R_LEFT;
    if(raw&BUTTON_RIGHT)held|=R_RIGHT;
    if(raw&BUTTON_A)held|=R_WARP;
    if(raw&BUTTON_B)held|=R_SLOW;
    if(raw&BUTTON_C)held|=R_JINGLE;
    if(raw&BUTTON_START)held|=R_PAUSE;
    return held;
}
#endif
