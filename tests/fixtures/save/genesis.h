#ifndef TEST_SAVE_GENESIS_H
#define TEST_SAVE_GENESIS_H
#include "save.h"
void SRAM_enable(void);
void SRAM_enableRO(void);
void SRAM_disable(void);
u8 SRAM_readByte(u32 offset);
void SRAM_writeByte(u32 offset, u8 value);
#endif
