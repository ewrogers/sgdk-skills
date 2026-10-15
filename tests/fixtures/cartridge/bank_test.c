#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bank_io.h"

static u8 rom[BANK_ROM_BYTES], views[BANK_SIZE * 2], sram[BANK_SAVE_BYTES];
static u16 banks[8] = {0,1,2,3,4,5,6,7};
static unsigned mappings, calls, unload_order, map_order, load_order, play_order, plays;
static unsigned sram_mode, disables, writes;
static bool fail_write;
static const u8 *loaded;
void *SYS_getFarDataSafe(void *p, u32 size)
{
    u32 addr = (u32)(unsigned long)p;
    assert(size && addr + size <= BANK_ROM_BYTES);
    u32 bank = addr / BANK_SIZE, offset = addr % BANK_SIZE;
    banks[6] = bank; banks[7] = bank + 1; ++mappings;
    memset(views, 0xCD, sizeof views);
    u32 available = BANK_ROM_BYTES - bank * BANK_SIZE;
    memcpy(views, rom + bank * BANK_SIZE, available < sizeof views ? available : sizeof views);
    return views + offset;
}
void SYS_setBank(u16 region, u16 bank) { assert(region == 3); banks[region] = bank; map_order = ++calls; }
void Z80_unloadDriver(void) { unload_order = ++calls; }
void XGM2_load(const u8 *song) { loaded = song; load_order = ++calls; }
void XGM2_setLoopNumber(s8 number) { assert(number == -1); ++calls; }
void XGM2_playTrack(u16 track) { assert(track == 0); ++plays; play_order = ++calls; }
void SRAM_enable(void) { assert(!sram_mode); sram_mode = 1; }
void SRAM_enableRO(void) { assert(!sram_mode); sram_mode = 2; }
void SRAM_disable(void) { assert(sram_mode); sram_mode = 0; ++disables; }
u8 SRAM_readByte(u32 offset) { assert(sram_mode && offset < sizeof sram); return sram[offset]; }
void SRAM_writeByte(u32 offset, u8 value)
{
    assert(sram_mode == 1 && offset < sizeof sram);
    if (!writes) assert(offset == BANK_SAVE_BYTES - 1 && value == 0);
    ++writes;
    if (!fail_write) sram[offset] = value;
}

static void model(void)
{
    BankModel a, b; bank_model_init(&a); bank_model_init(&b);
    for (unsigned i = 0; i < 30; ++i) bank_model_step(&a, BANK_RIGHT, 60);
    for (unsigned i = 0; i < 25; ++i) bank_model_step(&b, BANK_RIGHT, 50);
    assert(a.x == 172 && b.x == a.x && a.phase == b.phase);
    assert(bank_model_step(&a, BANK_SAVE, 60) == BANK_SAVE);
    assert(!bank_model_step(&a, BANK_SAVE, 60));
    BankModel before = a; assert(!bank_model_step(&a, 0, 0)); assert(!memcmp(&a, &before, sizeof a));
    for (unsigned i = 0; i < 500; ++i) bank_model_step(&a, BANK_LEFT | BANK_UP, 60);
    assert(a.x == 0 && a.y == 80);
    for (unsigned i = 0; i < 500; ++i) bank_model_step(&a, BANK_RIGHT | BANK_DOWN, 60);
    assert(a.x == 304 && a.y == 184);
}
static void record(void)
{
    assert(bank_crc16((const u8 *)"123456789", 9) == 0x29B1);
    BankModel a, b; bank_model_init(&a); bank_model_init(&b);
    a.x = 288; a.y = 180; a.stage = 1; a.music = false;
    u8 bytes[BANK_SAVE_BYTES], copy[BANK_SAVE_BYTES]; assert(bank_encode(&a, bytes));
    b.previous = BANK_LOAD; b.clock = 10; b.phase = 23;
    assert(bank_decode(&b, bytes)); assert(b.x == 288 && b.y == 180 && b.stage == 1 && !b.music);
    assert(b.previous == BANK_LOAD && b.clock == 10 && b.phase == 23);
    for (unsigned i = 0; i < BANK_SAVE_BYTES * 8; ++i) {
        memcpy(copy, bytes, sizeof copy); copy[i / 8] ^= 1u << (i % 8);
        BankModel before = b; assert(!bank_decode(&b, copy)); assert(!memcmp(&before, &b, sizeof b));
    }
    for (unsigned field = 4; field <= 11; ++field) {
        if (field == 7 || field == 9) continue;
        memcpy(copy, bytes, sizeof copy); copy[field] = 0xFF;
        u16 crc = bank_crc16(copy, 12); copy[12] = crc >> 8; copy[13] = crc;
        assert(!bank_decode(&b, copy));
    }
    a.x = 305; assert(!bank_encode(&a, bytes));
    assert(!bank_encode(NULL, bytes) && !bank_decode(NULL, bytes) && !bank_decode(&b, NULL));
}
static void far_copy(void)
{
    for (u32 i = 0; i < BANK_ROM_BYTES; ++i) rom[i] = (i / BANK_SIZE) ^ (i % 251);
    u8 copy[32], expected[32]; const void *source = (const void *)0x47FFF0UL;
    memcpy(expected, rom + 0x47FFF0, 32);
    assert(bank_copy(copy, 32, source, 32)); assert(!memcmp(copy, expected, 32));
    assert(banks[6] == 8 && banks[7] == 9);
    /* The queued COPY owns its bytes; a borrowed view is invalid after remap. */
    u8 *borrowed = SYS_getFarDataSafe((void *)source, 32), queued_copy[32];
    memcpy(queued_copy, borrowed, 32);
    assert(bank_copy(copy, 32, (void *)0x500000UL, 32));
    assert(memcmp(borrowed, expected, 32) && !memcmp(queued_copy, expected, 32));
    unsigned before = mappings;
    assert(!bank_copy(copy, 31, source, 32)); assert(!bank_copy(copy, 32, source, 0));
    assert(!bank_copy(NULL, 32, source, 32)); assert(!bank_copy(copy, 32, NULL, 32));
    assert(!bank_copy(copy, 32, (void *)(BANK_ROM_BYTES - 16UL), 32)); assert(mappings == before);
}
static void audio(void)
{
    assert(bank_audio_select((void *)0x480100UL, 768, true));
    assert(unload_order < map_order && map_order < load_order && load_order < play_order);
    assert(banks[3] == 9 && (unsigned long)loaded == 0x180100UL);
    unsigned played = plays;
    assert(bank_audio_select((void *)0x580000UL, 768, false)); assert(plays == played);
    assert(banks[3] == 11 && (unsigned long)loaded == 0x180000UL);
    u8 copy[32]; assert(bank_copy(copy, 32, (void *)0x47FFF0UL, 32)); assert(banks[3] == 11);
    unsigned before = calls;
    assert(!bank_audio_select((void *)0x480001UL, 256, true));
    assert(!bank_audio_select((void *)0x4FFF00UL, 512, true));
    assert(!bank_audio_select((void *)BANK_ROM_BYTES, 256, true));
    assert(!bank_audio_select(NULL, 256, true)); assert(!bank_audio_select((void *)0x480000UL, 0, true));
    assert(calls == before);
}
static void storage(void)
{
    BankModel a, b; bank_model_init(&a); bank_model_init(&b); a.stage = 1; a.x = 222;
    u16 before[8]; memcpy(before, banks, sizeof banks);
    assert(bank_store_write(&a) && !sram_mode && disables == 1 && writes == 17);
    assert(bank_store_read(&b) && b.x == a.x && b.stage == a.stage && disables == 2);
    assert(!memcmp(before, banks, sizeof banks));
    sram[6] ^= 1; BankModel original = b;
    assert(!bank_store_read(&b) && !memcmp(&b, &original, sizeof b) && !sram_mode);
    fail_write = true; writes = 0; assert(!bank_store_write(&a) && !sram_mode);
    a.stage = 2; unsigned count = disables; assert(!bank_store_write(&a) && disables == count);
}
int main(int argc, char **argv)
{
    assert(argc == 2);
    if (!strcmp(argv[1], "model")) model();
    else if (!strcmp(argv[1], "record")) record();
    else if (!strcmp(argv[1], "far")) far_copy();
    else if (!strcmp(argv[1], "audio")) audio();
    else if (!strcmp(argv[1], "storage")) storage();
    else return 2;
    puts(argv[1]); return 0;
}
