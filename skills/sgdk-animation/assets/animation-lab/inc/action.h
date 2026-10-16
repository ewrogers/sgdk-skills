#ifndef ACTION_H
#define ACTION_H
#ifdef AL_HOST
#include <stdint.h>
#include <stdbool.h>
typedef uint8_t u8;typedef uint16_t u16;typedef int16_t s16;typedef uint32_t u32;
#else
#include <genesis.h>
#endif

enum { AL_IDLE,AL_WALK,AL_ATTACK,AL_HURT,AL_DEAD,AL_CLIPS };
enum { AL_LEFT=1,AL_RIGHT=2,AL_A=4,AL_B=8,AL_KO=16 };
enum { AL_FOOT=1,AL_SWING=2,AL_HIT=4,AL_PAIN=8,AL_FALL=16,AL_END=32 };
typedef struct {s16 x,y,w,h;} AlBox;
typedef struct {u8 ticks,pose;u16 events;AlBox hit;} AlFrame;
typedef struct {const AlFrame *frames;u8 count;bool loop;} AlClip;
typedef struct {u16 age;u8 clip,frame,elapsed;bool entered,done;} AlClock;
typedef struct {
    AlClock clock;
    AlBox hit;
    s16 x;
    u16 attacks,hits;
    u8 pose,hp,buffer,flash;
    bool left,hit_latched;
} AlActor;
typedef struct {AlActor actor;u16 phase,previous,pending;u8 slow_phase;} AlGame;
typedef struct {u16 events[2];u8 count;} AlBatch;

extern const AlClip al_clips[AL_CLIPS];
bool al_play(AlClock *clock,u16 clip);
u16 al_clock_tick(AlClock *clock,AlFrame *shown);
void al_init(AlGame *game);
AlBox al_body(const AlActor *actor);
AlBox al_dummy(void);
bool al_overlap(AlBox a,AlBox b);
u16 al_tick(AlActor *actor,u16 held,u16 pressed);
void al_frame(AlGame *game,u16 held,u16 hz,bool paused,bool slow,AlBatch *batch);
#endif
