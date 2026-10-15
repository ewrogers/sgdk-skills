#include <genesis.h>
#include "game.h"
#include "view.h"
#include "sound.h"

_Static_assert(UNIT==(1<<FIX32_FRAC_BITS),"Model positions use SGDK fix32 scaling");
_Static_assert(UP==BUTTON_UP && DOWN==BUTTON_DOWN && LEFT==BUTTON_LEFT && RIGHT==BUTTON_RIGHT,
               "Cache the actual SGDK direction masks");
_Static_assert(A==BUTTON_A && B==BUTTON_B && C==BUTTON_C && START==BUTTON_START,
               "Cache the actual SGDK action masks");
static Game game;

int main(bool hard_reset)
{
    (void)hard_reset;
    JOY_init();
    game_init(&game);
    if (!view_start(&game))
    {
        VDP_drawText("PLATFORM LAB SETUP FAILED",2,1);
        VDP_setEnable(TRUE);
        while (TRUE) SYS_doVBlankProcess();
    }
    sound_start();
    u16 hz=SYS_isPAL()?50:60;
    while (TRUE)
    {
        u16 fps=SYS_getFPS();
        /* SGDK updates its joypad cache during frame processing. One snapshot
         * feeds one or two bounded 60 Hz simulation steps on NTSC/PAL. */
        game_frame(&game,JOY_readJoypad(JOY_1),hz);
        sound_frame(&game);
        view_draw(&game,hz,fps);
        SYS_doVBlankProcess();
    }
}
