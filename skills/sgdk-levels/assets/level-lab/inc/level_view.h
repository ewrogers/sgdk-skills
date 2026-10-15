#ifndef LEVEL_LAB_VIEW_H
#define LEVEL_LAB_VIEW_H
#include "world.h"

bool level_view_start(void);
void level_view_draw(const World *world, bool paused, bool teleport);
void level_view_stop(void);

#endif
