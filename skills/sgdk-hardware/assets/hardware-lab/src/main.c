#include <genesis.h>
#include "hardware.h"
#include "resources.h"
#include "font.h"
#include "readiness.h"
#include "lab_pages.h"
#include "lab_ui.h"
#include "usb_input_ed.h"

/* SGDK's reset entry retains application globals. Snapshot before reinitializing. */
static volatile u16 initialized_word=0x1234;
static volatile u16 zero_word;
static u16 upload[1024]; /* Retained, immutable while the DMA queue owns it. */
static u32 tiles[18*8];
static UsbInput remote_input;
static s16 mouse_x[2],mouse_y[2];
static bool mouse_seen[2];
static u16 pointer_x[2]={48,48},pointer_y[2]={8,8};
static void legacy_packet(void *context,const UiU8 *packet,UiU32 now)
{
    HlEdLink *link=context;u8 reply[HL_ED_PACKET_BYTES];
    for(u16 i=0;i<HL_ED_PACKET_BYTES;i++)
        if(hl_ed_receive(link,packet[i],now,reply))ui_ed_send(reply,HL_ED_PACKET_BYTES);
}
static void sample_input(HlEdLink *link,u32 now)
{
    UiPort physical[2]={{0},{0}};
    for(u16 i=0;i<2;i++) {
        u16 raw=JOY_readJoypad(i),type=JOY_getJoypadType(i);
        physical[i].kind=type==JOY_TYPE_MOUSE?UI_MOUSE:(type==JOY_TYPE_PAD3 || type==JOY_TYPE_PAD6?UI_PAD:UI_PHYSICAL);
        physical[i].held=raw;
        if(type==JOY_TYPE_MOUSE) {
            s16 x=JOY_readJoypadX(i),y=JOY_readJoypadY(i);
            /* Direct ports: accumulated mouse axes are screen-down in 2.11.
             * Read differences without replacing SDK polling or resetting axes. */
            if(mouse_seen[i]) {physical[i].dx=(s16)(x-mouse_x[i]);physical[i].dy=(s16)(y-mouse_y[i]);}
            mouse_x[i]=x;mouse_y[i]=y;mouse_seen[i]=TRUE;
            physical[i].held=(raw&BUTTON_LMB?1:0)|(raw&BUTTON_RMB?2:0)|(raw&BUTTON_MMB?4:0)|(raw&BUTTON_START?8:0);
        } else mouse_seen[i]=FALSE;
    }
    hl_ed_tick(link,now);ui_ed_poll(&remote_input,now,legacy_packet,link);
    ui_ed_finish(&remote_input,now,physical);
    for(u16 i=0;i<2;i++) {
        const UiPort *p=&remote_input.port[i];
        s32 x=pointer_x[i]+(p->kind==UI_MOUSE?p->dx:((p->held&BUTTON_RIGHT?1:0)-(p->held&BUTTON_LEFT?1:0)));
        s32 y=pointer_y[i]+(p->kind==UI_MOUSE?p->dy:((p->held&BUTTON_DOWN?1:0)-(p->held&BUTTON_UP?1:0)));
        pointer_x[i]=x<0?0:x>111?111:(u16)x;pointer_y[i]=y<0?0:y>15?15:(u16)y;
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
    u16 page=0,hz=SYS_isPAL() ? 50 : 60,refresh=0,mode=0;
    /* Signature plus read strobe; never touch the FIFO when absent. */
    const bool present=ui_ed_detect();
    HlEdLink link;hl_ed_init(&link,present,hz);ui_init(&remote_input,present,hz);
    for(u16 i=0;i<2;i++){mouse_seen[i]=FALSE;pointer_x[i]=48;pointer_y[i]=8;}
    u16 rejected=0,write_status=0;u32 logic=0,sequence=0;bool dirty=TRUE,music=FALSE,overflow=FALSE,valid=FALSE,reset_armed=FALSE,capture=FALSE,pcm=FALSE,first_screen=TRUE;
    HlClock clock;hl_clock_init(&clock,vtimer);
    while(TRUE) {
        SYS_doVBlankProcess();
        logic+=hl_clock_sample(&clock,vtimer,hz);
        sample_input(&link,vtimer);
        u16 held=remote_input.port[0].kind==UI_PAD?remote_input.port[0].held:0;
        u16 pressed=remote_input.port[0].kind==UI_PAD?remote_input.port[0].pressed:0;
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
            .missed=clock.missed,.sequence=sequence,.vints=clock.elapsed,
            .input={remote_input.port[0],remote_input.port[1]},
            .pointer_x={pointer_x[0],pointer_x[1]},.pointer_y={pointer_y[0],pointer_y[1]}};
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
