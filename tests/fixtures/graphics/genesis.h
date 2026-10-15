#ifndef GRAPHICS_TEST_GENESIS_H
#define GRAPHICS_TEST_GENESIS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
#define TRUE true
#define FALSE false
#define PAL0 0
#define PAL1 1
#define PAL2 2
#define PAL3 3
#define BUTTON_LEFT 0x0004
#define BUTTON_RIGHT 0x0008
#define BUTTON_A 0x0040
#define BUTTON_B 0x0010
#define BUTTON_C 0x0020
#define TILE_USER_INDEX 16
extern u16 mock_user_max;
#define TILE_USER_MAX_INDEX mock_user_max
#define TILE_ATTR(p, pr, v, h) (((p) << 13) | ((pr) << 15) | ((v) << 12) | ((h) << 11))
#define TILE_ATTR_FULL(p, pr, v, h, t) (TILE_ATTR(p, pr, v, h) | (t))
typedef enum {BG_A, BG_B, WINDOW} VDPPlane;
typedef enum {CPU, DMA, DMA_QUEUE, DMA_QUEUE_COPY} TransferMethod;
/* Deliberately minimal structs: real resource layout is checked in Docker. */
typedef struct {u16 numTile;} TileSet;
typedef struct {const u16 *data;} Palette;
typedef struct {const Palette *palette; const TileSet *tileset;} Image;
typedef struct {const Palette *palette;} SpriteDefinition;
typedef struct {int placeholder;} Sprite;
void SPR_init(void);
void SPR_end(void);
Sprite *SPR_addSprite(const SpriteDefinition *, s16, s16, u16);
void SPR_releaseSprite(Sprite *);
void SPR_update(void);
void SPR_setAutoAnimation(Sprite *, bool);
void SPR_setAnimAndFrame(Sprite *, s16, s16);
void SPR_setAnimationLoop(Sprite *, bool);
bool SPR_isAnimationDone(Sprite *);
void SPR_setPosition(Sprite *, s16, s16);
bool VDP_drawImageEx(VDPPlane, const Image *, u16, u16, u16, bool, bool);
void VDP_setHorizontalScrollVSync(VDPPlane, s16);
void PAL_setPalette(u16, const u16 *, TransferMethod);
void PAL_setColors(u16, const u16 *, u16, TransferMethod);
void PAL_fadeInPalette(u16, const u16 *, u16, bool);
void PAL_fadeOutPalette(u16, u16, bool);
bool PAL_isDoingFade(void);
void PAL_interruptFade(void);
bool SYS_doVBlankProcess(void);
#endif
