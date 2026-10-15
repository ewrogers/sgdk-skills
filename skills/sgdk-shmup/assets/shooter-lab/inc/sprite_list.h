#ifndef SHOOTER_SPRITE_LIST_H
#define SHOOTER_SPRITE_LIST_H
#include <genesis.h>
typedef struct { const SpriteDefinition *definition; u16 palette; } Actor;
void sprite_list_begin(u16 width);
void sprite_list_place(const Actor *sprite,s16 x,s16 y,u16 size,bool active,u16 frame,const u16 *tiles);
void sprite_list_end(void);
#endif
