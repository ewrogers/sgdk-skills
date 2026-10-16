/* Replace main.c only in a disposable project. No VDP, IRQ or audio emulation. */
#include "lab.h"
#include "replay_data.h"

static void checkpoint(const LabState *s)
{
    volatile u16 *port=(volatile u16 *)0xC00000;
    u32 digest=lab_digest(s);
    *port=s->tick; *port=s->active_ticks; *port=(u16)s->x; *port=(u16)s->y;
    *port=s->score; *port=s->fired; *port=s->paused;
    *port=(u16)(digest>>16); *port=(u16)digest;
}

int main(bool hard_reset)
{
    (void)hard_reset;
    for (u16 hz=50; hz<=60; hz+=10) {
        LabState state;
        LabClock clock={0};
        u16 frames=0;
        lab_reset(&state,REPLAY_SEED);
        checkpoint(&state);
        while (state.tick<REPLAY_TICKS) {
            frames++;
            u16 count=lab_frame_ticks(&clock,hz);
            while (count-- && state.tick<REPLAY_TICKS) {
                u16 held=0;
                if (!lab_input(reference_runs,REPLAY_COUNT,state.tick,&held)) return 3;
                lab_step(&state,held,false);
                if (state.tick%60==0 || state.tick==REPLAY_TICKS) checkpoint(&state);
            }
        }
        *(volatile u16 *)0xC00000=frames;
    }
    return 0;
}
