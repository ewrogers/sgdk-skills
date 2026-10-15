#ifndef TRANSITION_LAB_H
#define TRANSITION_LAB_H
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

typedef enum {
    SCENE_TITLE, SCENE_CONTINUE, SCENE_OPTIONS, SCENE_CREDITS,
    SCENE_SELECT, SCENE_PLAY, SCENE_COUNT
} SceneId;
enum { KEY_UP=1, KEY_DOWN=2, KEY_LEFT=4, KEY_RIGHT=8,
       KEY_ACCEPT=16, KEY_BACK=32, KEY_START=64 };
enum { ACTION_NONE=0, ACTION_SCENE=1, ACTION_LOAD=2,
       ACTION_OPTIONS=4 };
#define COIN_COUNT 8
#define ROOM_COUNT 4
#define LASER_CAPACITY 4
#define JOIN_TICKS 600

typedef struct { u16 held[2], pressed[2]; } LabInput;
typedef struct {
    u16 x[2], y[2], score[2];
    u8 joined, character[2], room, coins, carried[2], boss_state;
} Run;
typedef struct { u8 flags; SceneId target; } LabAction;
enum { AUDIO_JOIN=1,AUDIO_COIN=2,AUDIO_CLEAR=4,AUDIO_HIT=8,AUDIO_POOF=16,AUDIO_WIN=32,AUDIO_LASER=64 };
typedef struct { s16 x,y,vx,vy; u16 wait; s16 height,lift; } LooseCoin;
typedef struct { s16 x,y; u16 life; u8 direction; bool active; } Laser;
typedef struct {
    Run run;
    u16 join_remaining, phase, finish_ticks, animation_ticks[2];
    u8 menu, facing[2], audio_events;
    LooseCoin loose[COIN_COUNT];
    Laser lasers[LASER_CAPACITY];
    u16 boss_attack_clock,boss_windup,boss_animation,boss_path_wait;
    u8 boss_direction;
    bool boss_walking,boss_has_waypoint;
    s16 boss_waypoint_x,boss_waypoint_y;
    u16 invulnerable[2],boss_defeat_ticks;
    s16 boss_x,boss_y,mega_x,mega_y;
    bool moving[2], flip[2], paused, music, effects, has_save, advance_room;
} Lab;

void lab_init(Lab *lab);
void lab_enter(Lab *lab, SceneId scene);
void lab_restart_level(Lab *lab);
LabAction lab_step(Lab *lab, SceneId scene, const LabInput *input, u16 ticks);
void lab_input_sample(LabInput *input, u16 previous[2], const u16 held[2]);
u16 lab_clock(u16 *fraction, u16 hz);
bool lab_run_valid(const Run *run);
u16 lab_coin_x(u16 room,u16 index);
u16 lab_coin_y(u16 room,u16 index);
#endif
