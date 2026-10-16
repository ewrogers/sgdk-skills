#include "view.h"
#include "resources.h"
static const u8 *playing;
static bool paused;
static u16 volume=65535;
void sound_frame(const Game *g)
{
    const u8 *track;
    if(g->screen==FIELD||g->screen==DIALOGUE)
    {
        const u8 *zones[]={music_town,music_dungeon,music_forest,music_coast};track=zones[g->zone];
    }
    else if(g->screen==WORLD)track=music_travel;
    else if(g->screen==INTRO||g->screen==TITLE||g->screen==NAMING||g->screen==SAVES)track=music_title;
    else track=music_menu;
    if(track!=playing)
    {XGM2_load(track);XGM2_setLoopNumber(-1);XGM2_playTrack(0);playing=track;paused=false;volume=65535;}
    if(volume!=g->music_volume){XGM2_setFMVolume(g->music_volume);XGM2_setPSGVolume(g->music_volume);volume=g->music_volume;}
    if(paused==g->music_enabled)
    {if(g->music_enabled)XGM2_resume();else XGM2_pause();paused=!g->music_enabled;}
    if(g->events&CUE_TRAVEL)XGM2_playPCM(sfx_travel,sizeof sfx_travel,SOUND_PCM_CH2);
    else if(g->events&CUE_DENIED)XGM2_playPCM(sfx_denied,sizeof sfx_denied,SOUND_PCM_CH2);
    else if(g->events&CUE_ACCEPT)XGM2_playPCM(sfx_accept,sizeof sfx_accept,SOUND_PCM_CH2);
    else if(g->events&CUE_BACK)XGM2_playPCM(sfx_back,sizeof sfx_back,SOUND_PCM_CH2);
    else if(g->events&CUE_TALK)XGM2_playPCM(sfx_talk,sizeof sfx_talk,SOUND_PCM_CH2);
    else if(g->events&CUE_NAME)XGM2_playPCM(sfx_name,sizeof sfx_name,SOUND_PCM_CH2);
    else if(g->events&CUE_MOVE)XGM2_playPCM(sfx_move,sizeof sfx_move,SOUND_PCM_CH2);
}
