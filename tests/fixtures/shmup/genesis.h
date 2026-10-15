#ifndef SHOOTER_FAKE_SDK_H
#define SHOOTER_FAKE_SDK_H
#include "game.h"
#include <stddef.h>
typedef int8_t s8;
#define SOUND_PCM_CH2 2
#define SOUND_PCM_CH3 3
void XGM2_setFMVolume(u16 value);
void XGM2_setPSGVolume(u16 value);
void XGM2_load(const u8 *music);
void XGM2_setLoopNumber(s8 number);
void XGM2_playTrack(u16 track);
void XGM2_pause(void);
void XGM2_resume(void);
bool XGM2_stopPCM(u16 channel);
bool XGM2_playPCM(const u8 *sample,u32 length,u16 channel);
#endif
