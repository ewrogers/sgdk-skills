#ifndef BP_TEST_GENESIS_H
#define BP_TEST_GENESIS_H
#include "checked.h"
#include <stddef.h>
typedef int16_t s16;
typedef struct { u16 id; } SpriteDefinition;
typedef struct { u16 id; } Sprite;
#define PAL0 0
#define TILE_ATTR(p,pr,v,h) ((u16)0)
#define DMA_CRAM 1
Sprite *SPR_addSprite(const SpriteDefinition *, s16, s16, u16);
void SPR_releaseSprite(Sprite *);
void SYS_disableInts(void);
void SYS_enableInts(void);
bool DMA_copyAndQueueDma(u8, void *, u16, u16, u16);
#endif
