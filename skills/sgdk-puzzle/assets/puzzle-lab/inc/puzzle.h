#ifndef PUZZLE_LAB_H
#define PUZZLE_LAB_H
#ifdef PZ_HOST
#include <stdint.h>
#include <stdbool.h>
typedef uint8_t u8;typedef uint16_t u16;typedef int16_t s16;typedef uint32_t u32;
#else
#include <genesis.h>
#endif
#define PZ_W 12
#define PZ_H 9
#define PZ_CELLS (PZ_W*PZ_H)
#define PZ_CRATES 4
#define PZ_HISTORY 64
#define PZ_NONE 255
enum {PZ_FLOOR,PZ_WALL,PZ_GOAL,PZ_PLATE,PZ_GATE,PZ_PIT,PZ_EXIT};
enum {PZ_BLOCKED,PZ_WALK,PZ_PUSH,PZ_BRIDGE,PZ_FINISH};
enum {PZ_LEFT,PZ_RIGHT,PZ_UP,PZ_DOWN};
typedef struct {u8 player,crate[PZ_CRATES],filled;u16 moves;} PzState;
typedef struct {
    u8 terrain[PZ_CELLS],pit[PZ_CELLS],room;
    PzState state,initial,history[PZ_HISTORY];u16 head,count;
} PzBoard;
extern const char *const pz_rooms[3];
bool pz_load(PzBoard *b,u16 room);
bool pz_parse(PzBoard *b,const char *cells);
bool pz_gate_open(const PzBoard *b,const PzState *s);
bool pz_solved(const PzBoard *b,const PzState *s);
bool pz_complete(const PzBoard *b);
bool pz_valid(const PzBoard *b);
u16 pz_move(PzBoard *b,u16 direction);
bool pz_undo(PzBoard *b);
void pz_restart(PzBoard *b);
u32 pz_digest(const PzBoard *b);
#endif
