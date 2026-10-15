#ifndef BANK_LAB_MODEL_H
#define BANK_LAB_MODEL_H
#ifdef __m68k__
#include <types.h>
#else
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
#endif

enum { BANK_SAVE_BYTES = 16, BANK_STAGE_COUNT = 2,
       BANK_LEFT = 1, BANK_RIGHT = 2, BANK_UP = 4, BANK_DOWN = 8,
       BANK_STAGE = 16, BANK_SAVE = 32, BANK_LOAD = 64, BANK_MUSIC = 128 };
typedef struct {
    u16 x, y, previous, clock, phase;
    u8 stage;
    bool music;
} BankModel;

void bank_model_init(BankModel *model);
u16 bank_model_step(BankModel *model, u16 held, u16 hz);
u16 bank_crc16(const u8 *data, u16 count);
bool bank_encode(const BankModel *model, u8 out[BANK_SAVE_BYTES]);
bool bank_decode(BankModel *model, const u8 data[BANK_SAVE_BYTES]);
#endif
