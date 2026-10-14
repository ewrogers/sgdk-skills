#include <genesis.h>

/* Keep one call per game-loop iteration. These are sampled estimates, not
 * instruction counts. Read the globals using a supported debugger, or add a
 * low-frequency display in your own game. */
volatile u32 sample_fps;
volatile u16 sample_cpu_percent;

__attribute__((noinline)) void profile_sample(void)
{
    sample_fps = SYS_getFPS();
    sample_cpu_percent = SYS_getCPULoad();
}

int main(bool hard_reset)
{
    (void) hard_reset;
    while (TRUE)
    {
        /* Game updates and SPR_update(), if using the sprite engine, go here. */
        profile_sample();
        SYS_doVBlankProcess();
    }
    return 0;
}
