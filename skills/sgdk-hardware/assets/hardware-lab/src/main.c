#include <genesis.h>
#include "hardware.h"
#include "resources.h"
#include "font.h"

/* SGDK's reset entry retains application globals. Snapshot before reinitializing. */
static volatile u16 initialized_word=0x1234;
static volatile u16 zero_word;
static u16 upload[1024]; /* Retained, immutable while the DMA queue owns it. */
static u32 tiles[18*8];
static const u16 palette[16]={0x000,0x222,0x00e,0x0e0,0xe00,0x0ee,0xee0,0xe0e,
    0x444,0x666,0x888,0xaaa,0xccc,0xeee,0x08e,0xeee};
static const char *titles[]={"BOOT / REGION","VIDEO PATTERNS","SPRITE SCANLINES","DMA / AUDIO","CONTROLLERS","SRAM RECORD"};
static void text(const char *s,u16 y){VDP_drawText(s,2,y);}
static void field(const char *name,const char *value,u16 y)
{
    char line[HL_FIELD_WIDTH+1];
    if(hl_format_field(line,name,value))
        VDP_drawTextEx(BG_A,line,TILE_ATTR(PAL0,FALSE,FALSE,FALSE),2,y,DMA_QUEUE_COPY);
}
static void number(const char *name,u32 value,u16 y)
{
    char digits[32];uintToStr(value,digits,1);field(name,digits,y);
}
static void hex(const char *name,u32 value,u16 y)
{
    char digits[32];intToHex(value,digits,4);field(name,digits,y);
}
static void sprites(bool overflow)
{
    u16 index=0;
    /* Separate tests: 8px pieces reach the piece limit before pixel bandwidth. */
    u16 count=overflow ? 21 : 20;
    for(u16 i=0;i<count;i++,index++)
        VDP_setSpriteFull(index,8+i*14,104,SPRITE_SIZE(1,1),TILE_ATTR_FULL(PAL1,TRUE,FALSE,FALSE,3+i%6),index+1);
    /* 10x32 = 320; 11x32 = 352. Overlap keeps the test inside the viewport. */
    count=overflow ? 11 : 10;
    for(u16 i=0;i<count;i++,index++)
        VDP_setSpriteFull(index,4+i*28,152,SPRITE_SIZE(4,1),TILE_ATTR_FULL(PAL1,TRUE,FALSE,FALSE,3),index+1);
    VDP_setSpriteLink(index-1,0);VDP_updateSprites(index,DMA_QUEUE);
}
static bool read_record(u32 *sequence)
{
    u8 data[HL_RECORD_BYTES];SRAM_enableRO();
    for(u16 i=0;i<HL_RECORD_BYTES;i++)data[i]=SRAM_readByte(i);
    SRAM_disable();return hl_decode(data,sequence);
}
static bool write_record(u32 sequence)
{
    u8 data[HL_RECORD_BYTES];u32 actual=0;hl_encode(data,sequence);
    SRAM_enable();for(u16 i=0;i<HL_RECORD_BYTES;i++)SRAM_writeByte(i,data[i]);
    SRAM_disable();return read_record(&actual) && actual==sequence;
}
int main(bool hard_reset)
{
    u16 boot_data=initialized_word,boot_zero=zero_word;
    initialized_word=0xbeef;zero_word=0xcafe;
    /* Never rely on warm reset to clear these retained RAM arrays. */
    for(u16 i=0;i<1024;i++)upload[i]=0x5555;
    for(u16 t=0;t<18;t++)for(u16 y=0;y<8;y++) {
        u32 word=0;
        for(u16 x=0;x<8;x++) {
            u16 color=t<16 ? t : t==16 ? ((x^y)&1 ? 15 : 1) : (x&1 ? 15 : 1);
            word=word<<4|color;
        }
        tiles[t*8+y]=word;
    }
    VDP_setScreenWidth320();VDP_setScreenHeight224();VDP_setEnable(FALSE);
    VDP_loadFontData(font_tiles,96,CPU);VDP_loadTileData(tiles,1,18,CPU);
    PAL_setPalette(PAL0,palette,CPU);PAL_setPalette(PAL1,palette,CPU);
    VDP_setTextPalette(PAL0);VDP_setBackgroundColor(1);
    VDP_clearPlane(BG_A,FALSE);VDP_clearPlane(BG_B,FALSE);VDP_clearSprites();VDP_updateSprites(1,CPU);
    VDP_setEnable(TRUE);
    DMA_setMaxTransferSize(8192);Z80_setForceDelayDMA(TRUE);
    u16 page=0,previous=0,hz=SYS_isPAL() ? 50 : 60,refresh=0,mode=0;
    u16 rejected=0,write_status=0;u32 logic=0,sequence=0;bool dirty=TRUE,music=FALSE,overflow=FALSE,valid=FALSE,reset_armed=FALSE,capture=FALSE,pcm=FALSE;
    HlClock clock;hl_clock_init(&clock,vtimer);
    while(TRUE) {
        SYS_doVBlankProcess();
        logic+=hl_clock_sample(&clock,vtimer,hz);
        u16 held=JOY_readJoypad(JOY_1),pressed=held&~previous;previous=held;
        if(!(page==4 && capture) && (pressed&(BUTTON_LEFT|BUTTON_RIGHT))) {
            page=(page+((pressed&BUTTON_LEFT) ? 5 : 1))%6;dirty=TRUE;
            XGM2_pause();XGM2_stopPCM(SOUND_PCM_CH3);music=FALSE;pcm=FALSE;
            if(page==5)valid=read_record(&sequence);
        }
        if(pressed&BUTTON_C){hl_clock_init(&clock,vtimer);logic=0;rejected=0;refresh=9;}
        if(!(held&(BUTTON_A|BUTTON_B)))reset_armed=TRUE;
        if(page==0) {
            if(reset_armed && (pressed&BUTTON_A))SYS_reset();
            if(reset_armed && (pressed&BUTTON_B))SYS_hardReset();
        } else if(page==2 && (pressed&BUTTON_A)){overflow=!overflow;dirty=TRUE;}
        else if(page==3) {
            if(pressed&BUTTON_A) {music=!music;if(music)XGM2_play(reference_music);else XGM2_pause();dirty=TRUE;}
            if(pressed&BUTTON_B){mode=(mode+1)%3;dirty=TRUE;}
            if(pressed&BUTTON_START) {
                pcm=!pcm;
                if(pcm)pcm=XGM2_playPCMEx(reference_pcm,sizeof(reference_pcm),SOUND_PCM_CH3,5,FALSE,TRUE);
                else XGM2_stopPCM(SOUND_PCM_CH3);
                field("",pcm ? "START: CONTINUOUS PCM TONE ON" : "START: CONTINUOUS PCM TONE OFF",10);
            }
        } else if(page==4 && (pressed&BUTTON_A)) {
            capture=!capture;refresh=9;
            field("",capture ? "PAGE LOCKED: TEST DPAD / BUTTONS" : "A: LOCK PAGE TO TEST LEFT / RIGHT",22);
            field("",capture ? "A: UNLOCK PAGE       C: COUNTERS" : "LEFT / RIGHT: PAGE    C: COUNTERS",25);
        }
        else if(page==5 && (pressed&BUTTON_A)) {
            if(held&BUTTON_B)write_status=write_record(valid ? sequence+1 : 1) ? 1 : 2;
            valid=read_record(&sequence);dirty=TRUE;
        }
        if(dirty) {
            VDP_clearPlane(BG_A,TRUE);VDP_clearSprites();VDP_updateSprites(1,DMA_QUEUE);
            text("HARDWARE LAB",1);text(titles[page],3);
            text(page==4 && capture ? "A: UNLOCK PAGE       C: COUNTERS" : "LEFT / RIGHT: PAGE    C: COUNTERS",25);
            text("CONSOLE CHECKS STILL NEED HARDWARE",26);
            if(page==0) {
                text(hard_reset ? "SGDK ENTRY: HARD" : "SGDK ENTRY: RESET",6);
                text(SYS_isPAL() ? "VIDEO FLAG: PAL / NOMINAL 50" : "VIDEO FLAG: NTSC / NOMINAL 60",7);
                hex("ENTRY DATA: ",boot_data,9);hex("ENTRY BSS:  ",boot_zero,10);
                text("COLD EXPECTS 1234 / 0000",12);
                text("WARM MAY RETAIN BEEF / CAFE",13);
                text("A: SOFTWARE RESET",16);text("B: FORCED INIT (NOT POWER CYCLE)",17);
                text("COUNTERS ARE VINTS, NOT A STOPWATCH",22);
            } else if(page==1) {
                text("CHECK EDGES, STRIPES AND COLORS",5);
                for(u16 y=7;y<20;y++)for(u16 x=1;x<39;x++)
                    VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(PAL1,FALSE,FALSE,FALSE,1+(x+y)%16),x,y);
                for(u16 x=1;x<39;x++)VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(PAL1,FALSE,FALSE,FALSE,17+x%2),x,20);
                text("LOOK FOR CROP / SHIMMER / BANDING",22);
            } else if(page==2) {
                text(overflow ? "A: OVER LIMIT (DROPOUT EXPECTED)" : "A: AT LIMIT (ALL SHOULD FIT)",6);
                text(overflow ? "21 X 8PX: PIECE LIMIT" : "20 X 8PX: PIECE LIMIT",10);
                text(overflow ? "11 X 32PX: PIXEL LIMIT" : "10 X 32PX: PIXEL LIMIT",16);
                text("TWO ROWS TEST DIFFERENT LIMITS",22);sprites(overflow);
            } else if(page==3) {
                text(music ? "A: MUSIC ON" : "A: MUSIC OFF",6);
                number("B: EXTRA DMA BYTES: ",mode*1024,8);
                text(pcm ? "START: CONTINUOUS PCM TONE ON" : "START: CONTINUOUS PCM TONE OFF",10);
                text("COMPARE MUSIC ALONE, THEN PCM",12);
                text("LISTEN FOR CRACKLE / DROPOUTS",13);
                text("SAME RETAINED BUFFER EACH FRAME",16);
                text("FORCED DMA DELAY ON (3 LINES)",17);
            } else if(page==4) {
                text("TYPE / HELD BITS FROM SGDK",6);
                text("TEST REAL 3 / 6 BUTTON PADS TOO",17);
                text("UNPLUG / RESET / TRY BOTH PORTS",18);
                text("MOUSE / TAP: USE SGDK-INPUT LAB",20);
                text(capture ? "PAGE LOCKED: TEST DPAD / BUTTONS" : "A: LOCK PAGE TO TEST LEFT / RIGHT",22);
            } else {
                text("THIS ROM OWNS SRAM BYTES 0-11",6);
                text("A: READ     HOLD B + A: WRITE",8);
                text(valid ? "RECORD CRC: VALID" : "RECORD: EMPTY OR INVALID",11);
                number("SEQUENCE: ",valid ? sequence : 0,12);
                text("WRITE INCREMENTS SEQUENCE",15);
                text("REBOOT THEN READ TO CHECK RETENTION",17);
                text("FLASHCART SAVE FLUSH MAY BE NEEDED",19);
                text("READBACK IS NOT POWER-LOSS PROOF",21);
                if(write_status)text(write_status==1 ? "LAST WRITE: READBACK MATCHED" : "LAST WRITE: READBACK FAILED",23);
            }
            dirty=FALSE;refresh=9;
        }
        /* Refresh only counters; no full-screen redraw on every input poll. */
        if(++refresh>=10) {
            refresh=0;
            if(page==0){number("VINTS: ",clock.elapsed,19);number("LOGIC 60HZ UNITS: ",logic,20);number("MISSED LOOP INTERVALS: ",clock.missed,21);}
            if(page==3){number("MISSED LOOP INTERVALS: ",clock.missed,20);number("REJECTED UPLOADS: ",rejected,21);}
            if(page==4) {
                hex("P1 PORT: ",JOY_getPortType(PORT_1),8);hex("P1 TYPE: ",JOY_getJoypadType(JOY_1),9);hex("P1 HELD: ",held,10);
                hex("P2 PORT: ",JOY_getPortType(PORT_2),12);hex("P2 TYPE: ",JOY_getJoypadType(JOY_2),13);hex("P2 HELD: ",JOY_readJoypad(JOY_2),14);
            }
        }
        if(page==3 && mode) {
            u16 words=mode*512;
            if(!hl_upload_fits(HL_UPLOAD_START,words) || !DMA_canQueue(DMA_VRAM,words) ||
                !DMA_queueDma(DMA_VRAM,upload,HL_UPLOAD_START,words,2))rejected++;
        }
    }
    return 0;
}
