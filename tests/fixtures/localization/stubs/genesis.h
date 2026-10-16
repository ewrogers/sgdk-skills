#ifndef LOC_TEST_SDK_H
#define LOC_TEST_SDK_H
#include <stdio.h>
#include "loc.h"
#define TRUE true
#define FALSE false
enum {PAL0,PAL1,PAL2,PAL3};
enum {BG_A,BG_B,WINDOW};
enum {CPU,DMA,DMA_QUEUE,DMA_QUEUE_COPY};
#define TILE_FONT_INDEX 1440
#define TILE_ATTR(p,p1,v,h) ((u16)(((p)<<13)|((p1)<<15)|((v)<<12)|((h)<<11)))
#define TILE_ATTR_FULL(p,p1,v,h,t) ((u16)(TILE_ATTR(p,p1,v,h)|(t)))
#define RGB24_TO_VDPCOLOR(c) 0
typedef struct { u16 compression,numTile;const u32 *tiles; } TileSet;
void VDP_setScreenWidth320(void);
void VDP_setScreenHeight224(void);
void VDP_setPlaneSize(u16 w,u16 h,bool clear);
void VDP_setBackgroundColor(u16 color);
void PAL_setColor(u16 index,u16 color);
void PAL_setPalette(u16 palette,const u16 *colors,u16 method);
void VDP_loadTileData(const u32 *data,u16 index,u16 count,u16 method);
u16 VDP_loadTileSet(const TileSet *data,u16 index,u16 method);
void VDP_setTileMapDataRect(u16 plane,const u16 *data,u16 x,u16 y,u16 w,u16 h,u16 wm,u16 method);
void SRAM_enableRO(void);
void SRAM_enable(void);
void SRAM_disable(void);
u8 SRAM_readByte(u32 offset);
void SRAM_writeByte(u32 offset,u8 value);
#endif
