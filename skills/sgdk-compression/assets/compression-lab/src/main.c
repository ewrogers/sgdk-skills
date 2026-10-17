#include <genesis.h>
#include "catalog.h"

#define ART_A 256
#define ART_B (ART_A+128)
static CLLoader loader;
static u16 screen[28][40];
static u16 previous,page,sample,codec=2,active_sample,active_slot=ART_A;
static u16 target_slot=ART_B,fade,phase,motion,seconds,frames;
static u32 times[2][3];
static bool measured[2],measure=TRUE,redraw=TRUE;
static const char *const codecs[]={"RAW","APLIB","LZ4W"};
static const char *status="MEASURING / 16 DECODES EACH";
static const u16 ink[16]={0x0200,0x0442,0x0664,0x0886,0x0AA8,0x0CCA,0x0EEC,0,0,0,0,0,0,0,0,0x0EEE};

static u32 rom_size(void) {
    const volatile ROMHeader *header=&rom_header;
    return header->rom_end+1;
}
/* Keep this small bounded renderer out of release LTO's string-loop cloning. */
__attribute__((noipa))
static void put(const char *s,u16 x,u16 y,u16 pal) {
    u16 length=strlen(s);
    for(u16 i=0;i<length && x<40 && y<28;i++)
        screen[y][x++]=TILE_ATTR(pal,FALSE,FALSE,FALSE)+(TILE_FONT_INDEX+(u8)s[i]-32);
}
static void number(u32 n,u16 x,u16 y,u16 pal) {
    char s[12];uintToStr(n,s,1);put(s,x,y,pal);
}
static void fill(u16 x,u16 y,u16 w,u16 h,u16 tile,u16 pal) {
    for(u16 yy=y;yy<y+h && yy<28;yy++)for(u16 xx=x;xx<x+w && xx<40;xx++)
        screen[yy][xx]=TILE_ATTR_FULL(pal,FALSE,FALSE,FALSE,tile);
}
static void picture(void) {
    for(u16 y=0;y<8;y++)for(u16 x=0;x<16;x++)
        screen[y+6][x+12]=TILE_ATTR_FULL(PAL1,FALSE,FALSE,FALSE,active_slot+y*16+x);
}
static void palette(u16 strength) {
    static u16 last=65535;
    if(last==strength)return;
    last=strength;
    u16 p[16];
    for(u16 i=0;i<16;i++) {
        u16 c=cl_palette[i];
        p[i]=((((c>>9)&7)*strength/8)<<9)|((((c>>5)&7)*strength/8)<<5)|((((c>>1)&7)*strength/8)<<1);
    }
    /* Only at VBlank. UI palette is independent from fading picture colors. */
    PAL_setPalette(PAL1,p,CPU);
}
static int queue_art(const unsigned short *data,unsigned short offset,unsigned short bytes,void *ctx) {
    (void)ctx;
    if(!DMA_canQueue(DMA_VRAM,bytes/2) || DMA_getQueueSize()>=DMA_getMaxQueueSize())return 0;
    return DMA_queueDma(DMA_VRAM,(void *)data,target_slot*32+offset,bytes/2,2);
}
static u32 chunk_bytes(u16 which) {
    u32 n=0;for(u16 i=0;i<8;i++)n+=cl_chunks[which][i].stored;return n;
}
static void draw(void) {
    for(u16 y=0;y<28;y++)for(u16 x=0;x<40;x++)screen[y][x]=TILE_FONT_INDEX;
    put("COMPRESSION LAB",2,1,PAL0);put(SYS_isPAL()?"PAL 50":"NTSC 60",30,1,PAL2);
    const char *tabs[]={"COMPARE","LOAD","PRELOAD"};
    for(u16 i=0;i<3;i++){fill(2+i*13,3,10,1,97,i==page?PAL2:PAL0);put(tabs[i],3+i*13,3,i==page?PAL2:PAL0);}
    put(active_sample?"ENTROPY / POOR COMPRESSION":"AQUEDUCT / REPEATED ART",6,5,PAL0);
    picture();
    if(!page) {
        put("FORMAT   ROM B     EST US   RAM B",2,15,PAL2);
        for(u16 i=0;i<3;i++) {
            put(codecs[i],2,16+i,PAL0);number(cl_assets[sample][i].stored,11,16+i,PAL0);
            if(measured[sample])number(times[sample][i],21,16+i,PAL0);else put("--",21,16+i,PAL0);
            put(i?"4096":"0/4096",29,16+i,PAL0);
        }
        put("RAW RAM: DIRECT ROM / COPY BUFFER",2,20,PAL0);
        put("EST US = 16 DECODES / WALL TIME",2,21,PAL0);
        put("VRAM: SAME 4096 BYTES PER PICTURE",2,22,PAL0);
        put("WHOLE LAB ROM:",2,24,PAL0);number(rom_size(),17,24,PAL0);put("B",25,24,PAL0);
        put("A MEASURE   B SAMPLE   L/R PAGE",2,26,PAL0);
    } else {
        put(page==1?"FADE, DECODE, UPLOAD, FADE IN":"DECODE ONE INDEPENDENT CHUNK/FRAME",2,15,PAL2);
        put("FORMAT:",2,17,PAL0);put(page==1?codecs[codec]:"LZ4W CHUNKS",10,17,PAL0);
        put("STORED:",2,18,PAL0);number(page==1?cl_assets[active_sample^1][codec].stored:chunk_bytes(active_sample^1),10,18,PAL0);
        put("BUFFER: 4096 B",23,18,PAL0);
        fill(4,20,32,1,98,PAL0);fill(4,20,loader.sent*4,1,97,PAL2);
        put("512 B/FRAME   TWO VRAM SLOTS: 8192 B",2,22,PAL0);
        put("TICKS:",2,24,PAL0);number(seconds,9,24,PAL0);
        /* A live marker makes it visible that chunk preloading continues alongside UI work. */
        put("*",15+(motion/4)%20,24,PAL2);
        put(page==1?"A LOAD NEXT  C FORMAT  L/R PAGE":"A PRELOAD / SHOW READY  L/R PAGE",2,26,PAL0);
    }
    put(status,2,0,PAL2);
    for(u16 y=0;y<28;y++)VDP_setTileMapDataRow(BG_A,screen[y],y,0,40,DMA_QUEUE);
}
static void benchmark(void) {
    for(u16 c=0;c<3;c++) {
        u32 start=getSubTick();
        for(u16 n=0;n<16;n++)if(!cl_decode(sample,c,0,0,(u8 *)loader.data,CL_BYTES,0)) {
            status="DECODE SIZE CHECK FAILED";measure=FALSE;return;
        }
        times[sample][c]=((getSubTick()-start)*625+384)/768;
    }
    measured[sample]=TRUE;measure=FALSE;
    /* The measured sample also becomes the visible one. */
    VDP_loadTileData((const u32 *)(const void *)loader.data,active_slot,128,DMA_QUEUE);
    SYS_doVBlankProcess();
    while(DMA_getQueueSize())SYS_doVBlankProcess();
    active_sample=sample;status="READY / ESTIMATES INCLUDE INTERRUPTS";redraw=TRUE;
}
int main(bool hard_reset) {
    (void)hard_reset;
    SYS_disableInts();VDP_setEnable(FALSE);VDP_setScreenWidth320();VDP_setScreenHeight224();
    VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);
    PAL_setPalette(PAL0,ink,CPU);PAL_setPalette(PAL2,ink,CPU);
    PAL_setColor(32+15,0x0CEE);PAL_setColor(32+1,0x0640);
    u32 tile[8];for(u16 y=0;y<8;y++)tile[y]=0x11111111;VDP_loadTileData(tile,97,1,CPU);
    for(u16 y=0;y<8;y++)tile[y]=0x22222222;
    VDP_loadTileData(tile,98,1,CPU);
    cl_init(&loader);cl_decode(0,0,0,0,(u8 *)loader.data,CL_BYTES,0);
    VDP_loadTileData((const u32 *)(const void *)loader.data,active_slot,128,CPU);palette(8);
    DMA_setMaxTransferSize(6144);
    SYS_enableInts();SYS_doVBlankProcess();VDP_setEnable(TRUE);
    while(TRUE) {
        u16 pressed=JOY_readJoypad(JOY_1)&(u16)~previous;previous=JOY_readJoypad(JOY_1);
        if(pressed&(BUTTON_LEFT|BUTTON_RIGHT)) {
            cl_cancel(&loader);phase=0;fade=8;
            page=(page+((pressed&BUTTON_RIGHT)?1:2))%3;
            status=page==0?"A COMPARES ALL THREE FORMATS":page==1?"A LOADS THE OTHER PICTURE":"A PRELOADS THE OTHER PICTURE";
            redraw=TRUE;
        }
        if(!page && !loader.active) {
            if(pressed&BUTTON_B){sample^=1;measure=TRUE;redraw=TRUE;}
            if(pressed&BUTTON_A){measure=TRUE;redraw=TRUE;}
        }
        if(page==1 && !phase && pressed&BUTTON_C){codec=(codec+1)%3;redraw=TRUE;}
        if(page && pressed&BUTTON_A) {
            if(page==2 && cl_take(&loader)) {
                active_sample^=1;active_slot=target_slot;status="READY CACHE SHOWN / NO DECODE NOW";redraw=TRUE;
            } else if(!loader.active && !phase) {
                target_slot=active_slot==ART_A?ART_B:ART_A;
                if(cl_begin(&loader,active_sample^1,page==1?codec:2,page==2)) {
                    phase=page==1?1:0;fade=8;
                    status=page==1?"FADING OUT":"PRELOADING / OLD ART STAYS VISIBLE";redraw=TRUE;
                }
            }
        }
        if(++frames>=(SYS_isPAL()?50:60)){frames=0;seconds++;redraw=TRUE;}
        motion++;if(page && (motion&3)==0)redraw=TRUE;
        if(phase==1) {if(fade)fade--;else {phase=2;status="LOADING BEHIND BLACK";}}
        if(loader.active && (page==2 || phase==2)) {
            cl_step(&loader,cl_decode,queue_art,0);redraw=TRUE;
            if(loader.failed)status="DECODE FAILED / CHANGE PAGE TO RETRY";
        }
        if(page==1 && phase==2 && loader.ready) {
            cl_take(&loader);active_sample^=1;active_slot=target_slot;phase=3;
            status="LOADED / FADING IN";redraw=TRUE;
        }
        if(phase==3){if(fade<8)fade++;else {phase=0;status="LOADED / A LOADS THE OTHER PICTURE";}}
        if(page==2 && loader.ready)status="PRELOAD READY / PRESS A TO SHOW";
        if(!page && measure)status="MEASURING / 16 DECODES EACH";
        if(redraw){draw();redraw=FALSE;}
        SYS_doVBlankProcess();
        while(DMA_getQueueSize())SYS_doVBlankProcess();
        cl_complete(&loader);
        palette(page==1 && phase ? fade : 8);
        if(!page && measure)benchmark();
    }
}
