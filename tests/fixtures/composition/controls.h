#ifndef COURIER_CONTROLS_H
#define COURIER_CONTROLS_H
#include <genesis.h>
#include "model.h"
/* The game model's action bits are not SGDK's physical button masks. */
static inline u16 courier_controls(u16 raw)
{
    u16 held=0;
    if(raw&BUTTON_UP)held|=C_UP;
    if(raw&BUTTON_DOWN)held|=C_DOWN;
    if(raw&BUTTON_LEFT)held|=C_LEFT;
    if(raw&BUTTON_RIGHT)held|=C_RIGHT;
    if(raw&BUTTON_A)held|=C_A;
    if(raw&BUTTON_START)held|=C_START;
    return held;
}
#endif
