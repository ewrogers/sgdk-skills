#include "platform.h"
#define ART_A 256
#define ART_B 512
static u16 screen[28][40];
static u16 previous,phase,score,game_x=76,game_y=108,beacon=0,hazard_x=16;
static s16 hazard_dir=1;
static u16 published_kind;
static u16 active_slot=ART_A,target_slot=ART_B,variant,screen_ticks,fps,frame_count;
static u32 fps_start;
static bool allow_worker=TRUE,published=FALSE,fatal=FALSE;
static const char *status="TAG THE GOLD LIGHTS WHILE WORK RUNS";
static const u16 points[4][2]={{28,60},{124,156},{28,156},{124,60}};
static void text(const char *s,u16 x,u16 y,u16 pal) __attribute__((noipa));
static void text(const char *s,u16 x,u16 y,u16 pal) {
    u16 len=strlen(s);
    for(u16 i=0;i<len && x<40 && y<28;i++)screen[y][x++]=TILE_ATTR(pal,FALSE,FALSE,FALSE)+TILE_FONT_INDEX+(u8)s[i]-32;
}
static void number(u32 n,u16 x,u16 y) {char s[12];uintToStr(n,s,1);text(s,x,y,PAL0);}
static void bar(u16 count,u16 total) {
    u16 cells=total?(u32)count*36/total:0;
    for(u16 x=2;x<38;x++)screen[24][x]=TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,x<2+cells?97:98);
}
static void draw(void) {
    for(u16 y=0;y<28;y++)for(u16 x=0;x<40;x++)screen[y][x]=TILE_FONT_INDEX;
    text("SPARE TIME LAB",2,1,PAL0);text(SYS_isPAL()?"PAL 50":"NTSC 60",30,1,PAL0);
    text("A: UNPACK  B: WEATHER  C: CANCEL",2,3,PAL0);
    text("D-PAD: MOVE  START: HOLD WORKER",2,4,PAL0);
    for(u16 y=5;y<23;y++)for(u16 x=1;x<20;x++)
        screen[y][x]=TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,(x==1 || x==19 || y==5 || y==22)?98:99);
    text("TAG LIGHTS",5,5,PAL0);text(!published?"PUBLISHED RESULT":published_kind==MT_RADAR?"WEATHER MAP":"DECODED ATLAS",22,5,PAL0);
    if(published && published_kind==MT_RADAR) {
        text("RAIN",22,22,PAL0);text("MAX",34,22,PAL0);
        for(u16 x=0;x<6;x++)screen[22][27+x]=TILE_ATTR_FULL(PAL1,FALSE,FALSE,FALSE,103+x);
    }
    if(published)for(u16 y=0;y<16;y++)for(u16 x=0;x<16;x++)
        screen[y+6][x+22]=TILE_ATTR_FULL(PAL1,FALSE,FALSE,FALSE,active_slot+y*16+x);
    else {text("PRESS A OR B",24,11,PAL0);text("KEEP FLYING!",24,13,PAL0);}
    text("TAGS",2,23,PAL0);number(score,7,23);
    text("STACK",22,23,PAL0);number(mt_stack_used(mt_stack,MT_STACK_WORDS),28,23);text("/1984",32,23,PAL0);
    if(mt_work.state==MT_UPLOADING)bar(mt_work.sent,MT_BYTES);
    else bar(mt_work.progress,mt_work.total);
    text(status,2,25,PAL0);
    text(allow_worker?"WORKER: ON":"WORKER: HELD",2,26,PAL0);
    text("FG FPS",25,26,PAL0);number(fps,33,26);
    for(u16 y=0;y<28;y++)VDP_setTileMapDataRow(BG_A,screen[y],y,0,40,DMA_QUEUE);
}
static int queue_art(const unsigned short *data,unsigned short offset,unsigned short bytes,void *ctx) {
    (void)ctx;
    if(!DMA_canQueue(DMA_VRAM,bytes/2) || DMA_getQueueSize()>=DMA_getMaxQueueSize())return 0;
    return DMA_queueDma(DMA_VRAM,(void *)data,target_slot*32+offset,bytes/2,2);
}
static void game_tick(u16 held) {
    if((held&BUTTON_LEFT) && game_x>18)game_x-=2;
    if((held&BUTTON_RIGHT) && game_x<134)game_x+=2;
    if((held&BUTTON_UP) && game_y>50)game_y-=2;
    if((held&BUTTON_DOWN) && game_y<166)game_y+=2;
    hazard_x+=hazard_dir;
    if(hazard_x>=134)hazard_dir=-1;
    if(hazard_x<=18)hazard_dir=1;
    s16 dx=(s16)game_x-points[beacon][0],dy=(s16)game_y-points[beacon][1];
    if(dx>-8 && dx<8 && dy>-8 && dy<8){score++;beacon=(beacon+1)%4;}
    dx=(s16)game_x-hazard_x;dy=(s16)game_y-108;
    if(dx>-7 && dx<7 && dy>-7 && dy<7){game_x=76;game_y=156;}
}
static void sprites(void) {
    VDP_setSpriteFull(0,game_x,game_y,SPRITE_SIZE(1,1),TILE_ATTR_FULL(PAL2,TRUE,FALSE,FALSE,100),1);
    VDP_setSpriteFull(1,points[beacon][0],points[beacon][1],SPRITE_SIZE(1,1),TILE_ATTR_FULL(PAL2,TRUE,FALSE,FALSE,101),2);
    VDP_setSpriteFull(2,hazard_x,108,SPRITE_SIZE(1,1),TILE_ATTR_FULL(PAL2,TRUE,FALSE,FALSE,102),0);
    VDP_updateSprites(3,DMA_QUEUE);
}
int main(bool hard_reset) {
    (void)hard_reset;
    SYS_disableInts();VDP_setEnable(FALSE);VDP_setScreenWidth320();VDP_setScreenHeight224();
    VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);VDP_resetSprites();
    const u16 ui[16]={0x0200,0x0864,0x0422,0x0220,0,0,0,0,0,0,0,0,0,0,0,0x0EEE};
    const u16 actors[16]={0,0x0CEE,0x0ECA,0x06CE,0x046E,0x0EEE,0,0,0,0,0,0,0,0,0,0};
    PAL_setPalette(PAL0,ui,CPU);PAL_setPalette(PAL1,mt_palette,CPU);PAL_setPalette(PAL2,actors,CPU);
    u32 patterns[48]={0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,
        0x22222222,0x22222222,0x22222222,0x22222222,0x22222222,0x22222222,0x22222222,0x22222222,
        0x33333333,0x33333333,0x33333333,0x33313333,0x33333333,0x33333333,0x33333333,0x33333333,
        0x00011000,0x00155100,0x01511510,0x15511551,0x01511510,0x00155100,0x00011000,0,
        0x00030000,0x00353000,0x03555300,0x35555530,0x03555300,0x00353000,0x00030000,0,
        0x00444000,0x04555400,0x45545540,0x45444540,0x45545540,0x04555400,0x00444000,0};
    VDP_loadTileData(patterns,97,6,CPU);
    for(u16 c=6;c<=11;c++){u32 swatch[8];for(u16 y=0;y<8;y++)swatch[y]=0x11111111UL*c;VDP_loadTileData(swatch,103+c-6,1,CPU);}
    DMA_setMaxTransferSize(6144);
    SYS_enableInts();SYS_doVBlankProcess();VDP_setEnable(TRUE);mt_start_worker();fps_start=vtimer;screen_ticks=6;
    while(TRUE) {
        u16 held=JOY_readJoypad(JOY_1),pressed=held&(u16)~previous;previous=held;
        if(pressed&BUTTON_START)allow_worker=!allow_worker;
        if(!fatal && (pressed&(BUTTON_A|BUTTON_B))) {
            if(mt_request(&mt_work,(pressed&BUTTON_A)?MT_DECODE:MT_RADAR,variant)) {
                variant^=1;target_slot=active_slot==ART_A?ART_B:ART_A;status="WORK REQUESTED / KEEP FLYING";
            } else status="BUSY / C CANCELS BEFORE A NEW JOB";
        }
        if(pressed&BUTTON_C) {
            if(mt_cancel(&mt_work))status=allow_worker?"CANCEL REQUESTED":"RESUME WORKER TO ACKNOWLEDGE CANCEL";
        }
        if(mt_work.state==MT_CANCELLED){mt_acknowledge(&mt_work);status="CANCEL ACKNOWLEDGED / BUFFER FREE";}
        if(mt_work.state==MT_FAILED){mt_acknowledge(&mt_work);status="DECODE CHECK FAILED / TRY AGAIN";}
        if(!fatal && !mt_stack_guard(mt_stack,MT_GUARD_WORDS)) {
            TSK_stop();fatal=TRUE;status="STACK GUARD FAILED / RESET ROM";
        }
        phase+=60;u16 hz=SYS_isPAL()?50:60;
        while(phase>=hz){game_tick(held);phase-=hz;}
        if(!fatal)mt_upload_step(&mt_work,queue_art,0);
        if(mt_work.state==MT_RUNNING)status=mt_work.kind==MT_DECODE?"UNPACKING 8 PASSES IN USER TASK":"CALCULATING WEATHER IN USER TASK";
        if(mt_work.state==MT_UPLOADING)status="COMPUTE DONE / FOREGROUND UPLOAD";
        sprites();if(++screen_ticks>=6 || pressed){draw();screen_ticks=0;}
        if(!fatal && allow_worker)SYS_doVBlankProcess();
        else {
            /* Hold spare time without losing the saved user PC or stack. */
            u32 frame=vtimer;while(vtimer==frame);
            SYS_doVBlankProcessEx(IMMEDIATELY);
        }
        /* Sole DMA producer; at most 2240 text + 24 SAT + 512 art bytes. */
        while(DMA_getQueueSize()){u32 frame=vtimer;while(vtimer==frame);SYS_doVBlankProcessEx(IMMEDIATELY);}
        if(mt_upload_complete(&mt_work)){active_slot=target_slot;published_kind=mt_work.kind;published=TRUE;status="RESULT PUBLISHED / BUFFER FREE";screen_ticks=6;}
        frame_count++;u32 elapsed=vtimer-fps_start;
        if(elapsed>=hz){fps=(u32)frame_count*hz/elapsed;frame_count=0;fps_start=vtimer;}
    }
}
