#include <assert.h>
#include <string.h>
#include "view.h"
#include "resources.h"
static u16 colors[64],map[64],plane[40*28];
static const TileMap tilemap={8,8,map};
const TileSet art_font_ivory={96,0},art_font_gold={96,0},art_font_title={104,0};
static const Palette palette={colors};
#define IMAGE(name) const Image name={&palette,&art_font_gold,&tilemap}
IMAGE(art_panel);IMAGE(art_town);IMAGE(art_dungeon);IMAGE(art_forest);IMAGE(art_coast);
IMAGE(art_title);IMAGE(art_world);IMAGE(art_intro_sky);IMAGE(art_intro_front);IMAGE(art_icon_sword);
const SpriteDefinition art_hero_idle={.palette=&palette},art_travel_pin={0},art_title_mask0={0},art_title_mask1={0};
static unsigned palettes,uploads,blanks,queued;
void VDP_setTileMapDataRow(VDPPlane p,const u16 *data,u16 row,u16 x,u16 w,TransferMethod m)
{assert(p==BG_A&&row<28&&x==0&&w==40);memcpy(plane+row*40,data,w*2);if(m==DMA_QUEUE_COPY)queued++;}
void VDP_loadTileSet(const TileSet *s,u16 b,TransferMethod m){(void)s;(void)b;(void)m;uploads++;}
void PAL_setPalette(u16 p,const u16 *d,TransferMethod m){(void)p;(void)d;(void)m;palettes++;}
void PAL_setColors(u16 p,const u16 *d,u16 n,TransferMethod m){(void)p;(void)d;(void)n;(void)m;palettes++;}
void VDP_setEnable(bool enabled){if(!enabled)blanks++;}
void VDP_clearPlane(VDPPlane p,bool wait){(void)p;(void)wait;}
void VDP_setTileMapEx(VDPPlane p,const TileMap *t,u16 b,u16 x,u16 y,u16 sx,u16 sy,u16 w,u16 h,TransferMethod m)
{(void)p;(void)t;(void)b;(void)x;(void)y;(void)sx;(void)sy;(void)w;(void)h;(void)m;}
void VDP_loadTileData(const u32 *d,u16 b,u16 n,TransferMethod m){(void)d;(void)b;(void)n;(void)m;uploads++;}
void VDP_setHorizontalScroll(VDPPlane p,s16 x){(void)p;(void)x;}
void VDP_setHilightShadow(bool b){(void)b;}
void VDP_updateSprites(u16 n,TransferMethod m){(void)n;(void)m;}
void VDP_setSpriteFull(u16 n,s16 x,s16 y,u16 z,u16 a,u16 l){(void)n;(void)x;(void)y;(void)z;(void)a;(void)l;}
void VDP_setSpriteLink(u16 n,u16 l){(void)n;(void)l;}
void VDP_setPlaneSize(u16 w,u16 h,bool b){(void)w;(void)h;(void)b;}
void VDP_setWindowHPos(bool b,u16 n){(void)b;(void)n;}
void VDP_setWindowVPos(bool b,u16 n){(void)b;(void)n;}
void VDP_setBackgroundColor(u16 n){(void)n;}
void VDP_clearSprites(void){}
void VDP_setScreenWidth320(void){}
void VDP_setScreenHeight224(void){}
void SYS_disableInts(void){}
void SYS_enableInts(void){}
void actors_reset(void){}
void actors_frame(const Game *g){(void)g;}
const char *zone_name(uint8_t z){(void)z;return "Town";}
void ui_tick(const Game *g){(void)g;}
void ui_draw(const Game *g)
{ui_text("Name",3,2,false);ui_text(">",5+g->keyboard,12,true);ui_image(&art_icon_sword,PAL1,3,15);}
int main(void)
{
    Game g={0};g.screen=NAMING;g.dirty=true;view_frame(&g);
    unsigned p=palettes,u=uploads,b=blanks;u16 label=plane[2*40+3],image=plane[15*40+3];
    assert(label!=0&&image!=0&&queued==0);
    for(u16 i=1;i<8;i++)
    {
        g.keyboard=i;g.ui_dirty=true;queued=0;view_frame(&g);
        assert(palettes==p&&uploads==u&&blanks==b&&queued>0);
        assert(plane[2*40+3]==label&&plane[15*40+3]==image);
        assert(plane[12*40+5+i]!=plane[12*40+4+i]);
    }
    queued=0;view_frame(&g);assert(queued==0);
    return 0;
}
