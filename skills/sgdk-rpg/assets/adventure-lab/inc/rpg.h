#ifndef ADVENTURE_RPG_H
#define ADVENTURE_RPG_H
#ifdef SGDK_GCC
#include <types.h>
typedef u8 uint8_t;
typedef u16 uint16_t;
typedef u32 uint32_t;
typedef s16 int16_t;
#else
#include <stdint.h>
#include <stdbool.h>
#endif
#define PARTY_COUNT 3
#define ACTOR_COUNT 6
#define ITEM_COUNT 3
#define SAVE_BYTES 40

typedef enum
{
    MARA,
    IVO,
    NERI,
    CRAB,
    MOTH,
    ORRERY
} Species;
typedef enum
{
    STRIKE,
    ART,
    ITEM,
    GUARD
} ActionKind;
typedef enum
{
    PHYSICAL,
    TIDE,
    SPARK
} Element;
typedef enum
{
    LIVE,
    WON,
    LOST
} Outcome;
typedef enum
{
    HIT,
    HEAL,
    RESTORE,
    REVIVE,
    BRACE,
    WEAKEN,
    CHARGE
} EventKind;
typedef struct
{
    uint16_t hp, mp, attack, defense, speed;
} Stats;
typedef struct
{
    uint16_t hp, mp;
    uint8_t weapon;
} Member;
typedef struct
{
    Member party[PARTY_COUNT];
    uint16_t xp, gold;
    uint8_t items[ITEM_COUNT], route;
    bool complete;
} Campaign;
typedef struct
{
    Species species;
    Stats stats;
    uint16_t hp, mp;
    uint8_t soaked, weakened;
    bool guarding, charging;
} Fighter;
typedef struct
{
    ActionKind kind;
    uint8_t choice, target;
} Action;
typedef struct
{
    EventKind kind;
    uint8_t source, target;
    uint16_t amount;
    bool combo, critical;
} Event;
typedef struct
{
    Fighter actors[ACTOR_COUNT];
    uint8_t order[ACTOR_COUNT], cursor, round, encounter;
    uint32_t rng;
    Outcome outcome;
    bool reward_claimed;
    Event event;
} Battle;
typedef struct
{
    const char *name, *description;
    uint8_t cost;
    bool ally, dead;
} ArtDef;

/* Definitions and derived stats; immutable tables stay in ROM. */
uint8_t party_level(uint16_t xp);
Stats member_stats(const Campaign *campaign, uint8_t member);
const char *species_name(Species species);
const char *weapon_name(uint8_t member, uint8_t weapon);
const char *item_name(uint8_t item);
const ArtDef *art_definition(uint8_t member, uint8_t choice);

/* Persistent campaign operations. Rejected item uses consume nothing. */
void campaign_new(Campaign *campaign);
void campaign_rest(Campaign *campaign);
bool campaign_equip(Campaign *campaign, uint8_t member, uint8_t weapon);
bool campaign_item(Campaign *campaign, uint8_t item, uint8_t target);

/* Committed actions advance once; presentation reads the recorded event. */
void battle_begin(Battle *battle, const Campaign *campaign, uint8_t encounter, uint32_t seed);
uint8_t battle_actor(const Battle *battle);
bool battle_target(const Battle *battle, Action action, uint8_t target);
bool battle_commit(Battle *battle, Campaign *campaign, Action action);
bool battle_enemy(Battle *battle);
bool battle_claim(Battle *battle, Campaign *campaign);
void battle_sync(const Battle *battle, Campaign *campaign);

/* Versioned byte format. Failed decoding leaves the destination unchanged. */
void campaign_encode(const Campaign *campaign, uint8_t bytes[SAVE_BYTES], uint16_t sequence);
bool campaign_decode(Campaign *campaign, const uint8_t bytes[SAVE_BYTES], uint16_t *sequence);
#endif
