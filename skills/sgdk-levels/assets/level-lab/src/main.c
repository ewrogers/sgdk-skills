#include <genesis.h>
#include "level_data.h"
#include "level_view.h"

_Static_assert(WORLD_UNIT == (1 << FIX32_FRAC_BITS), "Use SGDK fix32 scaling");

static World world;
static const s16 beacons[4][2] = {{64, 64}, {944, 64}, {944, 944}, {64, 944}};

int main(bool hard_reset)
{
    u16 previous = 0, beacon = 0;
    u16 hz = SYS_isPAL() ? 50 : 60;
    bool paused = FALSE;
    (void)hard_reset;
    JOY_init();
    if (!world_init(&world, &level_collision, beacons[0][0], beacons[0][1])) {
        VDP_setEnable(FALSE);
        VDP_setScreenWidth320();
        PAL_setPalette(PAL0, palette_grey, CPU);
        VDP_drawText("SPAWN BLOCKED OR MAP INVALID", 2, 12);
        VDP_drawText("CHECK COLLISION AND BEACONS", 2, 14);
        VDP_setEnable(TRUE);
        while (TRUE) SYS_doVBlankProcess();
    }
    if (!level_view_start())
        while (TRUE) SYS_doVBlankProcess();
    while (TRUE) {
        u16 held = JOY_readJoypad(JOY_1);
        u16 pressed = held & (u16)~previous;
        u16 move = 0;
        bool warp = FALSE;
        previous = held;
        if (pressed & BUTTON_START) paused = !paused;
        if (pressed & BUTTON_A) {
            beacon = 0;
            warp = world_warp(&world, beacons[0][0], beacons[0][1]);
        } else if (pressed & BUTTON_B) {
            beacon = (beacon + 1) % 4;
            warp = world_warp(&world, beacons[beacon][0], beacons[beacon][1]);
        }
        if (held & BUTTON_LEFT) move |= WORLD_LEFT;
        if (held & BUTTON_RIGHT) move |= WORLD_RIGHT;
        if (held & BUTTON_UP) move |= WORLD_UP;
        if (held & BUTTON_DOWN) move |= WORLD_DOWN;
        for (u16 count = world_steps(&world, hz, paused); count; count--) world_tick(&world, move);
        level_view_draw(&world, paused, warp);
        SYS_doVBlankProcess();
    }
    return 0;
}
