#include "bank_model.h"

void bank_model_init(BankModel *m)
{
    *m = (BankModel){.x = 112, .y = 128, .music = 1};
}

u16 bank_model_step(BankModel *m, u16 held, u16 hz)
{
    if (!m || (hz != 50 && hz != 60)) return 0;
    u16 pressed = held & (u16)~m->previous;
    m->previous = held;
    m->clock += 60;
    while (m->clock >= hz) {
        m->clock -= hz;
        s16 x = m->x + 2 * (((held & BANK_RIGHT) != 0) - ((held & BANK_LEFT) != 0));
        s16 y = m->y + 2 * (((held & BANK_DOWN) != 0) - ((held & BANK_UP) != 0));
        m->x = x < 0 ? 0 : x > 304 ? 304 : x;
        m->y = y < 80 ? 80 : y > 184 ? 184 : y;
        m->phase = (m->phase + 1) % 240;
    }
    return pressed & (BANK_STAGE | BANK_SAVE | BANK_LOAD | BANK_MUSIC);
}

u16 bank_crc16(const u8 *data, u16 count)
{
    u16 crc = 0xFFFF;
    for (u16 i = 0; i < count; ++i) {
        crc ^= (u16)data[i] << 8;
        for (u16 bit = 0; bit < 8; ++bit)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}

bool bank_encode(const BankModel *m, u8 out[BANK_SAVE_BYTES])
{
    if (!m || !out || m->stage >= BANK_STAGE_COUNT || m->x > 304 || m->y < 80 || m->y > 184) return 0;
    const u8 data[12] = {'B', 'N', 'K', 'L', 1, m->stage,
                         m->x >> 8, m->x, m->y >> 8, m->y, m->music ? 1 : 0, 0};
    for (u16 i = 0; i < 12; ++i) out[i] = data[i];
    u16 crc = bank_crc16(out, 12);
    out[12] = crc >> 8; out[13] = crc; out[14] = 0x5A; out[15] = 0xA5;
    return 1;
}

bool bank_decode(BankModel *m, const u8 data[BANK_SAVE_BYTES])
{
    if (!m || !data || data[0] != 'B' || data[1] != 'N' || data[2] != 'K' || data[3] != 'L' ||
        data[4] != 1 || data[5] >= BANK_STAGE_COUNT || data[10] > 1 || data[11] ||
        data[14] != 0x5A || data[15] != 0xA5 || bank_crc16(data, 12) != ((u16)data[12] << 8 | data[13])) return 0;
    u16 x = (u16)data[6] << 8 | data[7], y = (u16)data[8] << 8 | data[9];
    if (x > 304 || y < 80 || y > 184) return 0;
    /* Loading persistent fields must not invent a fresh input edge or lose
       the display cadence accumulator. */
    m->x = x; m->y = y; m->stage = data[5]; m->music = data[10] != 0;
    return 1;
}
