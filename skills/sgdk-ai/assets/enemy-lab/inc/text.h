#ifndef ENEMY_TEXT_H
#define ENEMY_TEXT_H
#include "game.h"
bool text_field(char *out, unsigned capacity, const char *text, unsigned width);
bool text_number(char *out, unsigned capacity, u32 value, unsigned width);
#endif
