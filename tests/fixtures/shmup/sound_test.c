#include <assert.h>
#include "genesis.h"
#include "resources.h"
#include "view.h"
const u8 title_music[8]={10},stage_music[8]={1},boss_music[8]={9},clear_music[8]={2};
const u8 fire_sound[8]={3},pickup_sound[8]={4},bomb_sound[8]={5},warning_sound[8]={6},hit_sound[8]={7},kill_sound[8]={8};
static const u8 *music,*channel2,*channel3;
static u16 plays,loads,pauses,resumes,stops;
static s8 loops;
static u16 fm,psg;
void XGM2_setFMVolume(u16 value) { assert(value<=100); fm=value; }
void XGM2_setPSGVolume(u16 value) { assert(value<=100); psg=value; }
void XGM2_load(const u8 *data) { music=data; loads++; }
void XGM2_setLoopNumber(s8 number) { loops=number; }
void XGM2_playTrack(u16 track) { assert(track==0); plays++; }
void XGM2_pause(void) { pauses++; }
void XGM2_resume(void) { resumes++; }
bool XGM2_stopPCM(u16 ch) { assert(ch==2 || ch==3); stops++; return true; }
bool XGM2_playPCM(const u8 *data,u32 length,u16 channel)
{ assert(length==8); if (channel==2) channel2=data; else { assert(channel==3); channel3=data; } return true; }
int main(void)
{
    Game game={.state=TITLE}; sound_start(); assert(music==title_music && loops==-1 && loads==1);
    game.state=PLAY; game.events=EVENT_START; sound_frame(&game); assert(plays==2 && music==stage_music);
    game.events=EVENT_FIRE|EVENT_KILL; sound_frame(&game); assert(channel2==fire_sound && channel3==kill_sound);
    game.events=EVENT_PICKUP|EVENT_HURT|EVENT_BOMB; sound_frame(&game); assert(channel3==hit_sound);
    game.state=BOSS; game.events=EVENT_BOSS|EVENT_HURT; sound_frame(&game); assert(channel3==warning_sound);
    game.events=0; game.paused=true; sound_frame(&game); assert(pauses==1 && stops==2);
    sound_frame(&game); assert(pauses==1 && stops==2);
    game.paused=false; sound_frame(&game); assert(resumes==1);
    game.boss.age=45; sound_frame(&game); assert(fm==50 && psg==50);
    game.paused=true; game.boss.age=90; sound_frame(&game); assert(fm==50 && music==stage_music);
    game.paused=false; sound_frame(&game); assert(music==boss_music && fm==0 && plays==3);
    sound_frame(&game); assert(plays==3);
    game.boss.age=135; sound_frame(&game); assert(fm==50 && psg==50);
    game.boss.age=180; sound_frame(&game); assert(fm==100 && psg==100);
    game.state=CLEAR; sound_frame(&game); assert(music==clear_music && loops==0 && plays==4);
    for (u16 i=0;i<2000;i++) sound_frame(&game); assert(plays==4);
    game.state=PLAY; game.events=EVENT_START; sound_frame(&game); assert(music==stage_music && loops==-1 && plays==5);
    game.state=OVER; game.events=EVENT_HURT; sound_frame(&game); assert(pauses==3 && channel3==hit_sound);
    return 0;
}
