#include <assert.h>
#include <string.h>
#include "resources.h"
#include "fx_view.h"

static const u16 colors[16]={0};static const Palette palette={colors};
static const TileSet fx_tiles={0,1,0},lantern_tiles={0,4,0};
static const AnimationFrame fx_resource_frame={1,&fx_tiles,{{0,0,0,0,0,1}}},lantern_frame={1,&lantern_tiles,{{0,0,5,0,0,4}}};
static const AnimationFrame *fx_frames[8]={&fx_resource_frame,&fx_resource_frame,&fx_resource_frame,&fx_resource_frame,&fx_resource_frame,&fx_resource_frame,&fx_resource_frame,&fx_resource_frame};
static const AnimationFrame *lantern_frames[4]={&lantern_frame,&lantern_frame,&lantern_frame,&lantern_frame};
static const Animation fx_anim={8,fx_frames},lantern_anim={4,lantern_frames};
static const Animation *fx_anims[]={&fx_anim},*lantern_anims[]={&lantern_anim};
SpriteDefinition fx_image={8,8,1,1,1,fx_anims,&palette},lantern_image={16,16,4,1,1,lantern_anims,&palette};
typedef struct {s16 x,y;u8 size,link;u16 attr;} Entry;
static Entry sat[80];
static int fail_cache,loads,updates,resets,fills,texts;
static u16 submitted;
static s16 horizontal,vertical;
static bool drawing;

void VDP_setScreenWidth320(void){}
void VDP_setScreenHeight224(void){}
void VDP_setPlaneSize(u16 w,u16 h,bool clear){assert(w==64 && h==32 && clear);}
void VDP_setBackgroundColor(u16 c){assert(c==0);}
void PAL_setColor(u16 i,u16 c){(void)c;assert(!drawing && i<64);}
void PAL_setPalette(u16 p,const u16 *d,u16 m){assert(!drawing && p<4 && d && m==CPU);}
void VDP_loadTileData(const u32 *d,u16 i,u16 n,u16 m){
    assert(!drawing && i==16 && n==9 && m==CPU);
    for(u16 y=0;y<8;y++)assert(d[8*8+y]==0x11111111); /* HUD must be opaque. */
}
void VDP_setTileMapXY(u16 p,u16 t,u16 x,u16 y){assert(p==BG_B && (t&2047)>=16 && (t&2047)<24 && x<64 && y<32);}
void VDP_fillTileMapRect(u16 p,u16 t,u16 x,u16 y,u16 w,u16 h){
    assert(p==BG_A && (t&2047)==24 && !x && w==40 && ((y==0 && h==5)||(y==24 && h==4)));fills++;
}
void VDP_setTextPalette(u16 p){assert(p==PAL0);}
void VDP_drawText(const char *t,u16 x,u16 y){assert(x+strlen(t)<=40 && y<28);}
void VDP_drawTextEx(u16 p,const char *t,u16 a,u16 x,u16 y,u16 m){
    (void)a;assert(p==BG_A && m==DMA_QUEUE && x+strlen(t)<=40 && y<5);texts++;
}
void VDP_setHorizontalScrollVSync(u16 p,s16 value){assert(p==BG_B);horizontal=value;}
void VDP_setVerticalScrollVSync(u16 p,s16 value){assert(p==BG_B);vertical=value;}
u16 VDP_loadTileSet(const TileSet *tiles,u16 index,u16 method)
{
    assert(!drawing && method==CPU && index>=128 && index+tiles->numTile<=152);
    loads++;return loads!=fail_cache;
}
void VDP_resetSprites(void){assert(!drawing);memset(sat,0,sizeof(sat));resets++;}
void VDP_setSpriteFull(u16 i,s16 x,s16 y,u8 size,u16 attr,u8 link)
{
    assert(i<=64 && x>=0 && x<320 && y>=40 && y<192 && link<=64);
    sat[i]=(Entry){x,y,size,link,attr};
}
void VDP_updateSprites(u16 count,u16 method)
{
    assert(count<=65 && method==DMA_QUEUE);updates++;submitted=count;
    if(count)for(u16 i=0;i<count;i++)assert(sat[i].link==(i+1<count ? i+1 : 0));
}

static void reset(void){loads=updates=resets=fills=texts=0;drawing=false;}
int main(void)
{
    /* Every frame upload failure is handled; no per-particle heap exists. */
    for(int failure=1;failure<=12;failure++){
        reset();fail_cache=failure;
        assert(!fx_view_init());assert(loads==failure && updates==1 && !submitted);
        fx_view_end();assert(updates==1);
    }
    reset();fail_cache=0;
    fx_image.maxNumSprite=2;assert(!fx_view_init() && !loads);fx_image.maxNumSprite=1;
    assert(fx_view_init() && loads==12 && fills==4);
    assert(!fx_view_init());
    drawing=true;
    FxLab s;FxRender r;fx_init(&s,1);s.density=3;
    for(u16 frame=0;frame<500;frame++){
        fx_frame(&s,(frame%17==0 ? FX_A : 0)|(frame<160 ? FX_RIGHT : FX_LEFT),60);
        fx_select(&s,&r,64);fx_view_draw(&s,&r);fx_view_hud(&s,&r,60,60);
        assert(horizontal==-r.camera_x && vertical==r.camera_y);
        assert(submitted==r.count+1 && loads==12);
        assert((sat[0].attr&2047)==136+((s.ticks/8)&3)*4 && sat[0].size==5);
        for(u16 i=0;i<r.count;i++){
            assert((sat[i+1].attr&2047)==128+r.items[i].frame && !sat[i+1].size);
            assert(sat[i+1].x==r.items[i].x && sat[i+1].y==r.items[i].y);
        }
    }
    fx_clear(&s);fx_select(&s,&r,0);fx_view_draw(&s,&r);assert(submitted==1 && !sat[0].link);
    int before=texts;fx_view_hud(&s,&r,60,60);int after=texts;
    fx_view_hud(&s,&r,60,60);assert(texts==after && after>=before);
    drawing=false;fx_view_end();fx_view_end();assert(!submitted);
    return 0;
}
