#include <genesis.h>
#include "lab.h"
#include "scene_switch.h"
#include "scene_video.h"
#include "storage.h"
#include "view.h"
#include "resources.h"

static Lab lab;
static Checkpoint checkpoint;
static SceneSwitch flow;
static u16 hz;
static bool cleanup_ok=true;
static const char *notice="";

static bool save_run(void)
{
    Checkpoint next={lab.run,lab.music,lab.effects};
    if (!storage_save(&next)) { notice="SAVE FAILED / CHECKPOINT KEPT"; return false; }
    checkpoint=next; lab.has_save=true; notice="CHECKPOINT SAVED"; return true;
}
static void music_start(SceneId scene)
{
    XGM2_load(scene==SCENE_PLAY ? (lab.paused ? pause_music :
              lab.run.boss_state==2 ? victory_music :
              lab.run.room==3 ? boss_music : lab.run.room==2 ? room_three_music :
              lab.run.room==1 ? room_two_music : room_one_music) : menu_music);
    XGM2_setLoopNumber(scene==SCENE_PLAY && !lab.paused && lab.run.boss_state==2 ? 0 : -1);
    XGM2_playTrack(0);
    if (!lab.music) XGM2_pause();
}
static bool enter(SceneId scene)
{
    bool starting_level=scene==SCENE_PLAY && (lab.advance_room || flow.current==SCENE_SELECT);
    lab_enter(&lab,scene);
    if (!view_enter(scene,&lab,&checkpoint)) { notice="SCENE LOAD FAILED / BACK TO TITLE"; return false; }
    music_start(scene);
    if (starting_level && !save_run()) view_notice(notice);
    return true;
}
static void leave(void)
{
    view_leave();
    cleanup_ok=cleanup_ok && view_clean();
    if (!cleanup_ok) notice="RESOURCE CLEANUP CHECK FAILED";
}
static void fade_out(void) { scene_video_fade_out(hz/5); }
static void fade_in(void) { scene_video_fade_in(view_palette(),hz/5); }
static const ScenePort port={fade_out,fade_in,scene_video_fade_done,leave,enter};

static u16 keys(u16 raw)
{
    u16 out=0;
    if (raw & BUTTON_UP) out|=KEY_UP;
    if (raw & BUTTON_DOWN) out|=KEY_DOWN;
    if (raw & BUTTON_LEFT) out|=KEY_LEFT;
    if (raw & BUTTON_RIGHT) out|=KEY_RIGHT;
    if (raw & BUTTON_A) out|=KEY_ACCEPT;
    if (raw & BUTTON_B) out|=KEY_BACK;
    if (raw & BUTTON_START) out|=KEY_START;
    return out;
}
static void click(void)
{
    if (lab.effects) XGM2_playPCM(click_sound,sizeof click_sound,SOUND_PCM_CH2);
}
static void apply(LabAction action)
{
    if (action.flags & ACTION_LOAD) {
        Checkpoint loaded;
        if (!storage_load(&loaded)) {
            notice="CHECKPOINT UNAVAILABLE"; lab.has_save=false;
            scene_request(&flow,SCENE_TITLE); return;
        }
        checkpoint=loaded; lab.run=loaded.run; lab.music=loaded.music; lab.effects=loaded.effects;
        lab_restart_level(&lab); lab.advance_room=false;
        action.flags|=ACTION_SCENE;
        action.target=SCENE_PLAY;
    }
    if (action.flags & ACTION_OPTIONS) {
        if (lab.music) XGM2_resume(); else XGM2_pause();
        if (!lab.effects) { XGM2_stopPCM(SOUND_PCM_CH2); XGM2_stopPCM(SOUND_PCM_CH3); }
    }
    if (action.flags & ACTION_SCENE) scene_request(&flow,action.target);
}
static void fatal(void)
{
    VDP_setEnable(FALSE); XGM2_stop(); SPR_reset(); SPR_update(); SYS_doVBlankProcess();
    VDP_clearPlane(BG_A,TRUE); PAL_setPalette(PAL3,palette_grey,CPU);
    VDP_setTextPlane(BG_A); VDP_setTextPalette(PAL3);
    VDP_drawText("TRANSITION LAB / LOAD FAILED",4,12);
    VDP_drawText("CHECK RESOURCES AND MEMORY",5,15); VDP_setEnable(TRUE);
    while (TRUE) SYS_doVBlankProcess();
}
int main(bool hard_reset)
{
    (void)hard_reset; hz=SYS_isPAL() ? 50 : 60;
    JOY_init(); lab_init(&lab); scene_switch_init(&flow);
    lab.has_save=storage_load(&checkpoint);
    /* Warm the resident audio driver before recording a comparable heap baseline. */
    XGM2_load(menu_music);
    if (!view_init() || !enter(SCENE_TITLE)) fatal();
    fade_in(); flow.phase=SWITCH_IN;
    u16 previous[2]={keys(JOY_readJoypad(JOY_1)),keys(JOY_readJoypad(JOY_2))}, fraction=0;
    u32 shown_changes=0;
    view_status(notice,0);
    while (TRUE) {
        SYS_doVBlankProcess();
        /* Sample SGDK's cached snapshot once per pad, including during fades/pause.
           Keeping history alive consumes held Start/A instead of retriggering them. */
        u16 held[2]={keys(JOY_readJoypad(JOY_1)),keys(JOY_readJoypad(JOY_2))};
        LabInput input; lab_input_sample(&input,previous,held);
        u16 ticks=lab_clock(&fraction,hz);
        if (flow.phase==SWITCH_IDLE) {
            bool paused=lab.paused;
            u8 menu=lab.menu, joined=lab.run.joined;
            bool skip=flow.current==SCENE_SELECT &&
                (((joined&1) && (input.pressed[0]&KEY_ACCEPT)) || ((joined&2) && (input.pressed[1]&KEY_ACCEPT)));
            LabAction action=lab_step(&lab,flow.current,&input,ticks);
            if (flow.current==SCENE_PLAY && paused!=lab.paused) music_start(SCENE_PLAY);
            if (lab.audio_events&AUDIO_WIN) music_start(SCENE_PLAY);
            if (lab.effects) {
                if (lab.audio_events&AUDIO_JOIN) XGM2_playPCM(join_sound,sizeof join_sound,SOUND_PCM_CH3);
                if (lab.audio_events&AUDIO_LASER) XGM2_playPCM(laser_sound,sizeof laser_sound,SOUND_PCM_CH3);
                else if (lab.audio_events&AUDIO_POOF) XGM2_playPCM(poof_sound,sizeof poof_sound,SOUND_PCM_CH3);
                else if (lab.audio_events&AUDIO_HIT) XGM2_playPCM(hit_sound,sizeof hit_sound,SOUND_PCM_CH3);
                else if (lab.audio_events&(AUDIO_CLEAR|AUDIO_WIN)) XGM2_playPCM(clear_sound,sizeof clear_sound,SOUND_PCM_CH3);
                if (lab.audio_events&AUDIO_COIN) XGM2_playPCM(coin_sound,sizeof coin_sound,SOUND_PCM_CH2);
                else if (lab.menu!=menu || action.flags || skip) click();
            }
            apply(action);
            if (flow.phase==SWITCH_IDLE) view_draw(&lab);
        }
        scene_switch_step(&flow,&port);
        if (flow.phase==SWITCH_FAILED) fatal();
        if (flow.completed!=shown_changes) {
            shown_changes=flow.completed;
            if (flow.recovered) notice="LOAD RECOVERED / BACK TO TITLE";
            view_status(cleanup_ok ? notice : "RESOURCE CLEANUP CHECK FAILED",flow.completed);
        }
    }
}
