#include "rpg.h"
static const Stats enemy_stats[3] = {{44, 0, 14, 6, 6}, {34, 0, 11, 3, 13}, {220, 0, 24, 8, 8}};
static uint32_t next_random(Battle *b)
{
    uint32_t x = b->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    b->rng = x;
    return x;
}
static void outcome(Battle *b)
{
    bool party = false, enemy = false;
    for (uint8_t i = 0; i < 6; i++)
        if (b->actors[i].hp)
        {
            if (i < 3)
                party = true;
            else
                enemy = true;
        }
    b->outcome = !party ? LOST : !enemy ? WON : LIVE;
}
static void round_begin(Battle *b)
{
    if (b->round < 255)
        b->round++;
    for (uint8_t i = 0; i < 6; i++)
        b->order[i] = i;
    for (uint8_t i = 1; i < 6; i++)
    {
        uint8_t key = b->order[i], at = i;
        while (at && b->actors[b->order[at - 1]].stats.speed < b->actors[key].stats.speed)
        {
            b->order[at] = b->order[at - 1];
            at--;
        }
        b->order[at] = key;
    }
    b->cursor = 0;
}
static void advance(Battle *b)
{
    outcome(b);
    if (b->outcome != LIVE)
        return;
    do
    {
        b->cursor++;
        if (b->cursor == 6)
            round_begin(b);
    } while (!b->actors[b->order[b->cursor]].hp);
    b->actors[b->order[b->cursor]].guarding = false;
}
uint8_t battle_actor(const Battle *b)
{
    return b->order[b->cursor];
}
void battle_begin(Battle *b, const Campaign *c, uint8_t encounter, uint32_t seed)
{
    *b = (Battle){0};
    b->encounter = encounter > 2 ? 2 : encounter;
    b->rng = seed ? seed : 1;
    for (uint8_t i = 0; i < 3; i++)
    {
        Fighter *f = &b->actors[i];
        f->species = (Species)i;
        f->stats = member_stats(c, i);
        f->hp = c->party[i].hp;
        f->mp = c->party[i].mp;
    }
    static const Species sets[3][3] = {
        {CRAB, CRAB, CRAB}, {CRAB, MOTH, MOTH}, {ORRERY, MOTH, MOTH}};
    uint8_t count = b->encounter == 0 ? 2 : b->encounter == 1 ? 3 : 2;
    for (uint8_t i = 0; i < count; i++)
    {
        Fighter *f = &b->actors[i + 3];
        f->species = sets[b->encounter][i];
        f->stats = enemy_stats[f->species - CRAB];
        f->hp = f->stats.hp;
    }
    round_begin(b);
    outcome(b);
    if (b->outcome == LIVE)
        while (!b->actors[battle_actor(b)].hp)
            b->cursor++;
}
bool battle_target(const Battle *b, Action a, uint8_t target)
{
    if (target >= 6 || b->outcome != LIVE)
        return false;
    uint8_t actor = battle_actor(b);
    if (actor >= 3)
        return false;
    if (a.kind == GUARD)
        return target == actor;
    bool ally = false, dead = false;
    if (a.kind == ART)
    {
        const ArtDef *art = art_definition(actor, a.choice);
        if (!art)
            return false;
        ally = art->ally;
        dead = art->dead;
    }
    else if (a.kind == ITEM)
    {
        if (a.choice >= 3)
            return false;
        ally = true;
        dead = a.choice == 2;
    }
    else if (a.kind != STRIKE)
        return false;
    return (target < 3) == ally && (dead ? !b->actors[target].hp : b->actors[target].hp > 0);
}
static void finish_status(Fighter *f)
{
    if (f->soaked)
        f->soaked--;
    if (f->weakened)
        f->weakened--;
}
static void damage(Battle *b, uint8_t source, uint8_t target, uint16_t power, Element element)
{
    Fighter *attacker = &b->actors[source], *defender = &b->actors[target];
    uint32_t value = power;
    if (attacker->weakened)
        value = value * 2 / 3;
    value = value > defender->stats.defense ? value - defender->stats.defense : 1;
    bool combo = element == SPARK && defender->soaked;
    if (combo)
        value *= 2;
    bool critical = (next_random(b) & 15) == 0;
    if (critical)
        value = value * 3 / 2;
    if (defender->guarding)
        value = (value + 1) / 2;
    if (value > defender->hp)
        value = defender->hp;
    defender->hp -= (uint16_t)value;
    if (element == TIDE && defender->hp)
        defender->soaked = 2;
    if (!defender->hp)
    {
        defender->charging = false;
        defender->guarding = false;
        defender->soaked = 0;
        defender->weakened = 0;
    }
    b->event = (Event){HIT, source, target, (uint16_t)value, combo, critical};
}
bool battle_commit(Battle *b, Campaign *c, Action a)
{
    if (!battle_target(b, a, a.target))
        return false;
    uint8_t actor = battle_actor(b);
    Fighter *f = &b->actors[actor], *target = &b->actors[a.target];
    const ArtDef *art = a.kind == ART ? art_definition(actor, a.choice) : 0;
    if (art && f->mp < art->cost)
        return false;
    if (a.kind == ITEM)
    {
        if (!c->items[a.choice])
            return false;
        if ((a.choice == 0 && target->hp == target->stats.hp) ||
            (a.choice == 1 && target->mp == target->stats.mp))
            return false;
        c->items[a.choice]--;
        uint16_t before = a.choice == 1 ? target->mp : target->hp;
        if (a.choice == 0)
        {
            uint32_t v = (uint32_t)target->hp + 50;
            target->hp = v > target->stats.hp ? target->stats.hp : v;
        }
        else if (a.choice == 1)
        {
            uint32_t v = (uint32_t)target->mp + 12;
            target->mp = v > target->stats.mp ? target->stats.mp : v;
        }
        else
        {
            target->hp = target->stats.hp / 2;
            target->guarding = false;
        }
        b->event = (Event){a.choice == 1 ? RESTORE : a.choice == 2 ? REVIVE : HEAL, actor, a.target,
                           (a.choice == 1 ? target->mp : target->hp) - before,      false, false};
    }
    else if (a.kind == GUARD)
    {
        f->guarding = true;
        b->event = (Event){BRACE, actor, actor, 0, false, false};
    }
    else if (a.kind == STRIKE)
        damage(b, actor, a.target, f->stats.attack, PHYSICAL);
    else
    {
        /* Validation above is complete before MP, RNG or target state changes. */
        f->mp -= art->cost;
        if (actor == NERI)
        {
            uint16_t before = target->hp;
            uint32_t v = a.choice ? target->stats.hp / 2 : (uint32_t)target->hp + 44;
            target->hp = v > target->stats.hp ? target->stats.hp : v;
            b->event = (Event){a.choice ? REVIVE : HEAL, actor, a.target,
                               target->hp - before,      false, false};
        }
        else if (actor == IVO && a.choice == 1)
        {
            target->weakened = 2;
            b->event = (Event){WEAKEN, actor, a.target, 0, false, false};
        }
        else
            damage(b, actor, a.target, f->stats.attack + (actor == MARA ? (a.choice ? 18 : 6) : 12),
                   actor == IVO ? SPARK
                   : a.choice   ? PHYSICAL
                                : TIDE);
    }
    finish_status(f);
    advance(b);
    return true;
}
bool battle_enemy(Battle *b)
{
    if (b->outcome != LIVE)
        return false;
    uint8_t actor = battle_actor(b);
    if (actor < 3)
        return false;
    Fighter *f = &b->actors[actor];
    uint8_t target = (uint8_t)(next_random(b) % 3);
    for (uint8_t i = 0; i < 3 && !b->actors[target].hp; i++)
        target = (target + 1) % 3;
    if (f->species == ORRERY && !f->charging && b->round % 3 == 1)
    {
        f->charging = true;
        b->event = (Event){CHARGE, actor, actor, 0, false, false};
    }
    else
    {
        bool charged = f->charging;
        f->charging = false;
        damage(b, actor, target, f->stats.attack + (charged ? 22 : 0), PHYSICAL);
        if (f->species == MOTH && b->actors[target].hp)
            b->actors[target].weakened = 2;
    }
    finish_status(f);
    advance(b);
    return true;
}
void battle_sync(const Battle *b, Campaign *c)
{
    for (uint8_t i = 0; i < 3; i++)
    {
        c->party[i].hp = b->actors[i].hp;
        c->party[i].mp = b->actors[i].mp;
    }
}
bool battle_claim(Battle *b, Campaign *c)
{
    if (b->outcome != WON || b->reward_claimed)
        return false;
    b->reward_claimed = true;
    battle_sync(b, c);
    uint8_t old = party_level(c->xp);
    uint32_t xp = (uint32_t)c->xp + (b->encounter == 0 ? 25 : b->encounter == 1 ? 40 : 60);
    uint32_t gold = (uint32_t)c->gold + 30 + b->encounter * 25;
    c->xp = xp > 9999 ? 9999 : xp;
    c->gold = gold > 9999 ? 9999 : gold;
    uint16_t levels = party_level(c->xp) - old;
    for (uint8_t i = 0; i < 3; i++)
    {
        if (c->party[i].hp)
            c->party[i].hp += levels * 8;
        c->party[i].mp += levels * 2;
    }
    if (c->items[0] < 99)
        c->items[0]++;
    if (c->items[1] < 99)
        c->items[1]++;
    if (c->route == b->encounter)
    {
        if (c->route < 2)
            c->route++;
        else
            c->complete = true;
    }
    return true;
}
