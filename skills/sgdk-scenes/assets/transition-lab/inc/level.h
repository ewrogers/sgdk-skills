#ifndef TRANSITION_LEVEL_H
#define TRANSITION_LEVEL_H
#include "lab.h"
typedef struct { s16 x,y,w,h; } Block;
u16 level_block_count(u16 room);
const Block *level_block(u16 room,u16 index);
bool level_solid(u16 room,s16 x,s16 y,s16 w,s16 h);
bool level_next_step(u16 room,s16 x,s16 y,s16 target_x,s16 target_y,s16 *next_x,s16 *next_y);
#endif
