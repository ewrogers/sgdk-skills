#include <genesis.h>
#include "memory_lab.h"

static MemoryLab lab;
static u16 previous,page,clock_ticks;
static bool automatic,redraw=TRUE;
static const char *message="A: ALLOCATE SIX BLOCKS";
static u16 screen[28][40];

static void put(const char *value,u16 x,u16 y)
{
    while(*value && x<40 && y<28)screen[y][x++]=TILE_FONT_INDEX+(u8)*value++-32;
}

static void number(const char *label,u16 n,u16 x,u16 y)
{
    char digits[6];uintToStr(n,digits,5);put(label,x,y);put(digits,x+strlen(label),y);
}

static void block(u16 x,u16 y,u16 w,u16 h,u16 color)
{
    for(u16 row=y;row<y+h && row<28;row++)for(u16 col=x;col<x+w && col<40;col++)
        screen[row][col]=TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,96+color);
}

static bool draw(void)
{
    for(u16 y=0;y<28;y++)for(u16 x=0;x<40;x++)screen[y][x]=TILE_FONT_INDEX;
    put("MEMORY LAB",2,1);put(page ? "LIFETIMES" : "FRAGMENTATION",23,1);
    number("FREE ",MEM_getFree(),2,3);number("LARGEST ",MEM_getLargestFreeBlock(),20,3);
    number("USED ",MEM_getAllocated(),2,4);number("CYCLES ",page ? lab.room_loads : lab.cycles,20,4);
    put("COUNTERS INCLUDE ALLOCATOR HEADERS",2,6);
    block(2,8,2,1,2);put("SHARED 512 BYTES STAYS LIVE",5,8);
    if(!page) {
        put("SIX 1,024-BYTE PAYLOADS",2,10);
        for(u16 i=0;i<ML_BLOCKS;i++)block(2+i*6,12,4,2,lab.blocks[i] ? 1 : 3);
        put("BLUE LIVE / GREY FREE",2,15);
        put(lab.stage ? "TAIL RESERVED TO EXPOSE THE HOLES" : "TAIL AVAILABLE TO THE GAME",2,17);
        put(message,2,19);
        put(lab.stage==3 ? "PACK CANNOT MOVE LIVE BLOCKS" : "TRY A 1,536-BYTE REQUEST WITH B",2,21);
        put("A NEXT  B TRY 1536  C CLEAN UP",2,24);
    } else {
        block(2,11,2,1,lab.room[0] ? 1 : 3);
        put(lab.room[0] ? "ROOM 1,536 PAYLOAD BYTES" : "ROOM UNLOADED",5,11);
        block(2,14,2,1,lab.staging ? 4 : 3);
        put(lab.staging ? "128 BYTES BORROWED BY DMA" : "TRANSFER STAGING RELEASED",5,14);
        for(u16 i=0;i<4;i++)screen[14][33+i]=TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,112+i);
        put("TILES",32,16);
        put("SHARED OUTLIVES THE ROOM",2,17);
        put("STAGING OUTLIVES ITS QUEUED COPY",2,19);
        put("FREE ONLY AFTER DMA HAS DRAINED",2,21);
        put("A LOAD/UNLOAD  B UPLOAD  C CLEAN",2,24);
    }
    put(automatic ? "LEFT/RIGHT PAGE  START AUTO: ON" : "LEFT/RIGHT PAGE  START AUTO: OFF",2,26);
    if(!lab.healthy)put("CHECK FAILED - RESET THE ROM",2,23);
    /* The complete screen borrows fixed RAM until the frame queue drains. */
    if(DMA_getQueueSize()+28>DMA_getMaxQueueSize() || !DMA_canQueue(DMA_VRAM,28*40))return FALSE;
    for(u16 y=0;y<28;y++)
        if(!DMA_queueDma(DMA_VRAM,screen[y],VDP_getPlaneAddress(BG_A,0,y),40,2))return FALSE;
    return TRUE;
}

static void heap_action(void)
{
    if(!ml_heap_next(&lab)){message="NOT ENOUGH ROOM / CHECK FAILED";return;}
    const char *steps[]={"CLEAN: BASELINE RESTORED","LIVE BLOCKS: PRESS A FOR HOLES",
        "REQUEST BLOCKED: SEPARATED HOLES","PACKED: STILL NO CONTIGUOUS ROOM"};
    message=steps[lab.stage];
}

static void upload(void)
{
    if(!ml_stage_begin(&lab))return;
    /* Actual borrowed RAM, kept alive until SYS_doVBlankProcess drains the queue. */
    if(!DMA_canQueue(DMA_VRAM,ML_STAGE/2) ||
       !DMA_queueDma(DMA_VRAM,lab.staging,112*32,ML_STAGE/2,2))
        ml_stage_complete(&lab,TRUE); /* Nothing was queued on rejection. */
}

int main(bool hard_reset)
{
    (void)hard_reset;
    SYS_disableInts();VDP_setEnable(FALSE);
    VDP_setScreenWidth320();VDP_setScreenHeight224();
    VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);
    const u16 palette[16]={0x0200,0x0442,0x0EC6,0x06CE,0x0666,0x06E8,0,0,0,0,0,0,0,0,0,0x0EEE};
    PAL_setPalette(PAL0,palette,CPU);
    for(u16 i=0;i<5;i++) {
        u32 solid[8];for(u16 y=0;y<8;y++)solid[y]=0x11111111UL*(i+1);
        VDP_loadTileData(solid,96+i,1,CPU);
    }
    u32 blank[32]={0};VDP_loadTileData(blank,112,4,CPU);
    if(!ml_init(&lab)) {
        VDP_drawText("NOT ENOUGH MEMORY TO START LAB",2,12);
        SYS_enableInts();VDP_setEnable(TRUE);while(TRUE)SYS_doVBlankProcess();
    }
    DMA_setMaxTransferSize(4096);
    u16 hz=SYS_isPAL() ? 50 : 60;
    SYS_enableInts();SYS_doVBlankProcess();VDP_setEnable(TRUE);
    while(TRUE) {
        u16 held=JOY_readJoypad(JOY_1),pressed=held & (u16)~previous;previous=held;
        if(pressed&(BUTTON_LEFT|BUTTON_RIGHT)) {
            ml_room_unload(&lab);ml_heap_clear(&lab);page^=1;automatic=FALSE;redraw=TRUE;
            message="A: ALLOCATE SIX BLOCKS";
        }
        if(pressed&BUTTON_START){automatic=!automatic;clock_ticks=0;redraw=TRUE;}
        bool advance=(pressed&BUTTON_A)!=0;
        if(automatic && ++clock_ticks>=hz){clock_ticks=0;advance=TRUE;}
        if(advance) {
            if(!page)heap_action();
            else if(lab.room[0])ml_room_unload(&lab);
            else {ml_room_load(&lab);if(automatic)upload();}
            redraw=TRUE;
        }
        if(pressed&BUTTON_B) {
            if(page)upload();
            else message=ml_try_request(&lab) ? "REQUEST FITS - THEN FREED" : "REQUEST BLOCKED - GAME KEEPS GOING";
            redraw=TRUE;
        }
        if(pressed&BUTTON_C) {
            ml_room_unload(&lab);ml_heap_clear(&lab);message="CLEAN: BASELINE RESTORED";redraw=TRUE;
        }
        if(redraw)redraw=!draw();
        SYS_doVBlankProcess();
        while(DMA_getQueueSize())SYS_doVBlankProcess();
        if(lab.staging){ml_stage_complete(&lab,TRUE);redraw=TRUE;}
    }
}
