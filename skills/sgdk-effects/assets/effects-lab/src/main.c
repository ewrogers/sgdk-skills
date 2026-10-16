#include <genesis.h>
#include "effects.h"
#include "fx_view.h"

_Static_assert(FX_A==BUTTON_A && FX_B==BUTTON_B && FX_C==BUTTON_C && FX_START==BUTTON_START,
               "Cached SGDK input masks must match");
static FxLab lab;
static FxRender render;

int main(bool hard_reset)
{
    (void)hard_reset;
    SYS_disableInts(); VDP_setEnable(FALSE);
    fx_init(&lab,0xCAFE);
    if (!fx_view_init()) {
        VDP_drawText("EFFECTS LAB INITIALIZATION FAILED",1,12);
        VDP_setEnable(TRUE); SYS_enableInts();
        while (TRUE) SYS_doVBlankProcess();
    }
    u16 hz=SYS_isPAL() ? 50 : 60,frame=0;
    SYS_enableInts(); SYS_doVBlankProcess(); VDP_setEnable(TRUE);
    while (TRUE) {
        u16 fps=SYS_getFPS(); /* Once per rendered frame, separate from sprite counts. */
        fx_frame(&lab,JOY_readJoypad(JOY_1),hz);
        fx_select(&lab,&render,FX_DRAW_MAX);
        fx_view_draw(&lab,&render);
        if (!(frame++%6)) fx_view_hud(&lab,&render,hz,fps);
        SYS_doVBlankProcess();
    }
}
