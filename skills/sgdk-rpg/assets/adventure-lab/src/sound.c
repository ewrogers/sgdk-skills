#include "view.h"
#include "resources.h"
static const u8 *playing;
static bool paused;
void sound_frame(const Game *g)
{
    const u8 *track =
        g->screen == BATTLE    ? (g->battle.encounter == 2 ? music_guardian : music_battle)
        : g->screen == RESULT  ? (g->battle.outcome == WON ? music_victory : music_defeat)
        : g->screen == CREDITS ? music_ending
        : g->screen == TITLE || g->screen == HELP ? music_title
                                                  : music_harbor;
    if (track != playing)
    {
        XGM2_load(track);
        XGM2_setLoopNumber(-1);
        XGM2_playTrack(0);
        XGM2_setFMVolume(75);
        XGM2_setPSGVolume(75);
        playing = track;
        paused = false;
    }
    if (g->paused != paused)
    {
        if (g->paused)
            XGM2_pause();
        else
            XGM2_resume();
        paused = g->paused;
    }
#define CUE(flag, name)                                                                            \
    if (g->events & (flag))                                                                        \
    {                                                                                              \
        XGM2_playPCM(sfx_##name, sizeof sfx_##name, SOUND_PCM_CH2);                                \
        return;                                                                                    \
    }
    CUE(SFX_WIN, win);
    CUE(SFX_LOSE, lose);
    CUE(SFX_SAVE, save);
    CUE(SFX_DENIED, denied);
    CUE(SFX_HIT, hit);
    CUE(SFX_SPELL, spell);
    CUE(SFX_HEAL, heal);
    CUE(SFX_ACCEPT, accept);
    CUE(SFX_BACK, back);
    CUE(SFX_MOVE, move);
#undef CUE
}
