#include <genesis.h>
#include "resources.h"
#include "catalog.h"
#include "view.h"

enum { PANEL_BASE=16,FERRY_BASE=32 };
static u16 canvas[40*28];

static void cells(const u8 *text,u16 length,u16 x,u16 y,u16 palette)
{
    for (u16 i=0;i<length;i++) canvas[y*40+x+i]=TILE_ATTR_FULL(palette,TRUE,FALSE,FALSE,LOC_FONT_BASE+text[i]);
}

static bool message(const LanguageLab *s,u16 id,u16 x,u16 y,u16 palette,u16 page)
{
    u8 text[LOC_TEXT_MAX]; LocLayout layout;
    LocBox box=loc_boxes[id];
    if (!loc_expand(s->language,id,s->name,lab_quantity(s),text,sizeof(text)) ||
        !loc_layout(text,box,&layout) || x+box.width>40 || y+box.rows>28) return false;
    for (u16 row=0;row<box.rows;row++) {
        u16 source=page*box.rows+row;
        if (source<layout.lines) cells(layout.cells[source],box.width,x,y+row,palette);
    }
    return true;
}

static void label(const char *ascii,u16 x,u16 y,u16 palette)
{
    u8 text[40];u16 n=0;
    while (ascii[n] && n<40-x) { text[n]=(u8)(ascii[n]-31);n++; }
    cells(text,n,x,y,palette);
}

static void panel(u16 x,u16 y,u16 width,u16 height)
{
    for (u16 i=0;i<width;i++) {
        canvas[y*40+x+i]=TILE_ATTR_FULL(PAL2,TRUE,FALSE,FALSE,PANEL_BASE);
        canvas[(y+height-1)*40+x+i]=TILE_ATTR_FULL(PAL2,TRUE,TRUE,FALSE,PANEL_BASE);
    }
    for (u16 j=0;j<height;j++) {
        canvas[(y+j)*40+x]=TILE_ATTR_FULL(PAL2,TRUE,FALSE,FALSE,PANEL_BASE+1);
        canvas[(y+j)*40+x+width-1]=TILE_ATTR_FULL(PAL2,TRUE,FALSE,TRUE,PANEL_BASE+1);
    }
}

bool view_init(void)
{
    static const u32 border[16]={0,0,0,0xFFFFFFFF,0,0,0,0,
                                0x000F0000,0x000F0000,0x000F0000,0x000F0000,
                                0x000F0000,0x000F0000,0x000F0000,0x000F0000};
    static const u16 ferry_palette[16]={0x0000,0x0420,0x0640,0x0862,0x0A84,0x0424,0x0448,0x048C,
                                       0x08AA,0x08CE,0x0CEE,0,0,0,0,0};
    VDP_setScreenWidth320();VDP_setScreenHeight224();
    VDP_setPlaneSize(64,32,TRUE);
    VDP_setBackgroundColor(0);
    PAL_setColor(0,RGB24_TO_VDPCOLOR(0x0C1424));
    PAL_setColor(15,RGB24_TO_VDPCOLOR(0xFCFCD8));
    PAL_setColor(31,RGB24_TO_VDPCOLOR(0xFCD890));
    PAL_setColor(47,RGB24_TO_VDPCOLOR(0x6C90B4));
    PAL_setPalette(PAL3,ferry_palette,CPU);
    VDP_loadTileData(border,PANEL_BASE,2,CPU);
    return language_font.numTile==LOC_FONT_TILES && ferry_tiles.numTile==48 &&
           LOC_FONT_BASE+LOC_FONT_TILES<=(u16)TILE_FONT_INDEX &&
           VDP_loadTileSet(&language_font,LOC_FONT_BASE,CPU) &&
           VDP_loadTileSet(&ferry_tiles,FERRY_BASE,CPU);
}

bool view_draw(const LanguageLab *s)
{
    for (u16 i=0;i<40*28;i++) canvas[i]=TILE_ATTR_FULL(PAL0,TRUE,FALSE,FALSE,LOC_FONT_BASE);
    if (!message(s,TXT_TITLE,2,1,PAL1,0) || !message(s,TXT_LANGUAGE,3,4,PAL0,0)) return false;
    label("<",1,4,PAL2);label(">",38,4,PAL2);
    panel(1,6,38,17);
    u16 hint;
    if (s->screen==HOME) {
        static const u16 choices[]={TXT_TALK,TXT_INVENTORY,TXT_NAME};
        for (u16 i=0;i<3;i++) {
            if (!message(s,choices[i],5,9+i*3,i==s->selected ? PAL1 : PAL0,0)) return false;
            label(i==s->selected ? ">" : " ",3,9+i*3,PAL1);
        }
        for (u16 y=0;y<6;y++) for (u16 x=0;x<8;x++)
            canvas[(9+y)*40+28+x]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,FERRY_BASE+y*8+x);
        if (!message(s,TXT_ACCENT_SAMPLE,2,19,PAL0,0)) return false;
        hint=TXT_HOME_HINT;
    } else if (s->screen==TALK) {
        if (!message(s,TXT_DIALOGUE,2,10,PAL0,s->page)) return false;
        u8 text[LOC_TEXT_MAX];LocLayout layout;
        if (!loc_expand(s->language,TXT_DIALOGUE,s->name,0,text,sizeof(text)) ||
            !loc_layout(text,loc_boxes[TXT_DIALOGUE],&layout)) return false;
        char buffer[30];
        u16 pages=(layout.lines+loc_boxes[TXT_DIALOGUE].rows-1)/loc_boxes[TXT_DIALOGUE].rows;
        sprintf(buffer,"%u / %u %s",s->page+1,pages,s->page+1<pages ? ">" : "");
        label(buffer,30,20,PAL1);
        hint=TXT_DIALOGUE_HINT;
    } else if (s->screen==BAG) {
        if (!message(s,TXT_BAG_TITLE,2,8,PAL1,0) || !message(s,TXT_ITEM,2,11,PAL0,0) ||
            !message(s,lab_quantity(s)==1 ? TXT_FOUND_ONE : TXT_FOUND_MANY,2,16,PAL0,0)) return false;
        u8 number[7];
        if (!loc_number(s->language,lab_quantity(s),number,sizeof(number))) return false;
        u16 n=0;while (number[n]) n++;
        cells(number,n,30,13,PAL1);
        label("*",3,13,PAL1);
        hint=TXT_BAG_HINT;
    } else {
        if (!message(s,TXT_NAME_PROMPT,2,7,PAL1,0)) return false;
        for (u16 i=0;i<32;i++) {
            u16 x=5+(i%8)*4,y=12+(i/8)*2;
            cells(&loc_name_chars[i],1,x,y,i==s->cursor ? PAL1 : PAL0);
            if (i==s->cursor) {label("[",x-1,y,PAL1);label("]",x+1,y,PAL1);}
        }
        u16 n=0;while(s->name[n] && n<LOC_NAME_MAX)n++;
        cells(s->name,n,5,9,PAL1);
        label("_",5+n,9,PAL2);
        hint=TXT_NAME_HINT;
    }
    if (!message(s,hint,2,24,PAL0,0)) return false;
    char buffer[40];
    sprintf(buffer,"FONT %u/%u TILES  %u BYTES",LOC_FONT_TILES,LOC_FONT_BUDGET,LOC_FONT_TILES*32);
    label(buffer,2,27,PAL2);
    /* Queue copies each row. canvas may be rebuilt after this call. */
    VDP_setTileMapDataRect(BG_A,canvas,0,0,40,28,40,DMA_QUEUE_COPY);
    return true;
}
