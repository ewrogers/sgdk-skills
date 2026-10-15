#include <genesis.h>
#include "menu.h"
#include "view.h"

static u16 buttons(u16 pad)
{
    u16 result = 0;
    if (pad & BUTTON_UP) result |= MENU_UP;
    if (pad & BUTTON_DOWN) result |= MENU_DOWN;
    if (pad & BUTTON_LEFT) result |= MENU_LEFT;
    if (pad & BUTTON_RIGHT) result |= MENU_RIGHT;
    if (pad & BUTTON_A) result |= MENU_ACCEPT;
    if (pad & BUTTON_B) result |= MENU_BACK;
    if (pad & BUTTON_START) result |= MENU_START;
    return result;
}

int main(bool hard_reset)
{
    Menu menu;
    u16 fraction = 0;
    u16 hz = SYS_isPAL() ? 50 : 60;
    (void)hard_reset;
    JOY_init();
    menu_init(&menu);
    view_init();
    view_draw(&menu);
    while (true) {
        u16 elapsed;
        SYS_doVBlankProcess();
        /* JOY_readJoypad returns SGDK's cached state; sample it once here. */
        fraction += 1000;
        elapsed = fraction / hz;
        fraction %= hz;
        menu_step(&menu, buttons(JOY_readJoypad(JOY_1)), elapsed);
        view_draw(&menu);
    }
    return 0;
}
