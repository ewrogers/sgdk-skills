#include <genesis.h>
#include "resources.h"
#include "view.h"
static State previous;
static bool paused,boss_music_loaded;
static u16 volume;
static void set_volume(u16 value)
{
    if (value==volume) return;
    XGM2_setFMVolume(value); XGM2_setPSGVolume(value); volume=value;
}
void sound_start(void)
{
    XGM2_load(title_music); XGM2_setLoopNumber(-1); XGM2_playTrack(0);
    previous=TITLE; paused=false; boss_music_loaded=false; volume=101; set_volume(100);
}
void sound_frame(const Game *game)
{
    if (game->state!=previous)
    {
        if (game->state==CLEAR) { set_volume(100); XGM2_load(clear_music); XGM2_setLoopNumber(0); XGM2_playTrack(0); }
        else if ((game->events&EVENT_START)) { boss_music_loaded=false; set_volume(100); XGM2_load(stage_music); XGM2_setLoopNumber(-1); XGM2_playTrack(0); }
        else if (game->state==OVER) XGM2_pause();
    }
    if (game->paused!=paused)
    {
        if (game->paused) { XGM2_pause(); XGM2_stopPCM(SOUND_PCM_CH2); XGM2_stopPCM(SOUND_PCM_CH3); }
        else XGM2_resume();
    }
    if (!game->paused)
    {
        if (game->state==BOSS)
        {
            u16 age=game->boss.age;
            if (age<90) set_volume((90-age)*100/90);
            else
            {
                if (!boss_music_loaded)
                {
                    set_volume(0); XGM2_load(boss_music); XGM2_setLoopNumber(-1);
                    XGM2_playTrack(0); boss_music_loaded=true;
                    /* Loading a track can reset driver state. */
                    volume=101; set_volume(0);
                }
                set_volume(age<180?(age-90)*100/90:100);
            }
        }
        if (game->events&EVENT_BOSS) XGM2_playPCM(warning_sound,sizeof warning_sound,SOUND_PCM_CH3);
        else if (game->events&EVENT_HURT) XGM2_playPCM(hit_sound,sizeof hit_sound,SOUND_PCM_CH3);
        else if (game->events&EVENT_BOMB) XGM2_playPCM(bomb_sound,sizeof bomb_sound,SOUND_PCM_CH3);
        else if (game->events&EVENT_PICKUP) XGM2_playPCM(pickup_sound,sizeof pickup_sound,SOUND_PCM_CH3);
        else if (game->events&EVENT_KILL) XGM2_playPCM(kill_sound,sizeof kill_sound,SOUND_PCM_CH3);
        if (game->events&EVENT_FIRE) XGM2_playPCM(fire_sound,sizeof fire_sound,SOUND_PCM_CH2);
    }
    previous=game->state; paused=game->paused;
}
