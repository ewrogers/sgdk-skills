#include <assert.h>
#include <string.h>
#include "fake_sdk.h"
#include "resources.h"
#include "checkpoint.h"

char fake_cells[28][41]; u16 fake_ink[28][40];
u16 fake_events[4096],fake_event_count,fake_frames,fake_sram_mode,fake_sram_closes,fake_count;
bool fake_fade,fake_palette_pending,fake_sprite_pending;
int fake_fail_sprite_after=-1;
Sprite fake_sprites[32];
static u16 ink,hardware_count;
static bool initialized;
static u8 memory[CHECKPOINT_BYTES];
static const u16 palette[16]={0,2,4,6,8,10,12,14,32};
static const Palette pal={16,palette};
static const TileSet tiles={0,100,NULL};
const SpriteDefinition hero_image={4,&pal,false},coin_image={1,&pal,true},
    boss_image={16,&pal,false},smoke_image={16,&pal,false},mega_image={16,&pal,false},laser_image={4,&pal,false};
const Image title_art={&pal,&tiles};
static void event(u16 value) { if (fake_event_count<4096) fake_events[fake_event_count++]=value; }
void fake_reset(void)
{
    memset(fake_cells,' ',sizeof fake_cells); for (u16 y=0;y<28;++y) fake_cells[y][40]=0;
    memset(fake_ink,0,sizeof fake_ink); memset(fake_sprites,0,sizeof fake_sprites);
    memset(memory,255,sizeof memory); fake_event_count=fake_frames=fake_count=hardware_count=0;
    fake_sram_mode=fake_sram_closes=0; initialized=false;
    fake_fade=fake_palette_pending=fake_sprite_pending=false; fake_fail_sprite_after=-1; ink=3;
}
void fake_finish_fade(void) { fake_fade=false; fake_palette_pending=true; }
bool SYS_doVBlankProcess(void)
{
    event(EV_DRAIN); ++fake_frames; fake_palette_pending=false; fake_sprite_pending=false;
    hardware_count=fake_count; return true;
}
void VDP_setEnable(bool enabled)
{
    if (enabled) assert(!fake_palette_pending && !fake_sprite_pending);
    event(enabled ? EV_ON : EV_OFF);
}
void VDP_setScreenWidth320(void) {}
void VDP_setScreenHeight224(void) {}
void VDP_setPlaneSize(u16 w,u16 h,bool setup) { assert(w==64 && h==32 && setup); }
void VDP_setScrollingMode(u16 h,u16 v) { assert(!h && !v); }
void VDP_setHorizontalScroll(VDPPlane plane,s16 value) { assert(plane!=WINDOW && !value); }
void VDP_setVerticalScroll(VDPPlane plane,s16 value) { assert(plane!=WINDOW && !value); }
void VDP_setWindowHPos(u16 right,u16 pos) { assert(!right && !pos); }
void VDP_setWindowVPos(u16 down,u16 pos) { assert(!down && !pos); }
void VDP_clearPlane(VDPPlane plane,bool wait)
{
    assert(wait && !fake_palette_pending && !fake_sprite_pending); event(EV_CLEAR);
    if (plane==BG_A) for (u16 y=0;y<28;++y) memset(fake_cells[y],' ',40);
}
void VDP_setTextPlane(VDPPlane plane) { assert(plane==BG_A); }
void VDP_setTextPalette(u16 value) { assert(value<4); ink=value; }
void VDP_setBackgroundColor(u8 value) { assert(value<64); }
void VDP_drawText(const char *text,u16 x,u16 y)
{
    assert(y<28); for (u16 i=0;text[i];++i) { assert(x+i<40); fake_cells[y][x+i]=text[i]; fake_ink[y][x+i]=ink; }
}
void VDP_clearTextArea(u16 x,u16 y,u16 w,u16 h)
{
    assert(x+w<=40 && y+h<=28);
    for (u16 row=y;row<y+h;++row) memset(fake_cells[row]+x,' ',w);
}
void VDP_loadTileData(const u32 *data,u16 index,u16 count,TransferMethod method)
{ assert(data && (index==TILE_USER_INDEX || index==TILE_USER_INDEX+1) && count==1 && method==DMA); }
void VDP_fillTileMapRect(VDPPlane plane,u16 attr,u16 x,u16 y,u16 w,u16 h)
{ (void)attr; assert(plane==BG_B && x+w<=40 && y+h<=32); }
bool VDP_drawImageEx(VDPPlane plane,const Image *image,u16 attr,u16 x,u16 y,bool load_palette,bool dma)
{ (void)attr; assert(plane==BG_B && image==&title_art && x==10 && y==5 && !load_palette && dma); return true; }
void PAL_setColors(u16 index,const u16 *colors,u16 count,TransferMethod method)
{ assert(!fake_palette_pending && index+count<=64 && method==CPU && colors); event(EV_COLOR); }
void PAL_fadeOutAll(u16 frames,bool async) { assert(frames && async && !fake_fade); event(EV_OUT); fake_fade=true; }
void PAL_fadeInAll(const u16 *colors,u16 frames,bool async)
{ assert(colors && frames && async && !fake_palette_pending); event(EV_IN); fake_fade=true; }
bool PAL_isDoingFade(void) { return fake_fade; }
void XGM2_stop(void) { event(EV_STOP); }
void XGM2_stopPCM(SoundPCMChannel channel) { assert(channel==SOUND_PCM_CH2 || channel==SOUND_PCM_CH3); event(EV_PCM); }
void SPR_initEx(u16 tiles_reserved) { assert(tiles_reserved==80); initialized=true; }
bool SPR_isInitialized(void) { return initialized; }
void SPR_reset(void)
{
    assert(initialized && !fake_palette_pending && !fake_sprite_pending); event(EV_RESET);
    for (u16 i=0;i<32;++i) fake_sprites[i].active=false;
    fake_count=0;
}
void SPR_update(void)
{
    assert(initialized); event(EV_SPR); fake_sprite_pending=true;
    for (u16 i=0;i<32;++i) if (fake_sprites[i].active && fake_sprites[i].auto_animation) ++fake_sprites[i].frame;
}
u16 SPR_getNumActiveSprite(void) { return fake_count; }
Sprite *SPR_addSprite(const SpriteDefinition *definition,s16 x,s16 y,u16 attr)
{
    if (fake_fail_sprite_after==0) return NULL;
    if (fake_fail_sprite_after>0) --fake_fail_sprite_after;
    for (u16 i=0;i<32;++i) if (!fake_sprites[i].active) {
        Sprite *sprite=&fake_sprites[i]; *sprite=(Sprite){.x=x,.y=y,.active=true,.auto_animation=true,
            .palette=(attr>>13)&3,.definition=definition}; ++fake_count; return sprite;
    }
    return NULL;
}
void SPR_setAutoAnimation(Sprite *sprite,bool enabled) { assert(sprite && sprite->active); sprite->auto_animation=enabled; }
void SPR_setPosition(Sprite *sprite,s16 x,s16 y) { assert(sprite && sprite->active); sprite->x=x; sprite->y=y; }
void SPR_setAnimAndFrame(Sprite *sprite,s16 animation,s16 frame)
{
    assert(sprite && sprite->active && animation>=0 && animation<3 && frame>=0 && frame<(sprite->definition->coin ? 4 : 8));
    sprite->anim=animation; sprite->frame=frame;
}
void SPR_setHFlip(Sprite *sprite,bool flip) { assert(sprite && sprite->active); sprite->flip=flip; }
void SPR_setVFlip(Sprite *sprite,bool flip) { assert(sprite && sprite->active); sprite->vflip=flip; }
void SPR_setPalette(Sprite *sprite,u16 value) { assert(sprite && sprite->active && value<4); sprite->palette=value; }
void SPR_setVisibility(Sprite *sprite,SpriteVisibility value) { assert(sprite && sprite->active && (value==VISIBLE || value==HIDDEN)); }
u16 MEM_getFree(void) { return initialized ? 7680 : 8192; }
void SRAM_enable(void) { assert(!fake_sram_mode); fake_sram_mode=1; }
void SRAM_enableRO(void) { assert(!fake_sram_mode); fake_sram_mode=2; }
void SRAM_disable(void) { assert(fake_sram_mode); fake_sram_mode=0; ++fake_sram_closes; }
u8 SRAM_readByte(u32 offset) { assert(fake_sram_mode && offset<CHECKPOINT_BYTES); return memory[offset]; }
void SRAM_writeByte(u32 offset,u8 value) { assert(fake_sram_mode==1 && offset<CHECKPOINT_BYTES); memory[offset]=value; }
