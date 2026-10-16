#ifndef CARPET_GAME_H
#define CARPET_GAME_H
#ifdef __m68k__
#include <types.h>
#else
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef int16_t s16; typedef int32_t s32;
#endif
#define UNIT 16
#define SHOT_COUNT 20
#define BULLET_COUNT 24
#define ENEMY_COUNT 8
#define GEM_COUNT 6
#define EFFECT_COUNT 8
#define STAGE_TICKS 2700
#define BOSS_HP 160
#define HUD_HEIGHT 16
#define RIBBON_REACH 192

enum { UP=1,DOWN=2,LEFT=4,RIGHT=8,B=16,C=32,A=64,START=128 };
enum { EV_FIRE=1,EV_GEM=2,EV_UPGRADE=4,EV_HIT=8,EV_KILL=16,EV_BOSS=32,EV_CLEAR=64,EV_DENIED=128,EV_SELECT=256,EV_START=512,EV_GUARD=1024,EV_RIBBON=2048 };
typedef enum { TITLE,HOWTO,LOADOUT,STAGECARD,PLAY,BOSS,CLEAR,OVER,CREDITS } State;
typedef enum { SUNFIRE,MOONWEAVE,STORMGLASS } Loadout;
typedef struct { s16 x,y; } Point;
typedef struct { s16 x,y,vx,vy; u16 age,hit_mask; u8 kind,damage; bool active; } Shot;
typedef struct { s16 x,y,home; u16 age; u8 hp,kind; bool active; } Enemy;
typedef struct { s16 x,y; u16 age; bool active; } Gem;
typedef struct { s16 x,y; u8 age,kind; } Effect;
typedef struct { s16 x,y; u16 age,hp; u8 phase; bool active; } Boss;
typedef struct {
    Shot shots[SHOT_COUNT],bullets[BULLET_COUNT];
    Enemy enemies[ENEMY_COUNT]; Gem gems[GEM_COUNT]; Effect effects[EFFECT_COUNT];
    Boss boss; Point history[32];
    s16 x,y;
    u32 score,ticks;
    u16 age,stage_time,held,clock,events,invulnerable,spawn_index,enemy_kills;
    u8 stage,lives,speed,weapon,familiars,barrier,meter,fire_wait,history_head,ribbon_wait;
    State state; Loadout loadout;
    bool paused,ribbon_active;
} Game;
extern const s16 direction_x[16],direction_y[16];
void game_init(Game *game);
void game_begin(Game *game,Loadout loadout);
void game_frame(Game *game,u16 held,u16 hz);
void game_step(Game *game,u16 held,u16 pressed);
void game_stage_begin(Game *game,u8 stage);
void game_player(Game *game,u16 held,bool shooting);
void game_combat(Game *game);
void game_encounters(Game *game);
void game_hurt(Game *game);
void game_boss_damage(Game *game,u16 damage);
void game_enemy_kill(Game *game,u16 index);
void game_effect(Game *game,s16 x,s16 y,u8 kind);
void game_gem(Game *game,s16 x,s16 y);
void game_upgrade(Game *game);
bool game_upgrade_available(const Game *game,u16 slot);
void game_shot_motion(Game *game,Shot *shot);
void game_chain(Game *game,Shot *shot);
void game_ribbon(Game *game);
bool game_emit(Game *game,bool player,s16 x,s16 y,s16 vx,s16 vy,u8 kind,u8 damage);
u16 game_aim(s16 dx,s16 dy);
void game_familiar(const Game *game,u16 index,s16 *x,s16 *y);
bool game_boss_guarded(const Game *game);
#endif
