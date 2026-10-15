#ifndef BANK_LAB_IO_H
#define BANK_LAB_IO_H
#include <genesis.h>
#include "bank_model.h"

enum { BANK_ROM_BYTES = 0x600000, BANK_AUDIO_WINDOW = 3 };
bool bank_copy(void *destination, u16 capacity, const void *physical, u16 count);
bool bank_audio_select(const u8 *physical, u32 length, bool play);
bool bank_store_write(const BankModel *model);
bool bank_store_read(BankModel *model);
#endif
