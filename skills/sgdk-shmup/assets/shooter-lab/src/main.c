#include <genesis.h>
#include "game.h"
#include "view.h"
_Static_assert(A==BUTTON_A && B==BUTTON_B && C==BUTTON_C && START==BUTTON_START,"Use the SGDK action masks");
_Static_assert(UP==BUTTON_UP && DOWN==BUTTON_DOWN && LEFT==BUTTON_LEFT && RIGHT==BUTTON_RIGHT,"Use SGDK directions");
static Game game;
int main(bool hard_reset)
{
    (void)hard_reset; JOY_init(); game_init(&game);
    if (!view_start())
    {
        VDP_setWindowHPos(FALSE,0); VDP_setWindowVPos(FALSE,0); VDP_setTextPlane(BG_A);
        VDP_drawText("SHOOTER LAB SETUP FAILED",2,2);
        VDP_drawText(view_error(),2,4); VDP_setEnable(TRUE);
        while (TRUE) SYS_doVBlankProcess();
    }
    sound_start();
    u16 hz=SYS_isPAL()?50:60;
    while (TRUE)
    {
#if SHOOTER_PROFILE
        u16 fps=SYS_getFPS();
#else
        u16 fps=0;
#endif
        u16 held=JOY_readJoypad(JOY_1);
#if SHOOTER_PROFILE
        u32 before=getSubTick();
        game_frame(&game,held,hz);
        u32 now=getSubTick(); profile_simulation=(now-before)*hz/768; before=now;
        sound_frame(&game);
        now=getSubTick(); profile_audio=(now-before)*hz/768; before=now;
        view_frame(&game,held,hz,fps);
        profile_draw=(getSubTick()-before)*hz/768;
#else
        game_frame(&game,held,hz); sound_frame(&game); view_frame(&game,held,hz,fps);
#endif
        SYS_doVBlankProcess();
    }
}
