#include <genesis.h>
#include "audio.h"
#include "director.h"
#include "resources.h"
#include "desk_font.h"
#include "desk_ui.h"
static AdDirector director;
static u16 map[28][40],shown_map[28][40],page,selected,previous,blank_tile,line_tile,base;
static u16 scheduled,refused,last_event,elapsed,missed,pcm_rate;
static bool audition_paused;
static u16 elapsed_vblanks;
static const u32 transfer[64*8]={0};
static const char *const pages[]={"MUSIC","PATCH","SFX","MIX","CHECK"};
static const char *const tracks[]={"HARBOR LIGHTS","OPEN ROAD","NIGHT WATCH"};
static const char *const patches[]={"GLASS BELL","REED KEYS","ROUND BASS","PSG TONE","PCM DRUMS"};
static const char *const effects[]={"COIN","JUMP","IMPACT","ALERT"};
static const u8 *const songs[]={tune_harbor,tune_journey,tune_danger,tune_fanfare};
static const u8 *const solos[]={tune_bell,tune_reed,tune_bass,tune_psg,tune_drums};
static const u8 *const samples[]={snd_coin,snd_jump,snd_impact,snd_alert};
static const u32 lengths[]={sizeof(snd_coin),sizeof(snd_jump),sizeof(snd_impact),sizeof(snd_alert)};
static const u8 *const raw_samples[]={snd_coin_raw,snd_jump_raw,snd_impact_raw,snd_alert_raw};
static const u32 raw_lengths[]={sizeof(snd_coin_raw),sizeof(snd_jump_raw),sizeof(snd_impact_raw),sizeof(snd_alert_raw)};
static void text(const char *s,u16 x,u16 y)
{for(u16 i=0;x+i<40 && s[i];i++)map[y][x+i]=TILE_ATTR_FULL(PAL0,TRUE,FALSE,FALSE,TILE_FONT_INDEX+(u8)s[i]-32);}
static void center(const char *s,u16 y){text(s,(40-strlen(s))/2,y);}
static void number(u32 n,u16 x,u16 y){char s[16];sprintf(s,"%lu",n);text(s,x,y);}
static void meter(u16 x,u16 y,u16 pixels)
{for(u16 i=0;i<8;i++){u16 fill=pixels>i*8 ? pixels-i*8 : 0;if(fill>8)fill=8;map[y][x+i]=TILE_ATTR_FULL(PAL0,TRUE,FALSE,FALSE,base+fill);}}
static void draw(void)
{
    for(u16 y=0;y<28;y++)for(u16 x=0;x<40;x++)map[y][x]=TILE_ATTR_FULL(PAL0,TRUE,FALSE,FALSE,blank_tile);
    center("S O U N D   D E S K",1);
    for(u16 i=0;i<5;i++){u16 x=2+i*8;text(pages[i],x,3);if(i==page)for(u16 col=x;col<x+5;col++)map[4][col]=TILE_ATTR_FULL(PAL0,TRUE,FALSE,FALSE,line_tile);}
    if(page!=4)for(u16 y=6;y<24;y++)for(u16 x=1;x<14;x++)map[y][x]=0;
    if(page==0 || page==1) {
        u16 count=page==0 ? 3 : 5;text(page==0 ? "ARRANGEMENTS" : "SOUND PALETTE",16,6);
        for(u16 i=0;i<count;i++){text(i==selected ? ">" : " ",15,9+i*2);text(page==0 ? tracks[i] : patches[i],17,9+i*2);}
        text(audition_paused ? "PAUSED" : XGM2_isPlaying() ? "PLAYING" : "READY",17,21);
        text("A PLAY  B PAUSE  C STOP",8,25);center("UP / DOWN: SELECT",26);
    } else if(page==2) {
        text("SAMPLE BENCH",16,6);
        for(u16 i=0;i<4;i++){text(i==selected ? ">" : " ",15,9+i*2);text(effects[i],17,9+i*2);}
        text(last_event==1 ? "CLEAN EDGES" : last_event==2 ? "ABRUPT EDGES" : "LISTEN TO THE TAIL",16,19);
        text("PLAY",16,21);number(scheduled,22,21);text("BUSY",27,21);number(refused,32,21);
        center("A CLEAN  B ABRUPT  C STOP",25);center("UP / DOWN: SELECT",26);
    } else if(page==3) {
        text("SCENE CONDUCTOR",16,6);
        for(u16 i=0;i<3;i++){text(i==selected ? ">" : " ",15,9+i*2);text(tracks[i],17,9+i*2);}
        const char *state=director.timed_out ? "START TIMED OUT" : director.paused ? "PAUSED" : director.state==AD_FADING ? "FADING OUT" : director.jingle ? "FANFARE" : director.state==AD_STARTING ? "STARTING" : director.state==AD_PLAYING ? "PLAYING" : "READY";
        text(state,16,16);text("CH2",16,18);meter(22,18,director.voice[0].remaining>32 ? 64 : director.voice[0].remaining*2);
        text("CH3",16,20);meter(22,20,director.voice[1].remaining>32 ? 64 : director.voice[1].remaining*2);
        text("HELD",16,22);number(director.suppressed,22,22);text("DROP",27,22);number(director.dropped,32,22);
        center("A SCENE  B FANFARE  C SFX BURST",25);center("UP / DOWN: SELECT   START: PAUSE",26);
    } else {
        const AudioStatus *s=audio_status();
        const char *const states[]={"STOPPED","STARTING","PLAYING","PAUSING","PAUSED","RESUMING","FADING"};
        text("TRANSPORT",2,6);text(states[s->state],18,6);text("A PLAY  B PAUSE  C CLICK  UP ALARM",2,8);
        text("C+UP LOOP   C+DOWN MUSIC VOLUME",2,9);text("DOWN 1S FADE   START STOP",2,10);
        text("C+LEFT: SFX CHANNEL  C+RIGHT: DMA",2,12);
        text("LOOP",2,14);text(s->loop ? "ON" : "OFF",8,14);text("VOLUME",18,14);number(s->volume,27,14);
        text("SFX",2,16);text(s->music_channel ? "CH1 CONTEST" : "CH2 NORMAL",8,16);text("DMA",23,16);text(s->graphics_dma ? "2048 B" : "OFF",28,16);
        text("SCHEDULED",2,18);number(s->accepted,13,18);text("REJECTED",21,18);number(s->rejected,31,18);
        text("LAST 4S",2,20);if(s->timing_ready)number(s->measured_frames,13,20);else text("WAIT",13,20);
        text("EXPECT 240",22,20);text("PCM HZ",2,22);if(SYS_isPAL())text("N/A",12,22);else number(pcm_rate,12,22);
        text("MISSED",23,22);number(missed,31,22);text("WINDOW",2,23);number(s->measured_vblanks,10,23);text("VBLANKS",15,23);text("MUSIC",25,23);number(elapsed,31,23);
        center("FADES CHANGE FM/PSG, NOT PCM VOLUME",26);
    }
    for(u16 x=1;x<39;x++)map[5][x]=map[27][x]=TILE_ATTR_FULL(PAL0,TRUE,FALSE,FALSE,line_tile);
    center(SYS_isPAL() ? "PAL 50 HZ   LEFT/RIGHT: PAGES" : "NTSC 60 HZ  LEFT/RIGHT: PAGES",24);
    for(u16 y=0;y<28;y++) {
        bool changed=FALSE;for(u16 x=0;x<40;x++)if(shown_map[y][x]!=map[y][x])changed=TRUE;
        if(changed){DMA_queueDma(DMA_VRAM,map[y],VDP_getPlaneAddress(WINDOW,0,y),40,2);for(u16 x=0;x<40;x++)shown_map[y][x]=map[y][x];}
    }
}
static void stop_channels(void)
{XGM2_stopPCM(SOUND_PCM_CH1);XGM2_stopPCM(SOUND_PCM_CH2);XGM2_stopPCM(SOUND_PCM_CH3);}
static void command(AdCommand c)
{
    if(c.command==AD_FADE)XGM2_fadeOutAndStop(c.frames);
    else if(c.command==AD_STOP){XGM2_stop();stop_channels();}
    else if(c.command==AD_PLAY) {
        XGM2_stopPCM(SOUND_PCM_CH1);XGM2_play(songs[c.track]);XGM2_setLoopNumber(c.loop ? -1 : 0);
        XGM2_setFMVolume(85);XGM2_setPSGVolume(75);
    }
}
static void step(u16 held,u16 pressed)
{
    bool navigation=!(held&BUTTON_C) && (pressed&(BUTTON_LEFT|BUTTON_RIGHT));
    if(navigation) {
        if(XGM2_isProcessingFade())return;
        XGM2_stop();stop_channels();ad_init(&director);audition_paused=FALSE;
        page=(page+(pressed&BUTTON_RIGHT ? 1 : 4))%5;selected=0;last_event=0;
        if(page==4){audio_init(music,click,sizeof(click),alarm,sizeof(alarm));audio_seed_input(held);}
        return;
    }
    if(page==4){
        u16 diagnostic=held;
        if((held&BUTTON_C) && (held&(BUTTON_UP|BUTTON_DOWN)))diagnostic=held&BUTTON_UP ? BUTTON_LEFT : BUTTON_RIGHT;
        audio_step(diagnostic);return;
    }
    u16 count=page==0 || page==3 ? 3 : page==1 ? 5 : 4;
    if(pressed&BUTTON_UP)selected=(selected+count-1)%count;
    if(pressed&BUTTON_DOWN)selected=(selected+1)%count;
    if(page==3) {
        ad_clock_frames(&director,SYS_isPAL() ? 50 : 60,elapsed_vblanks);
        if(pressed&BUTTON_A)ad_scene(&director,selected);
        if(pressed&BUTTON_B)ad_jingle(&director);
        if(pressed&BUTTON_C){ad_post(&director,AD_COIN);ad_post(&director,AD_COIN);ad_post(&director,AD_JUMP);ad_post(&director,AD_IMPACT);ad_post(&director,AD_ALERT);}
        if((pressed&BUTTON_START) && director.state!=AD_FADING && director.state!=AD_IDLE) {
            ad_pause(&director,!director.paused);
            if(director.paused){XGM2_pause();XGM2_stopPCM(SOUND_PCM_CH2);XGM2_stopPCM(SOUND_PCM_CH3);}else XGM2_resume();
        }
        command(ad_service(&director,XGM2_isPlaying(),XGM2_isProcessingFade(),SYS_isPAL() ? 50 : 60,elapsed_vblanks));
        u8 fx[2];ad_flush(&director,fx);
        for(u16 c=0;c<2;c++)if(fx[c]!=AD_NONE && !XGM2_playPCMEx(samples[fx[c]],lengths[fx[c]],c ? SOUND_PCM_CH3 : SOUND_PCM_CH2,fx[c]==AD_ALERT ? 12 : fx[c]==AD_IMPACT ? 8 : fx[c]==AD_JUMP ? 4 : 2,FALSE,FALSE))ad_rejected(&director,c);
    } else if(page==2) {
        if(pressed&(BUTTON_A|BUTTON_B)) {
            bool raw=pressed&BUTTON_B;last_event=raw ? 2 : 1;
            if(XGM2_playPCMEx(raw ? raw_samples[selected] : samples[selected],raw ? raw_lengths[selected] : lengths[selected],SOUND_PCM_CH2,8,FALSE,FALSE))scheduled++;else refused++;
        }
        if(pressed&(BUTTON_C|BUTTON_START))stop_channels();
    } else {
        if(pressed&BUTTON_A) {
            XGM2_play(page==0 ? songs[selected] : solos[selected]);XGM2_setLoopNumber(-1);XGM2_setFMVolume(85);XGM2_setPSGVolume(75);audition_paused=FALSE;
        }
        if(pressed&(BUTTON_B|BUTTON_START)) {
            if(audition_paused){XGM2_resume();audition_paused=FALSE;}else if(XGM2_isPlaying()){XGM2_pause();audition_paused=TRUE;}
        }
        if(pressed&BUTTON_C){XGM2_stop();stop_channels();audition_paused=FALSE;}
    }
}
int main(bool hard_reset)
{
    (void)hard_reset;ad_init(&director);for(u16 y=0;y<28;y++)for(u16 x=0;x<40;x++)shown_map[y][x]=65535;SYS_disableInts();VDP_setEnable(FALSE);VDP_setScreenWidth320();VDP_setScreenHeight224();VDP_setPlaneSize(64,32,TRUE);
    VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);VDP_clearPlane(WINDOW,TRUE);VDP_setWindowVPos(FALSE,28);DMA_setMaxTransferSize(8192);
    base=TILE_USER_INDEX+desk.tileset->numTile;blank_tile=base+10;line_tile=base+9;
    if(base+80>=TILE_FONT_INDEX){SYS_enableInts();VDP_setEnable(TRUE);while(TRUE)SYS_doVBlankProcess();}
    VDP_drawImageEx(BG_B,&desk,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),0,0,FALSE,FALSE);
    VDP_loadFontData(desk_font,96,CPU);VDP_loadTileData(desk_meters,base,11,CPU);PAL_setPalette(PAL0,desk_colors,CPU);VDP_setBackgroundColor(1);
    Z80_loadDriver(Z80_DRIVER_XGM2,TRUE);Z80_setForceDelayDMA(TRUE);SYS_enableInts();SYS_doVBlankProcess();previous=JOY_readJoypad(JOY_1);draw();SYS_doVBlankProcess();VDP_setEnable(TRUE);
    u32 last_frame=vtimer,last_metrics=vtimer;u16 shown_state=65535,shown_voices=65535,shown_measure=65535,shown_window=65535;
    while(TRUE) {
        u32 now=vtimer,delta=now-last_frame;last_frame=now;elapsed_vblanks=delta>65535 ? 65535 : (u16)delta;
        u16 held=JOY_readJoypad(JOY_1),pressed=held&~previous;step(held,pressed);previous=held;
        bool dirty=pressed!=0;u16 state=page*64;
        if(page==3)state+=director.state+8*director.jingle+16*director.paused+32*director.timed_out;
        else if(page==4)state+=audio_status()->state;
        else if(page<2)state+=audition_paused ? 2 : XGM2_isPlaying() ? 1 : 0;
        if(state!=shown_state){shown_state=state;dirty=TRUE;}
        if(page==3) {
            u16 voices=(director.voice[0].remaining/6)*16+director.voice[1].remaining/6;
            if(voices!=shown_voices){shown_voices=voices;dirty=TRUE;}
        }
        if(page==4) {
            const AudioStatus *s=audio_status();
            if(s->graphics_dma)VDP_loadTileData(transfer,base+16,64,DMA_QUEUE);
            if(s->measured_frames!=shown_measure || s->measured_vblanks!=shown_window){shown_measure=s->measured_frames;shown_window=s->measured_vblanks;dirty=TRUE;}
            if(now-last_metrics>=(SYS_isPAL() ? 25 : 30)) {
                last_metrics=now;elapsed=(u16)XGM2_getElapsed();missed=XGM2_getDebugMissedFrames();if(!SYS_isPAL())pcm_rate=XGM2_getDebugPCMRate();dirty=TRUE;
            }
        }
        if(dirty)draw();
        SYS_doVBlankProcess();while(DMA_getQueueSize())SYS_doVBlankProcess();
    }
}
