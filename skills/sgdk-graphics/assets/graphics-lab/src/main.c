#include <genesis.h>
#include "resources.h"
#include "demo.h"

int main(bool hard_reset)
{
    u16 displayed_completions = 65535;
    (void)hard_reset;
    VDP_setEnable(FALSE);
    VDP_setScreenWidth320();
    VDP_setPlaneSize(64, 32, TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    VDP_setTextPlane(WINDOW);
    VDP_setTextPalette(PAL3);
    PAL_setPalette(PAL3, palette_grey, CPU);
    VDP_setWindowHPos(FALSE, 0);
    VDP_setWindowVPos(FALSE, 4);
    VDP_drawText("A: STOP/PLAY  B: BURST  C: FADE", 1, 1);
    VDP_drawText("BURSTS DONE:", 1, 2);
    if (!demo_init(&far_background, &near_background, &hero))
    {
        VDP_drawText("Graphics allocation failed", 1, 2);
        VDP_setEnable(TRUE);
        while (TRUE) SYS_doVBlankProcess();
    }
    SYS_doVBlankProcess(); /* Flush initial sprite upload while hidden. */
    VDP_setEnable(TRUE);
    while (TRUE)
    {
        demo_step(JOY_readJoypad(JOY_1));
        if (displayed_completions != demo_completions())
        {
            char text[8];
            displayed_completions = demo_completions();
            uintToStr(displayed_completions, text, 5);
            VDP_drawText(text, 14, 2);
        }
        SYS_doVBlankProcess();
    }
}
