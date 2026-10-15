#include <genesis.h>
#include "resources.h"
#include "sound.h"
static bool was_paused,was_complete,was_ready;
void sound_start(void)
{
    XGM2_load(course_music); XGM2_setLoopNumber(-1); XGM2_playTrack(0);
    was_paused=was_complete=was_ready=false;
    /* Keep this lab's two PCM effect channels separate from FM/PSG music. */
}
void sound_frame(const Game *game)
{
    if (game->complete!=was_complete)
    {
        XGM2_load(game->complete?victory_music:course_music);
        XGM2_setLoopNumber(game->complete?0:-1); XGM2_playTrack(0);
    }
    if (game->paused!=was_paused)
    {
        if (game->paused)
        { XGM2_pause(); XGM2_stopPCM(SOUND_PCM_CH2); XGM2_stopPCM(SOUND_PCM_CH3); }
        else XGM2_resume();
    }
    bool ready=game->player.charge==CHARGE_TICKS && game->player.air_dash;
    if (!game->paused)
    {
        if (game->events&EVENT_HURT) XGM2_playPCM(hit_sound,sizeof hit_sound,SOUND_PCM_CH3);
        else if (game->events&EVENT_DASH) XGM2_playPCM(dash_sound,sizeof dash_sound,SOUND_PCM_CH3);
        else if (ready && !was_ready) XGM2_playPCM(ready_sound,sizeof ready_sound,SOUND_PCM_CH3);
        else if (game->events&EVENT_STOMP) XGM2_playPCM(stomp_sound,sizeof stomp_sound,SOUND_PCM_CH3);
        else if (game->events&EVENT_JUMP) XGM2_playPCM(jump_sound,sizeof jump_sound,SOUND_PCM_CH3);
        if (game->events&(EVENT_PICKUP|EVENT_SECRET|EVENT_CHECKPOINT))
            XGM2_playPCM(coin_sound,sizeof coin_sound,SOUND_PCM_CH2);
    }
    was_paused=game->paused; was_complete=game->complete; was_ready=ready;
}
