#include "lab_ui.h"
#include "hardware.h"
#include "readiness_art.h"
#include "lab_art.h"
#include "build_id.h"
#define READY_TILE_START 19
#define LAB_TILE_START (READY_TILE_START+HL_READY_TILE_COUNT)
_Static_assert((LAB_TILE_START+HL_LAB_TILE_COUNT)*32<=HL_UPLOAD_START,
               "Diagnostic art must stay below the private DMA workload");
static const u16 colors[16]={0x200,0x200,0x422,0x644,0x8e4,0xea6,0xa88,0xeee,
    0x4ae,0x44e,0x422,0x644,0xa88,0x8e4,0xea6,0xeee};
static const u16 test_colors[16]={0x000,0x222,0x00e,0x0e0,0xe00,0x0ee,0xee0,0xe0e,
    0x444,0x666,0x888,0xaaa,0xccc,0xeee,0x08e,0xeee};
void hl_ui_load(void)
{
    VDP_loadTileData(hl_ready_tiles,READY_TILE_START,HL_READY_TILE_COUNT,CPU);
    VDP_loadTileData(hl_lab_tiles,LAB_TILE_START,HL_LAB_TILE_COUNT,CPU);
}
void hl_ui_text(const char *value,u16 x,u16 y,u16 pal)
{
    VDP_drawTextEx(BG_A,value,TILE_ATTR(pal,TRUE,FALSE,FALSE),x,y,DMA_QUEUE_COPY);
}
void hl_ui_hex(u32 value,u16 digits,u16 x,u16 y,u16 pal)
{
    char text[32];intToHex(value,text,digits);hl_ui_text(text,x,y,pal);
}
void hl_ui_field(const char *value,u16 x,u16 y,u16 width,u16 pal)
{
    char text[HL_FIELD_WIDTH+1];u16 i=0;
    if(width>HL_FIELD_WIDTH)return;
    while(i<width && value[i]){text[i]=value[i];i++;}
    while(i<width)text[i++]=' ';
    text[width]=0;hl_ui_text(text,x,y,pal);
}
void hl_ui_number(u32 value,u16 x,u16 y)
{
    char text[32];uintToStr(value,text,1);hl_ui_field(text,x,y,10,PAL2);
}
static void tiles(u16 first,u16 width,u16 height,u16 x,u16 y)
{
    for(u16 row=0;row<height;row++)for(u16 col=0;col<width;col++)
        VDP_setTileMapXY(BG_B,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,
            first+row*width+col),x+col,y+row);
}
void hl_ui_art(u16 offset,u16 width,u16 height,u16 x,u16 y)
{tiles(READY_TILE_START+offset,width,height,x,y);}
void hl_ui_solid(u16 color,u16 x,u16 y,u16 width,u16 height)
{
    for(u16 row=0;row<height;row++)for(u16 col=0;col<width;col++)
        VDP_setTileMapXY(BG_B,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,1+color),x+col,y+row);
}
void hl_ui_card(u16 y,u16 height)
{
    hl_ui_solid(2,2,y,36,height);
    hl_ui_art(HL_READY_CORNER_OFFSET,1,1,2,y);
    hl_ui_art(HL_READY_CORNER_OFFSET+1,1,1,37,y);
    hl_ui_art(HL_READY_CORNER_OFFSET+2,1,1,2,y+height-1);
    hl_ui_art(HL_READY_CORNER_OFFSET+3,1,1,37,y+height-1);
}
void hl_ui_meter(u16 head,u16 x,u16 y)
{
    for(u16 i=0;i<8;i++) {
        u16 state=i==head ? 2 : i==((head+7)%8) ? 1 : 0;
        hl_ui_art(HL_READY_METER_OFFSET+state,1,1,x+i,y);
    }
}
void hl_ui_bar(u16 active,u16 cells,u16 x,u16 y)
{
    for(u16 i=0;i<cells;i++)hl_ui_art(HL_READY_METER_OFFSET+(i<active ? 2 : 0),1,1,x+i,y);
}
void hl_ui_arrow(u16 direction,u16 x,u16 y,u16 pal)
{
    VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(pal,TRUE,FALSE,FALSE,
        LAB_TILE_START+HL_LAB_ARROW_OFFSET+direction),x,y);
}
void hl_ui_amp(u16 phase,bool active,u16 x,u16 y)
{
    static const u16 heights[8]={5,11,18,23,16,9,20,4};
    for(u16 col=0;col<4;col++) {
        u16 height=active ? heights[(phase+col*3)%8] : 0;
        for(u16 row=0;row<3;row++) {
            u16 bottom=(2-row)*8;
            u16 fill=height<=bottom ? 0 : height-bottom>=8 ? 8 : height-bottom;
            VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(PAL0,TRUE,FALSE,FALSE,
                LAB_TILE_START+HL_LAB_AMP_OFFSET+fill),x+col,y+row);
        }
    }
}
void hl_ui_begin(u16 page)
{
    u16 font[16];PAL_setPalette(PAL0,colors,CPU);
    for(u16 i=0;i<16;i++)font[i]=colors[i];
    font[1]=colors[2];
    if(page==0) {
        PAL_setPalette(PAL1,font,CPU);font[15]=colors[6];PAL_setPalette(PAL2,font,CPU);
        font[15]=colors[4];PAL_setPalette(PAL3,font,CPU);
    } else {
        PAL_setPalette(PAL1,test_colors,CPU);PAL_setPalette(PAL2,font,CPU);
        font[15]=colors[6];PAL_setPalette(PAL3,font,CPU);
    }
    hl_ui_text("SGDK",2,1,PAL0);
    hl_ui_text("BUILD " HL_BUILD_ID,38-(sizeof("BUILD " HL_BUILD_ID)-1),1,PAL0);
    if(page==0) {
        hl_ui_art(0,4,4,3,3);hl_ui_art(HL_READY_TITLE_OFFSET,18,2,9,3);
        hl_ui_text("ROM RUNNING",9,6,PAL0);
    } else {
        tiles(LAB_TILE_START+(page-1)*16,4,4,3,3);
        tiles(LAB_TILE_START+hl_lab_title_offsets[page-1],hl_lab_title_widths[page-1],2,9,3);
        static const char *captions[]={"01 / VIDEO SIGNAL","02 / SCANLINE LIMITS","03 / BUS CONTENTION","04 / CONTROLLER PORTS","05 / CARTRIDGE SRAM"};
        hl_ui_text(captions[page-1],9,6,PAL0);
    }
    hl_ui_solid(3,2,8,36,1);hl_ui_solid(4,2,8,7,1);
}
