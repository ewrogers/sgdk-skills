/* Minimal host drawing spy. Real resource/layout validation uses the SDK ROM. */
#ifndef ASTERFALL_VIEW_SPY_H
#define ASTERFALL_VIEW_SPY_H
#include <stdint.h>
#include <stdbool.h>
typedef uint16_t u16;typedef uint32_t u32;typedef int16_t s16;
typedef enum {CPU,DMA,DMA_QUEUE,DMA_QUEUE_COPY} TransferMethod;
typedef enum {BG_A,BG_B} VDPPlane;
typedef struct {u16 numTile;const u32 *tiles;} TileSet;
typedef struct {u16 w,h;const u16 *tilemap;} TileMap;
typedef struct {const u16 *data;} Palette;
typedef struct {const Palette *palette;const TileSet *tileset;const TileMap *tilemap;} Image;
typedef struct {u16 offsetX,offsetY,size,numTile;} FrameVDPSprite;
typedef struct {const TileSet *tileset;u16 numSprite;const FrameVDPSprite *frameVDPSprites;} AnimationFrame;
typedef struct {const AnimationFrame **frames;} Animation;
typedef struct {const Animation **animations;const Palette *palette;} SpriteDefinition;
#define TRUE true
#define FALSE false
#define PAL0 0
#define PAL1 1
#define PAL2 2
#define PAL3 3
#define TILE_FONT_INDEX 1440
#define TILE_USER_INDEX 16
#define TILE_ATTR(p,priority,h,v) (((p)<<13)|((priority)?0x8000:0))
#define TILE_ATTR_FULL(p,priority,h,v,tile) (TILE_ATTR(p,priority,h,v)|(tile))
#define SPRITE_SIZE(w,h) (((w)-1)<<2|((h)-1))
void VDP_setTileMapDataRow(VDPPlane,const u16 *,u16,u16,u16,TransferMethod);
void VDP_loadTileSet(const TileSet *,u16,TransferMethod);
void PAL_setPalette(u16,const u16 *,TransferMethod);
void PAL_setColors(u16,const u16 *,u16,TransferMethod);
void VDP_setTileMapEx(VDPPlane,const TileMap *,u16,u16,u16,u16,u16,u16,u16,TransferMethod);
void VDP_loadTileData(const u32 *,u16,u16,TransferMethod);
void VDP_clearPlane(VDPPlane,bool);
void VDP_setEnable(bool);
void VDP_setHorizontalScroll(VDPPlane,s16);
void VDP_setHilightShadow(bool);
void VDP_clearSprites(void);
void VDP_updateSprites(u16,TransferMethod);
void VDP_setSpriteFull(u16,s16,s16,u16,u16,u16);
void VDP_setSpriteLink(u16,u16);
void VDP_setScreenWidth320(void);
void VDP_setScreenHeight224(void);
void VDP_setPlaneSize(u16,u16,bool);
void VDP_setWindowHPos(bool,u16);
void VDP_setWindowVPos(bool,u16);
void VDP_setBackgroundColor(u16);
void SYS_disableInts(void);
void SYS_enableInts(void);
#endif
