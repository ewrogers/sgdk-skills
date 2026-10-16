#include "game.h"
uint16_t clock_step(FrameClock *c, uint16_t hz, uint16_t held, uint16_t pressed, uint16_t *nav)
{
    if (hz != 50 && hz != 60)
        hz = 60;
    c->remainder += 60;
    uint16_t dt = c->remainder / hz;
    c->remainder %= hz;
    uint16_t dpad = held & (UP | DOWN | LEFT | RIGHT);
    *nav = pressed & (UP | DOWN | LEFT | RIGHT);
    if (dpad && dpad == c->last_dpad)
    {
        c->held_age += dt;
        if (c->held_age >= 20)
        {
            c->held_age -= 7;
            *nav = dpad;
        }
    }
    else
        c->held_age = 0;
    c->last_dpad = dpad;
    return dt;
}
