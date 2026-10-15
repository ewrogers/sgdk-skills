#include "water_fx.h"

void water_fx_init(WaterFx *fx, s16 center_y, u16 waterline)
{
    *fx = (WaterFx){.underwater = center_y >= (s16)waterline};
}

void water_fx_step(WaterFx *fx, s16 center_y, u16 waterline)
{
    if (fx->splash_left) --fx->splash_left;
    bool submerged = center_y >= (s16)waterline;
    if (submerged != fx->underwater) fx->splash_left = WATER_SPLASH_TICKS;
    fx->underwater = submerged;
    for (u16 i = 0; i < WATER_BUBBLES; ++i) {
        WaterBubble *b = &fx->bubbles[i];
        if (!b->active) continue;
        --b->y; ++b->age;
        /* Retire beneath the opaque surface rather than spending its already
           full sprite-pixel budget on a bubble hidden by the foam. */
        if (b->y < (s16)waterline + 12) b->active = 0;
    }
    if (center_y < (s16)waterline + 20) { fx->emit_clock = 0; return; }
    if (++fx->emit_clock < 24) return;
    fx->emit_clock = 0;
    for (u16 i = 0; i < WATER_BUBBLES; ++i) {
        if (fx->bubbles[i].active) continue;
        fx->bubbles[i] = (WaterBubble){.y = center_y - 3, .active = 1};
        break; /* Pool exhaustion simply omits this cosmetic bubble. */
    }
}
