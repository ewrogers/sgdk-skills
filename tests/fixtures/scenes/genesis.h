#ifndef TRANSITION_FAKE_GENESIS_H
#define TRANSITION_FAKE_GENESIS_H
#include <stddef.h>
#include "lab.h"
#define TRUE true
#define FALSE false
#define PAL0 0
#define PAL1 1
#define PAL2 2
#define PAL3 3
#define TILE_USER_INDEX 96
#define TILE_USER_MAX_INDEX 1800
#define RGB24_TO_VDPCOLOR(rgb) ((((rgb)>>20)&14)|(((rgb)>>8)&224)|(((rgb)<<4)&3584))
#define TILE_ATTR(p,prio,v,h) ((p)<<13)
#define TILE_ATTR_FULL(p,prio,v,h,t) (((p)<<13)|(t))
#define HSCROLL_PLANE 0
#define VSCROLL_PLANE 0
typedef enum { VISIBLE,HIDDEN,AUTO_FAST,AUTO_SLOW } SpriteVisibility;
typedef enum { BG_A,BG_B,WINDOW } VDPPlane;
typedef enum { CPU,DMA,DMA_QUEUE,DMA_QUEUE_COPY } TransferMethod;
typedef enum { SOUND_PCM_CH1,SOUND_PCM_CH2,SOUND_PCM_CH3,SOUND_PCM_CH4 } SoundPCMChannel;
typedef struct { u16 length; const u16 *data; } Palette;
typedef struct { u16 compression,numTile; const u32 *tiles; } TileSet;
typedef struct { u16 maxNumTile; const Palette *palette; bool coin; } SpriteDefinition;
typedef struct { const Palette *palette; const TileSet *tileset; } Image;
typedef struct { s16 x,y,anim,frame; bool active,auto_animation,flip,vflip; u16 palette; const SpriteDefinition *definition; } Sprite;
bool SYS_doVBlankProcess(void);
void VDP_setEnable(bool enabled);
void VDP_setScreenWidth320(void);
void VDP_setScreenHeight224(void);
void VDP_setPlaneSize(u16 w,u16 h,bool setupVram);
void VDP_setScrollingMode(u16 h,u16 v);
void VDP_setHorizontalScroll(VDPPlane plane,s16 value);
void VDP_setVerticalScroll(VDPPlane plane,s16 value);
void VDP_setWindowHPos(u16 right,u16 pos);
void VDP_setWindowVPos(u16 down,u16 pos);
void VDP_clearPlane(VDPPlane plane,bool wait);
void VDP_setTextPlane(VDPPlane plane);
void VDP_setTextPalette(u16 palette);
void VDP_setBackgroundColor(u8 value);
void VDP_drawText(const char *text,u16 x,u16 y);
void VDP_clearTextArea(u16 x,u16 y,u16 w,u16 h);
void VDP_loadTileData(const u32 *data,u16 index,u16 count,TransferMethod method);
void VDP_fillTileMapRect(VDPPlane plane,u16 attr,u16 x,u16 y,u16 w,u16 h);
bool VDP_drawImageEx(VDPPlane plane,const Image *image,u16 attr,u16 x,u16 y,bool load_palette,bool dma);
void PAL_setColors(u16 index,const u16 *colors,u16 count,TransferMethod method);
void PAL_fadeOutAll(u16 frames,bool async);
void PAL_fadeInAll(const u16 *colors,u16 frames,bool async);
bool PAL_isDoingFade(void);
void XGM2_stop(void);
void XGM2_stopPCM(SoundPCMChannel channel);
void SPR_initEx(u16 tiles);
bool SPR_isInitialized(void);
void SPR_reset(void);
void SPR_update(void);
u16 SPR_getNumActiveSprite(void);
Sprite *SPR_addSprite(const SpriteDefinition *definition,s16 x,s16 y,u16 attr);
void SPR_setAutoAnimation(Sprite *sprite,bool enabled);
void SPR_setPosition(Sprite *sprite,s16 x,s16 y);
void SPR_setAnimAndFrame(Sprite *sprite,s16 animation,s16 frame);
void SPR_setHFlip(Sprite *sprite,bool flip);
void SPR_setVFlip(Sprite *sprite,bool flip);
void SPR_setPalette(Sprite *sprite,u16 palette);
void SPR_setVisibility(Sprite *sprite,SpriteVisibility value);
u16 MEM_getFree(void);
void SRAM_enable(void);
void SRAM_enableRO(void);
void SRAM_disable(void);
u8 SRAM_readByte(u32 offset);
void SRAM_writeByte(u32 offset,u8 value);
#endif
