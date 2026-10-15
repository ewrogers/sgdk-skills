#include "level.h"

static const u16 coin_x[ROOM_COUNT][COIN_COUNT]={
    {160,208,240,208,160,112,80,112},
    {48,80,112,144,176,208,240,272},
    {96,160,224,240,224,160,96,80},
    {48,80,240,272,272,240,80,48}
};
static const u16 coin_y[ROOM_COUNT][COIN_COUNT]={
    {80,96,128,160,176,160,128,96},
    {88,112,144,168,168,144,112,88},
    {88,80,88,128,168,176,168,128},
    {88,72,72,88,168,184,184,168}
};
static const Block blocks[ROOM_COUNT][6]={
    {{128,112,16,32},{176,112,16,32}},
    {{96,80,24,32},{200,168,24,24}},
    {{136,112,48,32},{0,0,0,0}},
    {{96,112,24,32},{200,112,24,32},{144,64,32,16},
     {144,192,32,16},{48,128,16,24},{256,128,16,24}}
};

u16 lab_coin_x(u16 room,u16 index) { return coin_x[room][index]; }
u16 lab_coin_y(u16 room,u16 index) { return coin_y[room][index]; }
u16 level_block_count(u16 room) { return room==3 ? 6 : room==2 ? 1 : 2; }
const Block *level_block(u16 room,u16 index) { return &blocks[room][index]; }
bool level_solid(u16 room,s16 x,s16 y,s16 w,s16 h)
{
    for (u16 i=0;i<level_block_count(room);++i) {
        const Block *block=level_block(room,i);
        if (x<block->x+block->w && block->x<x+w &&
            y<block->y+block->h && block->y<y+h) return true;
    }
    return false;
}
