#include "save.h"

#define COMMIT 0xA5
#define VERSION 1

const char save_password_alphabet[33] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

static u32 read32(const u8 *p)
{
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3];
}

static void write32(u8 *p, u32 value)
{
    p[0] = (u8)(value >> 24);
    p[1] = (u8)(value >> 16);
    p[2] = (u8)(value >> 8);
    p[3] = (u8)value;
}

void save_defaults(SaveState *state)
{
    state->score = 0;
    state->level = 1;
    state->lives = 3;
    state->music = true;
}

bool save_state_valid(const SaveState *state)
{
    return state && state->score <= SAVE_MAX_SCORE && state->level >= 1 &&
           state->level <= 63 && state->lives >= 1 && state->lives <= 9;
}

u16 save_crc16(const u8 *bytes, u16 length)
{
    u16 crc = 0xFFFF;
    u16 i;
    while (length--) {
        crc ^= (u16)*bytes++ << 8;
        for (i = 0; i < 8; i++)
            crc = (u16)((crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1);
    }
    return crc;
}

static bool io_valid(const SaveIo *io)
{
    return io && io->read && io->write && io->capacity >= SAVE_BYTES;
}

static bool read_slot(const SaveIo *io, u8 slot, u8 *bytes)
{
    u16 i;
    for (i = 0; i < SAVE_SLOT_BYTES; i++)
        if (!io->read(io->context, (u32)slot * SAVE_SLOT_BYTES + i, bytes + i))
            return false;
    return true;
}

static bool envelope_valid(const u8 *bytes)
{
    u16 crc = save_crc16(bytes, 29);
    return bytes[0] == 'S' && bytes[1] == 'G' && bytes[2] == 'S' &&
           bytes[3] == 'V' && bytes[31] == COMMIT &&
           bytes[29] == (u8)(crc >> 8) && bytes[30] == (u8)crc;
}

static bool decode(const u8 *bytes, SaveState *state)
{
    u16 i;
    if (bytes[5] != 8 || bytes[6] || bytes[7] || bytes[15] || bytes[14] > 1)
        return false;
    for (i = 20; i < 29; i++)
        if (bytes[i]) return false;
    state->level = bytes[12];
    state->lives = bytes[13];
    state->music = bytes[14] != 0;
    state->score = read32(bytes + 16);
    return save_state_valid(state);
}

/* Read both slots before choosing. Unknown valid versions are preserved. */
static SaveResult scan(const SaveIo *io, u8 bytes[2][SAVE_SLOT_BYTES], int *newest)
{
    bool valid[2] = {false, false};
    bool empty = true;
    SaveState ignored;
    u16 slot, i;
    *newest = -1;
    if (!io_valid(io)) return SAVE_IO_ERROR;
    for (slot = 0; slot < 2; slot++) {
        bool zero = true, erased = true;
        if (!read_slot(io, (u8)slot, bytes[slot])) return SAVE_IO_ERROR;
        for (i = 0; i < SAVE_SLOT_BYTES; i++) {
            if (bytes[slot][i] != 0) zero = false;
            if (bytes[slot][i] != 0xFF) erased = false;
        }
        if (!zero && !erased) empty = false;
        if (envelope_valid(bytes[slot])) {
            if (bytes[slot][4] != VERSION) return SAVE_UNSUPPORTED;
            valid[slot] = decode(bytes[slot], &ignored);
        }
    }
    if (valid[0] && valid[1]) {
        u32 difference = read32(bytes[1] + 8) - read32(bytes[0] + 8);
        if (difference == 0) {
            for (i = 0; i < SAVE_SLOT_BYTES; i++)
                if (bytes[0][i] != bytes[1][i]) return SAVE_CONFLICT;
            *newest = 0;
        } else if (difference == 0x80000000UL) {
            return SAVE_CONFLICT;
        } else {
            *newest = difference < 0x80000000UL ? 1 : 0;
        }
    } else if (valid[0] || valid[1]) {
        *newest = valid[0] ? 0 : 1;
    }
    return *newest >= 0 ? SAVE_OK : (empty ? SAVE_EMPTY : SAVE_CORRUPT);
}

SaveResult save_load(const SaveIo *io, SaveState *state, u8 *slot)
{
    u8 bytes[2][SAVE_SLOT_BYTES];
    int newest;
    SaveResult result;
    if (!state) return SAVE_BAD_STATE;
    result = scan(io, bytes, &newest);
    if (result != SAVE_OK) return result;
    decode(bytes[newest], state);
    if (slot) *slot = (u8)newest;
    return SAVE_OK;
}

SaveResult save_store(const SaveIo *io, const SaveState *state)
{
    u8 slots[2][SAVE_SLOT_BYTES], bytes[SAVE_SLOT_BYTES] = {0};
    u8 verify[SAVE_SLOT_BYTES];
    u8 target;
    u16 i, crc;
    u32 sequence, base;
    int newest;
    SaveResult result;
    if (!save_state_valid(state)) return SAVE_BAD_STATE;
    result = scan(io, slots, &newest);
    if (result != SAVE_OK && result != SAVE_EMPTY && result != SAVE_CORRUPT)
        return result;
    target = newest >= 0 ? (u8)(1 - newest) : 0;
    sequence = newest >= 0 ? read32(slots[newest] + 8) + 1 : 0;
    base = (u32)target * SAVE_SLOT_BYTES;
    bytes[0] = 'S'; bytes[1] = 'G'; bytes[2] = 'S'; bytes[3] = 'V';
    bytes[4] = VERSION;
    bytes[5] = 8;
    write32(bytes + 8, sequence);
    bytes[12] = state->level;
    bytes[13] = state->lives;
    bytes[14] = state->music ? 1 : 0;
    write32(bytes + 16, state->score);
    crc = save_crc16(bytes, 29);
    bytes[29] = (u8)(crc >> 8);
    bytes[30] = (u8)crc;

    /* Invalidate first, body second, commit last. Never modify the newest slot. */
    if (!io->write(io->context, base + 31, 0)) return SAVE_IO_ERROR;
    for (i = 0; i < 31; i++)
        if (!io->write(io->context, base + i, bytes[i])) return SAVE_IO_ERROR;
    if (!read_slot(io, target, verify)) return SAVE_IO_ERROR;
    for (i = 0; i < SAVE_SLOT_BYTES; i++)
        if (verify[i] != bytes[i]) return SAVE_IO_ERROR;
    if (!io->write(io->context, base + 31, COMMIT)) return SAVE_IO_ERROR;
    if (!read_slot(io, target, verify) || !envelope_valid(verify)) return SAVE_IO_ERROR;
    return SAVE_OK;
}

SaveResult save_erase(const SaveIo *io)
{
    u16 i;
    u8 verify;
    if (!io_valid(io)) return SAVE_IO_ERROR;
    for (i = 0; i < SAVE_BYTES; i++)
        if (!io->write(io->context, i, 0xFF)) return SAVE_IO_ERROR;
    for (i = 0; i < SAVE_BYTES; i++)
        if (!io->read(io->context, i, &verify) || verify != 0xFF) return SAVE_IO_ERROR;
    return SAVE_OK;
}

static void put_bits(u8 *bytes, u16 first, u16 count, u32 value)
{
    u16 i;
    for (i = 0; i < count; i++) {
        u16 bit = first + i;
        if ((value >> (count - i - 1)) & 1)
            bytes[bit / 8] |= (u8)(1U << (7 - bit % 8));
    }
}

static u32 get_bits(const u8 *bytes, u16 first, u16 count)
{
    u16 i;
    u32 value = 0;
    for (i = 0; i < count; i++) {
        u16 bit = first + i;
        value = (value << 1) | ((bytes[bit / 8] >> (7 - bit % 8)) & 1);
    }
    return value;
}

bool save_password_encode(const SaveState *state, char output[SAVE_PASSWORD_LENGTH + 1])
{
    u8 bytes[8] = {0};
    u16 i, crc;
    if (!output || !save_state_valid(state)) return false;
    put_bits(bytes, 0, 4, VERSION);
    put_bits(bytes, 4, 6, state->level);
    put_bits(bytes, 10, 4, state->lives);
    put_bits(bytes, 14, 1, state->music ? 1 : 0);
    put_bits(bytes, 15, 20, state->score);
    crc = save_crc16(bytes, 5);
    put_bits(bytes, 40, 16, crc);
    for (i = 0; i < SAVE_PASSWORD_LENGTH; i++)
        output[i] = save_password_alphabet[get_bits(bytes, i * 5, 5)];
    output[SAVE_PASSWORD_LENGTH] = 0;
    return true;
}

bool save_password_decode(const char *text, SaveState *state)
{
    u8 bytes[8] = {0};
    SaveState candidate;
    u16 i, index;
    if (!text || !state) return false;
    for (i = 0; i < SAVE_PASSWORD_LENGTH; i++) {
        char c = text[i];
        if (!c) return false;
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        for (index = 0; index < 32 && save_password_alphabet[index] != c; index++) {}
        if (index == 32) return false;
        put_bits(bytes, i * 5, 5, index);
    }
    if (text[SAVE_PASSWORD_LENGTH] || get_bits(bytes, 0, 4) != VERSION ||
        get_bits(bytes, 35, 5) || get_bits(bytes, 56, 4) ||
        get_bits(bytes, 40, 16) != save_crc16(bytes, 5)) return false;
    candidate.level = (u8)get_bits(bytes, 4, 6);
    candidate.lives = (u8)get_bits(bytes, 10, 4);
    candidate.music = get_bits(bytes, 14, 1) != 0;
    candidate.score = get_bits(bytes, 15, 20);
    if (!save_state_valid(&candidate)) return false;
    *state = candidate;
    return true;
}
