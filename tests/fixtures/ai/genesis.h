#ifndef ENEMY_FAKE_GENESIS_H
#define ENEMY_FAKE_GENESIS_H
#include <stddef.h>
#include "game.h"
#define TRUE true
#define FALSE false
#define PAL0 0
#define PAL1 1
#define PAL2 2
#define PAL3 3
#define TILE_USER_INDEX 16
#define RGB24_TO_VDPCOLOR(rgb) ((((rgb)>>20)&14)|(((rgb)>>8)&224)|(((rgb)<<4)&3584))
#define TILE_ATTR(p,prio,v,h) ((p)<<13)
#define TILE_ATTR_FULL(p,prio,v,h,t) (((p)<<13)|(t))
typedef enum { VISIBLE,HIDDEN,AUTO_FAST,AUTO_SLOW } SpriteVisibility;
typedef enum { BG_A,BG_B,WINDOW } VDPPlane;
typedef enum { CPU,DMA,DMA_QUEUE,DMA_QUEUE_COPY } TransferMethod;
typedef struct { const u16 *data; } Palette;
typedef struct { u16 maxNumTile,frames; const Palette *palette; } SpriteDefinition;
typedef struct {
    s16 x,y,animation,frame;
    u16 palette;
    bool visible,flip,vflip,automatic;
    const SpriteDefinition *definition;
} Sprite;
void VDP_setEnable(bool enabled);
void VDP_setScreenWidth320(void);
void VDP_setScreenHeight224(void);
void VDP_setPlaneSize(u16 w,u16 h,bool setupVram);
void VDP_setTextPlane(VDPPlane plane);
void VDP_setTextPalette(u16 palette);
void VDP_setBackgroundColor(u8 value);
void VDP_loadTileData(const u32 *data,u16 index,u16 count,TransferMethod method);
void VDP_fillTileMapRect(VDPPlane plane,u16 attr,u16 x,u16 y,u16 w,u16 h);
void VDP_clearTextArea(u16 x,u16 y,u16 w,u16 h);
void VDP_drawText(const char *text,u16 x,u16 y);
void PAL_setColors(u16 index,const u16 *colors,u16 count,TransferMethod method);
void SPR_initEx(u16 tiles);
bool SPR_isInitialized(void);
Sprite *SPR_addSprite(const SpriteDefinition *definition,s16 x,s16 y,u16 attr);
void SPR_setAutoAnimation(Sprite *sprite,bool enabled);
void SPR_setVisibility(Sprite *sprite,SpriteVisibility value);
void SPR_setPosition(Sprite *sprite,s16 x,s16 y);
void SPR_setAnimAndFrame(Sprite *sprite,s16 animation,s16 frame);
void SPR_setHFlip(Sprite *sprite,bool flip);
void SPR_setVFlip(Sprite *sprite,bool flip);
void SPR_setPalette(Sprite *sprite,u16 palette);
void SPR_update(void);
bool SYS_doVBlankProcess(void);
#endif
