#ifndef TEST_GENESIS_H
#define TEST_GENESIS_H
#include "dl_types.h"
#define SOUND_PCM_CH2 1
void XGM2_load(const u8 *song);
void XGM2_setLoopNumber(signed char loops);
void XGM2_playTrack(u16 track);
void XGM2_setFMVolume(u16 volume);
void XGM2_pause(void);
void XGM2_resume(void);
void XGM2_stopPCM(u16 channel);
bool XGM2_playPCM(const u8 *sample,u32 length,u16 channel);
#endif
