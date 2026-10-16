#include "view.h"
static u16 buttons(u16 raw)
{
    u16 value=0;
    if(raw&BUTTON_UP)value|=UP;
    if(raw&BUTTON_DOWN)value|=DOWN;
    if(raw&BUTTON_LEFT)value|=LEFT;
    if(raw&BUTTON_RIGHT)value|=RIGHT;
    if(raw&BUTTON_A)value|=CONFIRM;
    if(raw&BUTTON_B)value|=BACK;
    if(raw&BUTTON_C)value|=MAP;
    if(raw&BUTTON_START)value|=START;
    return value;
}
int main(bool hard_reset)
{
    (void)hard_reset;JOY_init();view_init();Game game;game_init(&game);
#ifdef ASTERFALL_PREVIEW_SCREEN
    /* Optional art-review entry; normal builds always begin with the intro. */
    game.screen=ASTERFALL_PREVIEW_SCREEN;
    game.name[0]='A';game.name[1]='r';game.name[2]='l';game.name[3]='e';game.name[4]='n';game.name[5]=0;game.name_length=5;
#endif
    u16 previous=0,repeat_age=0,previous_dpad=0;
    while(TRUE)
    {
        SYS_doVBlankProcess();
        u16 held=buttons(JOY_readJoypad(JOY_1)),pressed=held&~previous;
        u16 dpad=held&(UP|DOWN|LEFT|RIGHT),navigation=pressed&(UP|DOWN|LEFT|RIGHT);
        if(dpad&&dpad==previous_dpad)
        {if(repeat_age<60000)repeat_age++;if(repeat_age>=20&&repeat_age%7==0)navigation=dpad;}
        else repeat_age=0;
        previous_dpad=dpad;previous=held;
        game_step(&game,held,pressed,navigation);sound_frame(&game);view_frame(&game);
    }
    return 0;
}
