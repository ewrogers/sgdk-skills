#ifndef TRANSITION_TEXT_H
#define TRANSITION_TEXT_H
#include "lab.h"
bool text_field(char *out, unsigned capacity, const char *text, unsigned width);
bool text_number(char *out, unsigned capacity, u32 value, unsigned width);
#endif
