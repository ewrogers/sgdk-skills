#ifndef SHOOTER_GAME_H
#define SHOOTER_GAME_H
#ifdef __m68k__
#include <types.h>
#else
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;
#endif

#define UNIT 16
#define FIELD_W 240
#define FIELD_H 224
#define SHOT_COUNT 12
#define BULLET_COUNT 32
#define ENEMY_COUNT 6
#define PICKUP_COUNT 3
#define EFFECT_COUNT 6
#define WAVE_COUNT 6
#define BOSS_HEALTH 90

enum { UP=1,DOWN=2,LEFT=4,RIGHT=8,B=16,C=32,A=64,START=128 };
enum { EVENT_FIRE=1,EVENT_KILL=2,EVENT_HURT=4,EVENT_PICKUP=8,EVENT_BOMB=16,
       EVENT_BOSS=32,EVENT_CLEAR=64,EVENT_START=128 };
typedef enum { TITLE,PLAY,BOSS,CLEAR,OVER } State;
typedef enum { SCOUT,SWEEPER,TURRET } EnemyKind;
typedef enum { AIMED,FAN,RING,SPIRAL } Pattern;
typedef struct { s16 x,y,vx,vy; bool active; } Bullet;
typedef struct {
    s16 x,y,home;
    u16 age;
    u8 hp,flash;
    EnemyKind kind;
    bool active;
} Enemy;
typedef struct { s16 x,y; u8 kind; bool active; } Pickup;
typedef struct { s16 x,y,vx,vy; u8 life; } Effect;
typedef struct {
    s16 x,y;
    u16 age;
    u8 hp,flash;
    bool active;
} Boss;
typedef struct {
    Bullet shots[SHOT_COUNT],bullets[BULLET_COUNT];
    Enemy enemies[ENEMY_COUNT];
    Pickup pickups[PICKUP_COUNT];
    Effect effects[EFFECT_COUNT];
    Boss boss;
    s16 x,y;
    u32 ticks,score;
    u16 held,clock,events,spawn_index,invulnerable,bomb_flash,phase_time,denied;
    u8 lives,power,bombs,fire_wait,combo,kills,wave;
    State state;
    bool paused;
} Game;

typedef struct { u16 at; s16 x; EnemyKind kind; } Spawn;
extern const Spawn stage_spawns[24];
extern const s16 direction_x[16],direction_y[16];
bool game_video_layout_valid(u16 width,u16 height,u16 a,u16 b,u16 window);
void game_init(Game *game);
void game_start(Game *game);
void game_frame(Game *game,u16 held,u16 hz);
void game_step(Game *game,u16 held,u16 pressed);
bool game_shot(Game *game,s16 x,s16 y,s16 vx,s16 vy);
bool game_bullet(Game *game,s16 x,s16 y,u16 angle,s16 speed);
u16 game_pattern(Game *game,Pattern pattern,s16 x,s16 y,s16 target_x,s16 target_y,u16 phase);
u16 game_aim(s16 dx,s16 dy);
void game_projectiles(Game *game);
void game_enemies(Game *game);
void game_hurt(Game *game);
void game_enemy_kill(Game *game,u16 index);
void game_boss_damage(Game *game,u16 damage);
void game_add_score(Game *game,u32 amount);
void game_effect(Game *game,s16 x,s16 y);
void game_stage(Game *game);
#endif
