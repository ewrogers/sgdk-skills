#ifndef CARPET_SOUND_TEST_H
#define CARPET_SOUND_TEST_H
#include "game.h"
#include <stddef.h>
#define TRUE true
#define FALSE false
#define SOUND_PCM_CH2 2
#define SOUND_PCM_CH3 3
void XGM2_setFMVolume(u16);
void XGM2_setPSGVolume(u16);
void XGM2_load(const u8 *);
void XGM2_setLoopNumber(s16);
void XGM2_playTrack(u16);
void XGM2_pause(void);
void XGM2_resume(void);
void XGM2_stopPCM(u16);
void XGM2_playPCM(const u8 *,u32,u16);
void sound_start(void);
void sound_frame(const Game *);
#endif
