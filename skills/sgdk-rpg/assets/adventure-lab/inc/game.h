#ifndef ADVENTURE_GAME_H
#define ADVENTURE_GAME_H
#include "rpg.h"
typedef enum
{
    TITLE,
    HELP,
    CAMP,
    ROUTE,
    PARTY,
    BATTLE,
    RESULT,
    CREDITS
} Screen;
typedef enum
{
    COMMAND,
    ARTS,
    PACK,
    TARGET,
    RESOLVE
} BattleMenu;
enum
{
    UP = 1,
    DOWN = 2,
    LEFT = 4,
    RIGHT = 8,
    A = 16,
    B = 32,
    C = 64,
    START = 128
};
enum
{
    SFX_MOVE = 1,
    SFX_ACCEPT = 2,
    SFX_BACK = 4,
    SFX_DENIED = 8,
    SFX_HIT = 16,
    SFX_SPELL = 32,
    SFX_HEAL = 64,
    SFX_WIN = 128,
    SFX_LOSE = 256,
    SFX_SAVE = 512
};
typedef struct
{
    Campaign campaign, checkpoint;
    Battle battle;
    Screen screen;
    BattleMenu battle_menu, return_menu;
    uint16_t selection, member, tab, item, events;
    uint16_t elapsed, repeat;
    uint32_t ticks;
    int16_t x, y;
    uint8_t direction, walk_frame;
    Action pending;
    bool moving, paused, dirty, saved, save_request, load_request;
    const char *notice;
} Game;
typedef struct
{
    uint16_t remainder, held_age, last_dpad;
} FrameClock;
uint16_t clock_step(FrameClock *clock, uint16_t hz, uint16_t held, uint16_t pressed,
                    uint16_t *navigation);
bool battle_position(const Battle *battle, uint16_t actor, int16_t *x, int16_t *y);
void game_init(Game *game, bool saved);
void game_step(Game *game, uint16_t held, uint16_t pressed, uint16_t navigation, uint16_t ticks);
void game_load(Game *game, const Campaign *campaign);
const char *encounter_name(uint8_t encounter);
#endif
