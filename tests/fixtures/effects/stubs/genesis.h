#ifndef EFFECTS_TEST_SDK_H
#define EFFECTS_TEST_SDK_H
#include <string.h>
#include "effects.h"
#define TRUE true
#define FALSE false
#define COMPRESSION_NONE 0
#define TILE_FONT_INDEX 1440
#define RGB24_TO_VDPCOLOR(c) 0
#define TILE_ATTR(p,priority,v,h) ((u16)(((p)<<13)|((priority)<<15)|((v)<<12)|((h)<<11)))
#define TILE_ATTR_FULL(p,priority,v,h,t) ((u16)(TILE_ATTR(p,priority,v,h)|(t)))
enum { PAL0,PAL1,PAL2,PAL3 };
enum { BG_A,BG_B,WINDOW };
enum { CPU,DMA,DMA_QUEUE,DMA_QUEUE_COPY };
enum { HIDDEN,VISIBLE };
typedef struct {const u16 *data;} Palette;
typedef struct {u16 compression,numTile;const u32 *tiles;} TileSet;
typedef struct {u8 offsetY,offsetYFlip,size,offsetX,offsetXFlip,numTile;} FrameVDPSprite;
typedef struct {s8 numSprite;const TileSet *tileset;FrameVDPSprite frameVDPSprites[1];} AnimationFrame;
typedef struct {u8 numFrame;const AnimationFrame **frames;} Animation;
typedef struct {u16 w,h,maxNumTile,maxNumSprite,numAnimation;const Animation **animations;const Palette *palette;} SpriteDefinition;
void VDP_setScreenWidth320(void);
void VDP_setScreenHeight224(void);
void VDP_setPlaneSize(u16 w,u16 h,bool clear);
void VDP_setBackgroundColor(u16 color);
void PAL_setColor(u16 index,u16 color);
void PAL_setPalette(u16 palette,const u16 *data,u16 method);
void VDP_loadTileData(const u32 *data,u16 index,u16 count,u16 method);
void VDP_setTileMapXY(u16 plane,u16 tile,u16 x,u16 y);
void VDP_fillTileMapRect(u16 plane,u16 tile,u16 x,u16 y,u16 w,u16 h);
void VDP_setTextPalette(u16 palette);
void VDP_drawText(const char *text,u16 x,u16 y);
void VDP_drawTextEx(u16 plane,const char *text,u16 attr,u16 x,u16 y,u16 method);
void VDP_setHorizontalScrollVSync(u16 plane,s16 value);
void VDP_setVerticalScrollVSync(u16 plane,s16 value);

#define SPRITE_SIZE(w,h) ((((w)-1)<<2)|((h)-1))
u16 VDP_loadTileSet(const TileSet *tiles,u16 index,u16 method);
void VDP_resetSprites(void);
void VDP_setSpriteFull(u16 index,s16 x,s16 y,u8 size,u16 attr,u8 link);
void VDP_updateSprites(u16 count,u16 method);

#endif
