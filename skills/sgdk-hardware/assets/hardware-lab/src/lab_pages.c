#include "lab_pages.h"
#include "lab_ui.h"

static void label(const char *value,u16 y)
{hl_ui_text(value,4,y,PAL3);}
static void sprites(bool overflow)
{
    u16 index=0,count=overflow ? 21 : 20;
    for(u16 i=0;i<count;i++,index++)
        VDP_setSpriteFull(index,8+i*14,104,SPRITE_SIZE(1,1),TILE_ATTR_FULL(PAL1,TRUE,FALSE,FALSE,3+i%6),index+1);
    count=overflow ? 11 : 10;
    for(u16 i=0;i<count;i++,index++)
        VDP_setSpriteFull(index,4+i*28,152,SPRITE_SIZE(4,1),TILE_ATTR_FULL(PAL1,TRUE,FALSE,FALSE,3),index+1);
    VDP_setSpriteLink(index-1,0);VDP_updateSprites(index,DMA_QUEUE);
}
static void pad(u16 base,u16 port,u16 joy,u16 held)
{
    hl_ui_text("PORT",base,12,PAL3);hl_ui_hex(JOY_getPortType(port),4,base+8,12,PAL2);
    hl_ui_text("DEVICE",base,13,PAL3);hl_ui_hex(JOY_getJoypadType(joy),4,base+8,13,PAL2);
    hl_ui_text("HELD",base,14,PAL3);hl_ui_hex(held,4,base+8,14,PAL2);
    static const u16 bits[]={BUTTON_UP,BUTTON_DOWN,BUTTON_LEFT,BUTTON_RIGHT,
        BUTTON_A,BUTTON_B,BUTTON_C,BUTTON_X,BUTTON_Y,BUTTON_Z,BUTTON_START,BUTTON_MODE};
    static const char *names[]={"U","D","L","R","A","B","C","X","Y","Z","S","M"};
    for(u16 i=0;i<12;i++)hl_ui_text(names[i],base+(i<4 ? i : i-4)*2,i<4 ? 16 : 17,
                                   held&bits[i] ? PAL2 : PAL3);
}
void hl_pages_open(u16 page,const HlPageView *v)
{
    hl_ui_begin(page);
    hl_ui_text("L/R PAGES  C ZERO",2,26,PAL0);
    if(page==1) {
        hl_ui_card(10,13);label("RGB / NEUTRAL RAMP",11);
        for(u16 y=13;y<16;y++)for(u16 x=4;x<36;x++)
            VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(PAL1,FALSE,FALSE,FALSE,1+((x-4)/2+y-13)%16),x,y);
        label("1PX CHECKER / STRIPES",17);
        for(u16 y=19;y<22;y++)for(u16 x=4;x<36;x++)
            VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(PAL1,FALSE,FALSE,FALSE,17+x%2),x,y);
        for(u16 x=0;x<40;x+=39)for(u16 y=0;y<28;y+=27)
            VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(PAL1,FALSE,FALSE,FALSE,6),x,y);
        hl_ui_text("CHECK CROP / SHIMMER / BANDING",2,24,PAL0);
        hl_ui_text("EDGES: FOUR CORNER MARKS",2,25,PAL0);
    } else if(page==2) {
        hl_ui_card(10,13);
        hl_ui_text(v->overflow ? "OVER LIMIT / EXPECT DROPOUT" : "AT LIMIT / EXPECT ALL PIECES",4,11,PAL2);
        label(v->overflow ? "21 X 8PX / PIECE LIMIT" : "20 X 8PX / PIECE LIMIT",12);
        label(v->overflow ? "11 X 32PX / PIXEL LIMIT" : "10 X 32PX / PIXEL LIMIT",18);
        sprites(v->overflow);
        hl_ui_text("A: TOGGLE AT / OVER LIMIT",2,24,PAL0);
        hl_ui_text("PIECES / PIXELS TESTED SEPARATELY",2,25,PAL0);
    } else if(page==3) {
        hl_ui_card(10,6);label("SOUND ENGINE / XGM2",11);
        label("A MUSIC",12);label("START PCM",14);
        hl_ui_card(17,7);label("EXTRA VRAM TRANSFER",18);
        label("B EXTRA DMA",19);label("MISSED LOOPS",21);label("REJECTED QUEUE",22);
        hl_ui_text("LISTEN FOR CRACKLE / DROPOUTS",2,25,PAL0);
    } else if(page==4) {
        hl_ui_card(10,9);hl_ui_text("PORT 1",4,11,PAL2);hl_ui_text("PORT 2",22,11,PAL2);
        hl_ui_card(20,4);
        hl_ui_text("SGDK TYPE / HELD BITS",2,25,PAL0);
    } else if(page==5) {
        hl_ui_card(10,6);label("RECORD CRC",11);label("SEQUENCE",13);
        hl_ui_bar(v->valid ? 12 : 0,12,4,14);hl_ui_text("12 BYTE RECORD",20,14,PAL3);
        hl_ui_card(17,7);label("GUARDED WRITE",18);
        hl_ui_text("A READ / HOLD B + A WRITE",4,19,PAL2);
        label("OWNS SRAM BYTES 0-11",20);label("LAST WRITE",21);
        hl_ui_text("REBOOT + READ TO CHECK RETENTION",4,22,PAL2);
        hl_ui_text("READBACK IS NOT POWER-LOSS PROOF",2,25,PAL0);
    }
    hl_pages_update(page,v);
}
void hl_pages_update(u16 page,const HlPageView *v)
{
    if(page==3) {
        hl_ui_field(v->music ? "PLAYING" : "OFF",24,12,10,PAL2);
        hl_ui_field(v->pcm ? "LOOPING" : "OFF",24,14,10,PAL2);
        hl_ui_field(v->mode==0 ? "0 BYTES" : v->mode==1 ? "1024 BYTES" : "2048 BYTES",24,19,10,PAL2);
        hl_ui_bar(v->mode*16,32,4,20);
        hl_ui_number(v->missed,25,21);hl_ui_number(v->rejected,25,22);
    } else if(page==4) {
        pad(4,PORT_1,JOY_1,v->held);pad(22,PORT_2,JOY_2,JOY_readJoypad(JOY_2));
        hl_ui_field(v->capture ? "DPAD PAGE NAV: LOCKED" : "DPAD PAGE NAV: ENABLED",4,21,32,PAL2);
        hl_ui_field(v->capture ? "A UNLOCKS PAGE FOR NAVIGATION" : "A LOCKS PAGE FOR INPUT TEST",4,22,32,PAL3);
        hl_ui_field(v->capture ? "A UNLOCK  C ZERO" : "L/R PAGES  A LOCK  C ZERO",2,26,36,PAL0);
    } else if(page==5) {
        hl_ui_field(v->valid ? "VALID" : "EMPTY/INVALID",22,11,13,PAL2);
        hl_ui_number(v->valid ? v->sequence : 0,24,13);
        hl_ui_field(v->write_status==1 ? "MATCHED" : v->write_status==2 ? "FAILED" : "NOT RUN",24,21,10,PAL2);
    }
}
