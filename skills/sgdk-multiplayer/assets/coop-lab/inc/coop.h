#ifndef COOP_LAB_H
#define COOP_LAB_H
#ifdef CP_HOST
#include <stdint.h>
#include <stdbool.h>
typedef uint8_t u8;typedef uint16_t u16;typedef int16_t s16;typedef uint32_t u32;typedef int32_t s32;
#else
#include <genesis.h>
#endif
#define CP_PLAYERS 4
#define CP_DEVICES 8
#define CP_COINS 8
#define CP_ENEMIES 4
#define CP_NONE 255
#define CP_WORLD_W 512
#define CP_WORLD_H 384
#define CP_VIEW_W 320
#define CP_VIEW_H 184
#define CP_SPAN_X 256
#define CP_SPAN_Y 112
enum {CP_LEFT=1,CP_RIGHT=2,CP_UP=4,CP_DOWN=8,CP_A=16,CP_B=32,CP_C=64,CP_START=128};
enum {CP_PORT_OTHER,CP_PORT_PAD,CP_PORT_TEAM,CP_PORT_EA};
enum {CP_READER_OFF,CP_READER_PAD,CP_READER_TEAM,CP_READER_EA};
enum {CP_JOIN=1,CP_LEAVE=2,CP_HIT=4,CP_RESPAWN=8,CP_STUN=16,CP_PICKUP=32,CP_CLEAR=64,CP_PAUSE=128};
typedef struct {s16 x,y;} CpPoint;
typedef struct {u16 held[CP_DEVICES],valid;} CpInput;
typedef struct {u16 reader[2],mask;} CpPlan;
typedef struct {
    s16 x,y;u16 hp,respawn,invulnerable,dash,dash_cd,stun_cd,walk,facing,gems,deaths;
    u8 device;bool moving;
} CpPlayer;
typedef struct {
    CpPlayer player[CP_PLAYERS];u16 previous[CP_DEVICES];
    u16 enemy_stun[CP_ENEMIES],enemy_clock[CP_ENEMIES],phase,score,tie,events,pause_owner;
    s16 camera_x,camera_y;u32 ticks;u16 coin_mask;
    bool running,paused,won,tethered;
} CpGame;
extern const char cp_map[24][33];
extern const CpPoint cp_coins[CP_COINS],cp_beacons[4];
CpPlan cp_plan(u16 port1,u16 port2);
bool cp_walkable(s16 x,s16 y);
void cp_init(CpGame *g);
void cp_seed_inputs(CpGame *g,const CpInput *input);
u16 cp_count(const CpGame *g);
u16 cp_stun_charge(const CpPlayer *player);
CpPoint cp_enemy(const CpGame *g,u16 index);
bool cp_damage(CpGame *g,u16 player);
void cp_camera(CpGame *g);
u16 cp_frame(CpGame *g,const CpInput *input,u16 hz);
bool cp_invariant(const CpGame *g);
void cp_demo_input(const CpGame *g,u32 frame,CpInput *input);
#endif
