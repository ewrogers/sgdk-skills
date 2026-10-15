#ifndef BANK_TEST_GENESIS_H
#define BANK_TEST_GENESIS_H
#include "bank_model.h"
typedef int8_t s8;
#define TRUE true
#define FALSE false
#define BANK_SIZE 0x80000
void *SYS_getFarDataSafe(void *data, u32 size);
#define FAR_SAFE(data, size) SYS_getFarDataSafe((void *)(data), (size))
void SYS_setBank(u16 region, u16 bank);
void Z80_unloadDriver(void);
void XGM2_load(const u8 *song);
void XGM2_setLoopNumber(s8 number);
void XGM2_playTrack(u16 track);
void SRAM_enable(void);
void SRAM_enableRO(void);
void SRAM_disable(void);
u8 SRAM_readByte(u32 offset);
void SRAM_writeByte(u32 offset, u8 value);
#endif
