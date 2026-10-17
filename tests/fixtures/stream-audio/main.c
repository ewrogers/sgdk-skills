#include <genesis.h>
#include "resources.h"
#include "palettes.h"
#include "stream_io.h"
#include "view.h"
#include "controls.h"
enum { HERO_TILE=96,PANEL_TILE=104 };
static Route route;
static ScIO io;
static u16 columns[4][24],base_colors[64],fade_colors[64],owners[2]={65535,65535};
static const u8 *songs[]={theme0,theme1,theme0,fanfare};
static char old_title[40],old_status[40];
static bool face_left;
static bool column(u16 col,u16 buffer)
{
    if(!rt_column(&route,col,columns[buffer]))return false;
    return sc_queue_words(&io,DMA_VRAM,columns[buffer],VDP_getBGBAddress()+(col&63)*2,24,128);
}
static bool palettes(u16 camera)
{
    for(u16 area=camera/320;area<=(camera+319)/320;area++) {
        u16 bank=area&1;
        if(owners[bank]==area)continue;
        /* Only the fade owner writes CRAM while fading. */
        u16 saved[16];memcpy(saved,base_colors+16+bank*16,32);
        memcpy(base_colors+16+bank*16,area_colors[area],32);
        if(route.fade.state==SW_FADE_IDLE && route.fade_before.state==SW_FADE_IDLE &&
           !sc_queue_words(&io,DMA_CRAM,base_colors+16+bank*16,(1+bank)*32,16,2)) {
            memcpy(base_colors+16+bank*16,saved,32);return false;
        }
        owners[bank]=area;
    }
    return true;
}
static void audio_command(AdCommand c)
{
    if(c.command==AD_FADE)XGM2_fadeOutAndStop(c.frames);
    else if(c.command==AD_STOP) { XGM2_stop();XGM2_stopPCM(SOUND_PCM_CH1); }
    else if(c.command==AD_PLAY) {
        XGM2_stopPCM(SOUND_PCM_CH1);XGM2_play(songs[c.track]);XGM2_setLoopNumber(c.loop ? -1 : 0);
        XGM2_setFMVolume(85);XGM2_setPSGVolume(75);
    }
}
static bool text(const char *line,u16 row)
{
    if(io.used+76<=io.limit && DMA_canQueue(DMA_VRAM,38)) {
        VDP_drawTextEx(WINDOW,line,TILE_ATTR(PAL0,TRUE,FALSE,FALSE),1,row,DMA_QUEUE);io.used+=76;return true;
    }
    return false;
}
static void hud(void)
{
    char title[40],status[40];
    const char *zone=route.player>=640 ? "NIGHT QUAY" : "DAY GARDEN";
    sprintf(title,"%-12s %s",zone,route.paused ? "PAUSED" : route.pause_requested ? "PAUSE PENDING" : route.loading ? "LOADING" : "WALK LEFT / RIGHT");
    sprintf(status,"MUSIC %u %s   UPLOAD %uB",route.audio.current,route.audio.timed_out ? "FAILED" : route.audio.jingle ? "JINGLE" : route.audio.state==AD_FADING ? "FADE" : "LOOP",route.budget*32);
    u16 n=strlen(title);while(n<38)title[n++]=' ';title[n]=0;
    n=strlen(status);while(n<38)status[n++]=' ';status[n]=0;
    if(strcmp(title,old_title) && text(title,1))strcpy(old_title,title);
    if(strcmp(status,old_status) && text(status,2))strcpy(old_status,status);
}
int main(bool reset)
{
    (void)reset;
    SYS_disableInts();VDP_setEnable(FALSE);VDP_setScreenWidth320();VDP_setScreenHeight224();VDP_setPlaneSize(64,32,TRUE);
    VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);VDP_clearPlane(WINDOW,TRUE);
    u32 panel[8];for(u16 i=0;i<8;i++)panel[i]=0x11111111UL;
    VDP_loadTileData(panel,PANEL_TILE,1,CPU);VDP_loadTileData((const u32*)stream_hero,HERO_TILE,8,CPU);
    base_colors[0]=base_colors[1]=0x0202;base_colors[15]=0x0eee;
    memcpy(base_colors+48,hero_colors,32);PAL_setColors(0,base_colors,64,CPU);
    VDP_fillTileMapRect(WINDOW,TILE_ATTR_FULL(PAL0,TRUE,FALSE,FALSE,PANEL_TILE),0,0,40,28);
    VDP_setTextPlane(WINDOW);VDP_setTextPalette(PAL0);VDP_drawText("STREAM + SOUND",1,0);
    VDP_drawText("A WARP B 64/256B C JINGLE START PAUSE",1,3);
    VDP_drawText("WARMING THE RESIDENT TILE CACHE",4,12);VDP_drawText("LEFT / RIGHT TO EXPLORE",8,16);
    VDP_setWindowOnTop(28);VDP_resetSprites();DMA_setMaxTransferSize(SC_FRAME_BYTES);
    Z80_loadDriver(Z80_DRIVER_XGM2,TRUE);Z80_setForceDelayDMA(TRUE);
    SYS_enableInts();SYS_doVBlankProcess();VDP_setEnable(TRUE);
    rt_init(&route,rt_controls(JOY_readJoypad(JOY_1)));
    u16 hz=SYS_isPAL() ? 50 : 60;
    u32 last_vblank=vtimer;
    while(TRUE) {
        io=(ScIO){0,SC_FRAME_BYTES};bool was_paused=route.paused,was_loading=route.loading;
        u8 was_fade=route.fade.state;
        u16 raw=JOY_readJoypad(JOY_1);
        u32 now=vtimer,delta=now-last_vblank;last_vblank=now;
        if(!rt_begin_elapsed(&route,rt_controls(raw),hz,XGM2_isPlaying(),XGM2_isProcessingFade(),delta>65535 ? 65535 : (u16)delta)) {
            SYS_doVBlankProcess();if(!DMA_getQueueSize())rt_end(&route,true);continue;
        }
        if(route.paused!=was_paused) {
            if(route.paused) { XGM2_pause();XGM2_stopPCM(SOUND_PCM_CH2);XGM2_stopPCM(SOUND_PCM_CH3); }
            else XGM2_resume();
        }
        audio_command(route.command);
        if(was_fade==SW_FADE_IDLE && route.fade.state==SW_FADE_OUT)ad_post(&route.audio,AD_ALERT);
        VDP_setWindowOnTop(route.covered ? 28 : 4);
        sc_step(&route.cache,route.budget,sc_rom_read,sc_dma_tiles,&io);
        bool ok=rt_draw_ready(&route);u16 count=0;
        if(ok) {
            if(route.loading) {
                u16 start=route.camera/8+route.build;
                while(count<4 && route.build+count<sw_columns(route.camera)) {
                    if(!column(start+count,count)) { ok=false;break; }count++;
                }
            } else {
                u16 entering;
                if(sw_entering(route.camera,route.next_camera,&entering))ok=column(entering,0);
            }
            /* Columns precede palette replacement: rejection must keep the old picture valid. */
            if(ok)ok=palettes(route.loading ? route.camera : route.next_camera);
        }
        if(route.fade.state!=SW_FADE_IDLE || route.fade_before.state!=SW_FADE_IDLE) {
            for(u16 i=0;i<64;i++)fade_colors[i]=sw_fade_color(base_colors[i],route.fade.level);
            bool accepted=sc_queue_words(&io,DMA_CRAM,fade_colors,0,64,2);
            rt_fade_accepted(&route,accepted);if(!accepted)ok=false;
        }
        rt_publish(&route,ok,count);
        if(was_loading && !route.loading)ad_post(&route.audio,AD_COIN);
        u8 fx[2];ad_flush(&route.audio,fx);
        for(u16 c=0;c<2;c++)if(fx[c]!=AD_NONE && !XGM2_playPCMEx(c ? alarm : coin,c ? sizeof(alarm) : sizeof(coin),
            c ? SOUND_PCM_CH3 : SOUND_PCM_CH2,c ? 12 : 2,FALSE,FALSE))ad_rejected(&route.audio,c);
        if(raw&BUTTON_LEFT)face_left=true;else if(raw&BUTTON_RIGHT)face_left=false;
        if(!route.covered) {
            VDP_setSpriteFull(0,(s16)route.player-(s16)route.camera-8,188,SPRITE_SIZE(2,2),
                TILE_ATTR_FULL(PAL3,TRUE,FALSE,face_left,HERO_TILE),0);
            VDP_updateSprites(1,DMA_QUEUE);io.used+=8;
        } else { VDP_updateSprites(0,DMA_QUEUE);io.used+=8; }
        VDP_setHorizontalScrollVSync(BG_B,-(s16)route.camera);VDP_setVerticalScrollVSync(BG_B,-32);
        hud();SYS_doVBlankProcess();while(DMA_getQueueSize())SYS_doVBlankProcess();
        rt_end(&route,true);VDP_setWindowOnTop(route.covered ? 28 : 4);
    }
    return 0;
}
