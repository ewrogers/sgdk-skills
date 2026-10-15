#include <assert.h>
#include "genesis.h"
#include "resources.h"
#include "view.h"
const u8 title_music[8]={10},stage2_music[8]={17},over_music[8]={18},stage_music[8]={1},boss_music[8]={9},clear_music[8]={2};
const u8 empty_sound[8]={16};
const u8 fire_sound[8]={3},pickup_sound[8]={4},bomb_sound[8]={5},warning_sound[8]={6},hit_sound[8]={7},kill_sound[8]={8};
const u8 voice_twin[8]={19},voice_bomb[8]={11},voice_power[8]={12},voice_shield[8]={13},voice_boss[8]={14},voice_over[8]={15};
static const u8 *music,*channel2,*channel3;
static u16 plays,loads,pauses,resumes,stops,fm,psg;
static s8 loops;
static bool busy;
void XGM2_setFMVolume(u16 value) { assert(value<=100); fm=value; }
void XGM2_setPSGVolume(u16 value) { assert(value<=100); psg=value; }
void XGM2_load(const u8 *data) { music=data; loads++; }
void XGM2_setLoopNumber(s8 number) { loops=number; }
void XGM2_playTrack(u16 track) { assert(track==0); plays++; }
void XGM2_pause(void) { pauses++; }
void XGM2_resume(void) { resumes++; }
void XGM2_stopPCM(u16 ch) { assert(ch==2 || ch==3); if (ch==3) busy=false; stops++; }
u8 XGM2_isPlayingPCM(u16 mask) { assert(mask==SOUND_PCM_CH3_MSK); return busy?mask:0; }
bool XGM2_playPCM(const u8 *data,u32 length,u16 channel)
{ assert(length==8); if (channel==2) channel2=data; else { assert(channel==3); channel3=data; busy=true; } return true; }
int main(void)
{
    Game game={.state=TITLE}; sound_start(); assert(music==title_music && loops==-1 && loads==1);
    game.state=PLAY; game.events=EVENT_START; sound_frame(&game); assert(plays==2 && music==stage_music);
    game.events=EVENT_FIRE|EVENT_KILL; sound_frame(&game); assert(channel2==fire_sound && channel3==kill_sound);
    game.events=EVENT_PICKUP; sound_frame(&game); assert(channel3==voice_power);
    game.events=EVENT_KILL; sound_frame(&game); assert(channel3==voice_power); /* Don't cut off speech. */
    busy=false; game.pickup_kind=2; game.events=EVENT_PICKUP; sound_frame(&game); assert(channel3==voice_shield);
    busy=false; game.pickup_kind=1; sound_frame(&game); assert(channel3==voice_bomb);
    game.state=BOSS; game.events=EVENT_BOSS|EVENT_HURT; sound_frame(&game); assert(channel3==voice_boss);
    game.events=0; game.paused=true; sound_frame(&game); assert(pauses==1 && stops==2);
    sound_frame(&game); assert(pauses==1 && stops==2);
    game.paused=false; sound_frame(&game); assert(resumes==1);
    game.boss.age=45; sound_frame(&game); assert(fm==50 && psg==50);
    game.paused=true; game.boss.age=90; sound_frame(&game); assert(fm==50 && music==stage_music);
    game.paused=false; sound_frame(&game); assert(music==boss_music && fm==0 && plays==3);
    sound_frame(&game); assert(plays==3);
    game.boss.age=135; sound_frame(&game); assert(fm==50 && psg==50);
    game.boss.age=180; sound_frame(&game); assert(fm==100 && psg==100);
    game.state=CLEAR; sound_frame(&game); assert(plays==3 && channel2==bomb_sound);
    game.phase_time=BOSS_DEATH_TICKS-1; sound_frame(&game); assert(plays==3);
    game.phase_time=BOSS_DEATH_TICKS; sound_frame(&game); assert(music==clear_music && loops==0 && plays==4);
    for (u16 i=0;i<2000;i++) sound_frame(&game); assert(plays==4);
    game.state=PLAY; game.events=EVENT_START; sound_frame(&game); assert(music==stage_music && plays==5);
    game.events=EVENT_BOMB_EMPTY; sound_frame(&game); assert(channel2==empty_sound);
    game.state=OVER; game.phase_time=0; game.events=EVENT_HURT; sound_frame(&game);
    assert(music==over_music && loops==-1 && channel2==hit_sound);
    game.phase_time=45; game.events=0; sound_frame(&game); assert(channel3==voice_over);
    game.events=0; sound_frame(&game); assert(channel3==voice_over);
    busy=false; game.state=PLAY; game.stage=2; game.events=EVENT_START; sound_frame(&game);
    assert(music==stage2_music && loops==-1);
    game.events=EVENT_PICKUP; game.pickup_kind=3; sound_frame(&game); assert(channel3==voice_twin);
    return 0;
}
