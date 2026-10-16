#include <assert.h>
#include <string.h>
#include <genesis.h>
#include "catalog.h"
#include "view.h"
#include "prefs.h"

const TileSet language_font={0,LOC_FONT_TILES,0},ferry_tiles={0,48,0};
static u8 storage[6];
static u16 access,writes,cutoff,draws;
static u16 copied[40*28];
void VDP_setScreenWidth320(void){}
void VDP_setScreenHeight224(void){}
void VDP_setPlaneSize(u16 w,u16 h,bool clear){assert(w==64 && h==32 && clear);}
void VDP_setBackgroundColor(u16 color){assert(color==0);}
void PAL_setColor(u16 index,u16 color){(void)color;assert(index<48);}
void PAL_setPalette(u16 palette,const u16 *colors,u16 method){assert(palette==PAL3 && colors && method==CPU);}
void VDP_loadTileData(const u32 *data,u16 index,u16 count,u16 method){assert(data && index==16 && count==2 && method==CPU);}
u16 VDP_loadTileSet(const TileSet *data,u16 index,u16 method){assert(data && method==CPU && (index==32 || index==LOC_FONT_BASE));return TRUE;}
void VDP_setTileMapDataRect(u16 plane,const u16 *data,u16 x,u16 y,u16 w,u16 h,u16 wm,u16 method)
{
    assert(plane==BG_A && data && x==0 && y==0 && w==40 && h==28 && wm==40 && method==DMA_QUEUE_COPY);
    memcpy(copied,data,sizeof(copied));draws++;
    for(u16 i=0;i<40*28;i++) {
        u16 tile=data[i]&2047;
        assert((tile>=LOC_FONT_BASE && tile<LOC_FONT_BASE+LOC_FONT_TILES) ||
               (tile>=16 && tile<18) || (tile>=32 && tile<80));
    }
}
void SRAM_enableRO(void){assert(!access);access=1;}
void SRAM_enable(void){assert(!access);access=2;}
void SRAM_disable(void){assert(access);access=0;}
u8 SRAM_readByte(u32 offset){assert(access==1 && offset<6);return storage[offset];}
void SRAM_writeByte(u32 offset,u8 value){assert(access==2 && offset<6);if(writes++<cutoff)storage[offset]=value;}

int main(void)
{
    assert(view_init());
    LanguageLab lab;lab_init(&lab,0);
    for(u8 language=0;language<LOC_LANGUAGE_COUNT;language++)for(u8 screen=0;screen<4;screen++) {
        lab.language=language;lab.screen=screen;
        for(u8 quantity=0;quantity<5;quantity++) {
            lab.quantity=quantity;
            for(u16 i=0;i<8;i++)lab.name[i]=loc_name_chars[26];lab.name[8]=0;
            assert(view_draw(&lab));
            if(screen==HOME)assert((copied[9*40+5]&2047)>=LOC_FONT_BASE);
        }
    }
    assert(draws==80);
    u8 old[6];assert(loc_pref_encode(1,old));
    for(cutoff=0;cutoff<=7;cutoff++) {
        memcpy(storage,old,6);writes=0;prefs_save(2);assert(!access && writes==7);
        u8 expected=cutoff==0 ? 1 : cutoff==7 ? 2 : 0;
        assert(prefs_load()==expected && !access);
    }
    writes=0;prefs_save(255);assert(!access && !writes);
    return 0;
}
