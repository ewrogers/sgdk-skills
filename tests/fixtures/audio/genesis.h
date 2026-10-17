#ifndef AUDIO_TEST_GENESIS_H
#define AUDIO_TEST_GENESIS_H
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef uint32_t u32;
extern volatile u32 vtimer;
#define TRUE true
#define FALSE false
#define Z80_DRIVER_XGM2 5
#define BUTTON_UP 0x0001
#define BUTTON_DOWN 0x0002
#define BUTTON_LEFT 0x0004
#define BUTTON_RIGHT 0x0008
#define BUTTON_A 0x0040
#define BUTTON_B 0x0010
#define BUTTON_C 0x0020
#define BUTTON_START 0x0080
typedef enum { SOUND_PCM_CH_AUTO = -1, SOUND_PCM_CH1 = 0,
               SOUND_PCM_CH2, SOUND_PCM_CH3, SOUND_PCM_CH4 } SoundPCMChannel;
void Z80_loadDriver(u16 driver, bool waitReady);
void XGM2_setFMVolume(u16 value);
void XGM2_setPSGVolume(u16 value);
void XGM2_setLoopNumber(s8 value);
bool XGM2_isPlaying(void);
bool XGM2_isProcessingFade(void);
bool XGM2_playPCMEx(const u8 *, u32, SoundPCMChannel, u8, bool, bool);
void XGM2_stop(void);
void XGM2_stopPCM(SoundPCMChannel);
void XGM2_play(const u8 *);
void XGM2_pause(void);
void XGM2_resume(void);
void XGM2_fadeOutAndStop(u16);
u16 SYS_isPAL(void);
u32 XGM2_getElapsed(void);
void Z80_setForceDelayDMA(bool);
#endif
