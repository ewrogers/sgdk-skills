#include "view.h"
#include <assert.h>
#include <stdio.h>
Visual visuals[ART_COUNT];
u16 banner_tile=900;
static u32 font_pixels[96*8],uploaded[64*8];
const TileSet gradient_font={0,96,font_pixels};
static u16 entries,loads,submitted;
static struct {s16 x,y;u8 size;u16 tile,link;} sat[80];
void VDP_setSpriteFull(u16 at,s16 x,s16 y,u8 size,u16 tile,u16 link)
{assert(at<80);sat[at].x=x;sat[at].y=y;sat[at].size=size;sat[at].tile=tile;sat[at].link=link;entries++;}
void VDP_setSpriteLink(u16 at,u16 link){assert(at<80);sat[at].link=link;}
void VDP_updateSprites(u16 count,u16 method){assert(method==DMA_QUEUE);submitted=count;}
void VDP_loadTileData(const u32 *data,u16 tile,u16 count,u16 method)
{assert(method==DMA_QUEUE_COPY);assert(tile>=banner_tile && tile+count<=banner_tile+BANNER_LINE_TILES*2);memcpy(uploaded+(tile-banner_tile)*8,data,count*32);loads++;}
int main(void)
{
    const char *name="THE SAFFRON SKY BAZAAR";
    for(u16 i=0;i<96*8;i++)font_pixels[i]=i;
    sprites_invalidate_text();sprites_begin();sprite_text(name,68,104);sprite_text("A / START TO FLY",100,128);sprites_end();
    assert(entries==10 && submitted==10 && loads==2);
    /* A 22-letter stage banner must fit on one scanline: six pieces, not 22. */
    assert(sat[0].x==68 && sat[0].y==104 && sat[0].size==SPRITE_SIZE(4,1));
    assert(sat[5].x==228 && sat[5].size==SPRITE_SIZE(2,1));
    assert((sat[5].tile&0x7ff)==banner_tile+20 && sat[9].link==0);
    for(u16 i=0;name[i];i++)for(u16 y=0;y<8;y++)assert(uploaded[i*8+y]==(u32)((name[i]-32)*8+y));
    entries=0;sprites_begin();sprite_text(name,68,104);sprite_text("A / START TO FLY",100,128);sprites_end();assert(loads==2);
    sprites_invalidate_text();sprites_begin();sprite_text(name,68,104);assert(loads==3);
    const FrameVDPSprite quarter={.size=SPRITE_SIZE(2,2),.numTile=4};
    const AnimationFrame frame={.numSprite=1,.frameVDPSprites=&quarter};
    const AnimationFrame *frames[15];for(u16 i=0;i<15;i++)frames[i]=&frame;
    const Animation animation={.numFrame=15,.frames=frames};
    const Animation *animations[]={&animation};
    const SpriteDefinition definition={.w=16,.h=16,.animations=animations};
    visuals[ART_EFFECT].definition=&definition;visuals[ART_EFFECT].tiles[11]=700;
    entries=0;sprites_begin();sprite_shield(100*UNIT,100*UNIT,2);sprites_end();assert(submitted==6);
    assert(sat[0].x==84 && sat[0].y==84 && sat[3].x==100 && sat[3].y==100);
    assert((sat[1].tile&0x0800) && (sat[2].tile&0x1000) && (sat[3].tile&0x1800)==0x1800);
    entries=0;sprites_begin();sprite_shield(100*UNIT,100*UNIT,1);sprites_end();assert(submitted==5);
    puts("banner");return 0;
}
