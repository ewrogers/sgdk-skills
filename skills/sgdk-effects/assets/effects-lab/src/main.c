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
#ifdef FX_STRESS_TEST
    lab.weather=2;lab.density=3;
    u32 last_vblank=0;
    u16 missed=0,previous_missed=0xFFFF;
#endif
    if (!fx_view_init()) {
        VDP_drawText("EFFECTS LAB INITIALIZATION FAILED",1,12);
        VDP_setEnable(TRUE); SYS_enableInts();
        while (TRUE) SYS_doVBlankProcess();
    }
    u16 hz=SYS_isPAL() ? 50 : 60,frame=0;
    SYS_enableInts(); SYS_doVBlankProcess(); VDP_setEnable(TRUE);
    while (TRUE) {
        u16 fps=SYS_getFPS(); /* Once per rendered frame, separate from sprite counts. */
#ifdef FX_STRESS_TEST
        u32 now=vtimer;
        if (last_vblank && now-last_vblank>1 && missed<99) missed++;
        last_vblank=now;
        u16 held=(lab.ticks%240<120 ? FX_RIGHT : FX_LEFT) | (lab.ticks%12==0 ? FX_A : 0);
#else
        u16 held=JOY_readJoypad(JOY_1);
#endif
        fx_frame(&lab,held,hz);
        fx_select(&lab,&render,FX_DRAW_MAX);
        fx_view_draw(&lab,&render);
        if (!(frame++%6)) fx_view_hud(&lab,&render,hz,fps);
#ifdef FX_STRESS_TEST
        if (missed!=previous_missed) {
            char text[]="STRESS LATE 00";
            text[13]=(char)('0'+missed%10);text[12]=(char)('0'+missed/10);
            VDP_drawTextEx(BG_A,text,TILE_ATTR(PAL0,TRUE,FALSE,FALSE),2,4,DMA_QUEUE);
            previous_missed=missed;
        }
#endif
        SYS_doVBlankProcess();
    }
}
