#ifndef TRANSITION_SCENE_VIDEO_H
#define TRANSITION_SCENE_VIDEO_H
#include "lab.h"
void scene_video_leave(void);
void scene_video_fade_out(u16 frames);
void scene_video_fade_in(const u16 colors[64], u16 frames);
bool scene_video_fade_done(void);
#endif
