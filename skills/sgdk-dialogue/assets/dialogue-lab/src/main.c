#include <genesis.h>
#include "lab.h"
#include "view.h"
#include "sound.h"

static DialogueLab lab;
static u16 buttons(u16 value)
{
    return ((value & BUTTON_UP) ? LAB_UP : 0) | ((value & BUTTON_DOWN) ? LAB_DOWN : 0) |
        ((value & BUTTON_A) ? LAB_A : 0) | ((value & BUTTON_B) ? LAB_BACK : 0) |
        ((value & BUTTON_C) ? LAB_REPLAY : 0) | ((value & BUTTON_START) ? LAB_PAUSE : 0);
}

int main(bool hard_reset)
{
    (void)hard_reset;
    JOY_init(); lab_init(&lab);
    if (!view_init())
    {
        VDP_setEnable(TRUE); VDP_drawText("DIALOGUE LAB: RESOURCE ERROR",2,12);
        while (true) SYS_doVBlankProcess();
    }
    const u16 hz=SYS_isPAL() ? 50 : 60;
    sound_start();
    while (true)
    {
        SYS_doVBlankProcess();
        lab_frame(&lab,buttons(JOY_readJoypad(JOY_1)),hz);
        sound_frame(&lab);
        view_frame(&lab);
    }
    return 0;
}
