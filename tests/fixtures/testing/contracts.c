#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "lab.h"

static void edges(void)
{
    LabState s;
    lab_reset(&s,44257);
    for (u16 i=0; i<40; i++) lab_step(&s,LAB_A,false);
    assert(s.fired==1);
    lab_step(&s,0,false); lab_step(&s,LAB_A,false);
    assert(s.fired==2);
    s16 x=s.x,y=s.y;
    u16 rng=s.rng,active=s.active_ticks;
    lab_step(&s,LAB_START,false);
    for (u16 i=0; i<80; i++) lab_step(&s,LAB_START|LAB_RIGHT|LAB_A,false);
    assert(s.paused && s.x==x && s.y==y && s.rng==rng && s.active_ticks==active);
    lab_step(&s,0,false); lab_step(&s,LAB_START,false);
    assert(!s.paused && s.active_ticks==active+1);
}

static void tape(void)
{
    LabTape t={0};
    for (u16 i=0; i<LAB_RUNS; i++) assert(lab_record(&t,i&1));
    assert(t.count==LAB_RUNS && t.ticks==LAB_RUNS);
    assert(!lab_record(&t,0));
    assert(t.count==LAB_RUNS && t.ticks==LAB_RUNS);
    assert(lab_record(&t,1)); /* A full run table can still extend the last run. */
    assert(t.count==LAB_RUNS && t.ticks==LAB_RUNS+1);
    u16 held=99;
    assert(lab_input(t.runs,t.count,LAB_RUNS,&held) && held==1);
    held=99;
    assert(!lab_input(t.runs,t.count,t.ticks,&held) && held==99);
    assert(!lab_record(&t,256));
    LabRun invalid[]={{0,1}};
    assert(!lab_input(invalid,1,0,&held) && held==99);
    assert(!lab_input(t.runs,129,0,&held));
    assert(!lab_input(t.runs,t.count,0,0));
    t.count=0; t.ticks=0;
    for (u16 i=0; i<LAB_TICKS; i++) assert(lab_record(&t,LAB_A));
    assert(!lab_record(&t,LAB_A) && t.count==1 && t.runs[0].ticks==LAB_TICKS);
}

static void motion(void)
{
    LabState s;
    lab_reset(&s,44257);
    for (u16 i=0;i<100;i++) lab_step(&s,LAB_RIGHT,false);
    assert(s.x==138 && s.y==96 && s.score>=1);
    for (u16 i=0;i<50;i++) lab_step(&s,LAB_UP|LAB_RIGHT,false);
    assert(s.x>181 && s.y==40);
    for (u16 i=0;i<300;i++) lab_step(&s,LAB_DOWN|LAB_RIGHT,false);
    assert(s.x==312 && s.y==180);
    for (u16 i=0;i<300;i++) lab_step(&s,LAB_UP|LAB_LEFT,false);
    assert(s.x==8 && s.y==40);
}

static void pools(void)
{
    LabState s;
    lab_reset(&s,0);
    s.y=180;
    for (u16 i=0; i<5; i++) { lab_step(&s,0,false); lab_step(&s,LAB_A,false); }
    assert(s.fired==4);
    for (u16 i=0; i<100; i++) lab_step(&s,0,false);
    lab_step(&s,LAB_A,false);
    assert(s.fired==5);
}

static void clock_test(void)
{
    for (u16 hz=50;hz<=60;hz+=10) {
        LabClock clock={0};
        u32 ticks=0;
        for (u16 i=0;i<hz*30;i++) ticks+=lab_frame_ticks(&clock,hz);
        assert(ticks==1800 && clock.phase==0);
        clock.phase=17;
        assert(lab_frame_ticks(&clock,0)==0 && clock.phase==17);
        assert(lab_frame_ticks(&clock,59)==0 && clock.phase==17);
    }
}

static void recorded(void)
{
    /* Record frame-sampled live inputs, including a PAL two-update frame. */
    for (u16 source_hz=50; source_hz<=60; source_hz+=10) {
        LabTape tape={0};
        LabState live;
        LabClock clock={0};
        lab_reset(&live,44257);
        for (u16 frame=0;frame<source_hz*4;frame++) {
            u16 held=frame%24<12 ? LAB_RIGHT : LAB_UP;
            if (frame==4 || frame==30) held|=LAB_START;
            if (frame%10==0) held|=LAB_A;
            u16 count=lab_frame_ticks(&clock,source_hz);
            while (count--) {
                assert(lab_record(&tape,held));
                lab_step(&live,held,false);
            }
        }
        assert(tape.ticks==240 && live.active_ticks<live.tick && !live.paused);
        for (u16 playback_hz=50;playback_hz<=60;playback_hz+=10) {
            LabState replay;
            lab_reset(&replay,44257); clock.phase=0;
            while (replay.tick<tape.ticks) {
                u16 count=lab_frame_ticks(&clock,playback_hz);
                while (count-- && replay.tick<tape.ticks) {
                    u16 held=0;
                    assert(lab_input(tape.runs,tape.count,replay.tick,&held));
                    lab_step(&replay,held,false);
                }
            }
            assert(lab_digest(&live)==lab_digest(&replay));
        }
    }
}

static void soak(void)
{
    LabState a,b;
    lab_reset(&a,44257); lab_reset(&b,44257);
    for (u32 i=0;i<100000;i++) {
        u16 held=(i%400<200 ? LAB_RIGHT : LAB_LEFT) | (i%8==0 ? LAB_A:0);
        lab_step(&a,held,false); lab_step(&b,held,false);
        assert(lab_digest(&a)==lab_digest(&b));
        assert(a.x>=8 && a.x<=312 && a.y>=40 && a.y<=180);
        for (u16 j=0;j<LAB_BOLTS;j++) assert(a.bolts[j].life<=80);
    }
    lab_reset(&b,44257);
    LabState c; lab_reset(&c,44257);
    assert(lab_digest(&b)==lab_digest(&c));
}

int main(int argc,char **argv)
{
    if (argc!=2) return 2;
    if (!strcmp(argv[1],"edges")) edges();
    else if (!strcmp(argv[1],"tape")) tape();
    else if (!strcmp(argv[1],"motion")) motion();
    else if (!strcmp(argv[1],"pools")) pools();
    else if (!strcmp(argv[1],"clock")) clock_test();
    else if (!strcmp(argv[1],"recorded")) recorded();
    else if (!strcmp(argv[1],"soak")) soak();
    else return 2;
    puts(argv[1]); return 0;
}
