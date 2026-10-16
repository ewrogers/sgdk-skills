#include "sound.h"
#include <genesis.h>
#include "resources.h"
static bool paused;
static u16 last_tick;

void sound_start(void)
{
    paused=false; last_tick=0;
    XGM2_load(harbor_music); XGM2_setLoopNumber(-1); XGM2_playTrack(0);
    XGM2_setFMVolume(75);
}

void sound_frame(const DialogueLab *lab)
{
    if (paused != lab->paused)
    {
        if (lab->paused) { XGM2_pause(); XGM2_stopPCM(SOUND_PCM_CH2); }
        else XGM2_resume();
        paused=lab->paused;
    }
    if (paused || !lab->active) return;
    if (lab->events & DL_DONE_EVENT) XGM2_playPCM(cue_done,sizeof cue_done,SOUND_PCM_CH2);
    else if (lab->events & DL_CHOICE_EVENT) XGM2_playPCM(cue_choice,sizeof cue_choice,SOUND_PCM_CH2);
    else if (lab->events & (DL_PAGE_EVENT|DL_ENTER_EVENT)) XGM2_playPCM(cue_turn,sizeof cue_turn,SOUND_PCM_CH2);
    else if ((lab->events & DL_REVEAL_EVENT) && (u16)(lab->clock_ms-last_tick) >= 60)
    {
        XGM2_playPCM(cue_tick,sizeof cue_tick,SOUND_PCM_CH2); last_tick=(u16)lab->clock_ms;
    }
}
