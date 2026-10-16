#include "view.h"
#include "resources.h"
static State previous;
static const u8 *playing;
static bool paused,boss_loaded,clear_started;
static u16 volume=101,last_burst=65535;
static bool ribbon_sounding;
static u32 ribbon_tick;
static void set_volume(u16 value)
{
    if (value==volume) return;
    XGM2_setFMVolume(value); XGM2_setPSGVolume(value); volume=value;
}
static void track(const u8 *data,bool loop)
{
    if (data==playing) return;
    XGM2_load(data); XGM2_setLoopNumber(loop?-1:0); XGM2_playTrack(0); playing=data;
}
void sound_start(void)
{
    previous=TITLE; paused=false; boss_loaded=false; clear_started=false;
    ribbon_sounding=false; ribbon_tick=0;
    playing=NULL; track(music_title,TRUE); set_volume(100);
}
void sound_frame(const Game *game)
{
    if (game->state!=previous)
    {
        if (game->state==TITLE || game->state==HOWTO) { track(music_title,TRUE); set_volume(100); }
        else if (game->state==LOADOUT) { track(music_loadout,TRUE); set_volume(100); }
        else if (game->state==STAGECARD)
        {
            track(game->stage==1?music_stage1:game->stage==2?music_stage2:music_stage3,TRUE);
            set_volume(100); boss_loaded=false;
        }
        else if (game->state==CLEAR) { XGM2_pause(); clear_started=false; last_burst=65535; }
        else if (game->state==CREDITS) { track(music_credits,TRUE); set_volume(100); }
        else if (game->state==OVER) { track(music_over,TRUE); set_volume(100); }
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
            if (game->boss.age<75) set_volume((75-game->boss.age)*100/75);
            else
            {
                if (!boss_loaded)
                {
                    track(game->stage==1?music_boss1:game->stage==2?music_boss2:music_boss3,TRUE);
                    boss_loaded=true; volume=101; set_volume(0);
                }
                set_volume(game->boss.age<150?(game->boss.age-75)*100/75:100);
            }
        }
        if (game->state==CLEAR)
        {
            if (game->age<90)
            {
                u16 burst=game->age/18;
                if (burst!=last_burst) { XGM2_playPCM(sfx_kill,sizeof sfx_kill,SOUND_PCM_CH2); last_burst=burst; }
            }
            else if (!clear_started)
            { playing=NULL; track(music_clear,FALSE); set_volume(100); clear_started=true; }
        }
        if (game->events&EV_HIT) XGM2_playPCM(sfx_hit,sizeof sfx_hit,SOUND_PCM_CH3);
        else if (game->events&EV_CLEAR) XGM2_playPCM(sfx_clear,sizeof sfx_clear,SOUND_PCM_CH3);
        else if (game->events&EV_BOSS) XGM2_playPCM(sfx_boss,sizeof sfx_boss,SOUND_PCM_CH3);
        else if (game->events&EV_UPGRADE) XGM2_playPCM(sfx_upgrade,sizeof sfx_upgrade,SOUND_PCM_CH3);
        else if (game->events&EV_GEM) XGM2_playPCM(sfx_gem,sizeof sfx_gem,SOUND_PCM_CH3);
        else if (game->events&EV_SELECT) XGM2_playPCM(sfx_select,sizeof sfx_select,SOUND_PCM_CH3);
        else if (game->events&EV_DENIED) XGM2_playPCM(sfx_denied,sizeof sfx_denied,SOUND_PCM_CH3);
        if (game->ribbon_active)
        {
            if (!ribbon_sounding || game->ticks-ribbon_tick>=8)
            { XGM2_playPCM(sfx_ribbon,sizeof sfx_ribbon,SOUND_PCM_CH2); ribbon_tick=game->ticks; }
            ribbon_sounding=true;
        }
        else
        {
            if (ribbon_sounding) XGM2_stopPCM(SOUND_PCM_CH2);
            ribbon_sounding=false;
            if (game->events&EV_FIRE) XGM2_playPCM(sfx_fire,sizeof sfx_fire,SOUND_PCM_CH2);
            else if (game->events&EV_KILL) XGM2_playPCM(sfx_kill,sizeof sfx_kill,SOUND_PCM_CH2);
        }
    }
    if (game->paused) ribbon_sounding=false;
    previous=game->state; paused=game->paused;
}
