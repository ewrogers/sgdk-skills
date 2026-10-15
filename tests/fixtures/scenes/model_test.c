#include <assert.h>
#include <string.h>
#include "lab.h"
#include "arena.h"
#include "level.h"
#include "scene_switch.h"
#include "text.h"

static LabAction step(Lab *lab,SceneId scene,u16 p1,u16 p2,u16 held1,u16 held2,u16 ticks)
{
    LabInput input={{held1,held2},{p1,p2}};
    return lab_step(lab,scene,&input,ticks);
}
void test_routes(void)
{
    Lab lab; lab_init(&lab);
    assert(step(&lab,SCENE_TITLE,KEY_START,0,0,0,1).target==SCENE_SELECT);
    lab.menu=1; assert(!step(&lab,SCENE_TITLE,KEY_ACCEPT,0,0,0,1).flags);
    lab.has_save=true; assert(step(&lab,SCENE_TITLE,KEY_ACCEPT,0,0,0,1).target==SCENE_CONTINUE);
    lab_enter(&lab,SCENE_CONTINUE);
    assert(step(&lab,SCENE_CONTINUE,KEY_START,0,0,0,1).flags==ACTION_LOAD);
    assert(step(&lab,SCENE_CONTINUE,KEY_BACK,0,0,0,1).target==SCENE_TITLE);
    lab.menu=2; assert(step(&lab,SCENE_TITLE,0,KEY_START,0,0,1).target==SCENE_OPTIONS);
    lab_enter(&lab,SCENE_OPTIONS); assert(lab.music);
    assert(step(&lab,SCENE_OPTIONS,KEY_RIGHT,0,0,0,1).flags==ACTION_OPTIONS && !lab.music);
    step(&lab,SCENE_OPTIONS,KEY_DOWN,0,0,0,1);
    assert(step(&lab,SCENE_OPTIONS,KEY_ACCEPT,0,0,0,1).flags==ACTION_OPTIONS && !lab.effects);
    assert(step(&lab,SCENE_OPTIONS,KEY_BACK,0,0,0,1).target==SCENE_TITLE);
    assert(step(&lab,SCENE_CREDITS,0,KEY_ACCEPT,0,0,1).target==SCENE_TITLE);

}
void test_join(void)
{
    for (u16 hz=50;hz<=60;hz+=10) {
        Lab lab; lab_init(&lab); lab_enter(&lab,SCENE_SELECT);
        for (u16 i=0;i<800;++i) assert(!step(&lab,SCENE_SELECT,0,0,0,0,1).flags);
        assert(!lab.run.joined && lab.join_remaining==600);
        step(&lab,SCENE_SELECT,0,KEY_START,0,KEY_START,1);
        assert(lab.run.joined==2 && lab.join_remaining==600);
        u16 fraction=0;
        for (u16 frame=0;frame<hz*10;++frame) {
            u16 ticks=lab_clock(&fraction,hz);
            u16 first=frame==hz ? KEY_START : 0;
            LabAction action=step(&lab,SCENE_SELECT,first,0,0,KEY_START,ticks);
            assert((action.flags!=0)==(frame==hz*10-1));
            if (action.flags) assert(action.target==SCENE_PLAY);
        }
        assert(lab.run.joined==3 && lab.join_remaining==0);
    }
    Lab lab; lab_init(&lab); lab_enter(&lab,SCENE_SELECT);
    step(&lab,SCENE_SELECT,KEY_START,KEY_START,0,0,1);
    assert(lab.run.joined==3);
    step(&lab,SCENE_SELECT,KEY_RIGHT,0,0,0,1);
    assert(lab.run.character[0]==1 && lab.run.character[1]==1);
    assert(step(&lab,SCENE_SELECT,KEY_BACK,0,0,0,1).target==SCENE_TITLE);
}
void test_skip_and_edges(void)
{
    Lab lab; lab_init(&lab); lab_enter(&lab,SCENE_SELECT);
    assert(!step(&lab,SCENE_SELECT,KEY_ACCEPT,0,KEY_ACCEPT,0,1).flags && lab.join_remaining==600);
    step(&lab,SCENE_SELECT,KEY_START,0,KEY_START,0,1);
    step(&lab,SCENE_SELECT,0,0,0,0,1); assert(lab.join_remaining==599);
    step(&lab,SCENE_SELECT,KEY_ACCEPT,0,KEY_ACCEPT,0,1);
    assert(lab.join_remaining==540); /* Display 9, with a full second to its next tick. */
    for (u16 i=0;i<59;++i) step(&lab,SCENE_SELECT,0,0,KEY_ACCEPT,0,1);
    assert(lab.join_remaining==481);
    step(&lab,SCENE_SELECT,0,KEY_ACCEPT,0,KEY_ACCEPT,1);
    assert(lab.join_remaining==480); /* An unjoined controller cannot skip. */
    lab.run.joined=3; lab.join_remaining=121;
    step(&lab,SCENE_SELECT,KEY_ACCEPT,KEY_ACCEPT,0,0,2); assert(lab.join_remaining==60);
    assert(step(&lab,SCENE_SELECT,KEY_ACCEPT,0,0,0,1).target==SCENE_PLAY && !lab.join_remaining);
    u16 previous[2]={0},held[2]={KEY_START,KEY_START}; LabInput input;
    lab_input_sample(&input,previous,held); assert(input.pressed[0]==KEY_START && input.pressed[1]==KEY_START);
    for (u16 i=0;i<40;++i) { lab_input_sample(&input,previous,held); assert(!input.pressed[0] && !input.pressed[1]); }
    lab.run.joined=1; lab_enter(&lab,SCENE_PLAY); lab_step(&lab,SCENE_PLAY,&input,1);
    assert(!lab.paused); /* Same held Start survives a scene handoff without a new edge. */
    held[0]=held[1]=0; lab_input_sample(&input,previous,held);
    held[0]=KEY_START; lab_input_sample(&input,previous,held); lab_step(&lab,SCENE_PLAY,&input,1);
    assert(lab.paused);
}
void test_movement_pause_and_coins(void)
{
    Lab lab; lab_init(&lab); lab.run.joined=3; lab_enter(&lab,SCENE_PLAY);
    step(&lab,SCENE_PLAY,0,0,KEY_LEFT,0,1); assert(lab.facing[0]==1 && lab.flip[0] && lab.moving[0]);
    step(&lab,SCENE_PLAY,0,0,0,0,1); assert(lab.facing[0]==1 && lab.flip[0] && !lab.moving[0]);
    for (u16 i=0;i<17;++i) step(&lab,SCENE_PLAY,0,0,KEY_RIGHT,0,1);
    assert(lab.facing[0]==1 && !lab.flip[0] && lab.animation_ticks[0]==16);
    step(&lab,SCENE_PLAY,0,0,KEY_UP,0,1); assert(lab.facing[0]==2 && lab.animation_ticks[0]==0);
    step(&lab,SCENE_PLAY,0,0,KEY_DOWN,0,1); assert(lab.facing[0]==0);
    step(&lab,SCENE_PLAY,KEY_START,KEY_START,0,0,1); assert(lab.paused);
    Run frozen=lab.run; u16 phase=lab.phase,anim=lab.animation_ticks[0];
    for (u16 i=0;i<600;++i) step(&lab,SCENE_PLAY,0,0,KEY_RIGHT,KEY_LEFT,1);
    assert(!memcmp(&frozen,&lab.run,sizeof frozen) && phase==lab.phase && anim==lab.animation_ticks[0]);
    step(&lab,SCENE_PLAY,KEY_DOWN,0,0,0,1);
    LabAction save=step(&lab,SCENE_PLAY,KEY_ACCEPT,0,0,0,1);
    assert(save.flags==ACTION_SCENE && save.target==SCENE_TITLE);
    step(&lab,SCENE_PLAY,KEY_BACK,0,0,0,1); assert(!lab.paused);
    for (u16 i=0;i<COIN_COUNT;++i) {
        lab.run.x[0]=lab.run.x[1]=lab_coin_x(lab.run.room,i); lab.run.y[0]=lab.run.y[1]=lab_coin_y(lab.run.room,i);
        step(&lab,SCENE_PLAY,0,0,0,0,1);
        assert(lab.run.score[0]==i+1 && !lab.run.score[1]);
        if (i+1<COIN_COUNT) step(&lab,SCENE_PLAY,0,0,0,0,1);
    }
    assert(!lab.run.coins && lab.finish_ticks==1);
    for (u16 i=0;i<118;++i) assert(!step(&lab,SCENE_PLAY,0,0,KEY_RIGHT,0,1).flags);
    LabAction next=step(&lab,SCENE_PLAY,0,0,0,0,1);
    assert(next.target==SCENE_PLAY && lab.advance_room && lab.run.room==0);
    lab_enter(&lab,SCENE_PLAY); assert(lab.run.room==1 && lab.run.coins==255 && lab.run.score[0]==8);
    for (u16 i=0;i<COIN_COUNT;++i) {
        lab.run.x[1]=lab_coin_x(lab.run.room,i); lab.run.y[1]=lab_coin_y(lab.run.room,i);
        step(&lab,SCENE_PLAY,0,0,0,0,1);
    }
    lab.finish_ticks=119; next=step(&lab,SCENE_PLAY,0,0,0,0,1);
    assert(next.target==SCENE_PLAY && lab.advance_room);
    assert(lab.run.score[0]+lab.run.score[1]==16 && lab_run_valid(&lab.run));
    assert(!lab_clock(NULL,60)); u16 fraction=0; assert(!lab_clock(&fraction,55));
}

static char events[40]; static unsigned count; static bool done,fail,fail_title;
static void out(void) { events[count++]='O'; }
static void in(void) { events[count++]='I'; done=false; }
static bool ready(void) { return done; }
static void leave(void) { events[count++]='L'; }
static bool enter(SceneId scene) { events[count++]=scene==SCENE_TITLE ? 'T' : 'E'; return scene==SCENE_TITLE ? !fail_title : !fail; }
void test_switching(void)
{
    SceneSwitch flow; scene_switch_init(&flow); ScenePort port={out,in,ready,leave,enter};
    assert(!scene_request(&flow,(SceneId)-1));
    assert(scene_request(&flow,SCENE_PLAY) && !scene_request(&flow,SCENE_CREDITS));
    scene_switch_step(&flow,&port); assert(count==1 && events[0]=='O');
    for (unsigned i=0;i<30;++i) scene_switch_step(&flow,&port);
    assert(count==1); done=true; scene_switch_step(&flow,&port);
    assert(!memcmp(events,"OLEI",4) && flow.current==SCENE_PLAY && flow.completed==1);
    assert(flow.phase==SWITCH_IN); scene_switch_step(&flow,&port); assert(flow.phase==SWITCH_IN);
    done=true; scene_switch_step(&flow,&port); assert(flow.phase==SWITCH_IDLE);
    count=0; fail=true; scene_request(&flow,SCENE_OPTIONS); scene_switch_step(&flow,&port);
    done=true; scene_switch_step(&flow,&port);
    assert(!memcmp(events,"OLELTI",6) && flow.current==SCENE_TITLE && flow.recovered);
    done=true; scene_switch_step(&flow,&port);
    fail_title=true; count=0; scene_request(&flow,SCENE_OPTIONS); scene_switch_step(&flow,&port);
    done=true; scene_switch_step(&flow,&port); assert(flow.phase==SWITCH_FAILED);
    assert(!memcmp(events,"OLELT",5) && !scene_request(&flow,SCENE_PLAY));
}
void test_text(void)
{
    char buffer[12]; memset(buffer,'x',sizeof buffer);
    assert(text_field(buffer,sizeof buffer,"JOIN",10) && !strcmp(buffer,"JOIN      "));
    assert(text_field(buffer,sizeof buffer,"A\nB",3) && !strcmp(buffer,"A?B"));
    assert(text_number(buffer,sizeof buffer,0xFFFFFFFFUL,10) && !strcmp(buffer,"4294967295"));
    assert(text_number(buffer,sizeof buffer,10,2) && !strcmp(buffer,"10"));
    assert(text_number(buffer,sizeof buffer,9,2) && !strcmp(buffer," 9"));
    assert(text_number(buffer,sizeof buffer,100,2) && !strcmp(buffer,"##"));
    assert(!text_field(buffer,2,"ABCDE",2) && !text_number(buffer,2,1,2));
}

void test_boss(void)
{
    Lab lab; lab_init(&lab); lab.run.joined=3; lab.run.room=3;
    arena_reset(&lab);
    lab.run.carried[0]=3; lab.run.carried[1]=4; lab.run.coins=0xF8;
    lab.run.score[0]=14; lab.run.score[1]=13;
    lab.run.x[0]=100; lab.run.y[0]=100; lab.boss_x=100; lab.boss_y=100;
    step(&lab,SCENE_PLAY,0,0,0,0,1);
    assert(lab.audio_events&AUDIO_HIT);
    assert(lab.run.carried[0]==0 && lab.run.carried[1]==4 && lab.run.coins==0xFB);
    assert(lab.run.score[0]==12 && lab.run.score[1]==13 && lab.invulnerable[0]==90);
    assert(lab.loose[0].wait==45 && lab.loose[0].vx>0 && lab.loose[1].vx<0);
    assert(lab_run_valid(&lab.run));
    LooseCoin frozen[COIN_COUNT]; memcpy(frozen,lab.loose,sizeof frozen);
    step(&lab,SCENE_PLAY,KEY_START,0,0,0,1);
    for (unsigned i=0;i<60;++i) step(&lab,SCENE_PLAY,0,0,0,0,1);
    assert(!memcmp(frozen,lab.loose,sizeof frozen) && lab.invulnerable[0]==90);
    step(&lab,SCENE_PLAY,KEY_START,0,0,0,1);
    lab.run.x[0]=16; lab.run.y[0]=64; lab.run.x[1]=288; lab.run.y[1]=64;
    lab.invulnerable[0]=lab.invulnerable[1]=1000;
    for (unsigned tick=0;tick<600;++tick) {
        step(&lab,SCENE_PLAY,0,0,0,0,1);
        for (u16 i=0;i<2;++i) assert(lab.loose[i].x>=16*8 && lab.loose[i].x<=288*8 &&
            lab.loose[i].y>=64*8 && lab.loose[i].y<=192*8);
    }
    assert(!lab.loose[0].vx && !lab.loose[0].vy && !lab.loose[0].wait && !lab.loose[0].height && !lab.loose[0].lift);
    for (u16 i=0;i<COIN_COUNT;++i) if (lab.run.coins&(1u<<i)) {
        lab.run.x[0]=lab.loose[i].x/8; lab.run.y[0]=lab.loose[i].y/8;
        step(&lab,SCENE_PLAY,0,0,0,0,1);
    }
    assert(!lab.run.coins && lab.run.boss_state==1 && (lab.audio_events&AUDIO_POOF));
    assert(lab.run.score[0]+lab.run.score[1]==32 && lab_run_valid(&lab.run));
    lab.run.x[0]=16; lab.run.y[0]=64;
    for (u16 i=0;i<48;++i) step(&lab,SCENE_PLAY,0,0,0,0,1);
    assert(lab.boss_defeat_ticks==48 && lab.run.boss_state==1);
    lab.run.x[0]=lab.mega_x; lab.run.y[0]=lab.mega_y;
    step(&lab,SCENE_PLAY,0,0,0,0,1);
    assert(lab.run.boss_state==2 && (lab.audio_events&AUDIO_WIN));
    for (u16 i=0;i<179;++i) assert(!step(&lab,SCENE_PLAY,0,0,0,0,1).flags);
    LabAction end=step(&lab,SCENE_PLAY,0,0,0,0,1);
    assert(end.target==SCENE_CREDITS && end.flags==ACTION_SCENE);
    lab_restart_level(&lab); /* Continue of a valid older mid-room/finished record. */
    assert(lab.run.room==3 && lab.run.coins==255 && !lab.run.boss_state);
    assert(lab.run.score[0]+lab.run.score[1]==24 && !lab.run.carried[0] && !lab.run.carried[1]);
    assert(lab.run.x[0]==24 && lab.run.y[0]==184);
}

void test_lasers(void)
{
    static const s16 dx[]={32,23,0,-23,-32,-23,0,23};
    static const s16 dy[]={0,23,32,23,0,-23,-32,-23};
    Lab lab; lab_init(&lab); lab.run.joined=3; lab.run.room=3;
    arena_reset(&lab);
    lab.boss_attack_clock=149;
    lab.run.x[0]=288; lab.run.y[0]=192;
    lab.run.x[1]=16; lab.run.y[1]=64;
    step(&lab,SCENE_PLAY,0,0,0,0,1);
    assert(lab.boss_windup==45 && !lab.boss_walking);
    s16 x=lab.boss_x,y=lab.boss_y;
    u8 direction=lab.boss_direction;
    for (unsigned i=0;i<44;++i) {
        step(&lab,SCENE_PLAY,0,0,0,0,1);
        assert(lab.boss_x==x && lab.boss_y==y && lab.boss_direction==direction);
    }
    step(&lab,SCENE_PLAY,0,0,0,0,1);
    assert(!lab.boss_windup && lab.lasers[0].active && (lab.audio_events&AUDIO_LASER));
    Laser frozen[LASER_CAPACITY]; memcpy(frozen,lab.lasers,sizeof frozen);
    step(&lab,SCENE_PLAY,KEY_START,0,0,0,1);
    for (unsigned i=0;i<90;++i) step(&lab,SCENE_PLAY,0,0,0,0,1);
    assert(!memcmp(frozen,lab.lasers,sizeof frozen));
    step(&lab,SCENE_PLAY,KEY_START,0,0,0,1);
    for (u16 d=0;d<8;++d) {
        arena_restore(&lab);
        lab.lasers[0]=(Laser){144*8,128*8,90,d,true};
        step(&lab,SCENE_PLAY,0,0,0,0,1);
        assert(lab.lasers[0].x==144*8+dx[d] && lab.lasers[0].y==128*8+dy[d]);
        assert(lab.lasers[0].active && lab.lasers[0].life==89);
    }
    arena_restore(&lab); lab.boss_windup=1;
    for (u16 i=0;i<LASER_CAPACITY;++i) lab.lasers[i]=(Laser){144*8,128*8,90,0,true};
    step(&lab,SCENE_PLAY,0,0,0,0,1);
    assert(!(lab.audio_events&AUDIO_LASER)); /* Full pool never overwrites a live shot. */
    arena_restore(&lab); lab.run.carried[0]=1; lab.run.carried[1]=2; lab.run.coins=252;
    lab.run.score[0]=13; lab.run.score[1]=13;
    lab.run.x[0]=100; lab.run.y[0]=100;
    lab.lasers[0]=(Laser){96*8,100*8,90,0,true};
    step(&lab,SCENE_PLAY,0,0,0,0,1);
    assert(!lab.lasers[0].active && !lab.run.carried[0] && lab.run.carried[1]==2);
    assert(lab.run.score[0]==12 && lab.run.score[1]==13 && (lab.audio_events&AUDIO_HIT));
    lab.lasers[0]=(Laser){144*8,128*8,1,0,true};
    step(&lab,SCENE_PLAY,0,0,0,0,1); assert(!lab.lasers[0].active);
    lab.run.boss_state=1; lab.lasers[0]=(Laser){144*8,128*8,90,0,true};
    step(&lab,SCENE_PLAY,0,0,0,0,1); assert(!lab.lasers[0].active);
}


void test_radial_scatter(void)
{
    Lab lab; lab_init(&lab); lab.run.joined=1; lab.run.room=3; arena_reset(&lab);
    lab.run.coins=240; lab.run.carried[0]=15; lab.run.score[0]=28;
    lab.run.x[0]=144; lab.run.y[0]=128; lab.boss_x=144; lab.boss_y=128;
    LabInput idle={0}; lab_step(&lab,SCENE_PLAY,&idle,1);
    s16 sum_x=0,sum_y=0;
    for (u16 i=0;i<4;++i) {
        sum_x+=lab.loose[i].vx; sum_y+=lab.loose[i].vy;
        assert(lab.loose[i].lift>0 && lab.loose[i].height==0 && lab.loose[i].wait==45);
    }
    assert(!sum_x && !sum_y); /* No downward bias across a full radial burst. */
    assert(lab.loose[0].vx>0 && lab.loose[1].vy>0 && lab.loose[2].vx<0 && lab.loose[3].vy<0);
    lab.run.x[0]=16; lab.run.y[0]=64; lab.invulnerable[0]=1000;
    for (u16 tick=0;tick<120;++tick) {
        lab_step(&lab,SCENE_PLAY,&idle,1);
        for (u16 i=0;i<4;++i) assert(lab.loose[i].height>=0);
    }
    for (u16 i=0;i<4;++i) {
        const LooseCoin *coin=&lab.loose[i];
        assert(!coin->vx && !coin->vy && !coin->height && !coin->lift);
        assert(coin->x>100*8 && coin->x<188*8 && coin->y>84*8 && coin->y<172*8);
    }
}

void test_levels(void)
{
    for (u16 room=0;room<ROOM_COUNT;++room) {
        bool visited[65][137]={{false}},reachable[COIN_COUNT]={false};
        u16 queue_x[9000],queue_y[9000],head=0,tail=1;
        queue_x[0]=4; queue_y[0]=60; visited[60][4]=true; /* (24,184), P1 spawn. */
        while (head<tail) {
            u16 cx=queue_x[head],cy=queue_y[head++];
            s16 x=16+cx*2,y=64+cy*2;
            for (u16 i=0;i<COIN_COUNT;++i) {
                s16 coin_x=lab_coin_x(room,i)+4,coin_y=lab_coin_y(room,i)+4;
                if (x<coin_x+8 && coin_x<x+16 && y<coin_y+8 && coin_y<y+16) reachable[i]=true;
            }
            static const s16 dx[]={-1,1,0,0},dy[]={0,0,-1,1};
            for (u16 d=0;d<4;++d) {
                s16 nx=cx+dx[d],ny=cy+dy[d];
                if (nx<0 || nx>=137 || ny<0 || ny>=65 || visited[ny][nx] ||
                    level_solid(room,16+nx*2,64+ny*2,16,16)) continue;
                visited[ny][nx]=true; assert(tail<9000); queue_x[tail]=nx; queue_y[tail++]=ny;
            }
        }
        for (u16 i=0;i<COIN_COUNT;++i) {
            assert(reachable[i]);
            assert(!level_solid(room,lab_coin_x(room,i)+4,lab_coin_y(room,i)+4,8,8));
        }
        Lab lab; lab_init(&lab); lab.run.joined=1; lab.run.room=room; arena_reset(&lab);
        const Block *block=level_block(room,0);
        lab.run.x[0]=block->x-16; lab.run.y[0]=block->y;
        LabInput right={{KEY_RIGHT,0},{0}};
        for (u16 i=0;i<20;++i) lab_step(&lab,SCENE_PLAY,&right,1);
        assert(lab.run.x[0]==block->x-16);
        assert(!level_solid(room,block->x-16,block->y,16,16));
        assert(level_solid(room,block->x-15,block->y,16,16));
    }
}

void test_boss_path(void)
{
    Lab lab; lab_init(&lab); lab.run.joined=1; lab.run.room=3; arena_reset(&lab);
    lab.run.x[0]=240; lab.run.y[0]=112; lab.invulnerable[0]=2000;
    lab.boss_x=64; lab.boss_y=112;
    LabInput idle={0};
    for (u16 tick=0;tick<900;++tick) {
        lab_step(&lab,SCENE_PLAY,&idle,1);
        assert(!level_solid(3,lab.boss_x,lab.boss_y,32,32));
    }
    s16 dx=lab.boss_x+8-lab.run.x[0],dy=lab.boss_y+8-lab.run.y[0];
    assert(dx>-24 && dx<24 && dy>-24 && dy<24);
    s16 x=0,y=0;
    assert(!level_next_step(4,144,88,16,64,&x,&y));
    assert(!level_next_step(3,104,112,16,64,&x,&y));
    assert(!level_next_step(3,144,88,16,64,NULL,&y));
}
