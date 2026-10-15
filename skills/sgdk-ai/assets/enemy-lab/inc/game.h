#ifndef ENEMY_GAME_H
#define ENEMY_GAME_H
#ifdef __m68k__
#include <types.h>
#else
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
#endif

#define GRID_W 18
#define GRID_H 9
#define GRID_CELLS (GRID_W*GRID_H)
#define ENEMY_COUNT 6
#define SHOT_COUNT 8
#define ROUTE_CAPACITY 16
#define NAV_BUDGET 16
#define LEVEL_COUNT 3
#define STUN_COOLDOWN 180
#define ACTOR_INSET 2
#define ACTOR_SIZE 12
#define BOMB_COUNT 2
#define BOMB_FLIGHT 30
#define BOMB_FUSE 60
#define BOMB_BLAST 18
#define BOMB_REACH 2
#define GRENADE_RANGE 96
#define GRENADE_RECOVERY 240
#define TASER_TICKS 12
#define TASER_RECOVERY 60
#define SENTRY_RECOVERY 180
#define NO_CELL 0xFFFF
enum { UP=1,DOWN=2,LEFT=4,RIGHT=8,A=16,B=32,C=64,START=128 };
enum { EVENT_STUN=1,EVENT_HIT=2,EVENT_JOIN=4,EVENT_ALERT=8,EVENT_FIRE=16,EVENT_CLEAR=32 };
typedef struct { s16 x,y; } Point;
typedef enum { BOMBARDIER,SHOCKER,SENTRY } Kind;
typedef enum { IDLE,PATROL,CHASE,RETURN,WINDUP,RECOVER,STUNNED,ZAP } State;
typedef enum { PLAYING,CAUGHT,AREA_CLEAR,GAME_OVER,COMPLETE,INTRO_BOMBARDIER,INTRO_SHOCKER,INTRO_SENTRY } Phase;
typedef struct {
    Point pos;
    u16 animation,invulnerable,pulse,cooldown,hits;
    u8 facing;
    bool active,moving,flip;
} Player;
typedef struct {
    Point pos,home,patrol,last_seen;
    u16 route[ROUTE_CAPACITY],serial,memory,timer,animation,repath,alert;
    u8 route_len,route_at,target,facing,aim;
    Kind kind;
    State state;
    bool moving,flip,outbound,pending;
} Enemy;
typedef struct { s16 x,y; u16 life; u8 direction; bool active; } Shot;
typedef enum { BOMB_FREE,BOMB_FLYING,BOMB_FUSED,BOMB_EXPLODING } BombPhase;
typedef struct { Point from,target; u16 timer; BombPhase phase; } Bomb;
typedef struct {
    u16 parent[GRID_CELLS],queue[GRID_CELLS],head,tail,start,goal,serial;
    Point origin;
    u16 completed,failed,cancelled;
    u8 owner,cursor,expanded;
    u8 level;
    bool active;
} Planner;
typedef struct {
    Player players[2];
    Enemy enemies[ENEMY_COUNT];
    Shot shots[SHOT_COUNT];
    Bomb bombs[BOMB_COUNT];
    Planner nav;
    u32 tick;
    u16 phase_ticks,tries;
    u8 overlay,events,sensors,level;
    Phase phase;
    bool paused;
} Game;
typedef struct { u16 held[2],pressed[2]; } Input;

void game_init(Game *game);
void game_new_run(Game *game);
void game_load_level(Game *game,u16 level);
void game_frame(Game *game,const Input *input,u16 ticks);
u16 game_clock(u16 *fraction,u16 hz);
void game_input(Input *input,u16 previous[2],const u16 held[2]);
void enemy_change(Enemy *enemy,State state,u16 duration);
void enemy_tick(Game *game,u16 index);
void enemy_sense(Game *game,u16 index);
void enemy_contact(Game *game,u16 index);
Point enemy_taser_target(const Enemy *enemy);
bool enemy_taser_hits(u16 level,const Enemy *enemy,const Player *player);
bool bomb_throw(Game *game,u16 owner,Point target);
void bombs_tick(Game *game);
u16 bomb_cells(u16 level,const Bomb *bomb,Point cells[1+4*BOMB_REACH]);
void player_hit(Game *game,u16 player);
void shots_tick(Game *game);
bool shot_fire(Game *game,Point from,u8 direction);
u8 aim_direction(s16 x,s16 y);
u16 enemy_range(Kind kind);
bool enemy_cone(u8 facing,s16 dx,s16 dy);
bool enemy_visible(u16 level,const Enemy *enemy,const Player *player,bool tracking);
void nav_frame(Game *game,u16 budget);
void nav_request(Enemy *enemy);

#endif
