#include "lab_pages.h"
#include "lab_ui.h"

static void label(const char *value,u16 y)
{hl_ui_text(value,4,y,PAL3);}
static void sprites(bool overflow)
{
    u16 index=0,count=overflow ? 21 : 20;
    u16 start=32+(256-((count-1)*12+8))/2;
    for(u16 i=0;i<count;i++,index++)
        VDP_setSpriteFull(index,start+i*12,104,SPRITE_SIZE(1,1),TILE_ATTR_FULL(PAL1,TRUE,FALSE,FALSE,3+i%6),index+1);
    count=overflow ? 11 : 10;
    start=32+(256-((count-1)*22+32))/2;
    for(u16 i=0;i<count;i++,index++)
        VDP_setSpriteFull(index,start+i*22,152,SPRITE_SIZE(4,1),TILE_ATTR_FULL(PAL1,TRUE,FALSE,FALSE,3),index+1);
    VDP_setSpriteLink(index-1,0);VDP_updateSprites(index,DMA_QUEUE);
}
static void pad(u16 base,u16 joy,const UiPort *p,u16 x,u16 y)
{
    const char *source=p->remote?(p->kind==UI_MOUSE?"USB MOUSE":"USB PAD"):
        (p->kind==UI_MOUSE?"PHYSICAL MOUSE":p->kind==UI_PAD?"PHYSICAL PAD":"NO DEVICE");
    hl_ui_field(source,base,12,14,PAL2);
    hl_ui_text("HELD",base,13,PAL3);hl_ui_hex(p->held,4,base+8,13,PAL2);
    hl_ui_text("HW",base,14,PAL3);hl_ui_hex(JOY_getJoypadType(joy),2,base+3,14,PAL3);
    hl_ui_text("RAW",base+6,14,PAL3);hl_ui_hex(JOY_readJoypad(joy),4,base+10,14,PAL3);
    hl_ui_text("DX",base,15,PAL3);hl_ui_hex((u16)p->dx,4,base+2,15,PAL2);
    hl_ui_text("DY",base+8,15,PAL3);hl_ui_hex((u16)p->dy,4,base+10,15,PAL2);
    static const u16 bits[]={BUTTON_UP,BUTTON_DOWN,BUTTON_LEFT,BUTTON_RIGHT,
        BUTTON_A,BUTTON_B,BUTTON_C,BUTTON_X,BUTTON_Y,BUTTON_Z,BUTTON_START,BUTTON_MODE};
    static const char *names[]={"A","B","C","X","Y","Z","S","M"};
    if(p->kind==UI_MOUSE) {
        hl_ui_field("L   R   M   S",base,16,14,PAL3);
        for(u16 i=0;i<4;i++)hl_ui_text(i==0?"L":i==1?"R":i==2?"M":"S",base+i*4,16,p->held&(1u<<i)?PAL2:PAL3);
        hl_ui_field("MOUSE BUTTONS",base,17,15,PAL3);
    } else {
        hl_ui_field("",base,16,14,PAL3);hl_ui_field("",base,17,15,PAL3);
        for(u16 i=0;i<4;i++)hl_ui_arrow(i,base+i*2,16,p->held&bits[i]?PAL2:PAL3);
        for(u16 i=4;i<12;i++)hl_ui_text(names[i-4],base+(i-4)*2,17,p->held&bits[i]?PAL2:PAL3);
    }
    hl_ui_text("XY",base,18,PAL3);hl_ui_hex(x,4,base+2,18,PAL2);hl_ui_hex(y,4,base+9,18,PAL2);
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
        hl_ui_text("PLAY",32,11,PAL3);
        label("A MUSIC",12);label("START PCM",14);
        hl_ui_card(17,7);label("EXTRA VRAM TRANSFER",18);
        label("B EXTRA DMA",19);label("MISSED LOOPS",21);label("REJECTED QUEUE",22);
        hl_ui_text("LISTEN FOR CRACKLE / DROPOUTS",2,25,PAL0);
    } else if(page==4) {
        hl_ui_panel(2,10,17,13);hl_ui_panel(21,10,17,13);
        hl_ui_text("PORT 1",3,11,PAL2);hl_ui_text("PORT 2",22,11,PAL2);
        hl_ui_text("POINTER",3,19,PAL3);hl_ui_text("POINTER",22,19,PAL3);
    } else if(page==5) {
        hl_ui_card(10,6);label("RECORD CRC",11);label("SEQUENCE",13);
        hl_ui_bar(v->valid ? 12 : 0,12,4,14);hl_ui_text("12 BYTE RECORD",20,14,PAL3);
        hl_ui_card(17,7);label("GUARDED WRITE",18);
        hl_ui_text("A READ / HOLD B + A WRITE",4,19,PAL2);
        label("OWNS SRAM BYTES 0-11",20);label("LAST WRITE",21);
        hl_ui_text("REBOOT + READ TO CHECK RETENTION",4,22,PAL2);
        hl_ui_text("READBACK IS NOT POWER-LOSS PROOF",2,25,PAL0);
    }
    /* Main publishes the dynamic fields once after opening the page. */
}
void hl_pages_update(u16 page,const HlPageView *v)
{
    if(page==3) {
        const bool playing=v->music && XGM2_isPlaying();
        hl_ui_field(playing ? "PLAYING" : v->music ? "IDLE" : "OFF",24,12,7,PAL2);
        hl_ui_field(v->pcm ? "LOOPING" : "OFF",24,14,7,PAL2);
        hl_ui_amp(v->vints/10,playing,32,12);
        hl_ui_field(v->mode==0 ? "0 BYTES" : v->mode==1 ? "1024 BYTES" : "2048 BYTES",24,19,10,PAL2);
        hl_ui_bar(v->mode*16,32,4,20);
        hl_ui_number(v->missed,25,21);hl_ui_number(v->rejected,25,22);
    } else if(page==4) {
        pad(3,JOY_1,&v->input[0],v->pointer_x[0],v->pointer_y[0]);
        pad(22,JOY_2,&v->input[1],v->pointer_x[1],v->pointer_y[1]);
        /* Clear each interior separately: never paint across the panel gutter
         * or its bottom corners. Both pointer rows remain inside the cards. */
        for(u16 i=0;i<2;i++) {
            const u16 x=3+i*19;
            hl_ui_field("",x,20,14,PAL3);hl_ui_field("",x,21,14,PAL3);
            hl_ui_text("+",x+v->pointer_x[i]/8,20+v->pointer_y[i]/8,i?PAL3:PAL2);
        }
        hl_ui_field(v->capture ? "PAGES LOCKED / A: UNLOCK" : "L/R: PAGES / A: LOCK TEST",2,24,36,PAL0);
        hl_ui_field(v->input[0].remote ? "USB P1 ACTIVE / PAD PAUSED" :
                    v->input[1].remote ? "USB P2 ACTIVE / P1 READY" : "PHYSICAL PAD READY",2,25,36,PAL0);
        hl_ui_field("C: CLEAR COUNTERS",2,26,36,PAL0);
    } else if(page==5) {
        hl_ui_field(v->valid ? "VALID" : "EMPTY/INVALID",22,11,13,PAL2);
        hl_ui_number(v->valid ? v->sequence : 0,24,13);
        hl_ui_field(v->write_status==1 ? "MATCHED" : v->write_status==2 ? "FAILED" : "NOT RUN",24,21,10,PAL2);
    }
}
