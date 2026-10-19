#include <genesis.h>
#include "hardware.h"
#include "resources.h"
#include "font.h"
#include "readiness.h"
#include "lab_pages.h"
#include "lab_ui.h"

/* SGDK's reset entry retains application globals. Snapshot before reinitializing. */
static volatile u16 initialized_word=0x1234;
static volatile u16 zero_word;
static u16 upload[1024]; /* Retained, immutable while the DMA queue owns it. */
static u32 tiles[18*8];
#define ED_STATUS (*(volatile u16 *)0xa130d4)
#define ED_FIFO_STATUS (*(volatile u16 *)0xa130d2)
#define ED_FIFO_DATA (*(volatile u8 *)0xa130d1)
static void ed_poll(HlEdLink *link,u32 now)
{
    hl_ed_tick(link,now);
    if(!link->present)return;
    u16 count=ED_FIFO_STATUS & 0x7ff;
    if(count>32)count=32;
    for(u16 i=0;i<count;i++) {
        u8 reply[HL_ED_PACKET_BYTES];
        if(hl_ed_receive(link,ED_FIFO_DATA,now,reply)) {
            /* Krikzz CMD_USB_WR, length, payload; never wait for USB from ROM. */
            ED_FIFO_DATA=0x2b;ED_FIFO_DATA=0xd4;ED_FIFO_DATA=0x22;ED_FIFO_DATA=0xdd;
            ED_FIFO_DATA=0;ED_FIFO_DATA=HL_ED_PACKET_BYTES;
            for(u16 n=0;n<HL_ED_PACKET_BYTES;n++)ED_FIFO_DATA=reply[n];
        }
    }
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
    hl_ui_load();
    VDP_setTextPalette(PAL0);VDP_setBackgroundColor(1);
    VDP_clearPlane(BG_A,FALSE);VDP_clearPlane(BG_B,FALSE);VDP_clearSprites();VDP_updateSprites(1,CPU);
    VDP_setEnable(TRUE);
    DMA_setMaxTransferSize(8192);Z80_setForceDelayDMA(TRUE);
    u16 page=0,previous=0,hz=SYS_isPAL() ? 50 : 60,refresh=0,mode=0;
    /* Signature plus read strobe; never touch the FIFO when absent. */
    const u16 ed_first=ED_STATUS,ed_second=ED_STATUS;
    HlEdLink link;hl_ed_init(&link,hl_ed_signature(ed_first,ed_second),hz);
    u16 rejected=0,write_status=0;u32 logic=0,sequence=0;bool dirty=TRUE,music=FALSE,overflow=FALSE,valid=FALSE,reset_armed=FALSE,capture=FALSE,pcm=FALSE,first_screen=TRUE;
    HlClock clock;hl_clock_init(&clock,vtimer);
    while(TRUE) {
        SYS_doVBlankProcess();
        logic+=hl_clock_sample(&clock,vtimer,hz);
        ed_poll(&link,vtimer);
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
            if(pressed&BUTTON_A) {music=!music;if(music)XGM2_play(reference_music);else XGM2_pause();refresh=9;}
            if(pressed&BUTTON_B){mode=(mode+1)%3;refresh=9;}
            if(pressed&BUTTON_START) {
                pcm=!pcm;
                if(pcm)pcm=XGM2_playPCMEx(reference_pcm,sizeof(reference_pcm),SOUND_PCM_CH3,5,FALSE,TRUE);
                else XGM2_stopPCM(SOUND_PCM_CH3);
                refresh=9;
            }
        } else if(page==4 && (pressed&BUTTON_A)) {
            capture=!capture;refresh=9;
        }
        else if(page==5 && (pressed&BUTTON_A)) {
            if(held&BUTTON_B)write_status=write_record(valid ? sequence+1 : 1) ? 1 : 2;
            valid=read_record(&sequence);dirty=TRUE;
        }
        HlPageView view={.music=music,.pcm=pcm,.overflow=overflow,.capture=capture,.valid=valid,
            .mode=mode,.rejected=rejected,.write_status=write_status,.held=held,
            .missed=clock.missed,.sequence=sequence,.vints=clock.elapsed};
        if(dirty) {
            VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);
            VDP_clearSprites();VDP_updateSprites(1,DMA_QUEUE);
            if(page==0)hl_ready_open(hard_reset,boot_data,boot_zero,SYS_isPAL(),&link);
            else hl_pages_open(page,&view);
            if(first_screen) {
                /* Begin measurement after initial graphics/text have reached VRAM. */
                SYS_doVBlankProcess();hl_clock_init(&clock,vtimer);logic=0;first_screen=FALSE;
            }
            dirty=FALSE;refresh=9;
        }
        /* Refresh only counters; no full-screen redraw on every input poll. */
        if(++refresh>=10) {
            refresh=0;
            if(page==0)hl_ready_update(clock.elapsed,logic,clock.missed,&link);
            if(page!=0)hl_pages_update(page,&view);
        }
        if(page==3 && mode) {
            u16 words=mode*512;
            if(!hl_upload_fits(HL_UPLOAD_START,words) || !DMA_canQueue(DMA_VRAM,words) ||
                !DMA_queueDma(DMA_VRAM,upload,HL_UPLOAD_START,words,2))rejected++;
        }
    }
    return 0;
}
