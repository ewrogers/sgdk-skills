#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "save.h"

typedef struct {
    u8 bytes[SAVE_BYTES + 2];
    int writes, fail_at, reads, fail_read;
    bool ignore_write;
} Memory;

static bool read_byte(void *context, u32 offset, u8 *value)
{
    Memory *memory = context;
    assert(offset < SAVE_BYTES);
    if (memory->reads++ == memory->fail_read) return false;
    *value = memory->bytes[offset + 1];
    return true;
}

static bool write_byte(void *context, u32 offset, u8 value)
{
    Memory *memory = context;
    assert(offset < SAVE_BYTES);
    if (memory->writes++ == memory->fail_at) return false;
    if (!memory->ignore_write) memory->bytes[offset + 1] = value;
    return true;
}

static SaveIo fresh(Memory *memory, u8 value)
{
    SaveIo io = {memory, SAVE_BYTES, read_byte, write_byte};
    memset(memory, 0, sizeof(*memory));
    memset(memory->bytes, value, sizeof(memory->bytes));
    memory->bytes[0] = 0x42;
    memory->bytes[SAVE_BYTES + 1] = 0x24;
    memory->fail_at = memory->fail_read = -1;
    return io;
}

static SaveState sample(void)
{
    SaveState state = {123400, 7, 5, true};
    return state;
}

static bool equal(SaveState a, SaveState b)
{
    return a.score == b.score && a.level == b.level && a.lives == b.lives && a.music == b.music;
}

static void checksum(u8 *bytes)
{
    u16 crc = save_crc16(bytes, 29);
    bytes[29] = (u8)(crc >> 8);
    bytes[30] = (u8)crc;
}

static void sequence(u8 *bytes, u32 value)
{
    bytes[8] = (u8)(value >> 24); bytes[9] = (u8)(value >> 16);
    bytes[10] = (u8)(value >> 8); bytes[11] = (u8)value;
    checksum(bytes);
}

static void crc_case(void)
{
    assert(save_crc16((const u8 *)"123456789", 9) == 0x29B1);
    assert(save_crc16((const u8 *)"", 0) == 0xFFFF);
}

static void empty_case(void)
{
    Memory memory;
    SaveIo io = fresh(&memory, 0xFF);
    SaveState state = sample(), before = state;
    u8 slot = 99;
    assert(save_load(&io, &state, &slot) == SAVE_EMPTY);
    assert(equal(state, before) && slot == 99);
    io = fresh(&memory, 0);
    assert(save_load(&io, &state, 0) == SAVE_EMPTY);
    io = fresh(&memory, 0x53);
    assert(save_load(&io, &state, 0) == SAVE_CORRUPT);
    assert(equal(state, before));
    save_defaults(&state);
    assert(state.level == 1 && state.lives == 3 && state.score == 0 && state.music);
}

static void roundtrip_case(void)
{
    Memory memory;
    SaveIo io = fresh(&memory, 0xFF);
    SaveState state = sample(), loaded;
    u8 slot;
    assert(save_store(&io, &state) == SAVE_OK);
    assert(save_load(&io, &loaded, &slot) == SAVE_OK && slot == 0 && equal(state, loaded));
    assert(!memcmp(memory.bytes + 1, "SGSV\x01\x08\0\0\0\0\0\0\x07\x05\x01\0\0\x01\xE2\x08", 20));
    state.score = SAVE_MAX_SCORE; state.level = 63; state.lives = 9; state.music = false;
    assert(save_store(&io, &state) == SAVE_OK);
    assert(save_load(&io, &loaded, &slot) == SAVE_OK && slot == 1 && equal(state, loaded));
    state.score = 0; state.level = 1; state.lives = 1;
    assert(save_store(&io, &state) == SAVE_OK);
    assert(save_load(&io, &loaded, &slot) == SAVE_OK && slot == 0 && equal(state, loaded));
    assert(memory.bytes[0] == 0x42 && memory.bytes[SAVE_BYTES + 1] == 0x24);
}

static void recovery_case(void)
{
    Memory memory, original;
    SaveIo io = fresh(&memory, 0xFF);
    SaveState old = sample(), newer = old, loaded;
    unsigned i, bit;
    newer.score++;
    assert(save_store(&io, &old) == SAVE_OK && save_store(&io, &newer) == SAVE_OK);
    original = memory;
    for (i = 0; i < SAVE_SLOT_BYTES; i++) {
        for (bit = 0; bit < 8; bit++) {
            memory = original;
            memory.bytes[1 + SAVE_SLOT_BYTES + i] ^= (u8)(1U << bit);
            assert(save_load(&io, &loaded, 0) == SAVE_OK && equal(old, loaded));
        }
    }
    memory = original;
    memory.bytes[1 + 16] ^= 1; memory.bytes[1 + SAVE_SLOT_BYTES + 16] ^= 1;
    loaded = newer;
    assert(save_load(&io, &loaded, 0) == SAVE_CORRUPT && equal(newer, loaded));
}

static void interrupted_case(void)
{
    Memory memory, original;
    SaveIo io = fresh(&memory, 0xFF);
    SaveState old = sample(), newer = old, loaded;
    int cut;
    newer.score++;
    assert(save_store(&io, &old) == SAVE_OK);
    original = memory;
    for (cut = 0; cut <= 33; cut++) {
        memory = original; memory.writes = 0; memory.fail_at = cut;
        assert(save_store(&io, &newer) == (cut < 33 ? SAVE_IO_ERROR : SAVE_OK));
        memory.fail_at = -1;
        assert(save_load(&io, &loaded, 0) == SAVE_OK);
        assert(equal(loaded, cut < 33 ? old : newer));
        assert(!memcmp(memory.bytes + 1, original.bytes + 1, SAVE_SLOT_BYTES));
    }
    for (cut = 0; cut < 33; cut++) {
        io = fresh(&memory, 0xFF); memory.fail_at = cut;
        assert(save_store(&io, &newer) == SAVE_IO_ERROR);
        loaded = old;
        assert(save_load(&io, &loaded, 0) != SAVE_OK && equal(loaded, old));
    }
    /* Reuse an already committed slot: its old commit byte must be cleared. */
    io = fresh(&memory, 0xFF);
    assert(save_store(&io, &old) == SAVE_OK);
    assert(save_store(&io, &newer) == SAVE_OK);
    original = memory;
    old = newer; newer.score++;
    for (cut = 0; cut < 33; cut++) {
        memory = original; memory.writes = 0; memory.fail_at = cut;
        assert(save_store(&io, &newer) == SAVE_IO_ERROR);
        memory.fail_at = -1;
        assert(save_load(&io, &loaded, 0) == SAVE_OK && equal(loaded, old));
        assert(!memcmp(memory.bytes + 1 + SAVE_SLOT_BYTES,
                       original.bytes + 1 + SAVE_SLOT_BYTES, SAVE_SLOT_BYTES));
    }
}

static void versions_case(void)
{
    Memory memory, original;
    SaveIo io = fresh(&memory, 0xFF);
    SaveState state = sample(), loaded = state;
    assert(save_store(&io, &state) == SAVE_OK && save_store(&io, &state) == SAVE_OK);
    memory.bytes[1 + SAVE_SLOT_BYTES + 4] = 2;
    checksum(memory.bytes + 1 + SAVE_SLOT_BYTES);
    original = memory;
    assert(save_load(&io, &loaded, 0) == SAVE_UNSUPPORTED && equal(loaded, state));
    assert(save_store(&io, &state) == SAVE_UNSUPPORTED);
    assert(!memcmp(memory.bytes, original.bytes, sizeof(memory.bytes)));
}

static void schema_case(void)
{
    Memory memory, original;
    SaveIo io = fresh(&memory, 0xFF);
    SaveState state = sample(), loaded;
    const unsigned fields[] = {5, 6, 7, 12, 13, 14, 15, 16, 20, 28};
    unsigned i;
    assert(save_store(&io, &state) == SAVE_OK);
    original = memory;
    for (i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        memory = original;
        memory.bytes[1 + fields[i]] = 0xFF;
        checksum(memory.bytes + 1);
        loaded = state;
        assert(save_load(&io, &loaded, 0) == SAVE_CORRUPT && equal(loaded, state));
    }
    memory = original;
    state.level = 0;
    assert(save_store(&io, &state) == SAVE_BAD_STATE);
    state = sample(); state.lives = 10;
    assert(save_store(&io, &state) == SAVE_BAD_STATE);
    state = sample(); state.score = SAVE_MAX_SCORE + 1;
    assert(save_store(&io, &state) == SAVE_BAD_STATE);
    assert(!memcmp(memory.bytes, original.bytes, sizeof(memory.bytes)));
}

static void ordering_case(void)
{
    Memory memory;
    SaveIo io = fresh(&memory, 0xFF);
    SaveState state = sample(), loaded;
    u8 slot;
    assert(save_store(&io, &state) == SAVE_OK);
    sequence(memory.bytes + 1, 0xFFFFFFFFUL);
    assert(save_store(&io, &state) == SAVE_OK);
    assert(save_load(&io, &loaded, &slot) == SAVE_OK && slot == 1);
    assert(memory.bytes[1 + SAVE_SLOT_BYTES + 11] == 0);
    memcpy(memory.bytes + 1 + SAVE_SLOT_BYTES, memory.bytes + 1, SAVE_SLOT_BYTES);
    assert(save_load(&io, &loaded, &slot) == SAVE_OK && slot == 0);
    memory.bytes[1 + SAVE_SLOT_BYTES + 12]++;
    checksum(memory.bytes + 1 + SAVE_SLOT_BYTES);
    assert(save_load(&io, &loaded, 0) == SAVE_CONFLICT);
    assert(save_store(&io, &state) == SAVE_CONFLICT);
    sequence(memory.bytes + 1 + SAVE_SLOT_BYTES, 0x7FFFFFFFUL);
    assert(save_load(&io, &loaded, 0) == SAVE_CONFLICT);
}

static void io_case(void)
{
    Memory memory;
    SaveIo io = fresh(&memory, 0xFF);
    SaveState state = sample(), loaded = state;
    unsigned i;
    for (i = 0; i < SAVE_BYTES; i++) {
        io = fresh(&memory, 0xFF); memory.fail_read = (int)i;
        assert(save_store(&io, &state) == SAVE_IO_ERROR && memory.writes == 0);
    }
    io = fresh(&memory, 0xFF); memory.ignore_write = true;
    assert(save_store(&io, &state) == SAVE_IO_ERROR);
    io = fresh(&memory, 0xFF); io.capacity--;
    assert(save_load(&io, &loaded, 0) == SAVE_IO_ERROR && equal(loaded, state));
    assert(save_store(&io, &state) == SAVE_IO_ERROR && memory.writes == 0);
    assert(save_load(0, &loaded, 0) == SAVE_IO_ERROR);
    assert(save_store(&io, 0) == SAVE_BAD_STATE);
    io = fresh(&memory, 0xFF); assert(save_store(&io, &state) == SAVE_OK);
    memory.reads = 0; memory.fail_read = 0;
    assert(save_load(&io, &loaded, 0) == SAVE_IO_ERROR && equal(loaded, state));
    memory.fail_read = -1;
    assert(save_erase(&io) == SAVE_OK);
    assert(save_load(&io, &loaded, 0) == SAVE_EMPTY);
    assert(memory.bytes[0] == 0x42 && memory.bytes[SAVE_BYTES + 1] == 0x24);
}

static void verification_case(void)
{
    Memory memory, original;
    SaveIo io = fresh(&memory, 0xFF);
    SaveState old = sample(), newer = old, loaded;
    int failed_read;
    newer.score++;
    assert(save_store(&io, &old) == SAVE_OK);
    original = memory;
    for (failed_read = 0; failed_read < 128; failed_read++) {
        memory = original; memory.reads = 0; memory.fail_read = failed_read;
        assert(save_store(&io, &newer) == SAVE_IO_ERROR);
        memory.fail_read = -1;
        assert(save_load(&io, &loaded, 0) == SAVE_OK);
        assert(equal(loaded, failed_read < 96 ? old : newer));
        assert(!memcmp(memory.bytes + 1, original.bytes + 1, SAVE_SLOT_BYTES));
    }
}

static void password_case(void)
{
    SaveState state = sample(), decoded;
    char code[SAVE_PASSWORD_LENGTH + 1];
    u32 score;
    assert(save_password_encode(&state, code));
    assert(!strcmp(code, "CHMD2SJAZ4UA"));
    assert(save_password_decode(code, &decoded) && equal(state, decoded));
    assert(save_password_decode("chmd2sjaz4ua", &decoded) && equal(state, decoded));
    for (score = 0; score <= SAVE_MAX_SCORE; score += 997) {
        state.score = score; state.level = (u8)(score % 63 + 1);
        state.lives = (u8)(score % 9 + 1); state.music = score % 2 != 0;
        assert(save_password_encode(&state, code));
        assert(save_password_decode(code, &decoded) && equal(state, decoded));
    }
    state.score = SAVE_MAX_SCORE; state.level = 63; state.lives = 9;
    assert(save_password_encode(&state, code));
    assert(save_password_decode(code, &decoded) && equal(state, decoded));
}

static void password_bad_case(void)
{
    SaveState state = sample(), decoded = state;
    char code[SAVE_PASSWORD_LENGTH + 1], changed[SAVE_PASSWORD_LENGTH + 1];
    unsigned i, replacement;
    save_password_encode(&state, code);
    for (i = 0; i < SAVE_PASSWORD_LENGTH; i++) {
        for (replacement = 0; replacement < 32; replacement++) {
            if (code[i] == save_password_alphabet[replacement]) continue;
            memcpy(changed, code, sizeof(code));
            changed[i] = save_password_alphabet[replacement];
            assert(!save_password_decode(changed, &decoded) && equal(state, decoded));
        }
    }
    assert(!save_password_decode("CHMD2SJAZ4U", &decoded));
    assert(!save_password_decode("CHMD2SJAZ4UAA", &decoded));
    assert(!save_password_decode("0HMD2SJAZ4UA", &decoded));
    assert(!save_password_decode("CHMD-2SJA-Z4UA", &decoded));
    assert(!save_password_decode(0, &decoded));
    state.level = 0;
    assert(!save_password_encode(&state, code));
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (!strcmp(argv[1], "crc")) crc_case();
    else if (!strcmp(argv[1], "empty")) empty_case();
    else if (!strcmp(argv[1], "roundtrip")) roundtrip_case();
    else if (!strcmp(argv[1], "recovery")) recovery_case();
    else if (!strcmp(argv[1], "interrupted")) interrupted_case();
    else if (!strcmp(argv[1], "versions")) versions_case();
    else if (!strcmp(argv[1], "schema")) schema_case();
    else if (!strcmp(argv[1], "ordering")) ordering_case();
    else if (!strcmp(argv[1], "io")) io_case();
    else if (!strcmp(argv[1], "verification")) verification_case();
    else if (!strcmp(argv[1], "password")) password_case();
    else if (!strcmp(argv[1], "password_bad")) password_bad_case();
    else abort();
    puts(argv[1]);
    return 0;
}
