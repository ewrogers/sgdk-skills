#include "rpg.h"
static void put16(uint8_t *p, uint16_t value)
{
    p[0] = value >> 8;
    p[1] = value & 255;
}
static uint16_t get16(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | p[1];
}
static uint16_t crc16(const uint8_t *bytes, uint16_t count)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t n = 0; n < count; n++)
    {
        crc ^= (uint16_t)bytes[n] << 8;
        for (uint8_t bit = 0; bit < 8; bit++)
            crc = (uint16_t)(((uint32_t)crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0));
    }
    return crc;
}
void campaign_encode(const Campaign *c, uint8_t out[SAVE_BYTES], uint16_t sequence)
{
    for (uint8_t i = 0; i < SAVE_BYTES; i++)
        out[i] = 0;
    out[0] = 'B';
    out[1] = 'E';
    out[2] = 'L';
    out[3] = 'L';
    out[4] = 1;
    out[5] = c->route;
    out[6] = c->complete ? 1 : 0;
    put16(out + 8, sequence);
    put16(out + 10, c->xp);
    put16(out + 12, c->gold);
    for (uint8_t i = 0; i < 3; i++)
    {
        out[14 + i] = c->items[i];
        put16(out + 17 + i * 5, c->party[i].hp);
        put16(out + 19 + i * 5, c->party[i].mp);
        out[21 + i * 5] = c->party[i].weapon;
    }
    put16(out + 38, crc16(out, 38));
}
bool campaign_decode(Campaign *c, const uint8_t in[SAVE_BYTES], uint16_t *sequence)
{
    if (in[0] != 'B' || in[1] != 'E' || in[2] != 'L' || in[3] != 'L' || in[4] != 1 ||
        get16(in + 38) != crc16(in, 38))
        return false;
    if (in[5] > 2 || in[6] > 1 || in[7] || in[32] || in[33] || in[34] || in[35] || in[36] || in[37])
        return false;
    Campaign candidate = {0};
    candidate.route = in[5];
    candidate.complete = in[6] != 0;
    candidate.xp = get16(in + 10);
    candidate.gold = get16(in + 12);
    if (candidate.xp > 9999 || candidate.gold > 9999)
        return false;
    bool survivor = false;
    for (uint8_t i = 0; i < 3; i++)
    {
        candidate.items[i] = in[14 + i];
        Member *m = &candidate.party[i];
        m->hp = get16(in + 17 + i * 5);
        m->mp = get16(in + 19 + i * 5);
        m->weapon = in[21 + i * 5];
        if (candidate.items[i] > 99 || m->weapon > 1)
            return false;
        Stats s = member_stats(&candidate, i);
        if (m->hp > s.hp || m->mp > s.mp)
            return false;
        if (m->hp)
            survivor = true;
    }
    if (!survivor || (candidate.complete && candidate.route != 2))
        return false;
    *c = candidate;
    if (sequence)
        *sequence = get16(in + 8);
    return true;
}
