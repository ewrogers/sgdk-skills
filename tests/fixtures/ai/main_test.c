#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include "world.h"
#include "view.h"
#include "resources.h"

/* Execute the real adapter loop with recorded SDK calls. Only hardware I/O and
   rendering are replaced; phase changes and input handling run the actual model. */
#define main enemy_lab_main
#include "../../../skills/sgdk-ai/assets/enemy-lab/src/main.c"
#undef main

const u8 area_one_music[4]={0},area_two_music[4]={6},area_three_music[4]={7};
const u8 hit_sound[4]={1},stun_sound[4]={2},
    alert_sound[4]={3},fire_sound[4]={4},clear_sound[4]={5};
static jmp_buf finished;
static u32 frames,waited,reads,clears,pauses,resumes;
static u16 songs;
static bool pal;
static Phase ending;
static u16 step;

void JOY_init(void) {}
u16 JOY_readJoypad(u16 joy)
{
    assert(joy<2); ++reads;
    if (joy==JOY_1 && step==0 && game.phase>=INTRO_BOMBARDIER) return BUTTON_A;
    return joy==JOY_1 && step==3 && waited>=36000 ? BUTTON_START : 0;
}
u16 SYS_isPAL(void) { return pal; }
bool SYS_doVBlankProcess(void) { ++frames; assert(frames<37000); return true; }
bool view_init(void) { return true; }
void XGM2_load(const u8 *song)
{
    static const u8 *const tracks[]={area_one_music,area_two_music,area_three_music};
    assert(song==tracks[game.level]); ++songs;
}
void XGM2_setLoopNumber(s8 count) { assert(count==-1); }
void XGM2_playTrack(u16 track) { assert(!track); }
void XGM2_pause(void) { ++pauses; }
void XGM2_resume(void) { ++resumes; }
bool XGM2_playPCM(const u8 *sample,u32 length,SoundPCMChannel channel)
{
    assert(length==4 && (channel==SOUND_PCM_CH2 || channel==SOUND_PCM_CH3));
    if (sample==clear_sound) { assert(channel==SOUND_PCM_CH3); ++clears; }
    return true;
}
void SPR_reset(void) { assert(false); }
void SPR_update(void) { assert(false); }
void VDP_setEnable(bool value) { (void)value; assert(false); }
void VDP_drawText(const char *value,u16 x,u16 y) { (void)value; (void)x; (void)y; assert(false); }
void view_draw(const Game *drawn)
{
    assert(drawn==&game && reads==2*frames);
    if (step==0 && game.phase==PLAYING) {
        for (u16 i=0;i<ENEMY_COUNT;++i) enemy_change(&game.enemies[i],STUNNED,60000);
        game.players[0].pos=world_exit(game.level); step=1;
    } else if (step==1 && game.phase==AREA_CLEAR) {
        assert(clears==1); step=2;
    } else if (step==2) {
        if (game.phase==PLAYING) {
            if (ending==GAME_OVER) {
                assert(game.level==1 && clears==1);
                game.phase=CAUGHT; game.tries=0; game.phase_ticks=1; step=3;
            } else {
                for (u16 i=0;i<ENEMY_COUNT;++i) enemy_change(&game.enemies[i],STUNNED,60000);
                game.players[0].pos=world_exit(game.level);
            }
        } else if (game.phase==COMPLETE) { assert(clears==3 && songs==3); step=3; }
    } else if (step==3) {
        if (waited<36000) {
            assert(game.phase==ending && pauses==1 && !resumes && clears==(ending==COMPLETE ? 3 : 1));
            ++waited;
        } else {
            assert(game.phase==INTRO_BOMBARDIER && game.level==0 && game.tries==3);
            assert(pauses==1 && !resumes && clears==(ending==COMPLETE ? 3 : 1));
            assert(songs==(ending==COMPLETE ? 4 : 3)); longjmp(finished,1);
        }
    }
}
int main(void)
{
    for (u16 region=0;region<2;++region) for (u16 outcome=0;outcome<2;++outcome) {
        pal=region; ending=outcome ? COMPLETE : GAME_OVER;
        frames=waited=reads=clears=pauses=resumes=0; step=songs=0;
        if (!setjmp(finished)) enemy_lab_main(true);
    }
    puts("adapter"); return 0;
}
