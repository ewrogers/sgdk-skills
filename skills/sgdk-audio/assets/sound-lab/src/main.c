#include <genesis.h>
#include "audio.h"
#include "resources.h"

static void number(u16 value, u16 x, u16 y)
{
    char text[8];
    uintToStr(value, text, 5);
    VDP_drawText(text, x, y);
}

int main(bool hard_reset)
{
    static const char *names[] = {"STOPPED   ", "STARTING  ", "PLAYING   ",
        "PAUSING   ", "PAUSED    ", "RESUMING  ", "FADING    "};
    AudioStatus shown = {AUDIO_IDLE, FALSE, 0, 65535, 65535, 65535};
    (void)hard_reset;
    VDP_setScreenWidth320();
    VDP_drawText("SGDK SOUND LAB - XGM2", 8, 2);
    VDP_drawText("A Play/restart    B Pause/resume", 2, 5);
    VDP_drawText("C Click          Up Alarm", 2, 7);
    VDP_drawText("Left Loop        Right Music volume", 2, 9);
    VDP_drawText("Down Fade/stop   Start Stop", 2, 11);
    VDP_drawText("Music:", 2, 14);
    VDP_drawText("Loop:", 2, 16);
    VDP_drawText("Music volume:", 2, 18);
    VDP_drawText("Completed:", 2, 20);
    VDP_drawText("SFX accepted:", 2, 22);
    VDP_drawText("SFX rejected:", 2, 24);
    audio_init(music, click, sizeof(click), alarm, sizeof(alarm));
    VDP_drawText(names[AUDIO_IDLE], 18, 14);
    while (TRUE)
    {
        audio_step(JOY_readJoypad(JOY_1));
        const AudioStatus *now = audio_status();
        if (shown.state != now->state) VDP_drawText(names[now->state], 18, 14);
        if (shown.loop != now->loop) VDP_drawText(now->loop ? "ON " : "OFF", 18, 16);
        if (shown.volume != now->volume) number(now->volume, 18, 18);
        if (shown.completed != now->completed) number(now->completed, 18, 20);
        if (shown.accepted != now->accepted) number(now->accepted, 18, 22);
        if (shown.rejected != now->rejected) number(now->rejected, 18, 24);
        shown = *now;
        SYS_doVBlankProcess();
    }
}
