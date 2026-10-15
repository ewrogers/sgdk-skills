#include "bank_io.h"

bool bank_copy(void *destination, u16 capacity, const void *physical, u16 count)
{
    u32 address = (u32)(unsigned long)physical;
    if (!destination || !physical || !count || count > capacity ||
        address >= BANK_ROM_BYTES || count > BANK_ROM_BYTES - address) return FALSE;
    /* No other FAR producer runs between mapping and this immediate copy.
       The caller receives owned bytes, never a retained transient view. */
    const u8 *source = FAR_SAFE(physical, count);
    u8 *out = destination;
    for (u16 i = 0; i < count; ++i) out[i] = source[i];
    return TRUE;
}

bool bank_audio_select(const u8 *physical, u32 length, bool play)
{
    u32 address = (u32)(unsigned long)physical;
    u32 offset = address & (BANK_SIZE - 1);
    if (!physical || !length || (address & 255) || address >= BANK_ROM_BYTES ||
        length > BANK_SIZE - offset || length > BANK_ROM_BYTES - address) return FALSE;
    /* A stop command alone is asynchronous. Public unload installs the idle
       driver and quiesces all old music/PCM readers before the mapper write. */
    Z80_unloadDriver();
    SYS_setBank(BANK_AUDIO_WINDOW, address / BANK_SIZE);
    const u8 *mapped = (const u8 *)(unsigned long)(BANK_AUDIO_WINDOW * BANK_SIZE + offset);
    XGM2_load(mapped);
    XGM2_setLoopNumber(-1);
    if (play) XGM2_playTrack(0);
    return TRUE;
}

static void read_record(u8 *bytes)
{
    for (u16 i = 0; i < BANK_SAVE_BYTES; ++i) bytes[i] = SRAM_readByte(i);
}

bool bank_store_write(const BankModel *model)
{
    u8 bytes[BANK_SAVE_BYTES], check[BANK_SAVE_BYTES];
    if (!bank_encode(model, bytes)) return FALSE;
    SRAM_enable();
    SRAM_writeByte(BANK_SAVE_BYTES - 1, 0); /* Invalidate while updating this single slot. */
    for (u16 i = 0; i < BANK_SAVE_BYTES; ++i) SRAM_writeByte(i, bytes[i]);
    read_record(check);
    SRAM_disable();
    for (u16 i = 0; i < BANK_SAVE_BYTES; ++i) if (check[i] != bytes[i]) return FALSE;
    return TRUE;
}

bool bank_store_read(BankModel *model)
{
    u8 bytes[BANK_SAVE_BYTES];
    SRAM_enableRO();
    read_record(bytes);
    SRAM_disable();
    return bank_decode(model, bytes);
}
