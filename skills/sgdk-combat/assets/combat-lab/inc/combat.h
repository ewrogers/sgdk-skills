#ifndef COMBAT_LAB_H
#define COMBAT_LAB_H
#ifdef CC_HOST
#include <stdint.h>
#include <stdbool.h>
typedef uint8_t u8;typedef uint16_t u16;typedef int16_t s16;typedef uint32_t u32;typedef int32_t s32;
#else
#include <genesis.h>
#endif
#define CC_ACTORS 6
#define CC_SHOTS 4
#define CC_FX 6
#define CC_HITS 40
#define CC_NONE 255
enum {CC_LEFT=1,CC_RIGHT=2,CC_UP=4,CC_DOWN_KEY=8,CC_A=16,CC_B=32,CC_C=64,CC_START=128};
enum {CC_IDLE,CC_STRIKE,CC_GUARD,CC_CAST,CC_HURT,CC_DOWN};
enum {CC_PLAYER,CC_RAIDER,CC_SENTINEL,CC_ARCANIST};
enum {CC_TITLE,CC_PLAY,CC_CLEAR,CC_LOST,CC_COMPLETE};
enum {CC_SWING=1,CC_FIRE=2,CC_IMPACT=4,CC_BLOCK=8,CC_FALL=16,CC_JOIN=32,CC_WIN=64};
typedef struct {s16 x,y,w,h;} CcBox;
typedef struct {
    s16 x,y,kx,ky;u16 generation,hp,kind,team,facing,state,time,invulnerable,knock,down,bolt_cd,buffer,guard_cd,guard_wait,guard_hold;
    u16 hit_generation[CC_ACTORS],hit_mask;bool active;
} CcActor;
typedef struct {s16 x,y,dx,dy;u16 owner,generation,team,life;bool active;} CcShot;
typedef struct {s16 x,y;u16 life,kind;} CcFx;
typedef struct {
    u16 source,target,generation,team,damage;s16 dx,dy;bool guardable;
} CcHit;
typedef struct {
    CcActor actor[CC_ACTORS];CcShot shot[CC_SHOTS];CcFx fx[CC_FX];
    u16 previous[2],phase,mode,wave,hitstop,events,hit_count,block_count;u32 ticks,rng;
    bool joined[2],paused,friendly_fire;
} CcGame;
typedef struct {u16 held[2];} CcInput;
void cc_init(CcGame *g);
void cc_spawn(CcGame *g,u16 slot,u16 kind,u16 team,s16 x,s16 y);
void cc_round(CcGame *g,bool restart);
bool cc_alive(const CcActor *a);
bool cc_walkable(s16 x,s16 y);
bool cc_start(CcActor *a,u16 action);
bool cc_active(const CcActor *a);
CcBox cc_attack_box(const CcActor *a);
CcBox cc_swing_origin(const CcActor *a);
u16 cc_swing_frame(const CcActor *a);
bool cc_overlap(CcBox a,CcBox b);
bool cc_record(CcActor *a,u16 target,u16 generation);
bool cc_resolve(CcGame *g,const CcHit *hits,u16 count);
u16 cc_frame(CcGame *g,const CcInput *input,u16 hz);
u16 cc_enemies(const CcGame *g);
bool cc_invariant(const CcGame *g);
#endif
