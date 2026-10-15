#ifndef TRANSITION_ARENA_H
#define TRANSITION_ARENA_H
#include "lab.h"
void arena_reset(Lab *lab);
void arena_restore(Lab *lab);
void arena_tick(Lab *lab,const LabInput *input);
#endif
