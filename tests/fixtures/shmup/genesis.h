#ifndef SHOOTER_FAKE_SDK_H
#define SHOOTER_FAKE_SDK_H
#include "game.h"
#include <stddef.h>
typedef int8_t s8;
#define SOUND_PCM_CH2 2
#define SOUND_PCM_CH3 3
#define SOUND_PCM_CH3_MSK 4
u8 XGM2_isPlayingPCM(u16 mask);
void XGM2_setFMVolume(u16 value);
void XGM2_setPSGVolume(u16 value);
void XGM2_load(const u8 *music);
void XGM2_setLoopNumber(s8 number);
void XGM2_playTrack(u16 track);
void XGM2_pause(void);
void XGM2_resume(void);
void XGM2_stopPCM(u16 channel);
bool XGM2_playPCM(const u8 *sample,u32 length,u16 channel);
#define FALSE false
#define DMA_QUEUE 2
#define TILE_ATTR_FULL(p,priority,v,h,t) (((p)<<13)|(t))
typedef struct { u8 offsetY,offsetYFlip,size,offsetX,offsetXFlip,numTile; } FrameVDPSprite;
typedef struct { s8 numSprite; u8 timer; void *tileset,*collision; FrameVDPSprite frameVDPSprites[1]; } AnimationFrame;
typedef struct { u8 numFrame,loop; AnimationFrame **frames; } Animation;
typedef struct { u16 w,h; void *palette; u16 numAnimation; Animation **animations; u16 maxNumTile,maxNumSprite; } SpriteDefinition;
void VDP_setSpriteFull(u16 index,s16 x,s16 y,u8 size,u16 attribute,u8 link);
void VDP_setSpriteLink(u16 index,u8 link);
void VDP_updateSprites(u16 count,u16 transfer);
#endif
