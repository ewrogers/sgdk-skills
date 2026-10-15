#include <genesis.h>
#include "storage.h"
static bool read_byte(u16 offset, u8 *value)
{
    if (offset>=CHECKPOINT_BYTES) return false;
    *value=SRAM_readByte(offset); return true;
}
static bool write_byte(u16 offset, u8 value)
{
    if (offset>=CHECKPOINT_BYTES) return false;
    SRAM_writeByte(offset,value); return SRAM_readByte(offset)==value;
}
static const CheckpointIo io={read_byte,write_byte};
bool storage_load(Checkpoint *value)
{
    SRAM_enableRO(); bool ok=checkpoint_load(&io,value); SRAM_disable(); return ok;
}
bool storage_save(const Checkpoint *value)
{
    SRAM_enable(); bool ok=checkpoint_store(&io,value); SRAM_disable(); return ok;
}
