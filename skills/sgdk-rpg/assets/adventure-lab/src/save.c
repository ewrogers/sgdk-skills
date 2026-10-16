#include "view.h"
static bool newer(u16 a, u16 b)
{
    u16 distance = a - b;
    return distance != 0 && distance < 0x8000;
}
static void read_slot(u16 slot, u8 bytes[SAVE_BYTES])
{
    for (u16 i = 0; i < SAVE_BYTES; i++)
        bytes[i] = SRAM_readByte(slot * SAVE_BYTES + i);
}
static bool load(Campaign *campaign, u16 *sequence, u16 *slot)
{
    Campaign candidates[2];
    u16 seq[2];
    bool valid[2];
    u8 bytes[SAVE_BYTES];
    SRAM_enableRO();
    for (u16 i = 0; i < 2; i++)
    {
        read_slot(i, bytes);
        valid[i] = campaign_decode(&candidates[i], bytes, &seq[i]);
    }
    SRAM_disable();
    if (!valid[0] && !valid[1])
        return false;
    u16 latest = !valid[0] ? 1 : !valid[1] ? 0 : newer(seq[1], seq[0]) ? 1 : 0;
    *campaign = candidates[latest];
    if (sequence)
        *sequence = seq[latest];
    if (slot)
        *slot = latest;
    return true;
}
bool save_load(Campaign *campaign)
{
    return load(campaign, 0, 0);
}
bool save_write(const Campaign *campaign)
{
    Campaign old, verify;
    u8 bytes[SAVE_BYTES], readback[SAVE_BYTES];
    campaign_encode(campaign, bytes, 0);
    if (!campaign_decode(&verify, bytes, 0))
        return false;
    u16 sequence = 0, slot = 1;
    bool existing = load(&old, &sequence, &slot);
    u16 next = existing ? (u16)(sequence + 1) : 0, target = slot ^ 1;
    campaign_encode(campaign, bytes, next);
    SRAM_enable();
    SRAM_writeByte(target * SAVE_BYTES, 0);
    for (u16 i = 1; i < SAVE_BYTES; i++)
        SRAM_writeByte(target * SAVE_BYTES + i, bytes[i]);
    /* Signature byte is the final commit, leaving the older slot untouched. */
    SRAM_writeByte(target * SAVE_BYTES, bytes[0]);
    SRAM_disable();
    SRAM_enableRO();
    read_slot(target, readback);
    SRAM_disable();
    u16 found = 0;
    for (u16 i = 0; i < SAVE_BYTES; i++)
        if (readback[i] != bytes[i])
            return false;
    return campaign_decode(&verify, readback, &found) && found == next;
}
