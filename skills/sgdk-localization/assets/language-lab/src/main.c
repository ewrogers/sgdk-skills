#include <genesis.h>
#include "lab.h"
#include "catalog.h"
#include "prefs.h"
#include "view.h"

_Static_assert(KEY_A==BUTTON_A && KEY_C==BUTTON_C && KEY_START==BUTTON_START,
               "Lab controls must match SGDK");
static LanguageLab lab;

int main(bool hard_reset)
{
    (void)hard_reset;
    SYS_disableInts();VDP_setEnable(FALSE);
    bool valid=view_init();
    lab_init(&lab,prefs_load());
    if (!valid || !view_draw(&lab)) {
        VDP_drawText("LANGUAGE LAB INIT FAILED",6,12);
        VDP_setEnable(TRUE);SYS_enableInts();
        while (TRUE) SYS_doVBlankProcess();
    }
    SYS_enableInts();
    SYS_doVBlankProcess();VDP_setEnable(TRUE);
    while (TRUE) {
        u16 held=JOY_readJoypad(JOY_1);
        u16 pressed=held & (u16)~lab.held;
        bool language_changed=lab_input(&lab,held);
        if (language_changed) prefs_save(loc_languages[lab.language].id);
        if (pressed && !view_draw(&lab)) {
            /* A compiler/runtime layout mismatch is visible, never clipped away. */
            VDP_drawTextEx(BG_A,"LAYOUT ERROR",TILE_ATTR(PAL1,TRUE,FALSE,FALSE),2,26,DMA_QUEUE);
        }
        SYS_doVBlankProcess();
    }
}
