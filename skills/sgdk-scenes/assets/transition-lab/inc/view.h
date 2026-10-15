#ifndef TRANSITION_VIEW_H
#define TRANSITION_VIEW_H
#include "lab.h"
#include "checkpoint.h"
bool view_init(void);
void view_leave(void);
bool view_enter(SceneId scene, const Lab *lab, const Checkpoint *saved);
void view_draw(const Lab *lab);
const u16 *view_palette(void);
void view_status(const char *message, u32 changes);
void view_notice(const char *message);
bool view_clean(void);
#endif
