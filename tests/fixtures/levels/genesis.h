#ifndef LEVEL_TEST_GENESIS_H
#define LEVEL_TEST_GENESIS_H
#include "world.h"
#include <stddef.h>
#include <stdint.h>
typedef enum { BG_A, BG_B, WINDOW } VDPPlane;
typedef enum { CPU, DMA, DMA_QUEUE, DMA_QUEUE_COPY } TransferMethod;
typedef struct { u16 numTile; } TileSet;
typedef struct { u16 length; const u16 *data; } Palette;
typedef struct { u16 maxNumTile; const Palette *palette; } SpriteDefinition;
typedef struct { int unused; } MapDefinition;
typedef struct { int unused; } Map;
typedef struct { int unused; } Sprite;
#define PAL0 0
#define PAL1 1
#define PAL3 3
#define TILE_USER_INDEX 16
extern u16 test_tile_end;
#define TILE_USER_MAX_INDEX test_tile_end
#define HSCROLL_PLANE 0
#define VSCROLL_PLANE 0
#define TILE_ATTR(p, pr, vf, hf) (((p) << 13) | ((pr) << 15) | ((vf) << 12) | ((hf) << 11))
#define TILE_ATTR_FULL(p, pr, vf, hf, tile) (TILE_ATTR(p, pr, vf, hf) | (tile))
extern const u16 palette_grey[16];
void VDP_setEnable(bool);
void VDP_setScreenWidth320(void);
void VDP_setScreenHeight224(void);
void VDP_setPlaneSize(u16, u16, bool);
void VDP_setScrollingMode(u16, u16);
void VDP_setWindowHPos(u16, u16);
void VDP_setWindowVPos(u16, u16);
void VDP_setTextPlane(VDPPlane);
void VDP_setTextPalette(u16);
void VDP_setTextPriority(u16);
void VDP_drawText(const char *, u16, u16);
u16 uintToStr(u32, char *, u16);
void PAL_setPalette(u16, const u16 *, TransferMethod);
bool SYS_doVBlankProcess(void);
void SPR_initEx(u16);
void SPR_end(void);
Sprite *SPR_addSprite(const SpriteDefinition *, s16, s16, u16);
void SPR_setPosition(Sprite *, s16, s16);
void SPR_update(void);
void SPR_setAutoAnimation(Sprite *, bool);
void SPR_setHFlip(Sprite *, bool);
void SPR_setAnimAndFrame(Sprite *, s16, s16);
bool VDP_loadTileSet(const TileSet *, u16, TransferMethod);
Map *MAP_create(const MapDefinition *, VDPPlane, u16);
void MAP_scrollToEx(Map *, u32, u32, bool);
void MAP_release(Map *);
#endif
