#include "checked.h"

u32 bp_square(u16 value)
{
    /* Cast an operand, not the result of a possibly overflowing multiply. */
    return (u32)value * value;
}

u16 bp_add_clamped(u16 value, u16 amount)
{
    const u32 sum = (u32)value + amount;
    return sum > 65535u ? 65535u : (u16)sum;
}

u16 bp_elapsed(u16 now, u16 then)
{
    /* Deliberate modulo-65536 result; caller must sample within one wrap. */
    return (u16)(now - then);
}

bool bp_layer(u16 index, u16 *mask)
{
    if (!mask || index >= 16) return false;
    *mask = (u16)((u32)1 << index);
    return true;
}

static bool valid(const BpRecord *record)
{
    return record && record->health <= 1000 && !(record->flags & ~7u);
}

bool bp_decode(const u8 *bytes, u16 length, BpRecord *out)
{
    if (!bytes || !out || length != 8 || bytes[0] != 1) return false;
    BpRecord record;
    record.flags = bytes[1];
    record.health = (u16)(((u16)bytes[2] << 8) | bytes[3]);
    record.score = ((u32)bytes[4] << 24) | ((u32)bytes[5] << 16) |
                   ((u32)bytes[6] << 8) | bytes[7];
    if (!valid(&record)) return false;
    *out = record;
    return true;
}

bool bp_encode(const BpRecord *record, u8 *bytes, u16 capacity)
{
    if (!bytes || capacity < 8 || !valid(record)) return false;
    bytes[0] = 1;
    bytes[1] = record->flags;
    bytes[2] = (u8)(record->health >> 8);
    bytes[3] = (u8)record->health;
    bytes[4] = (u8)(record->score >> 24);
    bytes[5] = (u8)(record->score >> 16);
    bytes[6] = (u8)(record->score >> 8);
    bytes[7] = (u8)record->score;
    return true;
}
