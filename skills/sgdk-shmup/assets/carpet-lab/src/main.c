#include "view.h"
static Game game;
int main(bool hard_reset)
{
    (void)hard_reset; JOY_init(); game_init(&game);
    if (!view_start())
    {
        view_error_screen();
        while (TRUE) SYS_doVBlankProcess();
    }
    sound_start(); u16 hz=SYS_isPAL()?50:60;
    while (TRUE)
    {
        game_frame(&game,JOY_readJoypad(JOY_1),hz);
        sound_frame(&game);
        if (!view_frame(&game))
        {
            XGM2_pause(); XGM2_stopPCM(SOUND_PCM_CH2); XGM2_stopPCM(SOUND_PCM_CH3);
            view_error_screen();
            while (TRUE) SYS_doVBlankProcess();
        }
        SYS_doVBlankProcess();
    }
}
