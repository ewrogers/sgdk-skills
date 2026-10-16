#include <genesis.h>
#include "resources.h"
#include "fx_view.h"

enum { CACHE_BASE=128, CACHE_END=152, BACKGROUND_BASE=16 };
static u16 particle_tiles[8],lantern_tiles[4];
static bool initialized;
static char previous_counts[40],previous_status[40],previous_fps[6];

void fx_view_end(void)
{
    if (!initialized) return;
    VDP_updateSprites(0,DMA_QUEUE);
    initialized=false;
}

static bool cache(const SpriteDefinition *def,u16 base,u16 frames,u16 *dest,u16 expected_tiles)
{
    if (def->numAnimation!=1 || def->animations[0]->numFrame!=frames) return false;
    u16 total=0;
    for (u16 i=0;i<frames;i++) {
        const AnimationFrame *frame=def->animations[0]->frames[i];
        if ((frame->numSprite&0x7F)!=1 || frame->tileset->compression!=COMPRESSION_NONE) return false;
        total+=frame->tileset->numTile;
    }
    if (total!=expected_tiles || base+total>CACHE_END) return false;
    u16 index=base;
    for (u16 i=0;i<frames;i++) {
        const AnimationFrame *frame=def->animations[0]->frames[i];
        const FrameVDPSprite *piece=&frame->frameVDPSprites[0];
        if (piece->offsetX || piece->offsetY || piece->size!=SPRITE_SIZE(def->w/8,def->h/8)) return false;
        dest[i]=index;
        if (!VDP_loadTileSet(frame->tileset,index,CPU)) return false;
        index+=frame->tileset->numTile;
    }
    return index==base+expected_tiles;
}

bool fx_view_init(void)
{
    if (initialized) return false;
    VDP_setScreenWidth320(); VDP_setScreenHeight224();
    VDP_setPlaneSize(64,32,TRUE);
    VDP_setBackgroundColor(0);
    PAL_setColor(0,RGB24_TO_VDPCOLOR(0x101024));
    PAL_setColor(1,RGB24_TO_VDPCOLOR(0x101024));
    PAL_setColor(15,RGB24_TO_VDPCOLOR(0xFCFCD8));
    PAL_setPalette(PAL1,fx_image.palette->data,CPU);
    PAL_setPalette(PAL2,lantern_image.palette->data,CPU);
    static const u16 landscape[16]={0x0202,0x0202,0x0402,0x0424,0x0644,0x0866,0x0A88,0x0CAA,
        0x0246,0x0468,0x068A,0x08AC,0x0ACE,0x0CEE,0x0448,0x066A};
    PAL_setPalette(PAL3,landscape,CPU);
    static const u32 patterns[9*8]={
        0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,
        0x11121112,0x11111111,0x12111211,0x11111111,0x11121112,0x11111111,0x12111211,0x11111111,
        0x22222222,0x22222222,0x22222222,0x22222222,0x22222222,0x22222222,0x22222222,0x22222222,
        0x22232223,0x22222222,0x23222322,0x22222222,0x22232223,0x22222222,0x23222322,0x22222222,
        0x33333333,0x33333333,0x33333333,0x33333333,0x33333333,0x33333333,0x33333333,0x33333333,
        0x11111111,0x11166111,0x11166111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,
        0x45555554,0x41111114,0x41111114,0x41199114,0x41199114,0x41111114,0x41111114,0x44444444,
        0x888A8888,0x44444444,0x34343434,0x33333333,0x33333333,0x33333333,0x33333333,0x33333333,
        0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111,0x11111111
    };
    VDP_loadTileData(patterns,BACKGROUND_BASE,9,CPU);
    for (u16 y=0;y<32;y++) for (u16 x=0;x<64;x++) {
        u16 tile=y<10 ? 0 : y<14 ? 1 : y<18 ? 2 : y<22 ? 3 : 4;
        if (y<14 && (x*13+y*7)%47==0) tile=5;
        if (y>=18 && y<24 && (x%14==2 || x%14==3)) tile=6;
        if (y==23) tile=7;
        VDP_setTileMapXY(BG_B,TILE_ATTR_FULL(PAL3,FALSE,FALSE,FALSE,BACKGROUND_BASE+tile),x,y);
    }
    VDP_fillTileMapRect(BG_A,TILE_ATTR_FULL(PAL0,TRUE,FALSE,FALSE,BACKGROUND_BASE+8),0,0,40,5);
    VDP_fillTileMapRect(BG_A,TILE_ATTR_FULL(PAL0,TRUE,FALSE,FALSE,BACKGROUND_BASE+8),0,24,40,4);
    VDP_setTextPalette(PAL0);
    VDP_drawText("EFFECTS LAB / LANTERN GARDEN",2,1);
    VDP_drawText("A BURST    B WEATHER    C DENSITY",2,24);
    VDP_drawText("D-PAD MOVE    START PAUSE",2,25);
    VDP_drawText("UP + START: TOGGLE SCREEN SHAKE",2,26);
    VDP_drawText("COSMETIC POOL / SHARED FRAME CACHE",2,27);
    if (fx_image.maxNumTile!=1 || fx_image.maxNumSprite!=1 ||
        lantern_image.maxNumTile!=4 || lantern_image.maxNumSprite!=1 || CACHE_END>TILE_FONT_INDEX) return false;
    previous_counts[0]=previous_status[0]=previous_fps[0]=0;
    VDP_resetSprites(); initialized=true;
    if (!cache(&fx_image,CACHE_BASE,8,particle_tiles,8) ||
        !cache(&lantern_image,CACHE_BASE+8,4,lantern_tiles,16)) { fx_view_end(); return false; }
    return true;
}

void fx_view_draw(const FxLab *s,const FxRender *render)
{
    u16 frame=(s->ticks/8)&3;
    VDP_setSpriteFull(0,s->player_x-render->camera_x-8,s->player_y-render->camera_y-8,
        SPRITE_SIZE(2,2),TILE_ATTR_FULL(PAL2,TRUE,FALSE,FALSE,lantern_tiles[frame]),render->count ? 1 : 0);
    for (u16 i=0;i<render->count;i++) {
        const FxDraw *p=&render->items[i];
        VDP_setSpriteFull(i+1,p->x,p->y,SPRITE_SIZE(1,1),
            TILE_ATTR_FULL(PAL1,TRUE,FALSE,FALSE,particle_tiles[p->frame]),i+1<render->count ? i+2 : 0);
    }
    /* The persistent SDK SAT cache stays unchanged until this frame's VBlank. */
    VDP_updateSprites(render->count+1,DMA_QUEUE);
    /* BG_A is screen-space HUD; only the world plane and world sprites shake. */
    VDP_setHorizontalScrollVSync(BG_B,-render->camera_x);
    VDP_setVerticalScrollVSync(BG_B,render->camera_y);
}

static void two_digits(char *out,u16 value)
{
    if (value>99) value=99;
    char tens='0';
    while (value>=10) { value-=10; tens++; }
    out[0]=tens; out[1]=(char)('0'+value);
}

void fx_view_hud(const FxLab *s,const FxRender *render,u16 hz,u16 fps)
{
    char rate[]="00FPS",counts[]="LIVE 00/64 SHOWN 00 HIDDEN 00 60HZ";
    two_digits(rate,fps);
    two_digits(counts+5,fx_active(s));two_digits(counts+17,render->count);
    two_digits(counts+27,render->skipped);two_digits(counts+30,hz);
    char status[]="CLEAR  DENSITY 1/4  SHAKE ON       ";
    static const char *weather[]={"CLEAR","RAIN ","SNOW "};
    for (u16 i=0;i<5;i++) status[i]=weather[s->weather][i];
    status[15]=(char)('1'+s->density);
    if (!s->shake_enabled) {status[27]='F';status[28]='F';}
    if (s->paused) for (u16 i=0;i<5;i++)status[29+i]="PAUSE"[i];
    if (strcmp(rate,previous_fps)) {
        VDP_drawTextEx(BG_A,rate,TILE_ATTR(PAL0,TRUE,FALSE,FALSE),33,1,DMA_QUEUE);
        memcpy(previous_fps,rate,sizeof(rate));
    }
    if (strcmp(counts,previous_counts)) {
        VDP_drawTextEx(BG_A,counts,TILE_ATTR(PAL0,TRUE,FALSE,FALSE),2,2,DMA_QUEUE);
        memcpy(previous_counts,counts,sizeof(counts));
    }
    if (strcmp(status,previous_status)) {
        VDP_drawTextEx(BG_A,status,TILE_ATTR(PAL0,TRUE,FALSE,FALSE),2,3,DMA_QUEUE);
        memcpy(previous_status,status,sizeof(status));
    }
}
