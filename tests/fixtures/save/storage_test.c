#include <assert.h>
#include <string.h>
#include "storage.h"

static u8 physical[SAVE_BYTES * 2];
static int mode, opens, closes;
static bool reject_writes;

void SRAM_enable(void) { assert(mode == 0); mode = 1; opens++; }
void SRAM_enableRO(void) { assert(mode == 0); mode = 3; opens++; }
void SRAM_disable(void) { assert(mode != 0); mode = 0; closes++; }
u8 SRAM_readByte(u32 offset)
{
    assert(mode != 0 && offset < SAVE_BYTES);
    return physical[offset * 2 + 1];
}
void SRAM_writeByte(u32 offset, u8 value)
{
    assert(mode == 1 && offset < SAVE_BYTES);
    if (!reject_writes) physical[offset * 2 + 1] = value;
}

int main(void)
{
    SaveState state = {200, 2, 4, true}, loaded;
    u8 slot;
    unsigned i;
    memset(physical, 0xFF, sizeof(physical));
    assert(storage_load(&loaded, 0) == SAVE_EMPTY && mode == 0);
    assert(storage_store(&state) == SAVE_OK && mode == 0);
    state.score = 300;
    assert(storage_store(&state) == SAVE_OK && mode == 0);
    assert(storage_load(&loaded, &slot) == SAVE_OK && slot == 1 && loaded.score == 300);
    assert(storage_damage_newest() == SAVE_OK && mode == 0);
    assert(storage_load(&loaded, &slot) == SAVE_OK && slot == 0 && loaded.score == 200);
    reject_writes = true;
    assert(storage_store(&state) == SAVE_IO_ERROR && mode == 0);
    assert(storage_erase() == SAVE_IO_ERROR && mode == 0);
    reject_writes = false;
    assert(storage_erase() == SAVE_OK && mode == 0);
    assert(storage_damage_newest() == SAVE_EMPTY && mode == 0);
    for (i = 0; i < sizeof(physical); i += 2) assert(physical[i] == 0xFF);
    assert(opens == closes);
    return 0;
}
