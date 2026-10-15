#ifndef ENEMY_WORLD_H
#define ENEMY_WORLD_H
#include "game.h"
bool world_wall(u16 level,s16 column,s16 row);
bool world_solid(u16 level,s16 x,s16 y,s16 width,s16 height);
bool world_line(u16 level,Point from,Point to);
bool world_lob_line(u16 level,Point from,Point to);
u16 world_cell(Point position);
Point world_point(u16 cell);
u16 world_goal(u16 level,Point target);
bool world_move(u16 level,Point *position,s16 dx,s16 dy);
bool world_actor_solid(u16 level,Point position);
Point world_exit(u16 level);
Point world_entry(u16 level,u16 player);
bool world_entry_pad(u16 level,Point position);
#endif
