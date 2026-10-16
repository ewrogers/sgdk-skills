#include <assert.h>
#include <string.h>
#include "view.h"
static u8 memory[SAVE_BYTES * 2];
static int mode, budget, attempts;
void SRAM_enable(void)
{
    assert(mode == 0);
    mode = 2;
}
void SRAM_enableRO(void)
{
    assert(mode == 0);
    mode = 1;
}
void SRAM_disable(void)
{
    assert(mode);
    mode = 0;
}
u8 SRAM_readByte(u32 offset)
{
    assert(mode && offset < sizeof memory);
    return memory[offset];
}
void SRAM_writeByte(u32 offset, u8 value)
{
    assert(mode == 2 && offset < sizeof memory);
    attempts++;
    if (budget)
    {
        memory[offset] = value;
        if (budget > 0)
            budget--;
    }
}
static void reset(void)
{
    memset(memory, 0xFF, sizeof memory);
    mode = 0;
    budget = -1;
    attempts = 0;
}
int main(void)
{
    Campaign first, second, third, loaded;
    campaign_new(&first);
    second = first;
    second.gold = 40;
    third = second;
    third.gold = 90;
    reset();
    loaded = first;
    assert(!save_load(&loaded) && loaded.gold == 0 && mode == 0);
    for (int cut = 0; cut <= 41; cut++)
    {
        reset();
        assert(save_write(&first) && save_write(&second));
        budget = cut;
        bool committed = save_write(&third);
        assert(committed == (cut == 41) && mode == 0);
        assert(save_load(&loaded) && loaded.gold == (cut == 41 ? 90 : 40) && mode == 0);
    }
    reset();
    campaign_encode(&first, memory, 65535);
    campaign_encode(&second, memory + SAVE_BYTES, 0);
    assert(save_load(&loaded) && loaded.gold == 40);
    assert(save_write(&third));
    assert(save_load(&loaded) && loaded.gold == 90);
    memory[0] ^= 1;
    assert(save_load(&loaded) && loaded.gold == 40);
    reset();
    Campaign invalid = first;
    invalid.party[0].weapon = 2;
    attempts = 0;
    assert(!save_write(&invalid) && attempts == 0 && mode == 0);
    return 0;
}
