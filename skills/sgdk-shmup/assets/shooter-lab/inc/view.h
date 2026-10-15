#ifndef SHOOTER_VIEW_H
#define SHOOTER_VIEW_H
#include "game.h"
#define FRAME_CACHE_TILES 576
#ifndef SHOOTER_PROFILE
#define SHOOTER_PROFILE 0
#endif
#if SHOOTER_PROFILE
extern u16 profile_simulation,profile_audio,profile_draw;
#endif
bool view_start(void);
const char *view_error(void);
void view_frame(const Game *game,u16 held,u16 hz,u16 fps);
void sound_start(void);
void sound_frame(const Game *game);
#endif
