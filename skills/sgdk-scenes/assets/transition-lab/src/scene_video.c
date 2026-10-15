#include <genesis.h>
#include "scene_video.h"

void scene_video_fade_out(u16 frames)
{
    PAL_fadeOutAll(frames ? frames : 1, TRUE);
}
void scene_video_fade_in(const u16 colors[64], u16 frames)
{
    /* New queued sprite tiles/SAT must be present before showing the screen. */
    SYS_doVBlankProcess();
    VDP_setEnable(TRUE);
    PAL_fadeInAll(colors, frames ? frames : 1, TRUE);
}
bool scene_video_fade_done(void) { return !PAL_isDoingFade(); }
void scene_video_leave(void)
{
    /* Outgoing producers are already frozen by the main loop. PAL's final
       step may be queued even after isDoingFade becomes false: drain it first. */
    SYS_doVBlankProcess();
    VDP_setEnable(FALSE);
    XGM2_stop();
    XGM2_stopPCM(SOUND_PCM_CH2);
    XGM2_stopPCM(SOUND_PCM_CH3);
    /* All songs/samples here stay in resident ROM. A bank-remapping game needs
       a stronger audio quiescence barrier before invalidating its audio view. */
    SPR_reset();
    SPR_update();
    SYS_doVBlankProcess(); /* Upload the empty SAT, not just its CPU cache. */
    VDP_setPlaneSize(64,32,TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE,VSCROLL_PLANE);
    VDP_setHorizontalScroll(BG_A,0); VDP_setHorizontalScroll(BG_B,0);
    VDP_setVerticalScroll(BG_A,0); VDP_setVerticalScroll(BG_B,0);
    VDP_setWindowHPos(FALSE,0); VDP_setWindowVPos(FALSE,0);
    VDP_clearPlane(BG_A,TRUE); VDP_clearPlane(BG_B,TRUE); VDP_clearPlane(WINDOW,TRUE);
    u16 black[64]={0}; PAL_setColors(0,black,64,CPU);
}
