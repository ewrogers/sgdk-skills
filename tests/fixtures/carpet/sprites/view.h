#ifndef CARPET_SPRITE_TEST_H
#define CARPET_SPRITE_TEST_H
#include "game.h"
#include <string.h>
#define BANNER_LINE_TILES 24
#define TRUE true
#define FALSE false
#define PAL3 3
#define DMA_QUEUE_COPY 1
#define DMA_QUEUE 2
#define TILE_ATTR_FULL(p,priority,v,h,tile) ((tile)|((priority)?0x8000:0)|((v)?0x1000:0)|((h)?0x0800:0))
#define SPRITE_SIZE(w,h) ((((w)-1)<<2)|((h)-1))
typedef struct {u16 compression,numTile;u32 *tiles;} TileSet;
typedef struct {s16 offsetX,offsetY,offsetXFlip;u8 size,numTile;} FrameVDPSprite;
typedef struct {u16 numSprite;const FrameVDPSprite *frameVDPSprites;} AnimationFrame;
typedef struct {u16 numFrame;const AnimationFrame **frames;} Animation;
typedef struct {u16 w,h;const Animation **animations;} SpriteDefinition;
typedef enum {ART_HERO,ART_EFFECT,ART_COUNT} Art;
typedef struct {const SpriteDefinition *definition;u16 tiles[16],palette;} Visual;
extern Visual visuals[ART_COUNT];
extern u16 banner_tile;
void VDP_setSpriteFull(u16,s16,s16,u8,u16,u16);
void VDP_setSpriteLink(u16,u16);
void VDP_updateSprites(u16,u16);
void VDP_loadTileData(const u32 *,u16,u16,u16);
void sprites_begin(void);
void sprites_end(void);
void sprite_text(const char *,s16,s16);
void sprite_shield(s16,s16,u16);
void sprites_invalidate_text(void);
#endif
