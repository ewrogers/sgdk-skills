#include "resources.h"
#include <assert.h>
#include <stdio.h>
const u8 music_title[1]={1};
const u8 music_loadout[1]={2};
const u8 music_stage1[1]={3};
const u8 music_stage2[1]={4};
const u8 music_stage3[1]={5};
const u8 music_boss1[1]={6};
const u8 music_boss2[1]={7};
const u8 music_boss3[1]={8};
const u8 music_clear[1]={9};
const u8 music_credits[1]={10};
const u8 music_over[1]={11};
const u8 sfx_fire[1]={12};
const u8 sfx_gem[1]={13};
const u8 sfx_upgrade[1]={14};
const u8 sfx_hit[1]={15};
const u8 sfx_kill[1]={16};
const u8 sfx_boss[1]={17};
const u8 sfx_clear[1]={18};
const u8 sfx_select[1]={19};
const u8 sfx_denied[1]={20};
const u8 sfx_ribbon[1]={21};

static u16 plays[4],stops[4],pauses,resumes;static const u8 *last[4];
void XGM2_setFMVolume(u16 value){assert(value<=100);}
void XGM2_setPSGVolume(u16 value){assert(value<=100);}
void XGM2_load(const u8 *data){assert(data);}
void XGM2_setLoopNumber(s16 count){assert(count==-1 || count==0);}
void XGM2_playTrack(u16 track){assert(track==0);}
void XGM2_pause(void){pauses++;}
void XGM2_resume(void){resumes++;}
void XGM2_stopPCM(u16 channel){assert(channel==2 || channel==3);stops[channel]++;}
void XGM2_playPCM(const u8 *data,u32 size,u16 channel){assert(size==1 && channel>=2 && channel<=3);plays[channel]++;last[channel]=data;}
int main(void)
{
    Game game={.state=PLAY,.ribbon_active=true,.ticks=1};sound_start();sound_frame(&game);
    assert(plays[2]==1 && last[2]==sfx_ribbon);
    game.ticks=2;sound_frame(&game);assert(plays[2]==1);
    game.ticks=9;sound_frame(&game);assert(plays[2]==2);
    game.paused=true;sound_frame(&game);assert(pauses==1 && stops[2]==1 && stops[3]==1);
    sound_frame(&game);assert(plays[2]==2);
    game.paused=false;game.ticks=10;sound_frame(&game);assert(resumes==1 && plays[2]==3);
    game.events=EV_HIT;sound_frame(&game);assert(last[3]==sfx_hit && plays[2]==3);
    game.events=0;game.ribbon_active=false;sound_frame(&game);assert(stops[2]==2);
    game.events=EV_FIRE;sound_frame(&game);assert(last[2]==sfx_fire);
    puts("audio");return 0;
}
