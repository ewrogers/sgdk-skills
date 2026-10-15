#include <genesis.h>
#include "resources.h"
#include "view.h"
static State previous;
static bool paused;
void sound_start(void)
{
    XGM2_load(stage_music); XGM2_setLoopNumber(-1); XGM2_playTrack(0);
    previous=TITLE; paused=false;
}
void sound_frame(const Game *game)
{
    if (game->state!=previous)
    {
        if (game->state==CLEAR) { XGM2_load(clear_music); XGM2_setLoopNumber(0); XGM2_playTrack(0); }
        else if ((game->events&EVENT_START)) { XGM2_load(stage_music); XGM2_setLoopNumber(-1); XGM2_playTrack(0); }
        else if (game->state==OVER) XGM2_pause();
    }
    if (game->paused!=paused)
    {
        if (game->paused) { XGM2_pause(); XGM2_stopPCM(SOUND_PCM_CH2); XGM2_stopPCM(SOUND_PCM_CH3); }
        else XGM2_resume();
    }
    if (!game->paused)
    {
        if (game->events&EVENT_BOSS) XGM2_playPCM(warning_sound,sizeof warning_sound,SOUND_PCM_CH3);
        else if (game->events&EVENT_HURT) XGM2_playPCM(hit_sound,sizeof hit_sound,SOUND_PCM_CH3);
        else if (game->events&EVENT_BOMB) XGM2_playPCM(bomb_sound,sizeof bomb_sound,SOUND_PCM_CH3);
        else if (game->events&EVENT_PICKUP) XGM2_playPCM(pickup_sound,sizeof pickup_sound,SOUND_PCM_CH3);
        else if (game->events&EVENT_KILL) XGM2_playPCM(kill_sound,sizeof kill_sound,SOUND_PCM_CH3);
        if (game->events&EVENT_FIRE) XGM2_playPCM(fire_sound,sizeof fire_sound,SOUND_PCM_CH2);
    }
    previous=game->state; paused=game->paused;
}
