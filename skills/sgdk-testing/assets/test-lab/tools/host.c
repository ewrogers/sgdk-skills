#include <stdio.h>
#include <stdlib.h>
#include "lab.h"
#include "replay_data.h"

static void checkpoint(const LabState *s)
{
    printf("{\"tick\":%u,\"active\":%u,\"x\":%d,\"y\":%d,\"score\":%u,"
           "\"fired\":%u,\"paused\":%u,\"digest\":%lu}",
           s->tick,s->active_ticks,s->x,s->y,s->score,s->fired,s->paused,
           (unsigned long)lab_digest(s));
}

int main(int argc, char **argv)
{
    if (argc!=3) return 2;
    u16 hz=(u16)atoi(argv[1]);
    if (hz!=50 && hz!=60) return 2;
    bool fault=atoi(argv[2])!=0;
    LabState state;
    LabClock clock={0};
    lab_reset(&state,REPLAY_SEED);
    /* Known total, including a short final partial frame for custom tapes. */
    printf("{\"format\":\"sgdk-testlab-trace-v1\",\"replay\":\"%s\","
           "\"replay_sha256\":\"%s\",\"seed\":%u,\"tick_hz\":60,"
           "\"total_ticks\":%u,\"render_hz\":%u,\"checkpoints\":[",
           REPLAY_NAME,REPLAY_SHA,REPLAY_SEED,REPLAY_TICKS,hz);
    checkpoint(&state);
    u16 frames=0;
    while (state.tick<REPLAY_TICKS) {
        frames++;
        u16 count=lab_frame_ticks(&clock,hz);
        while (count-- && state.tick<REPLAY_TICKS) {
            u16 held=0;
            if (!lab_input(reference_runs,REPLAY_COUNT,state.tick,&held)) return 3;
            lab_step(&state,held,fault);
            if (state.tick%60==0 || state.tick==REPLAY_TICKS) {
                putchar(','); checkpoint(&state);
            }
        }
    }
    printf("],\"frames\":%u}\n",frames);
    return 0;
}
