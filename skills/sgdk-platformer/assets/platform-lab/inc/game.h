#ifndef PLATFORM_GAME_H
#define PLATFORM_GAME_H
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

#define UNIT 1024
#define CELL 16
#define WORLD_W 80
#define WORLD_H 18
#define ENEMY_COUNT 5
#define PICKUP_COUNT 10
#define COYOTE_TICKS 6
#define BUFFER_TICKS 6
#define CHARGE_TICKS 36
#define DASH_TICKS 12
#define MAX_FALL (8 * UNIT)
#define RUN_SPEED (11 * UNIT / 4)
#define RUN_ACCEL (UNIT * 9 / 20)
#define BRAKE (UNIT * 3 / 5)
#define GRAVITY (UNIT * 8 / 25)
#define JUMP_SPEED (-UNIT * 27 / 4)
#define SHORT_JUMP (-UNIT * 5 / 2)

enum { UP=1, DOWN=2, LEFT=4, RIGHT=8, B=16, C=32, A=64, START=128 };
typedef enum { EMPTY, SOLID, ONEWAY, RISE, FALL, BREAKABLE, COVER } Tile;
typedef enum { PATROL, WALKER, HOPPER, FLYER, ARMOR } EnemyKind;
enum { EVENT_JUMP=1, EVENT_STOMP=2, EVENT_HURT=4, EVENT_DASH=8,
       EVENT_PICKUP=16, EVENT_SECRET=32, EVENT_CHECKPOINT=64, EVENT_CLEAR=128 };
typedef struct {
    s32 x,y,vx,vy;
    u8 width,height;
    s16 wall,support;
    bool ground;
} Body;
typedef struct {
    Body body;
    s16 facing;
    u16 invulnerable,boost,charge;
    u8 coyote,buffer,drop,dash,lock,hearts;
    bool boots,battery,air_jump,air_dash,variable_jump,dashing;
} Player;
typedef struct {
    Body body;
    EnemyKind kind;
    s16 facing,home_x,home_y;
    u16 clock,retired;
    bool active,guarded;
} Enemy;
typedef struct { s32 x,y,previous_x; s16 direction; } Platform;
typedef struct { s16 x,y; u8 kind; bool collected; } Pickup;
typedef struct {
    Player player;
    Enemy enemies[ENEMY_COUNT];
    Platform platform;
    Pickup pickups[PICKUP_COUNT];
    u8 broken[(WORLD_W * WORLD_H + 7) / 8];
    u32 ticks;
    u16 held,clock,events,terrain_revision,score,deaths,celebration;
    s16 camera_x,camera_y;
    u8 checkpoint;
    bool paused,complete,secret,teleport;
} Game;

void game_init(Game *game);
void game_frame(Game *game,u16 held,u16 hz);
void game_step(Game *game,u16 held,u16 pressed);
void game_respawn(Game *game);
Tile game_tile(const Game *game,s16 x,s16 y);
Tile game_original_tile(s16 x,s16 y);
bool game_blocked(const Game *game,const Body *body,s32 x,s32 y);
bool game_overlap(const Body *a,const Body *b);
void game_move(Game *game,Body *body,s32 dx,s32 dy,bool step_up,bool break_blocks,u8 drop);
bool game_supported(const Game *game,s32 x,s32 feet);
void game_enemies_init(Game *game);
void game_enemies_step(Game *game);
void game_contacts(Game *game,s32 previous_feet,u16 held);
void game_hurt(Game *game,s16 direction);

#endif
