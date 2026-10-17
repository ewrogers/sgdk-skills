#ifndef BLOCK_LAB_H
#define BLOCK_LAB_H
#include "puzzle.h"
#define BL_W 10
#define BL_H 18
enum {BL_LEFT=1,BL_RIGHT=2,BL_SOFT=4,BL_ROTATE=8,BL_DROP=16};
typedef struct {
    u8 cells[BL_W*BL_H],bag[7],bag_pos,piece,next,rotation;
    s16 x,y;u16 gravity,lock,repeat,previous,phase,flash;
    u32 rng,score,lines;bool over;
} BlGame;
u16 bl_shape(u16 piece,u16 rotation);
void bl_init(BlGame *g,u32 seed);
bool bl_fits(const BlGame *g,s16 x,s16 y,u16 rotation);
bool bl_shift(BlGame *g,s16 dx,s16 dy);
bool bl_rotate(BlGame *g);
u16 bl_clear(BlGame *g);
u16 bl_drop(BlGame *g);
u16 bl_tick(BlGame *g,u16 held); /* bit 0 lock, bit 1 clear, bit 2 game over */
u16 bl_clock(u16 *phase,u16 hz);
bool bl_valid(const BlGame *g);
u32 bl_digest(const BlGame *g);
#endif
