#include "view.h"
static u16 buttons(u16 raw)
{
    u16 keys = 0;
    if (raw & BUTTON_UP)
        keys |= UP;
    if (raw & BUTTON_DOWN)
        keys |= DOWN;
    if (raw & BUTTON_LEFT)
        keys |= LEFT;
    if (raw & BUTTON_RIGHT)
        keys |= RIGHT;
    if (raw & BUTTON_A)
        keys |= A;
    if (raw & BUTTON_B)
        keys |= B;
    if (raw & BUTTON_C)
        keys |= C;
    if (raw & BUTTON_START)
        keys |= START;
    return keys;
}
int main(bool hard_reset)
{
    (void)hard_reset;
    JOY_init();
    view_init();
    static Game game;
    Campaign saved;
    game_init(&game, save_load(&saved));
#ifdef ADVENTURE_PREVIEW
    game.screen = ADVENTURE_PREVIEW;
    if (game.screen == BATTLE || game.screen == RESULT)
    {
        battle_begin(&game.battle, &game.campaign, 2, 42);
        game.battle_menu = COMMAND;
    }
#endif
    u16 previous = 0, hz = IS_PAL_SYSTEM ? 50 : 60;
    FrameClock clock = {0};
    while (TRUE)
    {
        SYS_doVBlankProcess();
        u16 held = buttons(JOY_readJoypad(JOY_1)), pressed = held & ~previous;
        u16 nav = 0, dt = clock_step(&clock, hz, held, pressed, &nav);
        previous = held;
        game_step(&game, held, pressed, nav, dt);
        if (game.load_request)
        {
            game.load_request = false;
            if (save_load(&saved))
                game_load(&game, &saved);
            else
            {
                game.notice = "Checkpoint unavailable. Start a voyage.";
                game.saved = false;
                game.dirty = true;
            }
        }
        if (game.save_request)
        {
            game.save_request = false;
            if (save_write(&game.campaign))
                game.saved = true;
            else
            {
                game.notice = "SRAM write failed. You can keep playing.";
                game.events |= SFX_DENIED;
                game.dirty = true;
            }
        }
        sound_frame(&game);
        view_frame(&game);
    }
    return 0;
}
