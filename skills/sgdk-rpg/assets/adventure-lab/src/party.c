#include "rpg.h"
static const Stats bases[3] = {{100, 20, 18, 7, 12}, {112, 18, 16, 10, 7}, {80, 32, 10, 5, 10}};
static const char *const names[] = {"Mara",       "Ivo",         "Neri", "Bell-shell Crab",
                                    "Chart Moth", "Tidal Orrery"};
static const char *const weapons[3][2] = {{"Keeper's Cutlass", "Storm Saber"},
                                          {"Tuning Wrench", "Copper Maul"},
                                          {"Star Chart", "Moon Lens"}};
static const char *const items[] = {"Tonic", "Ether", "Bell Seed"};
static const ArtDef arts[3][2] = {{{"Tidecut", "Cuts and soaks a foe", 4, false, false},
                                   {"Beacon", "A bright piercing strike", 7, false, false}},
                                  {{"Groundspark", "Double damage to soaked foes", 5, false, false},
                                   {"Dampen", "Weakens a foe for two turns", 3, false, false}},
                                  {{"Mend", "Restores 44 HP to an ally", 4, true, false},
                                   {"Wake", "Revives a fallen ally", 7, true, true}}};
uint8_t party_level(uint16_t xp)
{
    return xp >= 120 ? 4 : xp >= 65 ? 3 : xp >= 25 ? 2 : 1;
}
Stats member_stats(const Campaign *c, uint8_t member)
{
    if (member >= 3)
        return (Stats){0};
    Stats s = bases[member];
    uint16_t level = party_level(c->xp) - 1;
    s.hp += level * 8;
    s.mp += level * 2;
    s.attack += level * 3;
    s.defense += level;
    s.speed += level;
    if (c->party[member].weapon)
    {
        s.attack += 5;
        s.defense += 2;
        if (s.speed > 2)
            s.speed -= 2;
    }
    return s;
}
const char *species_name(Species s)
{
    return s <= ORRERY ? names[s] : "Unknown";
}
const char *weapon_name(uint8_t member, uint8_t weapon)
{
    return member < 3 && weapon < 2 ? weapons[member][weapon] : "Unknown";
}
const char *item_name(uint8_t item)
{
    return item < 3 ? items[item] : "Unknown";
}
const ArtDef *art_definition(uint8_t member, uint8_t choice)
{
    return member < 3 && choice < 2 ? &arts[member][choice] : 0;
}
void campaign_rest(Campaign *c)
{
    for (uint8_t i = 0; i < 3; i++)
    {
        Stats s = member_stats(c, i);
        c->party[i].hp = s.hp;
        c->party[i].mp = s.mp;
    }
}
void campaign_new(Campaign *c)
{
    *c = (Campaign){0};
    c->items[0] = 6;
    c->items[1] = 3;
    c->items[2] = 2;
    campaign_rest(c);
}
bool campaign_equip(Campaign *c, uint8_t member, uint8_t weapon)
{
    if (member >= 3 || weapon >= 2)
        return false;
    c->party[member].weapon = weapon;
    return true;
}
static uint16_t capped(uint16_t value, uint16_t add, uint16_t maximum)
{
    uint32_t result = (uint32_t)value + add;
    return result > maximum ? maximum : (uint16_t)result;
}
bool campaign_item(Campaign *c, uint8_t item, uint8_t target)
{
    if (item >= 3 || target >= 3 || !c->items[item])
        return false;
    Member *m = &c->party[target];
    Stats s = member_stats(c, target);
    if (item == 2)
    {
        if (m->hp)
            return false;
        m->hp = s.hp / 2;
    }
    else
    {
        if (!m->hp)
            return false;
        if (item == 0)
        {
            if (m->hp >= s.hp)
                return false;
            m->hp = capped(m->hp, 50, s.hp);
        }
        else
        {
            if (m->mp >= s.mp)
                return false;
            m->mp = capped(m->mp, 12, s.mp);
        }
    }
    c->items[item]--;
    return true;
}
