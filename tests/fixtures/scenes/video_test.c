#include <assert.h>
#include <string.h>
#include "fake_sdk.h"
#include "scene_video.h"
#include "storage.h"
#include "view.h"

void test_video(void)
{
    fake_reset(); SPR_initEx(80);
    scene_video_fade_out(12); assert(fake_fade && !scene_video_fade_done());
    /* Model the SDK's last fade step: flag cleared, last CRAM transfer queued. */
    fake_finish_fade(); assert(scene_video_fade_done() && fake_palette_pending);
    fake_event_count=0; scene_video_leave();
    const u16 expected[]={EV_DRAIN,EV_OFF,EV_STOP,EV_PCM,EV_PCM,EV_RESET,EV_SPR,EV_DRAIN,
                           EV_CLEAR,EV_CLEAR,EV_CLEAR,EV_COLOR};
    assert(fake_event_count==sizeof expected/sizeof *expected);
    assert(!memcmp(fake_events,expected,sizeof expected));
    assert(!fake_palette_pending && !fake_sprite_pending && !fake_count);
    u16 colors[64]={0}; fake_sprite_pending=true;
    fake_event_count=0; scene_video_fade_in(colors,0);
    assert(fake_event_count==3 && fake_events[0]==EV_DRAIN && fake_events[1]==EV_ON && fake_events[2]==EV_IN);
    fake_finish_fade(); scene_video_leave(); scene_video_leave();
}
void test_storage(void)
{
    fake_reset(); Lab lab; lab_init(&lab); lab.run.joined=2;
    Checkpoint next={lab.run,true,false},loaded;
    assert(!storage_load(&loaded) && !fake_sram_mode && fake_sram_closes==1);
    assert(storage_save(&next) && !fake_sram_mode && fake_sram_closes==2);
    assert(storage_load(&loaded) && loaded.run.joined==2 && !loaded.effects && !fake_sram_mode);
    next.run.joined=0; assert(!storage_save(&next) && !fake_sram_mode);
}
void test_view(void)
{
    fake_reset(); Lab lab; lab_init(&lab); Checkpoint saved={0};
    assert(view_init()); assert(view_enter(SCENE_TITLE,&lab,&saved));
    assert(strstr(fake_cells[14],"START") && strstr(fake_cells[16],"CONTINUE"));
    assert(fake_ink[2][14]==PAL2 && fake_ink[25][10]==PAL1 && fake_ink[18][14]==PAL3);
    view_leave(); assert(view_clean());
    lab_enter(&lab,SCENE_SELECT); assert(view_enter(SCENE_SELECT,&lab,&saved));
    assert(strstr(fake_cells[20],"JOIN TO START THE COUNTDOWN"));
    lab.run.joined=1; lab.join_remaining=540; view_draw(&lab);
    char expected[41]; memset(expected,' ',40); expected[40]=0;
    memcpy(expected+12,"STARTING IN",11); memcpy(expected+25," 9",2);
    assert(!strcmp(fake_cells[20],expected)); /* No stale JOIN text around countdown. */
    lab.join_remaining=60; view_draw(&lab); assert(!strncmp(fake_cells[20]+25," 1",2));
    view_leave(); assert(view_clean());
    lab_enter(&lab,SCENE_PLAY); assert(view_enter(SCENE_PLAY,&lab,&saved));
    assert(fake_count==9); /* One joined hero plus eight coins. */
    lab.facing[0]=1; lab.flip[0]=true; lab.moving[0]=true; lab.animation_ticks[0]=16;
    view_draw(&lab); assert(fake_sprites[0].anim==1 && fake_sprites[0].frame==2 && fake_sprites[0].flip);
    lab.facing[0]=2; lab.flip[0]=false; view_draw(&lab); assert(fake_sprites[0].anim==2 && !fake_sprites[0].flip);
    lab.run.coins=0; lab.finish_ticks=1; view_draw(&lab); assert(strstr(fake_cells[14],"ROOM CLEAR!"));
    lab.paused=true; view_draw(&lab); assert(strstr(fake_cells[10],"PAUSED"));
    u16 frame=fake_sprites[0].frame,events=fake_event_count;
    view_draw(&lab); assert(fake_event_count==events && fake_sprites[0].frame==frame);
    lab.paused=false; view_draw(&lab); assert(strstr(fake_cells[14],"ROOM CLEAR!"));
    view_leave(); assert(view_clean());
}
void test_rollback(void)
{
    fake_reset(); Lab lab; lab_init(&lab); lab.run.joined=3; Checkpoint saved={0};
    assert(view_init());
    for (unsigned i=0;i<40;++i) {
        lab_enter(&lab,SCENE_PLAY);
        fake_fail_sprite_after=3;
        assert(!view_enter(SCENE_PLAY,&lab,&saved) && fake_count==3);
        view_leave(); assert(view_clean());
        fake_fail_sprite_after=-1;
        assert(view_enter(SCENE_TITLE,&lab,&saved)); view_leave(); assert(view_clean());
        assert(view_enter(SCENE_PLAY,&lab,&saved) && fake_count==10);
        view_leave(); assert(view_clean());
    }
}

void test_start_checkpoints(void)
{
    fake_reset(); Lab lab; lab_init(&lab); lab_enter(&lab,SCENE_SELECT);
    lab.run.joined=3;
    for (u16 room=0;room<ROOM_COUNT;++room) {
        if (room) lab.advance_room=true;
        lab_enter(&lab,SCENE_PLAY);
        assert(lab.run.room==room && lab.run.coins==255 && !lab.run.boss_state);
        Checkpoint start={lab.run,lab.music,lab.effects},loaded;
        assert(storage_save(&start));
        LabInput idle={0};
        for (u16 i=0;i<COIN_COUNT;++i) {
            lab.run.x[0]=lab_coin_x(lab.run.room,i); lab.run.y[0]=lab_coin_y(lab.run.room,i);
            if (room==ROOM_COUNT-1) lab.invulnerable[0]=1000;
            LabAction action=lab_step(&lab,SCENE_PLAY,&idle,1);
            assert(action.flags==ACTION_NONE);
        }
        if (room==ROOM_COUNT-1) { lab.run.boss_state=2; lab.finish_ticks=179; }
        else lab.finish_ticks=119;
        LabAction end=lab_step(&lab,SCENE_PLAY,&idle,1);
        assert(end.flags==ACTION_SCENE); /* Completing a room does not write SRAM. */
        assert(storage_load(&loaded));
        assert(loaded.run.coins==255 && loaded.run.room==room && !loaded.run.boss_state);
        assert(loaded.run.x[0]==24 && loaded.run.y[0]==184);
        assert(loaded.run.score[0]+loaded.run.score[1]==room*COIN_COUNT);
        if (room==ROOM_COUNT-1) {
            lab.run=loaded.run; lab_enter(&lab,SCENE_PLAY);
            assert(!lab.run.boss_state && lab.run.coins==255 && lab.boss_defeat_ticks==0);
        }
    }
}
