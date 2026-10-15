#include <genesis.h>
#include "resources.h"
#include "view.h"
static State previous;
static bool paused,boss_music_loaded,announcing,clear_music_started,over_announced;
static u16 death_beat;
static u16 volume;
static void set_volume(u16 value)
{
    if (value==volume) return;
    XGM2_setFMVolume(value); XGM2_setPSGVolume(value); volume=value;
}
void sound_start(void)
{
    XGM2_load(title_music); XGM2_setLoopNumber(-1); XGM2_playTrack(0);
    previous=TITLE; paused=false; boss_music_loaded=false; announcing=false; clear_music_started=false; over_announced=false; death_beat=65535; volume=101; set_volume(100);
}
void sound_frame(const Game *game)
{
    if (game->state!=previous)
    {
        if (game->state==CLEAR) { XGM2_pause(); clear_music_started=false; death_beat=65535; }
        else if (game->state==CREDITS || game->state==TITLE) { set_volume(100); XGM2_load(game->state==CREDITS?credits_music:title_music); XGM2_setLoopNumber(-1); XGM2_playTrack(0); }
        else if ((game->events&EVENT_START)) { boss_music_loaded=false; set_volume(100); XGM2_load(game->stage==2?stage2_music:stage_music); XGM2_setLoopNumber(-1); XGM2_playTrack(0); }
        else if (game->state==OVER) { set_volume(100); XGM2_load(over_music); XGM2_setLoopNumber(-1); XGM2_playTrack(0); over_announced=false; }
    }
    if (game->paused!=paused)
    {
        if (game->paused) { XGM2_pause(); XGM2_stopPCM(SOUND_PCM_CH2); XGM2_stopPCM(SOUND_PCM_CH3); }
        else XGM2_resume();
    }
    if (!game->paused)
    {
        if (announcing && !XGM2_isPlayingPCM(SOUND_PCM_CH3_MSK)) announcing=false;
        if (game->state==CLEAR)
        {
            if (game->phase_time<BOSS_DEATH_TICKS)
            {
                u16 beat=game->phase_time/20;
                if (beat!=death_beat) { XGM2_playPCM(bomb_sound,sizeof bomb_sound,SOUND_PCM_CH2); death_beat=beat; }
            }
            else if (!clear_music_started)
            {
                set_volume(100); XGM2_load(clear_music); XGM2_setLoopNumber(0);
                XGM2_playTrack(0); clear_music_started=true;
            }
        }
        if (game->state==BOSS)
        {
            u16 age=game->boss.age;
            if (age<90) set_volume((90-age)*100/90);
            else
            {
                if (!boss_music_loaded)
                {
                    set_volume(0); XGM2_load(game->stage==2?reaver_music:boss_music); XGM2_setLoopNumber(-1);
                    XGM2_playTrack(0); boss_music_loaded=true;
                    /* Loading a track can reset driver state. */
                    volume=101; set_volume(0);
                }
                set_volume(age<180?(age-90)*100/90:100);
            }
        }
        const u8 *voice=NULL; u32 length=0;
        if (game->state==OVER && game->phase_time>=45 && !over_announced) { voice=voice_over; length=sizeof voice_over; over_announced=true; }
        else if (game->events&EVENT_BOSS) { voice=voice_boss; length=sizeof voice_boss; }
        else if ((game->events&EVENT_PICKUP) || !announcing)
        {
            if (game->events&EVENT_PICKUP)
            {
                if (game->pickup_kind==4) { voice=voice_spread; length=sizeof voice_spread; }
                else if (game->pickup_kind==3) { voice=voice_twin; length=sizeof voice_twin; }
                else if (game->pickup_kind==2) { voice=voice_shield; length=sizeof voice_shield; }
                else if (game->pickup_kind==1) { voice=voice_bomb; length=sizeof voice_bomb; }
                else { voice=voice_power; length=sizeof voice_power; }
            }
            else if (game->events&EVENT_BOMB) { voice=voice_bomb; length=sizeof voice_bomb; }
        }
        if (voice) { XGM2_stopPCM(SOUND_PCM_CH3); announcing=XGM2_playPCM(voice,length,SOUND_PCM_CH3); }
        else if (!announcing)
        {
            if (game->events&(EVENT_HURT|EVENT_SHIELD_BREAK)) XGM2_playPCM(hit_sound,sizeof hit_sound,SOUND_PCM_CH3);
            else if (game->events&EVENT_BLAST) XGM2_playPCM(bomb_sound,sizeof bomb_sound,SOUND_PCM_CH3);
            else if (game->events&EVENT_KILL) XGM2_playPCM(kill_sound,sizeof kill_sound,SOUND_PCM_CH3);
        }
        if (game->events&EVENT_HURT) XGM2_playPCM(hit_sound,sizeof hit_sound,SOUND_PCM_CH2);
        else if (game->events&EVENT_BOMB_EMPTY) XGM2_playPCM(empty_sound,sizeof empty_sound,SOUND_PCM_CH2);
        else if (game->events&EVENT_FIRE) XGM2_playPCM(fire_sound,sizeof fire_sound,SOUND_PCM_CH2);
    }
    previous=game->state; paused=game->paused;
}
