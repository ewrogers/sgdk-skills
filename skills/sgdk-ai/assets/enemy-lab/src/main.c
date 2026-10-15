#include <genesis.h>
#include "game.h"
#include "view.h"
#include "resources.h"

static Game game;
static u16 keys(u16 raw)
{
    u16 result=0;
    if (raw&BUTTON_UP) result|=UP;
    if (raw&BUTTON_DOWN) result|=DOWN;
    if (raw&BUTTON_LEFT) result|=LEFT;
    if (raw&BUTTON_RIGHT) result|=RIGHT;
    if (raw&BUTTON_A) result|=A;
    if (raw&BUTTON_B) result|=B;
    if (raw&BUTTON_C) result|=C;
    if (raw&BUTTON_START) result|=START;
    return result;
}
int main(bool hard_reset)
{
    (void)hard_reset; JOY_init(); game_init(&game);
    if (!view_init()) {
        SPR_reset(); SPR_update(); SYS_doVBlankProcess(); VDP_setEnable(FALSE);
        VDP_drawText("ENEMY LAB / SPRITE ALLOCATION FAILED",2,12); VDP_setEnable(TRUE);
        while (TRUE) SYS_doVBlankProcess();
    }
    XGM2_load(arena_music); XGM2_setLoopNumber(-1); XGM2_playTrack(0);
    u16 hz=SYS_isPAL() ? 50 : 60,fraction=0,previous[2]={0};
    while (TRUE) {
        SYS_doVBlankProcess();
        u16 held[2]={keys(JOY_readJoypad(JOY_1)),keys(JOY_readJoypad(JOY_2))};
        Input input; game_input(&input,previous,held);
        bool paused=game.paused || game.phase==GAME_OVER || game.phase==COMPLETE;
        game_frame(&game,&input,game_clock(&fraction,hz));
        bool silent=game.paused || game.phase==GAME_OVER || game.phase==COMPLETE;
        if (paused!=silent) { if (silent) XGM2_pause(); else XGM2_resume(); }
        if (game.events&EVENT_HIT) XGM2_playPCM(hit_sound,sizeof hit_sound,SOUND_PCM_CH3);
        else if (game.events&EVENT_ALERT) XGM2_playPCM(alert_sound,sizeof alert_sound,SOUND_PCM_CH3);
        if (game.events&(EVENT_STUN|EVENT_JOIN)) XGM2_playPCM(stun_sound,sizeof stun_sound,SOUND_PCM_CH2);
        else if (game.events&EVENT_FIRE) XGM2_playPCM(fire_sound,sizeof fire_sound,SOUND_PCM_CH2);
        view_draw(&game);
    }
}
