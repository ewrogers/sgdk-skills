#include <genesis.h>
#include "storage.h"

static bool read_byte(void *context, u32 offset, u8 *value)
{
    (void)context;
    if (offset >= SAVE_BYTES) return false;
    *value = SRAM_readByte(offset);
    return true;
}

static bool write_byte(void *context, u32 offset, u8 value)
{
    (void)context;
    if (offset >= SAVE_BYTES) return false;
    SRAM_writeByte(offset, value);
    return SRAM_readByte(offset) == value;
}

static const SaveIo io = {0, SAVE_BYTES, read_byte, write_byte};

SaveResult storage_load(SaveState *state, u8 *slot)
{
    SaveResult result;
    SRAM_enableRO();
    result = save_load(&io, state, slot);
    SRAM_disable();
    return result;
}

SaveResult storage_store(const SaveState *state)
{
    SaveResult result;
    SRAM_enable();
    result = save_store(&io, state);
    SRAM_disable();
    return result;
}

SaveResult storage_erase(void)
{
    SaveResult result;
    SRAM_enable();
    result = save_erase(&io);
    SRAM_disable();
    return result;
}

SaveResult storage_damage_newest(void)
{
    SaveState state;
    u8 slot;
    SaveResult result;
    SRAM_enable();
    result = save_load(&io, &state, &slot);
    if (result == SAVE_OK) {
        u32 offset = (u32)slot * SAVE_SLOT_BYTES + 16;
        if (!write_byte(0, offset, (u8)(SRAM_readByte(offset) ^ 1)))
            result = SAVE_IO_ERROR;
    }
    SRAM_disable();
    return result;
}
