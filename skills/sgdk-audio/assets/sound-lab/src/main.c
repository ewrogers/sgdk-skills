#include <genesis.h>
#include "audio.h"
#include "resources.h"

static void number(u16 value, u16 x, u16 y)
{
    char text[8];
    uintToStr(value, text, 5);
    VDP_drawText(text, x, y);
}

/* 64 unused pattern tiles, 2 KiB/frame. Persistent ROM source for queued DMA;
   this exercises SGDK's VBlank/audio bus protection without touching the HUD. */
static const u32 dma_tiles[64 * 8] = {0};

int main(bool hard_reset)
{
    static const char *const names[] = {"STOPPED   ", "STARTING  ", "PLAYING   ",
        "PAUSING   ", "PAUSED    ", "RESUMING  ", "FADING    "};
    AudioStatus shown = {.state = AUDIO_IDLE, .completed = 65535,
                         .accepted = 65535, .rejected = 65535};
    u16 refresh = 0;
    (void)hard_reset;
    VDP_setScreenWidth320();
    JOY_init();
    PAL_setColor(0, RGB24_TO_VDPCOLOR(0x101010));
    PAL_setColor(15, RGB24_TO_VDPCOLOR(0xCCDDEE));
    PAL_setColor(31, RGB24_TO_VDPCOLOR(0x88BBFF));
    VDP_setTextPalette(PAL1);
    VDP_drawText("SGDK SOUND LAB / FM + PSG + PCM", 2, 1);
    VDP_setTextPalette(PAL0);
    VDP_drawText("A Play/restart    B Pause/resume", 2, 3);
    VDP_drawText("C Click          Up Alarm", 2, 4);
    VDP_drawText("Left Loop        Right Music volume", 2, 5);
    VDP_drawText("Down 1s fade     Start Stop all", 2, 6);
    VDP_drawText("Hold C + Left: SFX channel", 2, 8);
    VDP_drawText("Hold C + Right: graphics DMA", 2, 9);
    VDP_drawText("Music:", 2, 11);
    VDP_drawText("Loop:     Music volume:", 2, 12);
    VDP_drawText("SFX channel:", 2, 13);
    VDP_drawText("Graphics DMA:", 2, 14);
    VDP_drawText("DMA GUARD: SDK BUS + SHORT DELAY", 2, 15);
    VDP_drawText(SYS_isPAL() ? "DETECTED PAL / 50 HZ" : "DETECTED NTSC / 60 HZ", 2, 16);
    VDP_drawText("SONG SOURCE: 60 HZ / 4-SECOND LOOP", 2, 17);
    VDP_drawText("Music frames:", 2, 18);
    VDP_drawText("Completed:", 2, 19);
    VDP_drawText("SFX accepted:", 2, 20);
    VDP_drawText("SFX rejected:", 2, 21);
    VDP_drawText("Drums own CH1; normal SFX use CH2.", 2, 23);
    VDP_drawText("Fade changes FM/PSG, not PCM volume.", 2, 24);
    VDP_drawText("PCM Hz:       Missed frames:", 2, 25);
    VDP_drawText("Last 4s: ----- (expect about 240)", 2, 26);
    audio_init(music, click, sizeof(click), alarm, sizeof(alarm));
    VDP_drawText(names[AUDIO_IDLE], 18, 11);
    VDP_drawText("CH2 / NORMAL", 18, 13);
    VDP_drawText("OFF             ", 18, 14);
    while (TRUE)
    {
        audio_step(JOY_readJoypad(JOY_1));
        const AudioStatus *now = audio_status();
        if (shown.state != now->state) VDP_drawText(names[now->state], 18, 11);
        if (shown.loop != now->loop) VDP_drawText(now->loop ? "ON " : "OFF", 8, 12);
        if (shown.volume != now->volume) number(now->volume, 27, 12);
        if (shown.completed != now->completed) number(now->completed, 18, 19);
        if (shown.accepted != now->accepted) number(now->accepted, 18, 20);
        if (shown.rejected != now->rejected) number(now->rejected, 18, 21);
        if (shown.music_channel != now->music_channel)
            VDP_drawText(now->music_channel ? "CH1 / CONTEST" : "CH2 / NORMAL ", 18, 13);
        if (shown.graphics_dma != now->graphics_dma)
            VDP_drawText(now->graphics_dma ? "2048 BYTES/FRAME" : "OFF             ", 18, 14);
        if (now->timing_ready && (!shown.timing_ready || shown.measured_frames != now->measured_frames))
            number(now->measured_frames, 11, 26);
        else if (shown.timing_ready && !now->timing_ready)
            VDP_drawText("-----", 11, 26);
        if (now->graphics_dma) VDP_loadTileData(dma_tiles, TILE_USER_INDEX, 64, DMA_QUEUE);
        if (++refresh >= now->refresh_hz / 2) {
            refresh = 0;
            number((u16)(XGM2_getElapsed() & 0xFFFF), 18, 18);
            /* SGDK 2.11 reads an 8-bit samples-per-frame counter. At 50 Hz,
               nominal 13.3 kHz exceeds 255 samples/frame, so it wraps. */
            if (now->refresh_hz == 60) number(XGM2_getDebugPCMRate(), 10, 25);
            else VDP_drawText("N/A  ", 10, 25);
            number(XGM2_getDebugMissedFrames(), 34, 25);
        }
        shown = *now;
        SYS_doVBlankProcess();
    }
}
