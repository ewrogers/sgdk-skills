#include <assert.h>
#include "genesis.h"
#include "sound.h"
#include "resources.h"
const u8 course_music[8]={1},victory_music[8]={2};
const u8 jump_sound[8]={3},coin_sound[8]={4},dash_sound[8]={5},ready_sound[8]={6},hit_sound[8]={7},stomp_sound[8]={8};
static const u8 *music,*channel2,*channel3;
static u16 plays,pauses,resumes,stops,loads;
static s8 loop;
void XGM2_load(const u8 *data) { music=data; loads++; }
void XGM2_setLoopNumber(s8 count) { loop=count; }
void XGM2_playTrack(u16 track) { assert(track==0); plays++; }
void XGM2_pause(void) { pauses++; }
void XGM2_resume(void) { resumes++; }
bool XGM2_stopPCM(u16 channel) { assert(channel==2 || channel==3); stops++; return true; }
bool XGM2_playPCM(const u8 *data,u32 length,u16 channel)
{
    assert(length==8);
    if (channel==2) channel2=data;
    else { assert(channel==3); channel3=data; }
    return true;
}
int main(void)
{
    Game game={0}; sound_start(); assert(music==course_music && plays==1 && loop==-1);
    game.events=EVENT_JUMP; sound_frame(&game); assert(channel3==jump_sound);
    game.events=EVENT_PICKUP|EVENT_HURT|EVENT_DASH;
    sound_frame(&game); assert(channel2==coin_sound && channel3==hit_sound);
    game.events=0; game.player.air_dash=true; game.player.charge=CHARGE_TICKS; sound_frame(&game);
    assert(channel3==ready_sound);
    channel3=NULL; sound_frame(&game); assert(channel3==NULL); /* Ready plays once. */
    game.player.charge=0; game.events=EVENT_DASH; sound_frame(&game); assert(channel3==dash_sound);
    game.events=0; game.paused=true; sound_frame(&game);
    assert(pauses==1 && stops==2); sound_frame(&game); assert(pauses==1 && stops==2);
    game.paused=false; sound_frame(&game); assert(resumes==1);
    game.complete=true; sound_frame(&game); assert(music==victory_music && loop==0 && plays==2);
    for (u16 i=0;i<2000;i++) sound_frame(&game);
    assert(loads==2 && plays==2);
    game.complete=false; sound_frame(&game); assert(music==course_music && loop==-1 && plays==3);
    return 0;
}
