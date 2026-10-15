#include "lab.h"
#include "arena.h"

static LabAction go(SceneId scene) { return (LabAction){ACTION_SCENE, scene}; }
static bool accept(u16 keys) { return (keys & (KEY_ACCEPT | KEY_START)) != 0; }
static u8 menu_move(u8 menu, u8 count, u16 pressed)
{
    if (pressed & KEY_UP) return (menu + count - 1) % count;
    if (pressed & KEY_DOWN) return (menu + 1) % count;
    return menu;
}
void lab_init(Lab *lab)
{
    *lab = (Lab){.music=true, .effects=true};
    arena_reset(lab);
}
void lab_enter(Lab *lab, SceneId scene)
{
    lab->menu = 0; lab->paused = false;
    lab->moving[0] = lab->moving[1] = false;
    if (scene == SCENE_SELECT) {
        lab->run = (Run){.character={0,1}};
        arena_reset(lab); lab->join_remaining = JOIN_TICKS;
    }
    if (scene == SCENE_PLAY && lab->advance_room) {
        ++lab->run.room; arena_reset(lab); lab->advance_room = false;
    }
    if (scene==SCENE_PLAY) arena_restore(lab);
}
void lab_restart_level(Lab *lab)
{
    /* Older development checkpoints may contain progress inside this room. */
    for (u16 p=0;p<2;++p)
        for (u16 i=0;i<COIN_COUNT;++i)
            if (lab->run.carried[p]&(1u<<i)) --lab->run.score[p];
    arena_reset(lab);
}
void lab_input_sample(LabInput *input, u16 previous[2], const u16 held[2])
{
    for (u16 p=0;p<2;++p) {
        input->held[p]=held[p]; input->pressed[p]=held[p] & (u16)~previous[p]; previous[p]=held[p];
    }
}
u16 lab_clock(u16 *fraction, u16 hz)
{
    if (!fraction || (hz != 50 && hz != 60)) return 0;
    *fraction += 60; u16 result = *fraction / hz; *fraction %= hz;
    return result;
}
bool lab_run_valid(const Run *r)
{
    if (!r || !r->joined || r->joined > 3 || r->room >= ROOM_COUNT) return false;
    for (u16 p=0; p<2; ++p)
        if (r->character[p] > 1 || r->x[p] < 16 || r->x[p] > 288 ||
            r->y[p] < 64 || r->y[p] > 192 || r->score[p] > COIN_COUNT * ROOM_COUNT) return false;
    if (r->boss_state>2 || (r->room!=ROOM_COUNT-1 && r->boss_state)) return false;
    if ((r->carried[0]&r->carried[1]) || (r->coins&r->carried[0]) || (r->coins&r->carried[1]) ||
        (u8)(r->coins|r->carried[0]|r->carried[1])!=255) return false;
    for (u16 p=0;p<2;++p) {
        u16 held=0; for (u16 i=0;i<8;++i) if (r->carried[p]&(1u<<i)) ++held;
        if (held>r->score[p] || (!(r->joined&(1u<<p)) && (held || r->score[p]))) return false;
    }
    u16 carried=0;
    for (u16 i=0;i<8;++i) if ((r->carried[0]|r->carried[1])&(1u<<i)) ++carried;
    if (r->score[0]+r->score[1]!=r->room*COIN_COUNT+carried) return false;
    if (r->boss_state && r->coins) return false;
    return true;
}
LabAction lab_step(Lab *lab, SceneId scene, const LabInput *input, u16 ticks)
{
    LabAction none = {0, scene};
    if (!lab || !input || (unsigned)scene >= SCENE_COUNT || ticks > 2) return none;
    lab->audio_events=0;
    u16 keys = input->pressed[0] | input->pressed[1];
    if (scene==SCENE_TITLE) lab->phase=(lab->phase+ticks)%240;
    if (scene == SCENE_TITLE) {
        lab->menu = menu_move(lab->menu, 4, keys);
        if (accept(keys)) {
            static const SceneId next[] = {SCENE_SELECT, SCENE_CONTINUE, SCENE_OPTIONS, SCENE_CREDITS};
            if (lab->menu != 1 || lab->has_save) return go(next[lab->menu]);
        }
    } else if (scene == SCENE_CONTINUE) {
        lab->menu = menu_move(lab->menu, 2, keys);
        if (keys & KEY_BACK) return go(SCENE_TITLE);
        if (accept(keys)) return lab->menu ? go(SCENE_TITLE) : (LabAction){ACTION_LOAD, SCENE_PLAY};
    } else if (scene == SCENE_OPTIONS) {
        lab->menu = menu_move(lab->menu, 3, keys);
        if (keys & KEY_BACK || (lab->menu == 2 && accept(keys))) return go(SCENE_TITLE);
        if ((keys & (KEY_LEFT | KEY_RIGHT)) || accept(keys)) {
            if (lab->menu == 0) lab->music = !lab->music;
            if (lab->menu == 1) lab->effects = !lab->effects;
            if (lab->menu < 2) return (LabAction){ACTION_OPTIONS, scene};
        }
    } else if (scene == SCENE_CREDITS) {
        if (keys & KEY_BACK || accept(keys)) return go(SCENE_TITLE);
    } else if (scene == SCENE_SELECT) {
        u8 previous_joined=lab->run.joined;
        bool was_joined = previous_joined != 0;
        for (u16 p=0; p<2; ++p) {
            if (input->pressed[p] & KEY_START) lab->run.joined |= 1u << p;
            if ((lab->run.joined & (1u << p)) && input->pressed[p] & (KEY_LEFT | KEY_RIGHT))
                lab->run.character[p] ^= 1;
        }
        if (lab->run.joined!=previous_joined) lab->audio_events|=AUDIO_JOIN;
        if (keys & KEY_BACK) return go(SCENE_TITLE);
        if (lab->run.joined) {
            u16 skips=0;
            for (u16 p=0;p<2;++p)
                if ((lab->run.joined & (1u<<p)) && (input->pressed[p] & KEY_ACCEPT)) ++skips;
            if (skips) {
                u16 seconds=(lab->join_remaining+59)/60;
                lab->join_remaining=seconds>skips ? (seconds-skips)*60 : 0;
            } else if (was_joined) {
                lab->join_remaining=ticks>=lab->join_remaining ? 0 : lab->join_remaining-ticks;
            }
            if (!lab->join_remaining) return go(SCENE_PLAY);
        }
    } else if (scene == SCENE_PLAY) {
        bool start = false;
        for (u16 p=0; p<2; ++p)
            if ((lab->run.joined & (1u << p)) && input->pressed[p] & KEY_START) start = true;
        /* Either joined player may pause; the menu uses the first joined player. */
        if (start) { lab->paused = !lab->paused; lab->menu = 0; return none; }
        if (lab->paused) {
            lab->menu = menu_move(lab->menu, 2, input->pressed[(lab->run.joined & 1) ? 0 : 1]);
            u16 pause_keys = input->pressed[(lab->run.joined & 1) ? 0 : 1];
            if (pause_keys & KEY_BACK) lab->paused = false;
            else if (accept(pause_keys)) {
                if (!lab->menu) lab->paused = false;
                else return go(SCENE_TITLE);
            }
            return none;
        }
        while (ticks--) arena_tick(lab, input);
        if (lab->finish_ticks >= (lab->run.room==ROOM_COUNT-1 ? 180 : 120)) {
            if (lab->run.room + 1 < ROOM_COUNT) {
                lab->advance_room = true;
                return go(SCENE_PLAY);
            }
            return go(SCENE_CREDITS);
        }
    }
    return none;
}
